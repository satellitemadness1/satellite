#pragma once
// satellite/bytecode/capsule_scopes.hpp -- WHERE A CAPSULE LIVES, and how a name
// written in a program finds it.
//
// A CAPSULE LIVES IN A SCOPE, AND A SCOPE IS A FILE OR A satellite.namespace. Until
// 2026-09-22 004 had neither: every capsule of every file went into ONE map by its
// bare name, so the last file read won. Measured that day, a main.satl that declares
// its own `greet()` and includes other.satl, which has a `greet()` too, printed
// "from other.satl" and exited 0 -- the program's own capsule silently replaced, and
// `other.greet()` refused as S201. The author's programs reuse 404 capsule names
// (`call_name` in 62 spacesuits), so the map could not have held them.
//
// THE RULES, AND WHOSE EACH ONE IS:
//
//   - A FILE IS A NAMESPACE, NAMED BY ITS STEM (include_shape.hpp has said so since
//     2026-09-16; 003 built it as spaceships). `other.greet()` reaches the file this
//     file includes as other.satl. (the author, 2026-09-13, of 003) "when we include a
//     file, we name that spaceship.spacesuit_name, or spaceship.capsule_name then? It
//     doesn't get dragged into the global namespace?" -- so a BARE name never reaches
//     into another file, and the refusal says the spelling that does.
//
//   - satellite.namespace IS THE OTHER KIND, declared inside a file or inside another
//     one: `satellite.namespace tools { satellite.capsule x() { } }`, reached as
//     `tools.x()`. (the author, 2026-09-22) "namespace will be namespace/space" --
//     satellite.space is its second spelling (words/aliases.tsv). It holds capsules
//     and other spaces and NOTHING ELSE: "we don't allow global variables at all so we
//     don't need to fit variables into satellite.space, variables belong to a capsule
//     or a spacesuit ... we can add variables later if we want".
//
//   - A FILE HAS ONE satellite.library, ITS VALUES WRITTEN AT ITS TOP (2026-09-23):
//     `satellite.library.span = 25`, read as satellite.library.span by the file's own
//     capsules and as satellite.library.settings.span by a file that includes it. They are
//     literals nothing changes -- (the author) "globals don't work, but satellite.library
//     does work". library_values.hpp.
//
//   - A BARE NAME IS LOOKED FOR IN ITS OWN SCOPE, THEN IN EACH SCOPE AROUND IT, and
//     stops at the file. So a capsule inside `tools` calls its neighbour as `y()` and
//     the file's own capsules as `helper()`, and the file calls it `tools.y()`.
//
//   - A NAME IS DECLARED ONCE IN A SCOPE -- two capsules, two spaces, or one of each.
//     (the author, POLYMORPH D9.3) "a class declared twice is an ERROR: name
//     collision". A capsule declared twice used to be the second silently winning.
//
//   - A FILE REACHES ONLY THE FILES IT INCLUDES ITSELF. THIS ONE IS NOT DECIDED: 003
//     did it this way and the author said, 2026-09-22, "I think differently for only
//     the last thing ... still thinking". It is built the narrow way because a
//     program that runs under it runs under any wider rule, so widening breaks
//     nothing -- and it is ONE place, `CapsuleTable::reach`'s comment says where.
//
//   - TWO FILES MAY SHARE A NAME. 003 refused that everywhere (its S1603); the
//     author's own view_forge_main.satl includes declarations/types/people/people.satl
//     AND declarations/objects/people/people.satl, and each declares different
//     capsules. So `people.x` looks in both, and only a name BOTH declare is refused
//     -- the one case where the program could mean either.
//
// RESOLVED BY THE CHECKER BEFORE ANYTHING RUNS, and by the walker again when a call is
// reached, through the same functions here -- one reader of the shape, two callers.

#include "bytecode_registry.hpp"
#include "capsule_key.hpp"
#include "suit_layout.hpp"
#include "type_shape.hpp"
#include "value.hpp"

#include <bitset>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace satellite004 {

// ONE OF A CAPSULE'S PARAMETERS: what it was declared, and what it is called
// inside the capsule. A TYPE AND THEN A NAME, which is how every other
// declaration in satellite is written -- `satellite.variable.number n`.
//
// A SHAPE AND NOT A WORD, because `satellite.main` has taken a shaped parameter
// since 004's first program: `satellite.main(satellite.container.list<satellite.variable.string> arguments)`.
// It is the same TypeShape a satellite.variable line keeps, read by the same
// read_type_shape, and measured by the same value_fits.
struct CapsuleParameter {
    TypeShape shape;            // satellite.variable.number, or a container with its <>
    std::string name;           // what the capsule's own body calls it

    token::Code declared() const { return shape.word; }
};

inline constexpr std::size_t kNoScope = static_cast<std::size_t>(-1);

// ONE CAPSULE: where its body begins, what it takes, and which scope declared it.
struct CapsuleSite {
    std::size_t row = 0;
    std::size_t body = 0;                       // the first code INSIDE the braces
    std::vector<CapsuleParameter> parameters;   // in written order; empty for `name()`

    std::size_t scope = kNoScope;   // the file or space it is declared in
    std::size_t declared_at = 0;    // its satellite.capsule code, for a refusal's caret
    std::string name;               // as its declaration writes it: greet, satellite.main
    std::string shown;              // as a report names it: greet, tools.greet, other.greet
    std::string key;                // unique in the program -- what a button keeps (capsule_key.hpp)

    // A SPACESUIT'S CAPSULE (2026-09-22): `suit` is that spacesuit's scope, and the
    // capsule runs on one of its objects. kNoScope for a capsule of a file or a space.
    std::size_t suit = kNoScope;
    bool is_public = true;          // false in satellite.protected, or outside any section
    bool constructor = false;       // its satellite.constructor: public, answers nothing

    // WHERE THE STATEMENTS START THAT ARE THE LAST THING IT DOES (program_walk.cpp's
    // TailCall): worked out by the walker the first time the capsule runs, so a
    // call asks one short list and nothing is walked twice.
    mutable std::vector<std::size_t> last_statements;
    mutable bool last_statements_known = false;

    // WHETHER A satellite.return WITH A VALUE STANDS IN ITS BODY (hands_back_a_value),
    // worked out once, the first time the checker asks.
    mutable bool hands_back = false;
    mutable bool hands_back_known = false;
};

// ONE satellite.library VALUE (2026-09-23): `satellite.library.span = 25`, written at the
// top of its file. (the author) "we need to design it so that globals don't work, but
// satellite.library does work" -- so a value is WRITTEN DOWN, one literal, read by every
// capsule of its file as satellite.library.span and by a file that includes it as
// satellite.library.settings.span, and nothing runs to make it and nothing changes it: a
// value every capsule could change IS a global. library_values.hpp has the rest.
struct LibraryValue {
    std::size_t row = 0;
    std::size_t declared_at = 0;    // its satellite.library code, for a refusal's caret
    std::size_t value_at = 0;       // the first code after its `=`
    // READ ONCE, BY THE CHECK, BEFORE ANYTHING RUNS (read_library_values), and only ever
    // copied after that -- so every body that reads it copies a value nothing writes.
    mutable Value value;
};

// A FILE, A satellite.namespace, OR A satellite.spacesuit. A file's `parent` is kNoScope.
//
// A SPACESUIT IS THE THIRD KIND (2026-09-22), and it is a scope for the same reason a
// space is: its capsules are found by name through it, and a name is declared once
// in it. What only a spacesuit has is a LAYOUT -- its fields, shared by every object
// (suit_layout.hpp) -- a constructor, and the rule that its capsules run on an object.
enum class ScopeKind { file, space, spacesuit };

inline constexpr std::size_t kNoSite = static_cast<std::size_t>(-1);

struct CapsuleScope {
    ScopeKind kind = ScopeKind::file;
    std::string name;               // a file's stem, or the space's or spacesuit's own name
    std::string within;             // the spaces and spacesuits from the file down, dotted; "" for a file
    std::string file;               // a file's path as load_program read it; "" for the others
    std::size_t parent = kNoScope;
    std::size_t row = 0;
    std::size_t begins = 0;         // the codes it holds in its row: a body,
    std::size_t ends = 0;           // or the whole row for a file
    std::size_t declared_at = 0;    // its satellite.namespace or satellite.spacesuit code; 0 for a file
    std::unordered_map<std::string, std::size_t> capsules;   // name -> CapsuleTable::sites
    std::unordered_map<std::string, std::size_t> spaces;     // name -> CapsuleTable::scopes
    std::unordered_map<std::string, std::size_t> suits;      // name -> CapsuleTable::scopes

    // A FILE'S satellite.library VALUES, by name (2026-09-23). Empty for a space and a
    // spacesuit: a file has one satellite.library, and its values stand at its top.
    std::unordered_map<std::string, LibraryValue> library;

    // A SPACESUIT'S OWN. `is_public` is whether a spacesuit declared INSIDE another may
    // be named from outside that one (it was written in its satellite.public); every
    // other spacesuit is reached as a file's or a space's capsules are.
    std::shared_ptr<satelliteSuitLayout> layout;
    std::size_t constructor = kNoSite;                        // its satellite.constructor, in sites
    bool is_public = true;

    // WHAT IT EXTENDS, as its header's brackets name it -- `eclipse(view_forge)` -- and the
    // spacesuit that reached, once resolve_types has run (suit_reach.cpp).
    std::vector<std::string> super_names;
    std::size_t super = kNoScope;

    bool is_a_suit() const { return kind == ScopeKind::spacesuit; }
};

// A REFUSAL THE SCAN FOUND, with the place it points at. The scan has no program to
// stop and no report to print, so check_program refuses these before anything runs,
// in the order they stand in the files.
struct ScopeTrouble {
    std::size_t row = 0;
    std::size_t at = 0;
    signed long long int code = 0;
    std::string why;
    // A CAPSULE BODY THE FILE ENDS INSIDE (2026-09-25): said only after the bodies are
    // checked, because what is wrong inside one -- `display({1, 2)`, a list's { never
    // closed, which then takes the capsule's own } -- is the better sentence, and the
    // checker already has it.
    bool after_the_bodies = false;
};

// WHAT A WRITTEN NAME REACHED. `site` is the capsule, or null and `why` says why not.
// `through_a_scope` is true when the FIRST name was a space or a file, so a refusal
// is about the rest of it; when it is false the first name is nothing of this
// table's, and may still be a variable -- the checker's business, not this one's.
struct Reached {
    const CapsuleSite *site = nullptr;
    bool through_a_scope = false;
    signed long long int code = 0;
    std::string why;
};

struct CapsuleTable {
    std::vector<CapsuleSite> sites;        // in file order, then position
    std::vector<CapsuleScope> scopes;
    std::vector<std::size_t> file_scope;   // row -> that file's scope
    // row -> each name it includes -> the files of that name (two when two share it)
    std::vector<std::unordered_map<std::string, std::vector<std::size_t>>> included;
    std::unordered_map<std::string, std::size_t> keys;   // CapsuleSite::key -> sites
    std::vector<std::vector<std::size_t>> in_row;        // row -> its scopes, in the order they open
    std::vector<ScopeTrouble> troubles;

    // The innermost scope holding the code at `at` in `row`, or kNoScope.
    std::size_t scope_at(std::size_t row, std::size_t at) const;

    // A BARE NAME from `scope`: that scope, then each around it, stopping at the file.
    const CapsuleSite *bare(std::size_t scope, const std::string &name) const;

    // A NAME AS WRITTEN -- `greet`, `tools.x`, `other.greet`, `other.tools.x` -- from
    // `scope`, with the sentence to say when it reaches nothing.
    Reached reach(std::size_t scope, const std::vector<std::string> &names) const;

    // WHAT A NAME ALREADY MEANS from `scope`, for a variable that would take it:
    // "the satellite.namespace tools", "the file tools.satl this file includes", or ""
    // when it is free. A variable named `other` would make `other.greet()` a method
    // call on a string, so the name is refused where it is declared.
    std::string already_names(std::size_t scope, const std::string &name) const;

    // The program's satellite.main: the MAIN FILE's. A file it includes may have one
    // of its own, to be run by itself, and that one is never this program's.
    const CapsuleSite *main() const;

    const CapsuleSite *by_key(const std::string &key) const;

    // A SPACESUIT'S NAME AS A TYPE IS WRITTEN -- `run_log`, `tagged_report.run_log`,
    // `ship.engine`, `tools.ship` -- from `scope`: its scope, or kNoScope with the
    // sentence in `why`. A bare name is looked for in `scope` and each scope around
    // it; a dotted one starts at a spacesuit, a space or an included file, and a
    // spacesuit declared inside another is reached from outside that one only when
    // it was written in its satellite.public (suit_reach.cpp).
    std::size_t suit_named(std::size_t scope, const std::vector<std::string> &names, std::string &why) const;

    // THE CAPSULE A BARE CALL MEANS ON THIS OBJECT: `site`, or the one the object's own
    // spacesuit declared again in its place. A method follows the object, called bare or
    // with a dot (2026-09-22) -- the rule is one, so which one runs never depends on how
    // the call was written.
    const CapsuleSite &on_the_object(const CapsuleSite &site, const UserDefinedHandle &self) const;

    // THE CONSTRUCTOR AN OBJECT'S ARGUMENTS GO TO: its spacesuit's own, or -- when it has
    // none -- its nearest supertype's (003's rule). kNoSite when none in the line has one.
    std::size_t constructor_of(std::size_t suit) const;

    // THE SPACESUIT `scope` IS INSIDE, ITSELF INCLUDED: the innermost one around it,
    // or kNoScope when it is in none. A capsule of a spacesuit runs on its object.
    std::size_t suit_around(std::size_t scope) const;

    // A MEMBER OF AN OBJECT OF `suit`, as `obj.name` reaches it from `from` (the scope the
    // line stands in): its capsule, or null -- and then `why` says why, `code` which
    // refusal: member_is_protected for a field or a protected capsule reached from
    // outside, satl_line_not_understood for a name the spacesuit does not have.
    const CapsuleSite *member(std::size_t suit, const std::string &name, std::size_t from, signed long long int &code,
                              std::string &why) const;
};

// EVERY SPACESUIT NAME IN A SHAPE -- itself, or between its < and > -- made into the
// scope it reaches from `scope`. False, with the sentence in `why`, at the first that
// reaches none. The scan does this for every header and field; the checker and the
// walker for a declaration inside a body.
bool resolve_shape(const CapsuleTable &table, std::size_t scope, TypeShape &shape, std::string &why);

// Every file's scopes and capsules, and every include resolved to the row it loaded.
// `filenames` is row for row with `registry` (bytecode_registry.hpp).
CapsuleTable capsules_in(const BytecodeRegistry &registry, const BytecodeFilenames &filenames);

// EVERY FILE `row` INCLUDES, IN THE ORDER THEY WERE LOADED, with the name it reaches
// each by. A sentence that names one of them must name the same one every run, and
// an unordered_map would let the hash choose.
std::vector<std::pair<std::string, std::size_t>> files_included_by(const CapsuleTable &table, std::size_t row);

// A DOTTED NAME, READ FROM THE TOKENS: `at` on a name, then any run of `.` and a
// name. A segment after a `.` may have lexed as a METHOD CODE -- `other.find()` when a
// file declares a capsule called find -- and is read back as its spelling. `at` is
// left on what follows the last name. False, with `at` unmoved, when `at` is not on a
// name.
bool dotted_names_at(const std::vector<std::bitset<16>> &row, std::size_t &at, std::vector<std::string> &names);

// THE KEY OF THE CAPSULE `names` REACHES, written at `at` in `row` -- what a button
// keeps, so a press runs the capsule ITS OWN file meant (expression.cpp). The file
// is found from where the row is (source_position.hpp's row_index_of). When there is
// no table to ask -- a typed line has none -- or the names reach nothing, the names
// are handed back joined: the checker refused that before anything ran, and
// run_capsule says so again.
std::string capsule_key_at(const CapsuleTable *table, const BytecodeRegistry *program,
                           const std::vector<std::bitset<16>> &row, std::size_t at,
                           const std::vector<std::string> &names);

} // namespace satellite004
