// satellite/prompt's line reader alone, with no language behind it, for
// check_prompt.py to type at through a real terminal (PLAN M0.6). It is a test
// harness, not satl: the session that runs a line is M0.6's next part.
//
//     make build/prompt_reader && python3 satellite/prompt/check_prompt.py
//
// Every line comes back as [the line, shown], or (N bytes) when it holds 1,000
// bytes or more; Ctrl-C answers (interrupted). `wait` sleeps a second and says
// (waited), so keys can be typed while a line "runs"; `drop` calls
// discard_pending(), as the session will on Ctrl-C. `exit` or the end of input
// ends it with (end), and the terminal's modes after it when there is one.

#include "line_reader.hpp"
#include "render.hpp"

#include "../machine/shown.hpp"

#include <string>
#include <termios.h>
#include <unistd.h>

int main()
{
    using namespace satellite004::prompt;
    LineReader reader;
    std::string line;
    for (;;) {
        const LineStatus status = reader.read("satl> ", line);
        if (status == LineStatus::EndOfFile)
            break;
        if (status == LineStatus::Interrupted) {
            write_all(STDOUT_FILENO, "(interrupted)\n");
            continue;
        }
        reader.remember(line);
        write_all(STDOUT_FILENO, line.size() >= 1000 ? "(" + std::to_string(line.size()) + " bytes)\n"
                                                     : "[" + satellite004::shown(line) + "]\n");
        if (line == "wait") {
            sleep(1);
            write_all(STDOUT_FILENO, "(waited)\n");
        }
        if (line == "drop")
            reader.discard_pending();
        if (line == "exit")
            break;
    }

    termios modes{};
    if (tcgetattr(STDIN_FILENO, &modes) != 0) {
        write_all(STDOUT_FILENO, "(end)\n");
        return 0;
    }
    const auto on = [&](tcflag_t flag) { return (modes.c_lflag & flag) != 0 ? "1" : "0"; };
    write_all(STDOUT_FILENO, std::string("(end) icanon=") + on(ICANON) + " echo=" + on(ECHO) + " isig=" + on(ISIG) + "\n");
    return 0;
}
