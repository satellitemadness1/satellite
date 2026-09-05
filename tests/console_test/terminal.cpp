// M14's done-when, on a real terminal: one satl per clause under forkpty(3),
// ASSERTED ON THE SCREEN AND NOT ON THE BYTES SATL WROTE -- the pty's line
// discipline (echo, ONLCR, ISIG) sits between the program and the person, and
// the person's side is the one every clause is about. This is the suite's one
// section that runs the REAL BINARY: the reader thread, the queue, the
// signal, and the kernel's own terminal are all load-bearing here, and a
// fixture that faked any of them would prove the fake.
//
// THE FIXTURE PROGRAMS ARE WRITTEN TO /tmp AT RUN TIME -- satc_test/
// writing.cpp's precedent -- because a program the test writes is a program
// the test cannot drift from.

#include "console_test.hpp"
#include "pty.hpp"

#include <string>

namespace console_test {

namespace {

// How many counter lines (a line that is only digits) the screen holds.
size_t counter_lines(const std::string &screen)
{
    size_t count = 0;
    size_t at = 0;
    while (at < screen.size()) {
        size_t end = screen.find('\n', at);
        if (end == std::string::npos)
            end = screen.size();
        size_t digits = 0;
        for (size_t i = at; i < end; i++)
            if (screen[i] >= '0' && screen[i] <= '9')
                digits++;
            else if (screen[i] != '\r')
                digits = 0;
        if (digits > 0 && digits + (screen[end - 1] == '\r' ? 1 : 0) == end - at)
            count++;
        at = end + 1;
    }
    return count;
}

// Clauses 1 and 2: the counter keeps rising while a line is half-typed, and
// nothing spins. `/q` goes in a byte at a time with a pause after each, and
// the number of all-digit lines must grow across every pause -- QUAD.md
// §3.4's requirement (1), the one claim M14 exists to make. The CPU spent
// across ~1.5 s of mostly-waiting must stay far under the wall, which is
// clause 2 and is why M13's sleep landed first.
void a_counter_rises_while_typing()
{
    const std::string program = fixture(
        "counter",
        "    satellite.variable.number i = 0\n"
        "    satellite.statement.while (i < 200)\n"
        "    {\n"
        "        satellite.variable.variant line\n"
        "        line = satellite.console.typed()\n"
        "        satellite.statement.if (line.holding() == \"string\")\n"
        "        {\n"
        "            satellite.console.display(line.held())\n"
        "            satellite.return(satellite)\n"
        "        }\n"
        "        satellite.console.display(i)\n"
        "        satellite.time.sleep(0.1)\n"
        "        i = i + 1\n"
        "    }\n"
        "    satellite.return(satellite)\n");

    Terminal t = run_on_a_terminal(program, 80, 24);
    check(t.wait_for("0", 3000), "the counter starts");

    const size_t before_slash = counter_lines(t.screen);
    t.type("/");
    t.collect_for(450);
    const size_t after_slash = counter_lines(t.screen);
    check(after_slash > before_slash,
          "the counter advanced while `/` sat half-typed -- the program did "
          "not block on the terminal");

    t.type("q");
    t.collect_for(450);
    const size_t after_q = counter_lines(t.screen);
    check(after_q > after_slash,
          "and again with `/q` half-typed -- every pause, not just one");

    t.type("\n");
    check(t.wait_for("/q\r\n", 2000),
          "return makes `/q` a line and typed() answers all of it at once");

    long cpu_ms = 0;
    const int status = t.finish(&cpu_ms);
    check(status == 0, "the counter program finished cleanly");
    check(cpu_ms < 700,
          "and it did not spin: " + std::to_string(cpu_ms) +
              " ms of CPU across ~1.5 s of waiting -- the blocking happens on "
              "a thread that waits (DESIGN 10.1), not in a poll loop");
    close(t.master);
}

// Clause 3: an empty line is a LINE; nobody typing is NOTHING.
void empty_is_a_line_and_nobody_is_nothing()
{
    const std::string program = fixture(
        "empty",
        "    satellite.variable.variant first\n"
        "    first = satellite.console.typed()\n"
        "    satellite.console.display(first.holding())\n"
        "    satellite.time.sleep(0.4)\n"
        "    satellite.variable.variant second\n"
        "    second = satellite.console.typed()\n"
        "    satellite.console.display(second.holding())\n"
        "    satellite.variable.string line = second.held()\n"
        "    satellite.console.display(line.size())\n"
        "    satellite.return(satellite)\n");

    Terminal t = run_on_a_terminal(program, 80, 24);
    check(t.wait_for("nothing", 3000),
          "before anything is typed, typed() answers nothing -- M12's state, "
          "consumed rather than invented");
    t.type("\n");
    check(t.wait_for("string", 2000),
          "return pressed IS a line, so the variant holds a string");
    check(t.wait_for("\r\n0\r\n", 2000), "and that line is empty -- size 0");
    check(t.finish() == 0, "the empty-line program finished cleanly");
    close(t.master);
}

// Clause 4 and clause 5's working half: the prompt appears BEFORE the
// program blocks -- observable only on a terminal, where line buffering
// would otherwise hold an unterminated prompt back -- and the place form
// writes its variable.
void the_prompt_appears_before_the_wait()
{
    const std::string program = fixture(
        "prompt",
        "    satellite.variable.string who = satellite.console.input(\"who? \")\n"
        "    satellite.console.display(who)\n"
        "    satellite.variable.string place = \"\"\n"
        "    satellite.console.input(\"and? \", place)\n"
        "    satellite.console.display(place)\n"
        "    satellite.return(satellite)\n");

    Terminal t = run_on_a_terminal(program, 80, 24);
    check(t.wait_for("who? ", 3000),
          "the un-newlined prompt reached the screen with NOTHING typed yet "
          "-- the drain barrier, observable only here");
    t.type("moon\n");
    check(t.wait_for("moon\r\n", 2000), "the answer comes back displayed");
    check(t.wait_for("and? ", 2000), "the place form prompts the same way");
    t.type("stars\n");
    check(t.wait_for("stars\r\n", 2000),
          "`input(prompt, target)` wrote its place and the program read it "
          "back out");
    check(t.finish() == 0, "the prompt program finished cleanly");
    close(t.master);
}

// Clause 6's interrupt half: Ctrl-C AT THE PROMPT is the byte 0x03 through
// the pty -- the kernel's ISIG turns it into SIGINT, which is exactly the
// road a person's finger takes -- and the run reports the interrupt, not the
// end of input. (The end-of-input half needs no terminal and eval-level
// fixtures cannot reach it either; it is proved in this suite's transcript
// home, MILESTONES/M14.md, via `< /dev/null`.)
void control_c_is_not_the_end_of_input()
{
    const std::string program = fixture(
        "interrupt",
        "    satellite.variable.string who = satellite.console.input(\"halt? \")\n"
        "    satellite.console.display(who)\n"
        "    satellite.return(satellite)\n");

    Terminal t = run_on_a_terminal(program, 80, 24);
    check(t.wait_for("halt? ", 3000), "the program is at its prompt");
    t.type("\x03");
    check(t.wait_for("Ctrl-C", 3000),
          "the report names the interrupt -- S0730's sentence, never S1001's "
          "end of input: getting those two backwards is the regression PLAN "
          "M14 says not to rediscover");
    const int status = t.finish();
    check(status == 130, "and the exit is 130 -- got " + std::to_string(status));
    close(t.master);
}

// Clause 7: the terminal's facts are LIVE. Resize the pty between two asks
// and both numbers move, with no restart anywhere.
void width_and_height_are_live()
{
    const std::string program = fixture(
        "resize",
        "    satellite.console.display(satellite.console.width)\n"
        "    satellite.time.sleep(0.5)\n"
        "    satellite.console.display(satellite.console.width)\n"
        "    satellite.console.display(satellite.console.height)\n"
        "    satellite.return(satellite)\n");

    Terminal t = run_on_a_terminal(program, 100, 40);
    check(t.wait_for("100", 3000), "the first ask answers the pty's 100");

    winsize smaller{};
    smaller.ws_col = 66;
    smaller.ws_row = 22;
    ioctl(t.master, TIOCSWINSZ, &smaller);

    check(t.wait_for("66", 3000), "after a resize the width is 66 -- asked "
                                  "fresh, never sampled at startup");
    check(t.wait_for("22", 2000), "and the height moved with it");
    check(t.finish() == 0, "the resize program finished cleanly");
    close(t.master);
}

// Clause 8: `home()` and `display()` alternated in a tight loop never tear
// -- every escape lands at the start of a line, because both went through
// the printer's queue. v1 wrote its escapes straight to the fd, "a frame
// shredded between two queued lines".
void escapes_never_land_inside_a_line()
{
    const std::string program = fixture(
        "frames",
        "    satellite.variable.number i = 0\n"
        "    satellite.statement.while (i < 60)\n"
        "    {\n"
        "        satellite.console.home()\n"
        "        satellite.console.display(\"XXXXXXXXXXXXXXXXXXXX\")\n"
        "        i = i + 1\n"
        "    }\n"
        "    satellite.return(satellite)\n");

    Terminal t = run_on_a_terminal(program, 80, 24);
    check(t.finish() == 0, "the frame program finished cleanly");

    size_t escapes = 0;
    bool torn = false;
    for (size_t i = 0; i < t.screen.size(); i++) {
        if (t.screen[i] != '\033')
            continue;
        escapes++;
        if (i != 0 && t.screen[i - 1] != '\n')
            torn = true;
    }
    check(escapes >= 60, "every home() reached the screen");
    check(!torn, "and no escape landed inside a queued line -- both went "
                 "through the queue, not the fd");
    close(t.master);
}

} // namespace

void section_terminal()
{
    a_counter_rises_while_typing();
    empty_is_a_line_and_nobody_is_nothing();
    the_prompt_appears_before_the_wait();
    control_c_is_not_the_end_of_input();
    width_and_height_are_live();
    escapes_never_land_inside_a_line();
}

} // namespace console_test
