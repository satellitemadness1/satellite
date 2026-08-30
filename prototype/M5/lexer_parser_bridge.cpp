// Lexer & Parser Diagnostic Bridge implementation -- Milestone 5.

#include "lexer_parser_bridge.hpp"
#include "suggester.hpp"
#include "satellite_words/words_spellings.hpp"
#include "satellite_words/words_walk.hpp"

namespace satellite {

namespace {

bool is_child_member(NodeIndex parent_idx, NodeIndex child_idx, const AstArena &arena)
{
    if (parent_idx == kNullNode || child_idx == kNullNode) return false;
    const AstNode &p = arena.get(parent_idx);
    if (p.kind == NodeKind::Member) {
        const auto &mem = std::get<MemberExpr>(p.data);
        return mem.target == child_idx;
    }
    return false;
}

void validate_ast_paths(
    NodeIndex idx,
    NodeIndex parent_idx,
    const AstArena &arena,
    DiagnosticReporter &reporter,
    uint32_t file_id)
{
    if (idx == kNullNode) return;
    const AstNode &node = arena.get(idx);

    // Only validate topmost MemberExpr in a dot chain to avoid duplicate reports
    if (node.kind == NodeKind::Member && !is_child_member(parent_idx, idx, arena)) {
        const auto &mem = std::get<MemberExpr>(node.data);
        std::string full_path = mem.name;
        NodeIndex cur = mem.target;
        bool rooted = false;
        while (cur != kNullNode) {
            const AstNode &target_node = arena.get(cur);
            if (target_node.kind == NodeKind::Member) {
                const auto &m = std::get<MemberExpr>(target_node.data);
                full_path = m.name + "." + full_path;
                cur = m.target;
            } else if (target_node.kind == NodeKind::SatelliteLit) {
                full_path = "satellite." + full_path;
                rooted = true;
                break;
            } else if (target_node.kind == NodeKind::Name) {
                const auto &n = std::get<NameExpr>(target_node.data);
                full_path = n.text + "." + full_path;
                if (n.text == "satellite") rooted = true;
                break;
            } else {
                break;
            }
        }
        if (rooted) {
            Span sp = node.span;
            sp.file = file_id;
            Diagnostic d = validate_path(full_path, sp);
            if (d.code != ErrorCode::None) {
                reporter.report(std::move(d));
            }
        }
    }

    // Traverse children based on node kind
    switch (node.kind) {
    case NodeKind::Member: {
        const auto &m = std::get<MemberExpr>(node.data);
        validate_ast_paths(m.target, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::Call: {
        const auto &call = std::get<CallExpr>(node.data);
        validate_ast_paths(call.target, idx, arena, reporter, file_id);
        for (NodeIndex arg : call.args) validate_ast_paths(arg, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::Unary: {
        const auto &u = std::get<UnaryExpr>(node.data);
        validate_ast_paths(u.operand, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::Binary: {
        const auto &b = std::get<BinaryExpr>(node.data);
        validate_ast_paths(b.left, idx, arena, reporter, file_id);
        validate_ast_paths(b.right, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::Block: {
        const auto &blk = std::get<BlockStmt>(node.data);
        for (NodeIndex st : blk.statements) validate_ast_paths(st, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::If: {
        const auto &s = std::get<IfStmt>(node.data);
        validate_ast_paths(s.condition, idx, arena, reporter, file_id);
        validate_ast_paths(s.then_branch, idx, arena, reporter, file_id);
        validate_ast_paths(s.else_branch, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::While: {
        const auto &s = std::get<WhileStmt>(node.data);
        validate_ast_paths(s.condition, idx, arena, reporter, file_id);
        validate_ast_paths(s.body, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::For: {
        const auto &s = std::get<ForStmt>(node.data);
        validate_ast_paths(s.init, idx, arena, reporter, file_id);
        validate_ast_paths(s.condition, idx, arena, reporter, file_id);
        validate_ast_paths(s.step, idx, arena, reporter, file_id);
        validate_ast_paths(s.body, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::VarDecl: {
        const auto &s = std::get<VarDeclStmt>(node.data);
        validate_ast_paths(s.init, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::Assign: {
        const auto &s = std::get<AssignStmt>(node.data);
        validate_ast_paths(s.target, idx, arena, reporter, file_id);
        validate_ast_paths(s.value, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::ExprStmt: {
        const auto &s = std::get<ExprStmt>(node.data);
        validate_ast_paths(s.expr, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::Return: {
        const auto &s = std::get<ReturnStmt>(node.data);
        validate_ast_paths(s.value, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::Capsule: {
        const auto &c = std::get<CapsuleDecl>(node.data);
        validate_ast_paths(c.body, idx, arena, reporter, file_id);
        break;
    }
    case NodeKind::Spacesuit: {
        const auto &suit = std::get<SpacesuitDecl>(node.data);
        for (const auto &item : suit.items) {
            if (std::holds_alternative<FieldDecl>(item)) {
                const auto &f = std::get<FieldDecl>(item);
                validate_ast_paths(f.init, idx, arena, reporter, file_id);
            } else if (std::holds_alternative<MethodDecl>(item)) {
                const auto &m = std::get<MethodDecl>(item);
                validate_ast_paths(m.capsule.body, idx, arena, reporter, file_id);
            }
        }
        break;
    }
    default:
        break;
    }
}

} // namespace

Diagnostic validate_path(
    std::string_view path,
    Span base_span,
    words::Words *)
{
    words::Walk walk = words::walk(path);
    if (walk.error == words::WalkError::NONE) {
        return Diagnostic{};
    }

    if (walk.error == words::WalkError::NOT_ROOTED) {
        return Diagnostic::error(
            ErrorCode::E0201_PathNotRooted,
            "language-owned path must be rooted at 'satellite'",
            base_span);
    }

    if (walk.error == words::WalkError::NO_SUCH_WORD) {
        std::string_view rest = path.substr(walk.offset);
        size_t end_idx = rest.find_first_of(".(");
        std::string_view seg = (end_idx == std::string_view::npos) ? rest : rest.substr(0, end_idx);

        Span seg_span = base_span;
        seg_span.start = base_span.start + walk.offset;
        seg_span.end = seg_span.start + static_cast<uint32_t>(seg.size());

        auto under_node = static_cast<words::NodeId>(walk.under);
        std::string_view under_name = words::spelling_of(under_node);
        if (under_name.empty() && under_node == words::NodeId::SATELLITE)
            under_name = "satellite";

        auto suggestion = suggest_trie_word(under_node, seg);
        std::string msg;
        if (suggestion.has_value()) {
            msg = format_trie_did_you_mean(seg, *suggestion, under_name);
        } else {
            msg = "no '" + std::string(seg) + "' under '" + std::string(under_name) + "'";
        }

        Diagnostic d = Diagnostic::error(ErrorCode::E0202_NoSuchWord, std::move(msg), seg_span);
        if (suggestion.has_value()) {
            d.with_suggestion("did you mean '" + *suggestion + "'?", seg_span, *suggestion);
        }
        return d;
    }

    if (walk.error == words::WalkError::NO_SUCH_SHAPE) {
        std::string_view rest = path.substr(walk.offset);
        Span seg_span = base_span;
        seg_span.start = base_span.start + walk.offset;
        seg_span.end = base_span.start + static_cast<uint32_t>(rest.size());

        auto under_node = static_cast<words::NodeId>(walk.under);
        std::string_view under_name = words::spelling_of(under_node);

        std::string msg = "unrecognized call shape for '" + std::string(under_name) + "'";
        return Diagnostic::error(ErrorCode::E0203_NoSuchShape, std::move(msg), seg_span);
    }

    return Diagnostic::error(
        ErrorCode::E0204_TrailingPathSegment,
        "unexpected trailing characters after path match",
        base_span);
}

Diagnostic translate_lexer_error(const Token &tok, uint32_t file_id)
{
    Span sp{tok.start, tok.end, tok.line, file_id};
    ErrorCode code = ErrorCode::E0001_InvalidCharacter;

    if (tok.text.find("unterminated string") != std::string::npos) {
        code = ErrorCode::E0002_UnterminatedString;
    } else if (tok.text.find("escape") != std::string::npos) {
        code = ErrorCode::E0003_InvalidEscapeSequence;
    } else if (tok.text.find("hexadecimal") != std::string::npos || tok.text.find("binary") != std::string::npos) {
        code = ErrorCode::E0005_InvalidBitsLiteral;
    } else if (tok.text.find("duration") != std::string::npos) {
        code = ErrorCode::E0006_InvalidDurationLiteral;
    } else if (tok.text.find("number") != std::string::npos) {
        code = ErrorCode::E0004_InvalidNumberLiteral;
    }

    return Diagnostic::error(code, tok.text, sp);
}

Diagnostic translate_parser_error(const ParseError &err, const std::string &)
{
    ErrorCode code = ErrorCode::E0101_UnexpectedToken;
    if (err.message.find("expected") != std::string::npos) {
        code = ErrorCode::E0102_ExpectedToken;
    }
    return Diagnostic::error(code, err.message, err.span);
}

CompileAnalysis diagnose_source(const std::string &source, const std::string &filename)
{
    CompileAnalysis result;
    uint32_t file_id = result.sources.add(source, filename);

    // 1. Lexing
    std::vector<Token> tokens = lex(source);
    bool lex_has_err = false;
    for (const auto &tok : tokens) {
        if (tok.kind == TokenKind::Error) {
            result.reporter.report(translate_lexer_error(tok, file_id));
            lex_has_err = true;
        }
    }

    if (lex_has_err) {
        result.ok = false;
        return result;
    }

    // 2. Parsing
    ParseResult pres = parse(tokens, result.arena, result.words, file_id);
    result.program = std::move(pres.program);

    for (const auto &perr : pres.errors) {
        result.reporter.report(translate_parser_error(perr, source));
    }

    // 3. Path Validation against words trie
    for (NodeIndex item : result.program.items) {
        validate_ast_paths(item, kNullNode, result.arena, result.reporter, file_id);
    }

    result.ok = !result.reporter.has_errors();
    return result;
}

} // namespace satellite
