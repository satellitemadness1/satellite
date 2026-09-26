#pragma once
// satellite/bytecode/program_walk.hpp -- the whole program, loaded and run out
// of bytecode_registry.
//
// THREE PIECES, AND EACH IS WHY A PART OF THE REGISTRY IS SHAPED AS IT IS:
//
//   load_program()  fills the registry, ONE ROW A FILE -- the main .satl first,
//                   then every file its includes name, and their includes after
//                   them. This is what the second vector was always for (the
//                   author, 2026-09-16: "we have to take in other satellite
//                   files, like other includes").
//
//   CapsuleTable    where each capsule's body starts, and in which FILE or
//                   satellite.namespace it lives (capsule_scopes.hpp). The
//                   language's own words are found by CODE in the function table;
//                   a user's capsules are found by NAME there, because a user's
//                   name has no number. Until 2026-09-22 it was one map by bare
//                   name across every file, and a program's own capsule could be
//                   silently replaced by an included file's.
//
//   run_main()      walks main's body. A call is a POSITION, never an object:
//                   nothing is allocated to run a line, which is the whole
//                   difference between this and 003's tree.
//
// THERE ARE NO GLOBALS (the author, 2026-09-16): "we begin exe inside of main,
// and end exe inside of main... the only globals are the includes, other
// files". So load_program reads a file's includes and its capsule declarations
// and NOTHING else -- a statement outside a capsule has nowhere to put its
// result and no moment to run in, and file_can_run() refuses such a file.

#include "bytecode_registry.hpp"
#include "capsule_scopes.hpp"
#include "expression.hpp"
#include "function_table.hpp"
#include "include_shape.hpp"
#include "type_shape.hpp"
#include "value.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace satellite004 {

// WHAT AN ARGUMENT IS WORTH is now value.hpp's Value, and it holds a
// satellite_number rather than an `unsigned long long int` count. The author,
// 2026-09-16: "satellite.console.display(satellite.console.display(\"Hello,
// World!\")) is correct satellite code, and it should work" -- so a call is an
// argument, its answer becomes the outer call's argument, and Value carries that
// answer between them. expression.hpp is where an expression made of those
// values is worked out, and where a token reaches a fast path.

// Reading one row, shared by the walker and the checker so the two cannot
// disagree about where a statement or a body ends -- which is the defect
// PROGRESS §6.5 lists as "two implementations of runnable", kept from happening
// twice.
token::Code code_at(const std::vector<std::bitset<16>> &row, std::size_t at);
std::size_t past_the_statement(const std::vector<std::bitset<16>> &row, std::size_t at);
std::size_t past_matching_brace(const std::vector<std::bitset<16>> &row, std::size_t from);
std::size_t brace_after(const std::vector<std::bitset<16>> &row, std::size_t at);

// WHERE A for's THREE PARTS BEGIN. `satellite.statement.for` is the one statement
// written with semicolons (MILESTONES M20.A, the author's own line):
//
//     satellite.statement.for(satellite.variable.number my_int = 0; my_int < 9; my_int + 1)
//
// Shared by the walker and the checker for the same reason past_the_statement is:
// two readers of one shape drift apart, and this one has three places to drift.
// `ok` is false unless there is a `(`, exactly two semicolons outside any nested
// brackets, and the `)` that closes it -- all on the statement's own line.
struct ForHeader {
    bool ok = false;
    std::size_t declaration = 0;   // the first code inside the `(`
    std::size_t condition = 0;     // the code after the first `;`
    std::size_t step = 0;          // the code after the second `;` -- the `)` itself when the step is empty
    std::size_t closing = 0;       // the `)`
};
ForHeader for_header(const std::vector<std::bitset<16>> &row, std::size_t at);

// WHERE A satellite.return's ANSWER BEGINS, `at` on the return -- or 0 when it hands
// back nothing: `satellite.return()`, and `satellite.return(satellite)`, which is how
// main ends a program. Shared by the walker and the checker, one reader of one shape.
std::size_t return_value_at(const std::vector<std::bitset<16>> &row, std::size_t at);

// CAN THIS CAPSULE EVER HAND BACK A VALUE: a satellite.return with a value stands
// somewhere in its body -- nothing in its header says (satellite.returns was taken
// out, 2026-09-24). A capsule that can never answer is refused where its answer
// would be used, before anything runs.
bool hands_back_a_value(const BytecodeRegistry &registry, const CapsuleSite &site);

// What the third part IS: +1 for `<name>++`, -1 for `<name>--`, 0 for an ordinary
// expression. Shared for the same reason for_header is, and because the two
// refusals it gives are shape and belong to the checker. program_walk.cpp says why.
signed long long int for_step_moves_by(const std::vector<std::bitset<16>> &row,
                                       const ForHeader &parts,
                                       const std::string &name,
                                       int &moves_by,
                                       std::string &why);

// A CAPSULE'S PARAMETERS, ITS SITE AND THE TABLE OF THEM ARE capsule_scopes.hpp's,
// with the scan that finds them (capsules_in) -- a capsule now has a SCOPE, and the
// scan that knows scopes is the one that knows capsules.

// Reads `main_file` and every file its includes name, breadth first, one row a
// file. A file already loaded is not loaded twice, so a cycle of includes ends
// instead of running forever. Answers success, or the code load_satl gave.
signed long long int load_program(const std::string &main_file,
                                  StartupThreads &threads,
                                  unsigned long long int batches,
                                  BytecodeRegistry &registry,
                                  BytecodeFilenames &filenames,
                                  MachineState &state);

// NOTHING RUNS BEFORE THE WHOLE PROGRAM IS CHECKED. Every capsule body is
// walked and every statement in it judged BEFORE main is entered, so a program
// that cannot finish does not half-print first -- which check.sh asserts in as
// many words ("nothing ran before the refusal").
//
// Written fresh against the bytecode rather than borrowed (the author,
// 2026-09-16: "forget the prototype, just use 003 or preferably make everything
// new"). The prototype's compile_satl could not do this job: it knows neither a
// user's own capsule nor any include spelling past the first, and refused a
// working program outright.
//
// Answers success, or satl_line_not_understood (13) for a statement there is no
// scenario for, string_error (4) for an argument that is an expression, or
// int_error (3) for a number too large to hold.
signed long long int check_program(const BytecodeRegistry &registry,
                                   const CapsuleTable &capsules,
                                   const FunctionTable &functions,
                                   MachineState &state);

// Runs satellite.main's body, and whatever it calls. Answers success, or the
// machine code the program stopped on.
signed long long int run_main(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              MachineState &state);

// ONE CAPSULE, BY ITS KEY (CapsuleSite::key), with its own frame -- the same thing
// run_statements does for `my_capsule()` written in a program, reached from outside
// the walker. A KEY AND NOT A NAME since scopes (2026-09-22): two files may each have
// a `when_pressed`, and a button has to run the one its own file meant, so
// expression.cpp hands the window the key of the capsule the name reached.
//
// IT EXISTS FOR A BUTTON (SATELLITE_WINDOW.md WIN-11). A GTK signal has a
// capsule's NAME and nothing else, and the walker's own capsule arm is inside an
// anonymous namespace, so without this the window would have had to carry a
// second copy of "call a capsule" -- two readers of one shape, which is the
// defect program_walk.hpp already exists to avoid twice over.
//
// A NEW VariableTable EVERY TIME, because a capsule cannot see the caller's
// names and there are no globals. **THE ARGUMENTS ARE THE ONLY WAY IN**, which
// is why they exist: until 2026-09-21 a pressed capsule could print and write
// files and touch nothing else, because nothing could be handed to it.
signed long long int run_capsule(const BytecodeRegistry &registry,
                                 const CapsuleTable &capsules,
                                 const FunctionTable &functions,
                                 const std::string &key,
                                 std::vector<Value> arguments,
                                 MachineState &state);

// ONE DECLARATION STATEMENT, `at` on it and left past it -- `satellite.variable.number n
// = 5`, `satellite.container.list<x> xs`, `run_log log(path)` -- the same reader a
// capsule's body and an object's fields both run their declarations through (2026-09-22).
// `was_one` false, and nothing done, when the statement is not a declaration.
// `as_a_field` is true while an object's own fields are made (suit_run.hpp).
signed long long int run_declaration(const BytecodeRegistry &registry,
                                     const CapsuleTable &capsules,
                                     const FunctionTable &functions,
                                     std::size_t which_row,
                                     std::size_t &at,
                                     VariableTable &variables,
                                     MachineState &state,
                                     bool as_a_field,
                                     bool &was_one);

// ONE CAPSULE BY ITS SITE, FOR ITS ANSWER (2026-09-22) -- what expression.cpp calls for
// `n = five()`, `x = other.greet()` and `log.call_open()`: the same run_site every
// other road to a capsule ends in. `self` is the object a spacesuit's capsule runs on
// (null for any other capsule), and `answer`, when not null, receives what its
// satellite.return handed back -- refused with capsule_gave_no_answer (53) when none did.
// ONE OF THE PROGRAM'S CAPSULES ON A THREAD OF ITS OWN (thread_calls.cpp, 2026-09-23): as
// run_capsule_for, on `self` for a spacesuit's capsule, and its answer optional --
// `answered` says whether it handed one back, and ending without one is not a fault.
signed long long int run_capsule_on_a_thread(const BytecodeRegistry &registry,
                                            const CapsuleTable &capsules,
                                            const FunctionTable &functions,
                                            const CapsuleSite &site,
                                            std::vector<Value> arguments,
                                            const UserDefinedHandle &self,
                                            MachineState &state,
                                            Value &answer,
                                            bool &answered);

// `called_row` and `called_at`, when given, are where the call's name was written: an
// argument its capsule refuses is shown there, and at the statement being walked otherwise.
signed long long int run_capsule_for(const BytecodeRegistry &registry,
                                     const CapsuleTable &capsules,
                                     const FunctionTable &functions,
                                     const CapsuleSite &site,
                                     std::vector<Value> arguments,
                                     const UserDefinedHandle &self,
                                     MachineState &state,
                                     Value *answer,
                                     const std::vector<std::bitset<16>> *called_row = nullptr,
                                     std::size_t called_at = 0);

// WHAT THE PROMPT REMEMBERS BETWEEN LINES (the author, 2026-09-24: "the prompt has to
// remember what you type in ... it has to be built to have persistence"). One table of
// variables for the whole session, the same table a capsule's body has, so a name a
// line declares is there for every line after it -- its value, its declared type and
// shape, and a file it holds -- still open, and saved after every line, so closing
// the window loses nothing a finished line wrote. VALUES ARE KEPT, NEVER TEXT: nothing typed
// is run again to rebuild them, which was 003's ERROR #26 (an input() asked again).
struct TypedLineMemory {
    VariableTable variables;
    // THE FILES WHOSE SAVE FAILED AND WAS SAID, so it is said once and not after every
    // line that follows; a file that saves again leaves this, and is said again if it
    // fails again. Only files a kept name still holds are ever in it.
    std::unordered_map<const satellite_file *, bool> unsaved;
};

// ONE TYPED LINE, tokenised as row 0 of its own registry: the prompt's way in
// (PLAN M0.6). The same six shapes a capsule's body has, checked and then run --
// no wrapper around the line and no second reader.
//
// THE CHECK STARTS FROM WHAT `kept` HOLDS, each name with the type it was declared,
// and judges the line against a copy: a refused line changes nothing. Declaring a
// kept name again REPLACES it at the prompt, where a program refuses a second
// declaration -- satellite.system.delete is not built, so a refusal would keep a
// mistyped name for the whole session. The run writes into `kept` itself, so what
// the next line sees is exactly what this one made, and a line that failed half way
// leaves only what it finished. A typed line still has no capsules to call, and
// blocks are refused at the prompt.
signed long long int check_typed_line(const BytecodeRegistry &registry,
                                      const FunctionTable &functions,
                                      const TypedLineMemory &kept,
                                      MachineState &state);
signed long long int run_typed_line(const BytecodeRegistry &registry,
                                    const FunctionTable &functions,
                                    TypedLineMemory &kept,
                                    MachineState &state);

// A STATEMENT AT THE PROMPT WHEN THE SESSION HAS DECLARED CAPSULES OR SPACESUITS (the author,
// 2026-09-25): it is the body of `site`, a hidden capsule at the end of what the session
// declared (session.cpp), checked with the kept names as its first names and then run in
// `kept`'s own table -- so what it declares is kept, and it can call what was declared.
signed long long int check_prompt_statements(const BytecodeRegistry &registry,
                                             const CapsuleTable &capsules,
                                             const CapsuleSite &site,
                                             const FunctionTable &functions,
                                             const TypedLineMemory &kept,
                                             MachineState &state);
signed long long int run_prompt_statements(const BytecodeRegistry &registry,
                                           const CapsuleTable &capsules,
                                           const CapsuleSite &site,
                                           const FunctionTable &functions,
                                           TypedLineMemory &kept,
                                           MachineState &state);

// THE SESSION IS OVER: every file a kept name holds is saved and closed, and a save
// that fails is said, as at the end of a capsule's body. success or that failure.
signed long long int forget_typed_lines(TypedLineMemory &kept);

} // namespace satellite004
