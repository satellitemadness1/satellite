// What a finished program reports back to the shell: satellite.return's value
// as an exit status, clamped rather than masked. Part of the interp_test
// binary; the harness it calls is declared in interp_test.hpp.

#include "interp_test.hpp"

#include "interpreter/interp.hpp"

#include <string>

using namespace satellite;

void interp_test_exit_status()
{
    // --- exit status -------------------------------------------------------
    // satellite.return(satellite) is success (§2).
    {
        InterpResult r = run_program("satellite.capsule satellite.main()\n"
                                     "{\n"
                                     "    satellite.return(satellite)\n"
                                     "}\n",
                                     {"prog"});
        check(r.ok && r.status == 0, "return(satellite) is status 0");
    }
    {
        InterpResult r = run_program("satellite.capsule satellite.main()\n"
                                     "{\n"
                                     "    satellite.return(3)\n"
                                     "}\n",
                                     {"prog"});
        check(r.ok && r.status == 3, "return(3) is status 3");
    }
    {
        InterpResult r = run_program("nope\n", {"prog"});
        check(!r.ok && r.status == 1, "a failing program is status 1");
    }
    // Clamped, not masked: `n & 0xff` would turn 256 into 0 and report success
    // for a program that said it failed.
    {
        InterpResult r = run_program("satellite.capsule satellite.main()\n"
                                     "{\n"
                                     "    satellite.return(256)\n"
                                     "}\n",
                                     {"prog"});
        check(r.status == 255, "return(256) clamps to 255, not 0");
    }
    {
        InterpResult r = run_program("satellite.capsule satellite.main()\n"
                                     "{\n"
                                     "    satellite.return(0 - 5)\n"
                                     "}\n",
                                     {"prog"});
        check(r.status == 1, "a negative status is failure, not success");
    }
}
