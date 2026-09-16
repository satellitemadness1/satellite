// The satellite Closure Compiler implementation.
// Milestone 11 Prototype in prototype/M11.

#include "compiler.hpp"
#include "satellite_string/satellite_string.hpp"
#include "search.hpp"

#include <algorithm>
#include <iostream>

namespace satellite {

namespace {

static std::string resolve_method_full_path(const std::string &type_prefix,
                                           const std::string &method_name,
                                           size_t arg_count)
{
    if (type_prefix == "satellite.container.list") {
        if (method_name == "append") return "satellite.container.list.append";
        if (method_name == "size") return "satellite.container.list.size";
        if (method_name == "sort") {
            return (arg_count == 0) ? "satellite.container.list.sort()" : "satellite.container.list.sort(direction)";
        }
        if (method_name == "sort_down") {
            return (arg_count == 0) ? "satellite.container.list.sort_down()" : "satellite.container.list.sort_down(key)";
        }
        if (method_name == "sort_up") return "satellite.container.list.sort_up(key)";
        if (method_name == "contains") return "satellite.container.list.contains(x)";
        if (method_name == "index_of") return "satellite.container.list.index_of(x)";
        if (method_name == "empty") return "satellite.container.list.empty";
        if (method_name == "clear") return "satellite.container.list.clear";
        if (method_name == "first") return "satellite.container.list.first";
        if (method_name == "last") return "satellite.container.list.last";
        if (method_name == "truncate") return "satellite.container.list.truncate(n)";
        if (method_name == "reserve") return "satellite.container.list.reserve(n)";
        if (method_name == "remove_first") return "satellite.container.list.remove_first()";
        if (method_name == "remove_last") return "satellite.container.list.remove_last()";
        if (method_name == "remove_at") return "satellite.container.list.remove_at(n)";
        if (method_name == "remove") return "satellite.container.list.remove(x)";
        if (method_name == "insert") return "satellite.container.list.insert(n, x)";
        if (method_name == "join") return "satellite.container.list.join(separator)";
        if (method_name == "reverse") return "satellite.container.list.reverse";
        if (method_name == "sum") return "satellite.container.list.sum";
        if (method_name == "max") return "satellite.container.list.max";
        if (method_name == "min") return "satellite.container.list.min";
    } else if (type_prefix == "satellite.container.map") {
        if (method_name == "set") return "satellite.container.map.set(k, v)";
        if (method_name == "get") return "satellite.container.map.get(k)";
        if (method_name == "has") return "satellite.container.map.has(k)";
        if (method_name == "size") return "satellite.container.map.size";
        if (method_name == "empty") return "satellite.container.map.empty";
        if (method_name == "clear") return "satellite.container.map.clear";
        if (method_name == "remove") return "satellite.container.map.remove(k)";
        if (method_name == "keys") return "satellite.container.map.keys";
        if (method_name == "values") return "satellite.container.map.values";
    } else if (type_prefix == "satellite.variable.string") {
        if (method_name == "size") return "satellite.variable.string.size";
        if (method_name == "empty") return "satellite.variable.string.empty";
        if (method_name == "find") return "satellite.variable.string.find(x)";
        if (method_name == "contains") return "satellite.variable.string.contains(x)";
        if (method_name == "substring") return "satellite.variable.string.substring(start, end)";
        if (method_name == "starts_with") return "satellite.variable.string.starts_with(x)";
        if (method_name == "ends_with") return "satellite.variable.string.ends_with(x)";
        if (method_name == "lower") return "satellite.variable.string.lower";
        if (method_name == "upper") return "satellite.variable.string.upper";
        if (method_name == "split") return "satellite.variable.string.split(separator)";
        if (method_name == "trim") return "satellite.variable.string.trim";
        if (method_name == "replace") return "satellite.variable.string.replace(a, b)";
        if (method_name == "to_number") return "satellite.variable.string.to_number";
        if (method_name == "append") return "satellite.variable.string.append(x)";
        if (method_name == "clear") return "satellite.variable.string.clear";
        if (method_name == "at") return "satellite.variable.string.at(n)";
    }
    return type_prefix + "." + method_name;
}

// ---------------------------------------------------------------------------
// Concrete Expression Closures
// ---------------------------------------------------------------------------

class ConstClosure : public IExprClosure {
public:
    explicit ConstClosure(Value val) : val_(std::move(val)) {}
    Value eval(ExecContext &/*ctx*/) const override { return val_; }
private:
    Value val_;
};

class SlotReadClosure : public IExprClosure {
public:
    explicit SlotReadClosure(size_t slot) : slot_(slot) {}
    Value eval(ExecContext &ctx) const override {
        return ctx.current_frame().get(slot_);
    }
private:
    size_t slot_;
};

class GlobalReadClosure : public IExprClosure {
public:
    explicit GlobalReadClosure(std::string name) : name_(std::move(name)) {}
    Value eval(ExecContext &ctx) const override {
        return ctx.get_global(name_);
    }
private:
    std::string name_;
};

class MemberAccessClosure : public IExprClosure {
public:
    MemberAccessClosure(ExprPtr target, std::string member_name)
        : target_(std::move(target)), member_name_(std::move(member_name)) {}

    Value eval(ExecContext &ctx) const override {
        Value target_val = target_ ? target_->eval(ctx) : Value::nil();
        std::string type_prefix;
        if (target_val.is_string()) {
            type_prefix = "satellite.variable.string";
        } else if (target_val.is_number()) {
            type_prefix = "satellite.variable.number";
        } else if (target_val.is_bool()) {
            type_prefix = "satellite.variable.bool";
        } else if (target_val.is_list()) {
            type_prefix = "satellite.container.list";
        } else if (std::holds_alternative<MapRef>(target_val)) {
            type_prefix = "satellite.container.map";
        }

        if (!type_prefix.empty()) {
            std::string full_path = resolve_method_full_path(type_prefix, member_name_, 0);
            words::Walk w = words::walk(full_path);
            if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
                if (const auto *entry = DispatchTable::instance().get(w.id)) {
                    return entry->fn(ctx, {target_val});
                }
            }
        }
        return Value::nil();
    }
private:
    ExprPtr target_;
    std::string member_name_;
};

class UnaryClosure : public IExprClosure {
public:
    UnaryClosure(std::string op, ExprPtr child)
        : op_(std::move(op)), child_(std::move(child)) {}

    Value eval(ExecContext &ctx) const override {
        Value v = child_ ? child_->eval(ctx) : Value::nil();
        if (op_ == "-") {
            if (v.is_number()) return Value::number(std::get<Number>(v).negated());
            return Value::nil();
        }
        if (op_ == "!" || op_ == "not") {
            return Value::boolean(!v.is_truthy());
        }
        if (op_ == "+") {
            return v;
        }
        return Value::nil();
    }
private:
    std::string op_;
    ExprPtr child_;
};

class BinaryClosure : public IExprClosure {
public:
    BinaryClosure(std::string op, ExprPtr left, ExprPtr right)
        : op_(std::move(op)), left_(std::move(left)), right_(std::move(right)) {}

    Value eval(ExecContext &ctx) const override {
        if (op_ == "&&" || op_ == "and" || op_ == "&") {
            Value lv = left_ ? left_->eval(ctx) : Value::nil();
            if (!lv.is_truthy()) return Value::boolean(false);
            Value rv = right_ ? right_->eval(ctx) : Value::nil();
            return Value::boolean(rv.is_truthy());
        }
        if (op_ == "||" || op_ == "or" || op_ == "|") {
            Value lv = left_ ? left_->eval(ctx) : Value::nil();
            if (lv.is_truthy()) return Value::boolean(true);
            Value rv = right_ ? right_->eval(ctx) : Value::nil();
            return Value::boolean(rv.is_truthy());
        }

        Value lv = left_ ? left_->eval(ctx) : Value::nil();
        Value rv = right_ ? right_->eval(ctx) : Value::nil();

        if (op_ == "+") {
            if (lv.is_number() && rv.is_number()) {
                return Value::number(std::get<Number>(lv) + std::get<Number>(rv));
            }
            if (lv.is_string() && rv.is_string()) {
                SatString res = *std::get<Str>(lv) + *std::get<Str>(rv);
                return Value::sat_string(res);
            }
            if (lv.is_string()) {
                SatString res = *std::get<Str>(lv) + encode(rv.to_string());
                return Value::sat_string(res);
            }
            if (rv.is_string()) {
                SatString res = encode(lv.to_string()) + *std::get<Str>(rv);
                return Value::sat_string(res);
            }
        } else if (op_ == "-") {
            if (lv.is_number() && rv.is_number()) {
                return Value::number(std::get<Number>(lv) - std::get<Number>(rv));
            }
        } else if (op_ == "*") {
            if (lv.is_number() && rv.is_number()) {
                return Value::number(std::get<Number>(lv) * std::get<Number>(rv));
            }
        } else if (op_ == "/") {
            if (lv.is_number() && rv.is_number()) {
                return Value::number(std::get<Number>(lv) / std::get<Number>(rv));
            }
        } else if (op_ == "==") {
            return Value::boolean(lv == rv);
        } else if (op_ == "!=") {
            return Value::boolean(lv != rv);
        } else if (op_ == "<") {
            if (lv.is_number() && rv.is_number()) {
                return Value::boolean(std::get<Number>(lv) < std::get<Number>(rv));
            }
            if (lv.is_string() && rv.is_string()) {
                return Value::boolean(*std::get<Str>(lv) < *std::get<Str>(rv));
            }
        } else if (op_ == "<=") {
            if (lv.is_number() && rv.is_number()) {
                return Value::boolean(std::get<Number>(lv) <= std::get<Number>(rv));
            }
            if (lv.is_string() && rv.is_string()) {
                return Value::boolean(*std::get<Str>(lv) <= *std::get<Str>(rv));
            }
        } else if (op_ == ">") {
            if (lv.is_number() && rv.is_number()) {
                return Value::boolean(std::get<Number>(lv) > std::get<Number>(rv));
            }
            if (lv.is_string() && rv.is_string()) {
                return Value::boolean(*std::get<Str>(lv) > *std::get<Str>(rv));
            }
        } else if (op_ == ">=") {
            if (lv.is_number() && rv.is_number()) {
                return Value::boolean(std::get<Number>(lv) >= std::get<Number>(rv));
            }
            if (lv.is_string() && rv.is_string()) {
                return Value::boolean(*std::get<Str>(lv) >= *std::get<Str>(rv));
            }
        }
        return Value::nil();
    }
private:
    std::string op_;
    ExprPtr left_;
    ExprPtr right_;
};

class ListLitClosure : public IExprClosure {
public:
    explicit ListLitClosure(std::vector<ExprPtr> elements)
        : elements_(std::move(elements)) {}

    Value eval(ExecContext &ctx) const override {
        auto list = std::make_shared<List>();
        list->reserve(elements_.size());
        for (const auto &elem : elements_) {
            list->push_back(std::make_shared<Value>(elem ? elem->eval(ctx) : Value::nil()));
        }
        return Value(std::const_pointer_cast<const List>(list));
    }
private:
    std::vector<ExprPtr> elements_;
};

class IndexClosure : public IExprClosure {
public:
    IndexClosure(ExprPtr target, ExprPtr index)
        : target_(std::move(target)), index_(std::move(index)) {}

    Value eval(ExecContext &ctx) const override {
        Value tv = target_ ? target_->eval(ctx) : Value::nil();
        Value iv = index_ ? index_->eval(ctx) : Value::nil();

        if (tv.is_list() && iv.is_number()) {
            long long idx = 0;
            if (std::get<Number>(iv).to_integer(idx)) {
                auto list_ref = std::get<ListRef>(tv);
                if (list_ref) {
                    long long len = static_cast<long long>(list_ref->size());
                    if (idx < 0) idx += len;
                    if (idx >= 0 && static_cast<size_t>(idx) < list_ref->size()) {
                        auto item_ptr = (*list_ref)[static_cast<size_t>(idx)];
                        return item_ptr ? *item_ptr : Value::nil();
                    }
                }
            }
        }

        if (std::holds_alternative<MapRef>(tv)) {
            auto map_ref = std::get<MapRef>(tv);
            if (map_ref) {
                std::string canonical;
                if (map_key_of(iv, canonical)) {
                    auto it = map_ref->index.find(canonical);
                    if (it != map_ref->index.end()) {
                        const auto &val_ptr = map_ref->entries[it->second].value;
                        return val_ptr ? *val_ptr : Value::nil();
                    }
                }
                std::string error;
                auto target_ptr = std::make_shared<Value>(tv);
                auto pattern_ptr = std::make_shared<Value>(iv);
                ValuePtr found = search_collect(target_ptr, pattern_ptr, false, ctx.max_depth(), error);
                if (found && found->is_list()) {
                    auto fl = std::get<ListRef>(*found);
                    if (fl && !fl->empty() && (*fl)[0]) {
                        return *(*fl)[0];
                    }
                }
            }
        }

        if (tv.is_string() && iv.is_number()) {
            long long idx = 0;
            if (std::get<Number>(iv).to_integer(idx)) {
                auto str_ref = std::get<Str>(tv);
                if (str_ref) {
                    long long len = static_cast<long long>(str_ref->size());
                    if (idx < 0) idx += len;
                    if (idx >= 0 && static_cast<size_t>(idx) < str_ref->size()) {
                        return Value::sat_string(SatString{(*str_ref)[static_cast<size_t>(idx)]});
                    }
                }
            }
        }

        return Value::nil();
    }
private:
    ExprPtr target_;
    ExprPtr index_;
};

// ---------------------------------------------------------------------------
// Concrete Statement Closures
// ---------------------------------------------------------------------------

class ExprStmtClosure : public IStmtClosure {
public:
    explicit ExprStmtClosure(ExprPtr expr) : expr_(std::move(expr)) {}
    ExecResult exec(ExecContext &ctx) const override {
        if (expr_) {
            Value v = expr_->eval(ctx);
            return ExecResult::normal(v);
        }
        return ExecResult::normal();
    }
private:
    ExprPtr expr_;
};

class VarDeclStmtClosure : public IStmtClosure {
public:
    VarDeclStmtClosure(int32_t slot, std::string name, ExprPtr init, std::string type_name = "")
        : slot_(slot), name_(std::move(name)), init_(std::move(init)), type_name_(std::move(type_name)) {}

    ExecResult exec(ExecContext &ctx) const override {
        Value val = init_ ? init_->eval(ctx) : Value::nil();
        if (val.is_nil()) {
            if (type_name_ == "list") {
                val = Value(std::make_shared<const List>());
            } else if (type_name_ == "map") {
                val = Value(std::make_shared<const MapBody>());
            }
        }
        if (is_local_slot(slot_)) {
            ctx.current_frame().set(slot_, val);
        }
        ctx.set_global(name_, std::move(val));
        return ExecResult::normal();
    }
private:
    int32_t slot_;
    std::string name_;
    ExprPtr init_;
    std::string type_name_;
};

class AssignStmtClosure : public IStmtClosure {
public:
    AssignStmtClosure(int32_t slot, std::string name, ExprPtr expr)
        : slot_(slot), name_(std::move(name)), expr_(std::move(expr)) {}

    ExecResult exec(ExecContext &ctx) const override {
        Value val = expr_ ? expr_->eval(ctx) : Value::nil();
        if (is_local_slot(slot_)) {
            ctx.current_frame().set(slot_, val);
        }
        ctx.set_global(name_, std::move(val));
        return ExecResult::normal();
    }
private:
    int32_t slot_;
    std::string name_;
    ExprPtr expr_;
};

class IndexAssignStmtClosure : public IStmtClosure {
public:
    IndexAssignStmtClosure(int32_t slot, std::string name, ExprPtr subscript, ExprPtr value)
        : slot_(slot), name_(std::move(name)), subscript_(std::move(subscript)), value_(std::move(value)) {}

    ExecResult exec(ExecContext &ctx) const override {
        Value cur = is_local_slot(slot_) ? ctx.current_frame().get(slot_) : ctx.get_global(name_);
        Value sub = subscript_ ? subscript_->eval(ctx) : Value::nil();
        Value val = value_ ? value_->eval(ctx) : Value::nil();

        if (cur.is_list()) {
            auto list_ref = std::get<ListRef>(cur);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            long long idx = 0;
            if (sub.is_number() && std::get<Number>(sub).to_integer(idx)) {
                long long len = static_cast<long long>(mutable_list->size());
                if (idx < 0) idx += len;
                if (idx >= 0 && static_cast<size_t>(idx) < mutable_list->size()) {
                    (*mutable_list)[static_cast<size_t>(idx)] = std::make_shared<Value>(val);
                } else if (idx == len) {
                    mutable_list->push_back(std::make_shared<Value>(val));
                }
            }
            Value updated = Value(std::const_pointer_cast<const List>(mutable_list));
            if (is_local_slot(slot_)) {
                ctx.current_frame().set(slot_, updated);
            }
            ctx.set_global(name_, std::move(updated));
        } else if (std::holds_alternative<MapRef>(cur)) {
            auto map_ref = std::get<MapRef>(cur);
            auto next_map = std::make_shared<MapBody>(map_ref ? *map_ref : MapBody{});
            std::string canonical;
            if (map_key_of(sub, canonical)) {
                auto it = next_map->index.find(canonical);
                if (it != next_map->index.end()) {
                    next_map->entries[it->second].value = std::make_shared<Value>(val);
                } else {
                    next_map->index[canonical] = next_map->entries.size();
                    next_map->entries.push_back(MapEntry{
                        std::make_shared<Value>(sub),
                        std::make_shared<Value>(val)
                    });
                }
            }
            Value updated = Value(std::const_pointer_cast<const MapBody>(next_map));
            if (is_local_slot(slot_)) {
                ctx.current_frame().set(slot_, updated);
            }
            ctx.set_global(name_, std::move(updated));
        }
        return ExecResult::normal();
    }
private:
    int32_t slot_;
    std::string name_;
    ExprPtr subscript_;
    ExprPtr value_;
};

class ReturnStmtClosure : public IStmtClosure {
public:
    explicit ReturnStmtClosure(ExprPtr expr) : expr_(std::move(expr)) {}
    ExecResult exec(ExecContext &ctx) const override {
        Value val = expr_ ? expr_->eval(ctx) : Value::nil();
        return ExecResult::return_val(val);
    }
private:
    ExprPtr expr_;
};

class BlockStmtClosure : public IStmtClosure {
public:
    explicit BlockStmtClosure(std::vector<StmtPtr> stmts)
        : stmts_(std::move(stmts)) {}

    ExecResult exec(ExecContext &ctx) const override {
        ExecResult last = ExecResult::normal();
        for (const auto &stmt : stmts_) {
            if (!stmt) continue;
            if (interrupt_requested()) {
                return ExecResult::interrupted();
            }
            last = stmt->exec(ctx);
            if (last.status != ExecStatus::Normal)
                return last;
        }
        return last;
    }
private:
    std::vector<StmtPtr> stmts_;
};

class IfStmtClosure : public IStmtClosure {
public:
    IfStmtClosure(ExprPtr cond, StmtPtr then_branch, StmtPtr else_branch)
        : cond_(std::move(cond)), then_branch_(std::move(then_branch)),
          else_branch_(std::move(else_branch)) {}

    ExecResult exec(ExecContext &ctx) const override {
        if (interrupt_requested()) {
            return ExecResult::interrupted();
        }
        Value cv = cond_ ? cond_->eval(ctx) : Value::nil();
        if (cv.is_truthy()) {
            if (then_branch_) return then_branch_->exec(ctx);
        } else {
            if (else_branch_) return else_branch_->exec(ctx);
        }
        return ExecResult::normal();
    }
private:
    ExprPtr cond_;
    StmtPtr then_branch_;
    StmtPtr else_branch_;
};

class WhileStmtClosure : public IStmtClosure {
public:
    WhileStmtClosure(ExprPtr cond, StmtPtr body)
        : cond_(std::move(cond)), body_(std::move(body)) {}

    ExecResult exec(ExecContext &ctx) const override {
        while (cond_ && cond_->eval(ctx).is_truthy()) {
            if (interrupt_requested()) {
                return ExecResult::interrupted();
            }
            if (body_) {
                ExecResult res = body_->exec(ctx);
                if (res.status == ExecStatus::Return)
                    return res;
                if (res.status == ExecStatus::Break)
                    break;
                if (res.status == ExecStatus::Interrupted)
                    return res;
            }
        }
        return ExecResult::normal();
    }
private:
    ExprPtr cond_;
    StmtPtr body_;
};

class ForStmtClosure : public IStmtClosure {
public:
    ForStmtClosure(StmtPtr init, ExprPtr cond, StmtPtr step, StmtPtr body)
        : init_(std::move(init)), cond_(std::move(cond)),
          step_(std::move(step)), body_(std::move(body)) {}

    ExecResult exec(ExecContext &ctx) const override {
        if (init_) init_->exec(ctx);
        while (!cond_ || cond_->eval(ctx).is_truthy()) {
            if (interrupt_requested()) {
                return ExecResult::interrupted();
            }
            if (body_) {
                ExecResult res = body_->exec(ctx);
                if (res.status == ExecStatus::Return)
                    return res;
                if (res.status == ExecStatus::Break)
                    break;
                if (res.status == ExecStatus::Interrupted)
                    return res;
            }
            if (step_) step_->exec(ctx);
        }
        return ExecResult::normal();
    }
private:
    StmtPtr init_;
    ExprPtr cond_;
    StmtPtr step_;
    StmtPtr body_;
};

} // namespace

// ---------------------------------------------------------------------------
// Compiler Implementation
// ---------------------------------------------------------------------------

Compiler::Compiler(const Program &program, const AstArena &arena,
                   const ResolveResult &resolve, words::Words &words)
    : program_(program), arena_(arena), resolve_(resolve), words_(words)
{
}

bool Compiler::flatten_dotted_path(NodeIndex node_idx, std::string &out_path)
{
    if (node_idx == kNullNode) return false;
    const auto &node = arena_.get(node_idx);
    if (std::holds_alternative<SatelliteLit>(node.data)) {
        out_path = "satellite";
        return true;
    }
    if (std::holds_alternative<NameExpr>(node.data)) {
        out_path = std::get<NameExpr>(node.data).text;
        return true;
    }
    if (std::holds_alternative<MemberExpr>(node.data)) {
        const auto &mem = std::get<MemberExpr>(node.data);
        if (!flatten_dotted_path(mem.target, out_path)) return false;
        out_path += "." + mem.name;
        return true;
    }
    return false;
}

ExprPtr Compiler::compile_expr(NodeIndex node_idx)
{
    if (node_idx == kNullNode) return nullptr;
    const auto &node = arena_.get(node_idx);

    if (std::holds_alternative<NumberLit>(node.data)) {
        const auto &lit = std::get<NumberLit>(node.data);
        Number n;
        if (Number::parse(lit.text, n)) {
            return std::make_unique<ConstClosure>(Value::number(n));
        }
        return std::make_unique<ConstClosure>(Value::number(Number(0)));
    }

    if (std::holds_alternative<StringLit>(node.data)) {
        const auto &lit = std::get<StringLit>(node.data);
        return std::make_unique<ConstClosure>(Value::sat_string(encode(lit.text)));
    }

    if (std::holds_alternative<SatelliteLit>(node.data)) {
        return std::make_unique<ConstClosure>(Value::satellite_singleton());
    }

    if (std::holds_alternative<BitsLit>(node.data)) {
        const auto &lit = std::get<BitsLit>(node.data);
        auto bits = std::make_shared<Bits>();
        bits->radix = lit.radix;
        bits->digits = lit.text;
        return std::make_unique<ConstClosure>(Value(bits));
    }

    if (std::holds_alternative<NameExpr>(node.data)) {
        const auto &name = std::get<NameExpr>(node.data);
        if (name.text == "satellite.bool.true" || name.text == "true") {
            return std::make_unique<ConstClosure>(Value::boolean(true));
        }
        if (name.text == "satellite.bool.false" || name.text == "false") {
            return std::make_unique<ConstClosure>(Value::boolean(false));
        }
        int32_t slot = resolve_.table.get_slot(node_idx);
        if (is_local_slot(slot)) {
            return std::make_unique<SlotReadClosure>(slot);
        }
        return std::make_unique<GlobalReadClosure>(name.text);
    }

    if (std::holds_alternative<UnaryExpr>(node.data)) {
        const auto &un = std::get<UnaryExpr>(node.data);
        return std::make_unique<UnaryClosure>(un.op, compile_expr(un.operand));
    }

    if (std::holds_alternative<BinaryExpr>(node.data)) {
        const auto &bin = std::get<BinaryExpr>(node.data);
        return std::make_unique<BinaryClosure>(bin.op, compile_expr(bin.left), compile_expr(bin.right));
    }

    if (std::holds_alternative<ListLit>(node.data)) {
        const auto &list = std::get<ListLit>(node.data);
        std::vector<ExprPtr> elements;
        elements.reserve(list.elements.size());
        for (NodeIndex elem_idx : list.elements) {
            elements.push_back(compile_expr(elem_idx));
        }
        return std::make_unique<ListLitClosure>(std::move(elements));
    }

    if (std::holds_alternative<IndexExpr>(node.data)) {
        const auto &idx = std::get<IndexExpr>(node.data);
        return std::make_unique<IndexClosure>(compile_expr(idx.target), compile_expr(idx.subscript));
    }

    if (std::holds_alternative<CallExpr>(node.data)) {
        const auto &call = std::get<CallExpr>(node.data);
        std::vector<ExprPtr> args;
        args.reserve(call.args.size());
        for (NodeIndex arg_idx : call.args) {
            args.push_back(compile_expr(arg_idx));
        }

        std::string dotted;
        words::PathId path_id = words::kNoPath;
        std::string callee_name;

        if (flatten_dotted_path(call.target, dotted)) {
            std::string full_path = dotted;
            if (full_path == "satellite.system.threshold") {
                full_path = args.empty() ? "satellite.system.threshold()" : "satellite.system.threshold(n)";
            }
            words::Walk w = words::walk(full_path);
            if (w.error == words::WalkError::NONE) {
                path_id = w.id;
            }
            callee_name = full_path;
        }

        ExprPtr target_expr = nullptr;
        if (call.target != kNullNode &&
            std::holds_alternative<MemberExpr>(arena_.get(call.target).data)) {
            const auto &mem = std::get<MemberExpr>(arena_.get(call.target).data);
            target_expr = compile_expr(mem.target);
            callee_name = mem.name;
        } else {
            target_expr = compile_expr(call.target);
        }

        // Literal option fold (WORD_NUMBERS §1.5)
        if (callee_name == "sort" && call.args.size() == 1) {
            const auto &arg_node = arena_.get(call.args[0]);
            if (std::holds_alternative<StringLit>(arg_node.data)) {
                const auto &slit = std::get<StringLit>(arg_node.data);
                if (slit.text == "down") {
                    words::Walk w = words::walk("satellite.container.list.sort_down()");
                    if (w.error == words::WalkError::NONE) path_id = w.id;
                } else if (slit.text == "up") {
                    words::Walk w = words::walk("satellite.container.list.sort()");
                    if (w.error == words::WalkError::NONE) path_id = w.id;
                }
            }
        }

        words::PathId folded = resolve_.table.get_folded_path(node_idx);
        if (folded != words::kNoPath) {
            path_id = folded;
        }

        return std::make_unique<CallExprClosure>(
            std::move(target_expr), std::move(args), path_id, callee_name);
    }

    if (std::holds_alternative<MemberExpr>(node.data)) {
        const auto &mem = std::get<MemberExpr>(node.data);
        std::string dotted;
        if (flatten_dotted_path(node_idx, dotted)) {
            if (dotted == "satellite.bool.true")
                return std::make_unique<ConstClosure>(Value::boolean(true));
            if (dotted == "satellite.bool.false")
                return std::make_unique<ConstClosure>(Value::boolean(false));
            words::Walk w = words::walk(dotted);
            if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
                if (dotted == "satellite.bool.true")
                    return std::make_unique<ConstClosure>(Value::boolean(true));
                if (dotted == "satellite.bool.false")
                    return std::make_unique<ConstClosure>(Value::boolean(false));
            }
        }
        return std::make_unique<MemberAccessClosure>(compile_expr(mem.target), mem.name);
    }

    return nullptr;
}

StmtPtr Compiler::compile_stmt(NodeIndex node_idx)
{
    if (node_idx == kNullNode) return nullptr;
    const auto &node = arena_.get(node_idx);

    if (std::holds_alternative<ExprStmt>(node.data)) {
        const auto &st = std::get<ExprStmt>(node.data);
        return std::make_unique<ExprStmtClosure>(compile_expr(st.expr));
    }

    if (std::holds_alternative<VarDeclStmt>(node.data)) {
        const auto &st = std::get<VarDeclStmt>(node.data);
        int32_t slot = resolve_.table.get_slot(node_idx);
        std::string type_name = st.type.name;
        return std::make_unique<VarDeclStmtClosure>(slot, st.name, compile_expr(st.init), type_name);
    }

    if (std::holds_alternative<AssignStmt>(node.data)) {
        const auto &st = std::get<AssignStmt>(node.data);
        if (st.target != kNullNode && std::holds_alternative<IndexExpr>(arena_.get(st.target).data)) {
            const auto &idx_node = std::get<IndexExpr>(arena_.get(st.target).data);
            int32_t slot = 0;
            std::string name;
            if (idx_node.target != kNullNode) {
                slot = resolve_.table.get_slot(idx_node.target);
                if (std::holds_alternative<NameExpr>(arena_.get(idx_node.target).data)) {
                    name = std::get<NameExpr>(arena_.get(idx_node.target).data).text;
                }
            }
            return std::make_unique<IndexAssignStmtClosure>(
                slot, name, compile_expr(idx_node.subscript), compile_expr(st.value));
        }

        int32_t slot = 0;
        std::string name;
        if (st.target != kNullNode) {
            slot = resolve_.table.get_slot(st.target);
            if (std::holds_alternative<NameExpr>(arena_.get(st.target).data)) {
                name = std::get<NameExpr>(arena_.get(st.target).data).text;
            }
        }
        return std::make_unique<AssignStmtClosure>(slot, name, compile_expr(st.value));
    }

    if (std::holds_alternative<ReturnStmt>(node.data)) {
        const auto &st = std::get<ReturnStmt>(node.data);
        return std::make_unique<ReturnStmtClosure>(compile_expr(st.value));
    }

    if (std::holds_alternative<BlockStmt>(node.data)) {
        const auto &st = std::get<BlockStmt>(node.data);
        std::vector<StmtPtr> stmts;
        stmts.reserve(st.statements.size());
        for (NodeIndex s_idx : st.statements) {
            stmts.push_back(compile_stmt(s_idx));
        }
        return std::make_unique<BlockStmtClosure>(std::move(stmts));
    }

    if (std::holds_alternative<IfStmt>(node.data)) {
        const auto &st = std::get<IfStmt>(node.data);
        return std::make_unique<IfStmtClosure>(
            compile_expr(st.condition), compile_stmt(st.then_branch), compile_stmt(st.else_branch));
    }

    if (std::holds_alternative<WhileStmt>(node.data)) {
        const auto &st = std::get<WhileStmt>(node.data);
        return std::make_unique<WhileStmtClosure>(
            compile_expr(st.condition), compile_stmt(st.body));
    }

    if (std::holds_alternative<ForStmt>(node.data)) {
        const auto &st = std::get<ForStmt>(node.data);
        return std::make_unique<ForStmtClosure>(
            compile_stmt(st.init), compile_expr(st.condition),
            compile_stmt(st.step), compile_stmt(st.body));
    }

    return nullptr;
}

std::shared_ptr<CapsuleClosure> Compiler::compile_capsule(const CapsuleInfo &info)
{
    auto cap = std::make_shared<CapsuleClosure>();
    cap->name = info.name;
    cap->param_count = info.param_count;
    cap->slot_count = info.slot_count;
    cap->path_id = info.path_id;

    if (info.capsule && info.capsule->body != kNullNode) {
        cap->body = compile_stmt(info.capsule->body);
    }
    return cap;
}

CompiledProgram Compiler::compile()
{
    CompiledProgram prog;

    for (const auto &pair : resolve_.capsules) {
        const auto &info = pair.second;
        auto cap = compile_capsule(info);
        prog.capsules[pair.first] = cap;
    }

    for (NodeIndex stmt_idx : program_.items) {
        if (stmt_idx == kNullNode) continue;
        const auto &node = arena_.get(stmt_idx);
        if (std::holds_alternative<CapsuleDecl>(node.data) ||
            std::holds_alternative<SpacesuitDecl>(node.data) ||
            std::holds_alternative<IncludeDecl>(node.data))
            continue;
        prog.top_level.push_back(compile_stmt(stmt_idx));
    }

    return prog;
}

CompiledProgram compile(const Program &program, const AstArena &arena,
                        const ResolveResult &resolve, words::Words &words)
{
    Compiler comp(program, arena, resolve, words);
    return comp.compile();
}

} // namespace satellite

