// satellite.directory's three words, with no interpreter around them (PLAN M0.6):
// what change() and list() answer for the directories a person actually meets.
//
//     make build/directory_cases && build/directory_cases    (check.sh runs it)
//
// THE LIBRARY'S OWN HEADER IS THE SUBJECT, not the .so: satellite-numbers builds
// the same directory_words.hpp into 1.18.1.so, 1.18.4.so and 1.18.5.so, and their
// .cpp files are describe functions with no behaviour of their own. Checking the
// header checks all three.
//
// THE CTRL-C FLAG IS CHECKED WITH THE FLAG ALREADY RAISED, which is the one way
// to check it without a race: a listing that is asked to stop before it starts
// answers `interrupted` and NO names, which is the same answer it gives when the
// key arrives half way through a million entries.

#include "../../satellite-numbers/directory_words.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

using namespace satellite004;

namespace {

int failed = 0;

void check(bool right, const std::string &what)
{
    failed += right ? 0 : 1;
    std::printf("%s %s\n", right ? "ok  " : "FAIL", what.c_str());
}

bool holds(const std::vector<std::string> &names, const std::string &name)
{
    for (const std::string &one : names)
        if (one == name)
            return true;
    return false;
}

std::string here()
{
    char *where = ::getcwd(nullptr, 0);
    const std::string answer = where != nullptr ? where : "";
    std::free(where);
    return answer;
}

} // namespace

int main(int argc, char **argv)
{
    const std::string room = argc > 1 ? argv[1] : "/tmp";
    const std::string made = room + "/directory_cases";
    const std::string deep = made + "/nested";
    std::string command = "rm -rf '" + made + "' && mkdir -p '" + deep + "' && touch '" + made + "/plain' '" + made +
                          "/.hidden' && ln -s nowhere '" + made + "/dangling'";
    if (std::system(command.c_str()) != 0)
        return std::printf("FAIL could not make the directories to check\n"), 1;

    // CHANGE ANSWERS A VALUE AND NEVER AN ERROR, whatever is wrong with the path.
    const std::string started_in = here();
    DirectoryReply moved = directory_words::change(made, true, nullptr);
    check(moved.flag && moved.code == success && here() == made, "change into a directory answers true, and moves");
    moved = directory_words::change(made + "/plain", true, nullptr);
    check(!moved.flag && moved.code == success, "change to a file answers false, and is not an error");
    moved = directory_words::change(made + "/nowhere_at_all", true, nullptr);
    check(!moved.flag && moved.code == success, "change to a missing path answers false, and is not an error");
    check(here() == made, "... and neither moved the working directory");
    moved = directory_words::change(std::string("x\0y", 3), true, nullptr);
    check(moved.code == path_holds_a_nul, "change to a path holding a NUL is refused by name");

    // LIST: the names, and the three answers a caller wants kept apart.
    DirectoryReply listed = directory_words::list(std::string(), false, nullptr);
    check(listed.code == success && listed.names.size() == 4, "list() reads the working directory");
    check(!holds(listed.names, ".") && !holds(listed.names, ".."), "... without . and ..");
    check(holds(listed.names, ".hidden"), "... with the dotfiles kept");
    check(holds(listed.names, "dangling"), "... and a dangling link, which no stat would answer for");
    check(listed.names == std::vector<std::string>({".hidden", "dangling", "nested", "plain"}), "... sorted");

    const DirectoryReply elsewhere = directory_words::list(deep, true, nullptr);
    check(elsewhere.code == success && elsewhere.names.empty(), "list(d) of an empty directory answers no names");

    DirectoryReply refused = directory_words::list(made + "/nowhere_at_all", true, nullptr);
    check(refused.code == directory_not_found, "list of a missing path is directory_not_found");
    refused = directory_words::list(made + "/plain", true, nullptr);
    check(refused.code == not_a_directory, "list of a file is not_a_directory");
    refused = directory_words::list(std::string("x\0y", 3), true, nullptr);
    check(refused.code == path_holds_a_nul, "list of a path holding a NUL is refused by name");

    if (::geteuid() != 0) {
        command = "chmod 000 '" + deep + "'";
        if (std::system(command.c_str()) == 0) {
            refused = directory_words::list(deep, true, nullptr);
            check(refused.code == directory_unreadable && !refused.reason.empty(),
                  "list of a directory it may not read is directory_unreadable, with the system's reason");
            command = "chmod 755 '" + deep + "'";
            (void)std::system(command.c_str());
        }
    }

    // THE CTRL-C FLAG, raised before the call: nothing half-read comes back.
    const volatile sig_atomic_t stop = 1;
    const DirectoryReply stopped = directory_words::list(std::string(), false, &stop);
    check(stopped.code == interrupted && stopped.names.empty(), "a listing asked to stop answers interrupted, with no names");

    // A DIRECTORY BIGGER THAN ONE getdents BUFFER, read whole and in order.
    const std::string many = made + "/many";
    command = "mkdir -p '" + many + "' && cd '" + many + "' && for i in $(seq 1 20000); do : > f$i; done";
    if (std::system(command.c_str()) == 0) {
        const DirectoryReply all = directory_words::list(many, true, nullptr);
        check(all.code == success && all.names.size() == 20000, "20,000 entries list whole");
        bool in_order = true;
        for (std::size_t i = 1; i < all.names.size(); ++i)
            in_order = in_order && all.names[i - 1] < all.names[i];
        check(in_order, "... in one order, the same on every machine");
    }

    (void)directory_words::change(started_in, true, nullptr);
    command = "chmod -R 755 '" + made + "' 2>/dev/null; rm -rf '" + made + "'";
    (void)std::system(command.c_str());
    std::printf("%s\n", failed == 0 ? "every directory case passed" : "a directory case FAILED");
    return failed == 0 ? 0 : 1;
}
