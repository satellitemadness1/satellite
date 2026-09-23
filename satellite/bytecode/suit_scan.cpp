// satellite/bytecode/suit_scan.cpp -- THE SCAN, INSIDE A SPACESUIT (2026-09-22).
//
// The author's shape, as all of his spacesuits write it (the_combine's run_log):
//
//     satellite.spacesuit run_log()
//     {
//         satellite.protected
//         {
//             satellite.variable.string path = ""
//         }
//         satellite.constructor(satellite.variable.string path_input)
//         {
//             path = path_input
//         }
//         satellite.public
//         {
//             satellite.capsule call_path() satellite.returns(satellite.variable.string)
//             {
//                 satellite.return(path)
//             }
//         }
//     }
//
// HIS DECISIONS, 2026-09-22: the sections are protected, public and constructor, and
// `satellite.constructor(args) { }` sits BESIDE the other two. What a line inside one
// may be is 003's rule (parser_declarations.cpp's suit_body): a field -- a type and a
// name -- a capsule, or a section; a member written outside every section is
// protected. A section inside a section is refused here: 003 accepted it and its own
// design called it "expressible under this rule and means nothing", and its code
// then gave it the opposite access to its comment's.
//
// A SPACESUIT INSIDE A SPACESUIT IS NEW IN 004 -- 003 refused it (S0207) and no design
// of either says what it is. The author asked for it on 2026-09-22 ("we also need
// classes inside of classes"). What is built, and is this file's choice, not his:
// it is a TYPE declared inside another, as in C++. Its objects are made by the outer
// spacesuit's capsules and fields as `inner`, and from outside as `outer.inner` only
// when it was written in satellite.public. Its capsules run on an object of IT, so they
// see its fields and not the outer's -- there is no outer object inside it to see.
// A spacesuit declared inside a CAPSULE is POLYMORPH M1 and is still refused: whether
// it exists per class or per object, and what a second run makes, are his to rule.

#include "capsule_scan.hpp"

#include "program_walk.hpp"
#include "word_codes.hpp"

#include <utility>

namespace satellite004 {
namespace scan {
namespace {

using token::Code;

const Code kCapsule = word::code_of(1, 2);        // satellite.capsule
const Code kSpacesuit = word::code_of(1, 10);     // satellite.spacesuit, and satellite.class
const Code kProtected = word::code_of(1, 11);     // satellite.protected
const Code kPublic = word::code_of(1, 12);        // satellite.public
const Code kConstructor = word::code_of(1, 24);   // satellite.constructor

// A FIELD'S DECLARATION, `k` on the line's first code: a type -- a word, a word with its
// <>, or a spacesuit's name -- and then a name. True with `k` on the name when it is one.
bool a_field_at(const std::vector<std::bitset<16>> &row, std::size_t &k, TypeShape &shape)
{
    const Code first = code_at(row, k);
    if (first == token::name_token) {
        std::size_t after = k;
        std::vector<std::string> names;
        dotted_names_at(row, after, names);
        if (code_at(row, after) != token::name_token)
            return false;
        shape.word = kSpacesuit;
        shape.suit_names = std::move(names);
        k = after;
        return true;
    }
    if (!word::is_word_code(first) || !is_a_type_word(first))
        return false;
    const Code second = code_at(row, k + 1);
    if (second != token::name_token && second != token::less_than_token)
        return false;
    std::size_t after = k;
    unsigned int pending = 0;
    std::string ignored;
    if (!read_type_shape(row, after, shape, pending, ignored) || pending != 0 ||
        code_at(row, after) != token::name_token)
        return false;
    k = after;
    return true;
}

// `satellite.protected` OR `satellite.public`, AND ITS `{`: answers the brace, or 0.
// Empty brackets are allowed -- 004's word table has `satellite.protected()` -- and
// arguments are MILESTONES M35's `satellite.protected(args)`, parked by the author.
std::size_t section_brace(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r, std::size_t at)
{
    const std::string spelled = word::spelling_of(code_at(row, at));
    std::size_t k = at + 1;
    if (code_at(row, k) == token::left_parenthesis_token) {
        if (code_at(row, k + 1) != token::right_parenthesis_token) {
            refuse(table, r, at, not_built_yet,
                   spelled + "(...) takes nothing yet -- the author parked " + spelled +
                       "(args) as its own milestone (MILESTONES M35), so write " + spelled + " and its { }");
            return 0;
        }
        k += 2;
    }
    while (code_at(row, k) == token::comment_token || code_at(row, k) == token::line_end_token) ++k;
    if (code_at(row, k) != token::left_brace_token) {
        refuse(table, r, at, satl_line_not_understood,
               spelled + " is a section of its spacesuit, and what it holds goes between a { and a } after it");
        return 0;
    }
    return k;
}

} // namespace

bool open_suit(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r, std::size_t file_scope,
               std::vector<Open> &open, std::size_t &i)
{
    const std::size_t at = i;
    const std::size_t here = open.back().scope;
    std::size_t k = i + 1;
    if (code_at(row, k) != token::name_token) {
        refuse(table, r, at, satl_line_not_understood,
               "satellite.spacesuit needs a name -- satellite.spacesuit ship(), and then its sections between { and }");
        i = past_the_statement(row, at);
        return false;
    }
    const std::string name = text_at(row, k);

    // `name()` AND `name` ARE THE SAME DECLARATION (003, the author's ruling of
    // 2026-09-09). A SPACESUIT'S NAME BETWEEN THE BRACKETS IS WHAT IT EXTENDS -- 003's
    // single supertype, which ten of the author's spacesuits use: `eclipse(view_forge)`,
    // `infinity_subject(creator_data)`. ONE name, as 003 refused a second (its S0201).
    // A BUILT TYPE between them is MILESTONES M35 -- `my_class_name(satellite.variable.string)`
    // with `satellite.supertype.parameter` in its protected part -- and what that means
    // is still the author's to decide.
    std::vector<std::string> super_names;
    if (code_at(row, k) == token::left_parenthesis_token) {
        const std::size_t open = k;
        ++k;
        if (code_at(row, k) == token::name_token) {
            dotted_names_at(row, k, super_names);
        } else if (word::is_word_code(code_at(row, k))) {
            refuse(table, r, at, not_built_yet,
                   "satellite.spacesuit " + name + "(" + word::spelling_of(code_at(row, k)) + ") -- a spacesuit "
                   "extending a built type is MILESTONES M35, certain and not built yet: what it means is still "
                   "the author's to decide. Between the brackets today goes another satellite.spacesuit's name");
            ++k;
        }
        if (code_at(row, k) != token::right_parenthesis_token) {
            refuse(table, r, at, satl_line_not_understood,
                   "satellite.spacesuit " + name + "(...) extends ONE spacesuit, named between its brackets, and "
                   "its ) is never reached -- a spacesuit has one supertype or none");
            k = open;
            while (k < row.size() && code_at(row, k) != token::right_parenthesis_token &&
                   code_at(row, k) != token::line_end_token) {
                if (token::carries_a_count(code_at(row, k))) { skip_payload(row, k); continue; }
                ++k;
            }
        }
        if (code_at(row, k) == token::right_parenthesis_token) ++k;
    }
    while (code_at(row, k) == token::comment_token || code_at(row, k) == token::line_end_token) ++k;
    if (code_at(row, k) != token::left_brace_token) {
        refuse(table, r, at, satl_line_not_understood,
               "satellite.spacesuit " + name + " has no body -- its satellite.protected, satellite.public and "
               "satellite.constructor go between a { and a } on the lines after it");
        i = past_the_statement(row, at);
        return false;
    }
    const std::size_t brace = k;

    // A SECOND ONE OF A NAME IS STILL OPENED, as a second space is: its braces balance
    // and the rest is still read, and only its name is never made reachable.
    const bool named = free_in(table, here, name, r, at, "spacesuit");
    const std::size_t made = table.scopes.size();
    const CapsuleScope &around = table.scopes[here];
    CapsuleScope suit;
    suit.kind = ScopeKind::spacesuit;
    suit.name = name;
    suit.within = around.within.empty() ? name : around.within + "." + name;
    suit.parent = here;
    suit.row = r;
    suit.begins = brace + 1;
    suit.ends = row.size();
    suit.declared_at = at;
    // ONE DECLARED INSIDE ANOTHER IS REACHED FROM OUTSIDE IT only when written in its
    // satellite.public; one at a file's top or in a space, as that file's capsules are.
    suit.is_public = !in_a_suit(open.back()) || open.back().what == Opened::public_part;
    suit.layout = std::make_shared<satelliteSuitLayout>();
    suit.layout->name = name;
    suit.layout->shown = r == 0 ? suit.within : table.scopes[file_scope].name + "." + suit.within;
    suit.layout->suit = made;
    suit.layout->row = r;
    suit.super_names = std::move(super_names);
    table.scopes.push_back(std::move(suit));
    if (named)
        table.scopes[here].suits[name] = made;
    table.in_row.back().push_back(made);
    open.push_back({made, Opened::suit});
    i = brace + 1;
    return true;
}

void suit_line(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r, std::size_t file_scope,
               std::vector<Open> &open, std::size_t &i)
{
    const Open top = open.back();
    const std::size_t here = top.scope;
    const Code code = code_at(row, i);
    const std::string suit = table.scopes[here].within;

    if (code == token::line_end_token || code == token::comment_token) { ++i; return; }
    // A CHARACTER WITH NO CODE -- a no-break space pasted as indentation -- is stepped
    // over, one code, as a capsule's body steps over it.
    if (code == token::error_token) { skip_payload(row, i); return; }

    // ITS `}`: a section's, or the spacesuit's own.
    if (code == token::right_brace_token) {
        if (top.what == Opened::suit)
            table.scopes[here].ends = i;
        open.pop_back();
        ++i;
        return;
    }

    // A SECTION, IN THE SPACESUIT'S BODY -- never inside another section.
    if (code == kProtected || code == kPublic) {
        if (top.what != Opened::suit) {
            refuse(table, r, i, satl_line_not_understood,
                   std::string(word::spelling_of(code)) + " is written inside another section of the spacesuit " +
                       suit + ", and a section inside a section means nothing -- write the two side by side");
            const std::size_t brace = body_after(row, i + 1);
            i = brace == 0 ? past_the_statement(row, i) : past_matching_brace(row, brace);
            return;
        }
        const std::size_t brace = section_brace(table, row, r, i);
        if (brace == 0) {
            const std::size_t any = body_after(row, i + 1);
            i = any == 0 ? past_the_statement(row, i) : past_matching_brace(row, any);
            return;
        }
        open.push_back({here, code == kPublic ? Opened::public_part : Opened::protected_part});
        i = brace + 1;
        return;
    }

    // ITS satellite.constructor: public wherever it is written (003's rule), one of
    // them, and answering nothing -- "what a constructor produces is the object" (003's
    // S0525). A site like any capsule's, named in no map: it is never called by name.
    if (code == kConstructor) {
        const std::size_t at = i;
        std::vector<CapsuleParameter> parameters;
        TypeShape returns;
        std::string trouble;
        const std::string shown = table.scopes[here].layout->shown + "'s satellite.constructor";
        const std::size_t brace = header_rest(row, i + 1, shown, parameters, returns, trouble);
        if (brace == 0) {
            refuse(table, r, at, satl_line_not_understood,
                   shown + " has no body -- what it does goes between a { and a } after its brackets");
            i = past_the_statement(row, at);
            return;
        }
        if (!trouble.empty())
            refuse(table, r, at, satl_line_not_understood, trouble);
        if (returns.word != 0)
            refuse(table, r, at, satl_line_not_understood,
                   "the satellite.constructor of " + suit + " cannot declare satellite.returns -- what a "
                   "constructor produces is the object");
        if (table.scopes[here].constructor != kNoSite) {
            refuse(table, r, at, name_declared_twice,
                   "the spacesuit " + suit + " has two satellite.constructor sections, and an object is made one way");
        } else {
            CapsuleSite site;
            site.row = r;
            site.body = brace + 1;
            site.parameters = std::move(parameters);
            site.scope = here;
            site.declared_at = at;
            site.name = "satellite.constructor";
            site.shown = shown;
            site.key = std::to_string(r) + ":" + suit + ".satellite.constructor";
            site.suit = here;
            site.is_public = true;
            site.constructor = true;
            table.scopes[here].constructor = table.sites.size();
            table.keys[site.key] = table.sites.size();
            table.sites.push_back(std::move(site));
        }
        i = past_matching_brace(row, brace);
        return;
    }

    // A CAPSULE -- a method of its objects -- public only in satellite.public.
    if (code == kCapsule) {
        const std::size_t at = i;
        std::size_t k = i + 1;
        std::string name;
        if (word::is_word_code(code_at(row, k))) {
            name = word::spelling_of(code_at(row, k));
            ++k;
        } else if (code_at(row, k) == token::name_token) {
            name = text_at(row, k);
        } else {
            refuse(table, r, at, satl_line_not_understood, "satellite.capsule needs a name");
            i = past_the_statement(row, at);
            return;
        }
        std::vector<CapsuleParameter> parameters;
        TypeShape returns;
        std::string trouble;
        const std::size_t brace = header_rest(row, k, "satellite.capsule " + name, parameters, returns, trouble);
        if (brace == 0) {
            refuse(table, r, at, satl_line_not_understood,
                   "satellite.capsule " + name + " has no body -- what it does goes between a { and a }");
            i = past_the_statement(row, at);
            return;
        }
        if (!trouble.empty())
            refuse(table, r, at, satl_line_not_understood, trouble);
        if (word::is_word_code(code_at(row, at + 1)))
            refuse(table, r, at, satl_line_not_understood,
                   name + " is a word of the language, and a spacesuit's capsule has a name of its own");
        declare_capsule(table, r, file_scope, here, at, brace, name, std::move(parameters), std::move(returns),
                        top.what);
        i = past_matching_brace(row, brace);
        return;
    }

    // A SPACESUIT INSIDE THIS ONE (the file's header says what it is).
    if (code == kSpacesuit) {
        open_suit(table, row, r, file_scope, open, i);
        return;
    }

    // A FIELD: a type and a name, and whatever gives it its value. Its statement is run
    // where it stands each time an object is made (suit_run.cpp), and judged by the
    // checker as a declaration -- so here it is only named and its type kept.
    {
        std::size_t k = i;
        TypeShape shape;
        if (a_field_at(row, k, shape)) {
            const std::size_t at = i;
            const std::string name = text_at(row, k);
            if (free_in(table, here, name, r, at, "field")) {
                SuitField field;
                field.made = shape.is_a_suit() && code_at(row, k) == token::left_parenthesis_token;
                field.name = name;
                field.shape = std::move(shape);
                field.row = r;
                field.at = at;
                field.is_public = top.what == Opened::public_part;
                table.scopes[here].layout->fields.push_back(std::move(field));
            }
            i = past_the_statement(row, at);
            return;
        }
    }

    // ITS OWN NAME WITH BRACKETS AND A BODY -- `darkening() { }` -- is C++'s constructor.
    // 20 of the author's lines are written so (view_forge, the_blue_moon, global_view), and
    // 003 refused every one (its S0207): satellite's is satellite.constructor, beside the
    // sections, and inventing a second spelling for it is not this file's to do.
    if (code == token::name_token) {
        std::size_t k = i;
        if (text_at(row, k) == table.scopes[here].name && code_at(row, k) == token::left_parenthesis_token) {
            refuse(table, r, i, satl_line_not_understood,
                   table.scopes[here].name + "() with a body is how C++ writes a constructor -- a satellite spacesuit's "
                   "is satellite.constructor(...) { }, beside satellite.protected and satellite.public");
            const std::size_t brace = body_after(row, i);
            i = brace == 0 ? past_the_statement(row, i) : past_matching_brace(row, brace);
            return;
        }
    }
    // A WORD THAT DECLARES, AND IS NOT A TYPE 004 HAS -- `satellite.container.map<...> feeds`.
    if (word::is_word_code(code) && !is_a_type_word(code) &&
        (code_at(row, i + 1) == token::name_token || code_at(row, i + 1) == token::less_than_token)) {
        refuse(table, r, i, satl_line_not_understood,
               std::string(word::spelling_of(code)) + " is not a type satellite 004 has built, so no field can be "
               "declared with it -- the types built are satellite.variable.number, .string, .binary, .percentage, "
               ".file, .bool, .infinity, .float, .hex, .color, .fraction and satellite.container.list, .index and "
               ".multiple");
        i = past_the_statement(row, i);
        return;
    }

    const std::string holds = "the spacesuit " + suit + " holds satellite.protected, satellite.public, "
                              "satellite.constructor, fields, capsules and other spacesuits";
    if (code == word::code_of(1, 28))
        refuse(table, r, i, satl_line_not_understood,
               "a satellite.namespace goes at the top of a file or inside another one, not inside a spacesuit -- " +
                   holds);
    else if (code == word::code_of(1, 1))
        refuse(table, r, i, satl_line_not_understood,
               "satellite.include goes at the top of the file, not inside the spacesuit " + suit);
    else if (starts_a_library_line(code))
        refuse(table, r, i, satl_line_not_understood,
               "a satellite.library value is written at the top of its file, not inside the spacesuit " + suit +
                   " -- a field is a satellite.variable line, and each object has its own");
    else if (code == token::left_brace_token)
        refuse(table, r, i, satl_line_not_understood, holds + ", and this { opens none of them");
    else
        refuse(table, r, i, satl_line_not_understood, holds + ", and this line is none of them");
    if (code == token::left_brace_token)
        i = past_matching_brace(row, i);
    else
        i = past_the_statement(row, i);
}

} // namespace scan
} // namespace satellite004
