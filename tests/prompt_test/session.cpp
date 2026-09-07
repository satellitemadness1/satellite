// The real `satl --repl`, driven through a terminal. See prompt_test.hpp
// clause 4, and pty.hpp for why a pipe would not do.

#include "prompt_test.hpp"

#include "pty.hpp"

#include <cstddef>
#include <string>

namespace prompt_test {

void section_session()
{
    // 1 -- A TYPED LINE RUNS. The claim session.cpp is built on: a line that is
    // not a legal file (S0204 lists the four things a file holds, and a
    // `display` is none of them) becomes one by being wrapped.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("satellite.console.display(\"TYPED AND RAN\")");
        check(p.wait_for("TYPED AND RAN", 4000), "a typed line runs");
        p.line("exit");
        check(p.finish() == 0, "the exit word leaves cleanly");
    }

    // 2 -- A DIAGNOSTIC IS ON THE LINE THE PERSON TYPED. The wrapper puts three
    // lines above the body, so an un-rebased caret would say line 4 -- and the
    // user would go looking at a file that does not exist for a line they never
    // wrote.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("satellite.console.display(nothing_declares_this)");
        check(p.wait_for("<prompt>:1:", 4000),
              "a mistake on the first typed line is reported as line 1");
        p.line("exit");
        p.finish();
    }

    // 2b -- AND A RUN-TIME DIAGNOSTIC IS REBASED TOO, which clause 2 does not
    // cover: it reports a RESOLVE-time mistake, and those arrive through the
    // four passes. S0721 -- a path the language numbers and nothing implements
    // -- is raised by op_dispatch while the program is RUNNING, so it comes
    // back on the Machine instead and went out with the wrapper's line number
    // on it until this was noticed. `satellite.help` is the shortest way to
    // reach it and will stay reachable until M18 builds help.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("satellite.help");
        check(p.wait_for("S0721", 4000), "an unbuilt path refuses at run time");
        check(holds(p.screen, "<prompt>:1:"),
              "and the refusal is on the line the person typed");
        p.line("exit");
        p.finish();
    }

    // 3 -- CTRL-C IS THE BYTE 0x03 AND ABANDONS THE LINE, NOT THE SESSION.
    // DESIGN §10.2's two meanings: with ISIG off no signal is raised at all,
    // so what is being proved here is that the prompt saw a KEY.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.type("satellite.console.display(\"MUST NOT RUN\")");
        p.type("\003");
        p.line("satellite.console.display(\"STILL HERE\")");
        check(p.wait_for("STILL HERE", 4000), "the session survived Ctrl-C");
        // The abandoned line was echoed as it was typed, so its text is on the
        // screen; what must NOT be there is a second copy on a line of its own,
        // which is what running it would have printed.
        check(!holds(p.screen, "MUST NOT RUN\r\nmadness") &&
                  !holds(p.screen, "\nMUST NOT RUN\r\n"),
              "the abandoned line did not run");
        p.line("exit");
        p.finish();
    }

    // 4 -- A BLOCK IS ENTERED OVER SEVERAL LINES, WITH THE BRACE ON ITS OWN.
    // The shape every example in this tree is written in.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("satellite.statement.for (satellite.variable.number i = 0; i < 3; i = i + 1)");
        p.line("{");
        p.line("satellite.console.display(i + 1000)");
        p.line("}");
        p.gather(1500);
        // THE MARKER IS COMPUTED AND IS NOT IN THE TEXT THAT WAS TYPED, and
        // getting here took two wrong versions. The first asserted `0`, `1` and
        // `2` were on screen -- every one of them is in the loop header the
        // user typed. The second counted a literal "TICK" and was defeated by
        // the prompt's own redraw: a line is repainted on EVERY keystroke, so
        // the echo alone holds the word several times. `1001` and `1002` can
        // only have been printed BY the loop -- the source says `1000` -- so
        // they are evidence and the other two were not. Both wrong versions
        // passed against a deliberate mutation of block.cpp's opens_body.
        check(holds(p.screen, "1001") && holds(p.screen, "1002"),
              "a block typed over four lines runs its body three times");
        p.line("exit");
        p.finish();
    }

    // 5 -- A CAPSULE DECLARED AT THE PROMPT IS STILL THERE ON THE NEXT LINE.
    // The top-level half of session.cpp's accumulation, and the thing that
    // makes the prompt worth having with no session state under it.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("satellite.capsule double_it(satellite.variable.number n) satellite.returns(satellite.variable.number)");
        p.line("{");
        p.line("satellite.return(n * 2)");
        p.line("}");
        p.line("satellite.console.display(double_it(21))");
        check(p.wait_for("42", 4000), "a capsule declared at the prompt can be called");
        p.line("satellite.console.display(double_it(50))");
        check(p.wait_for("100", 4000), "and is still there a line later");
        p.line("exit");
        p.finish();
    }

    // 6 -- `help` IS REFUSED AND POINTS AT THE PATH. DESIGN §1: a bare word is
    // the user's, so the prompt says where the thing they want lives rather
    // than quietly meaning it.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("help");
        check(p.wait_for("satellite.help()", 4000),
              "a bare `help` is refused and names the path");
        p.line("exit");
        p.finish();
    }

    // 7 -- `run <file>` RUNS ONE, which is how a program is started from the
    // prompt and what M18's help store will be asked about afterwards.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("run example/hello_world.satl");
        check(p.wait_for("Hello, World!", 5000), "`run` runs a file");
        p.line("exit");
        p.finish();
    }

    // 8 -- A GLOBAL DECLARED, THEN ASSIGNED TO, EACH ON ITS OWN LINE.
    //
    // THE ASSIGNMENT MUST BE ITS OWN ENTRY OR THIS CLAUSE PROVES NOTHING, and
    // the first version of it did not: it put `satellite.library.counter = 9`
    // inside an `if` block, so the ENTRY began with `satellite.statement` and
    // the scanner never saw a library line at all. It passed with the fix
    // disabled. What the fix does is decide where a BARE
    // `satellite.library.<name> = ...` line goes -- the first mention declares,
    // every later one assigns -- so only a bare line exercises it, and placing
    // both at the top level is S0291.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("satellite.library.counter = 5");
        p.line("satellite.library.counter = 9");
        p.gather(1200);
        check(!holds(p.screen, "S0291"),
              "assigning to a global already declared is not a redeclaration");
        p.line("exit");
        p.finish();
    }

    // 9 -- A DECLARATION SURVIVES ITS LINE, which is the session state M22
    // ended with and the thing a person hits first.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.line("satellite.variable.number n = 6");
        p.line("satellite.console.display(n * 7)");
        check(p.wait_for("42", 4000), "a local declared on one line is there on the next");
        p.line("satellite.variable.string s = \"hello\"");
        p.line("satellite.console.display(n + s.size())");
        check(p.wait_for("11", 4000), "and so is a second one, of another type");
        p.line("n = 100");
        p.line("satellite.console.display(n + s.size())");
        check(p.wait_for("105", 4000), "an assignment to a kept variable sticks");
        p.line("exit");
        p.finish();
    }

    // 10 -- `satellite.system.persist` TURNS IT OFF -- `1 22 8`, WORD_NUMBERS
    // §2.7, and the first child of `satellite.system` anything implements.
    //
    // THE SPELLING IS `satellite.bool.false` AND NOT `false`, which is the
    // clause's second job: a bare `false` is a name the user owns and answers
    // S0511, so a banner or a document that showed the short form would be
    // teaching a command that does not run.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        check(holds(p.screen, "satellite.system.persist(satellite.bool.true)"),
              "the banner says what persist is set to, in a spelling that runs");
        p.line("satellite.variable.number kept = 5");
        p.line("satellite.system.persist(satellite.bool.false)");
        p.line("satellite.variable.number dropped = 9");
        p.line("satellite.console.display(kept + 700)");
        check(p.wait_for("705", 4000),
              "what was already held survives persist being turned off");
        p.line("satellite.console.display(dropped)");
        check(p.wait_for("S0511", 4000),
              "and what was declared after it is not kept");
        p.line("exit");
        p.finish();
    }

    // 11 -- CTRL-D ON AN EMPTY LINE ENDS THE SESSION, exit status 0.
    {
        Prompt p = start_a_prompt();
        p.wait_for("Type `exit`", 4000);
        p.type("\004");
        check(p.finish() == 0, "Ctrl-D ends the session cleanly");
    }
}

} // namespace prompt_test
