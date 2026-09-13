# satellite -- what gets compiled, and what gets linked into what.
#
# ONE FILE PER NAME, spelled out rather than wildcarded. A wildcard here would
# quietly compile a file somebody is part way through writing, and quietly stop
# compiling one that got renamed; this list fails loudly on the first and
# noisily on the second.

# satl -- the interpreter. Links NO GUI, and that is measured rather than tidy.
#
# MEASURED HERE, 2026-08-26, rather than quoted. The first satellite's source
# says gtk4 and vte pull "119 shared objects"; on this machine its satl-term
# resolves 79 and maps 78 (`ldd | wc -l`, and LD_DEBUG=libs counted by "calling
# init:"). 119 is not this machine's number and is not repeated. What holds is
# the shape of it -- 78 against satl's 6 -- and the cost, which the first
# satellite measured at 25.9 ms with the link against 2.5 ms without.
#
# This build, best of five runs of 200 invocations:
#
#     bare int main(){return 0;}          1.74 ms
#     satl (M1, opening information)      1.75 ms
#     satl --version                      1.75 ms
#
# So satl's own share of starting up is about 0.01 ms, which is the whole point
# of taking the measurement now: every later milestone has a floor to be
# compared against, and a regression has somewhere to be attributed.
#
# RE-TAKEN AT M3, 2026-08-29, load 0.34, best of five runs of 200 -- the floor
# above being used, which is what it was written for. satl gained three objects:
# the lexer, its dump, and satellite_string.
#
# MEASURED AGAINST A STATIC BARE BINARY THIS TIME, and the first attempt got
# that wrong in a way worth recording. satl is shipped STATIC (048-static.mk),
# and a static binary skips the dynamic loader entirely -- so timing it against
# the DYNAMIC `int main(){return 0;}` above made satl look 0.9 ms FASTER than an
# empty program. That is a linking difference wearing a performance result's
# clothes. Both sides static:
#
#     bare int main(){return 0;}          0.606 ms
#     satl (opening information)          0.618 ms
#     satl --version                      0.624 ms
#     satl --words          (271 lines)   0.766 ms
#     satl --tokens hello_world.satl      0.678 ms
#     satl --tokens class_test.satl       0.840 ms   (255 tokens, the largest)
#
# So satl's own share of starting up is about 0.012 ms, which REPRODUCES M1's
# 0.01 ms rather than merely resembling it -- three milestones and three objects
# later. Reading and lexing a 273-byte program costs about 0.06 ms on top; the
# largest example in the tree costs 0.22 ms.
#
# The 2026-08-26 figures above are dynamic and are left as they were taken. They
# are not comparable to these and are not restated as if they were.
#
# RE-TAKEN AT M6, 2026-08-31, load 0.98, best of five runs of 200, both sides
# static -- and this is the re-measurement that found something. It is also the
# FIRST ONE SINCE M3: M4, M4.5 and M5 each landed on 2026-08-30 without one,
# which PLAN §9 asks for every milestone and nobody did. So the table covers
# four milestones, and the attribution below is what says which of them moved
# the number.
#
#     bare int main(){return 0;}          0.552 ms   (M3: 0.606)
#     satl (opening information)          1.174 ms   (M3: 0.618)
#     satl --version                      1.172 ms   (M3: 0.624)
#     satl --words          (271 lines)   1.354 ms   (M3: 0.766)
#     satl --tokens hello_world.satl      1.259 ms   (M3: 0.678)
#     satl --tokens class_test.satl       1.560 ms   (M3: 0.840)
#     satl --limits                       1.899 ms   (new at M6)
#     satl --limits example/satellite_config.ini
#                                         1.931 ms   (new at M6)
#
# SATL'S OWN SHARE OF STARTING UP WENT FROM 0.018 ms TO 0.620 ms, WHICH IS 34
# TIMES THE FIGURE M1 TOOK AND M3 REPRODUCED, and all of it is M6's: the bare
# binary got 0.054 ms FASTER between the two dates, so the machine is not what
# moved. `satl --version` is the row to read -- it does nothing but say what it
# is, and it now takes twice as long as an empty program.
#
# WHERE IT GOES. Measured the same way, one process per piece and cumulative,
# so that thread teardown lands on the clock a shell loop uses rather than the
# one the main thread experiences:
#
#     bare int main(){return 0;}                    0.585 ms
#     + the config lookup (readlink, access)        0.584 ms   +0.000
#     + hardware_threads(), mem_total_bytes() x2    0.621 ms   +0.037
#     + physical_cores()                            1.044 ms   +0.423
#     + the pool builder and the watchdog           1.274 ms   +0.230
#
# TWO THIRDS OF IT IS physical_cores(), AND NOTHING AT M6 READS WHAT IT
# ANSWERS. It walks /sys/devices/system/cpu/cpu*/topology/ at two files per
# CPU -- 48 opens on this 24-thread machine, about 7.6 us each -- and the only
# consumer of the count is the CORE_COUNT row `satl --limits` prints. That
# command pays it TWICE, in limits.cpp's from_the_machine() and again in
# dump.cpp's machine block; every other command in the program pays it once for
# nothing. Timed directly against these same objects: hardware_threads() 1.5 us,
# physical_cores() 367.7 us, mem_total_bytes() 11.4 us, mem_available_bytes()
# 11.5 us, process_memory_bytes() 7.4 us, stack_limit_bytes() 0.5 us.
#
# AND THE POOL COSTS 0.14 ms WHERE PLAN §4.5.1.2 SAYS ~20 us, WHICH IS ONE
# NUMBER READ AGAINST TWO CLOCKS RATHER THAN A WRONG ONE. That section is right
# about the main thread: spawning one builder costs it ~20 us instead of ~590,
# and strace confirms two clone3 calls and not twenty-four, because the process
# is gone long before the builder has made the other 23. The PROCESS pays for
# them anyway. N detached threads that park and never run, measured here:
#
#     0: 0.561 ms   1: 0.648   2: 0.702   4: 0.770
#     8: 0.855      16: 1.053  24: 1.318
#
# -- the first thread costs 87 us and every one after it 28 us, which
# reproduces config_internal.hpp's ~25.6 us as the MARGINAL cost and adds the
# one-time price of a process becoming threaded at all. satl starts two, the
# pool builder and the watchdog, for 0.14 ms. §4.3's floor is wall clock per
# invocation, and a thread the kernel must tear down before the parent's wait()
# returns is on that clock whether or not the main thread waited for it.
#
# AND RE-TAKEN AGAIN THE SAME DAY, WITH THE FINDING FIXED. The table above
# stands as it was measured; this is what satl costs once physical_cores() stops
# being read on every run. Same method, load 0.78.
#
#     bare int main(){return 0;}          0.556 ms   (was 0.552)
#     satl (opening information)          0.735 ms   (was 1.174)
#     satl --version                      0.724 ms   (was 1.172)
#     satl --words          (271 lines)   0.887 ms   (was 1.354)
#     satl --tokens hello_world.satl      0.818 ms   (was 1.259)
#     satl --tokens class_test.satl       1.128 ms   (was 1.560)
#     satl --limits                       1.462 ms   (was 1.899)
#
# SATL'S OWN SHARE IS 0.168 ms, DOWN FROM 0.620, and `satl --version` now opens
# NO FILES AT ALL -- strace counts zero openat against fifty before it, because
# the config lookup is an access() and everything else was the machine being
# read for a row nobody had asked for. What remains is 0.14 ms of pool builder
# and watchdog, which is PLAN §4.5.1.2's decision costing what §4.5.1.2 decided
# to spend, and about 0.03 ms of everything else. It is still 9x M3's 0.018 ms
# and every microsecond of the difference is now a thread that was started on
# purpose.
#
# WHAT CHANGED IS THE FILE FORMAT AND NOT A CACHE. satellite_config.ini can now
# say `CORE_COUNT=arguments.machine.cores` -- DESIGN §7.7's own pairing of
# the three settings with the three paths -- so a value is either a number
# somebody wrote or the machine's own answer, resolved when something asks for
# it. limits.cpp used to fill all three in from the machine BEFORE opening the
# file, because the file had no way to say "the machine"; that order is gone and
# with it the read. MILESTONES/M6.md §9.4.
#
# `satl --limits` KEPT 0.42 ms OF ITS OWN, separately: it was asking the machine
# for the same three facts twice, once for the settings block and once for the
# machine block. dump.cpp reads each once now, which is not a saving so much as
# the difference between reporting a machine and reporting it from two different
# instants.
#
# `make startup` TAKES THIS TABLE NOW, as of 2026-08-31, and every figure above
# was taken by hand. The harness is 067-startup.mk, startup.rows and startup.sh,
# and PLAN §9's rule -- "startup is re-measured every milestone" -- had no target
# behind it until then, which is why the M6 block above covers four milestones.
# The baseline it diffs against is the LAST table above, the one after the fix;
# startup.rows says why it is that one and not the one before it. This block
# stays here and stays prose, because §9's other rule is that a number goes
# beside the decision it justifies and the decisions are in this file. What moved
# into startup.rows is only the part a program has to read.
#
# AND BUILDING IT CORRECTED "BOTH SIDES STATIC", WHICH IS THIS BLOCK'S OWN
# INSTRUCTION. That line is M3's, written after a static satl timed against a
# dynamic empty program came out 0.9 ms FASTER than doing nothing. It is right,
# and it is not the fact underneath it. Measured 2026-08-31:
#
#                        floor   satl --version   satl's own share
#     dynamic            1.457        1.633             0.176
#     static             0.568        0.755             0.187
#     the table above    0.556        0.724             0.168
#
# The absolute columns are 0.9 ms apart. THE SHARE IS NOT. The loader is a
# constant this build pays TWICE -- once in the floor and once in satl -- so
# subtracting the floor removes it, and what is left is the number a milestone is
# answerable for. The rule is BOTH SIDES LINKED THE SAME WAY, and 067-startup.mk
# links its floor through the same $(CXX) $(CXXFLAGS) $(LDFLAGS)
# $(STATIC_LDFLAGS) $(LINK_ENV) that 050-build.mk links satl through -- so at any
# setting of STATIC, and not only at full, the two sides cannot drift apart. That
# is stronger than an instruction: there is no second place left to get it wrong.
# Run at STATIC=full the harness reproduces the table above row for row -- floor
# 0.567, --version 0.764, --words 0.915, --tokens 0.843 and 1.144, --limits 1.481.
#
# RE-TAKEN AT M7, 2026-08-31, load 1.50, STATIC=full -- and this is the first
# re-measurement `make startup` took rather than a person. 067-startup.mk is the
# target and startup.rows is the baseline it diffed against, which is the table
# immediately above this paragraph.
#
#     bare int main(){return 0;}          0.546 ms   (M6: 0.556)
#     satl (opening information)          0.719 ms   (M6: 0.735)
#     satl --version                      0.725 ms   (M6: 0.724)
#     satl --words                        0.902 ms   (M6: 0.887)
#     satl --tokens hello_world.satl      0.819 ms   (M6: 0.818)
#     satl --tokens class_test.satl       1.121 ms   (M6: 1.128)
#     satl --limits                       1.457 ms   (M6: 1.462)
#     satl --limits example/satellite_config.ini
#                                         1.490 ms   (never measured before)
#     satl --resolve hello_world.satl     0.856 ms   (new at M7)
#     satl --resolve frames.satl          1.113 ms   (new at M7)
#
# SATL'S OWN SHARE IS 0.179 ms AGAINST M6's 0.168, WHICH IS 0.011 ms AND IS SIX
# OBJECTS. The resolver does not run unless `--resolve` asks for it, so
# `--version` cannot be paying for the pass; what moved is the size of the image
# the loader maps -- name_resolver/ is six translation units and programs/ gained
# three more when main.cpp's arms were split out. Nothing else changed, and every
# other row is inside the noise of a machine at load 1.50 against M6's 0.78.
#
# `satl --resolve` COSTS 0.13 ms MORE THAN `--tokens` ON THE SAME FILE, which is
# the parse, the resolve and SATC.md §4's reading order together. It is the first
# row in this table that is a whole pipeline rather than one pass.
#
# AND IT IS A WARM NUMBER. startup.sh runs 200 invocations per batch and the
# first of them writes the `.satc` the other 199 read, so a cold run is averaged
# in at one part in two hundred. What the cache actually saves is counted rather
# than timed -- MILESTONES/M4.5.md §5's clause could never have been read off a
# clock at this size, and MILESTONES/M7.md §5 is why.

# AND THE STACK WAS WIDENED ON 2026-08-31, WHICH COST ONE SYSCALL. satl now asks
# the kernel for a bigger stack at startup (machine_limits/limits.hpp) because
# `ulimit -s`'s 8 MiB is a shell's soft DEFAULT with an unlimited hard limit
# behind it -- not a kernel wall, and raisable without root. Measured the same
# way, STATIC=full:
#
#     satl --version      0.734 ms, share 0.186   (was 0.179)
#
# +0.007 ms, which is inside the run-to-run noise on a machine at load 1.5 and is
# one setrlimit(2). What it buys: 500,000 nested brackets parse and unparse where
# 19,000 used to segfault.
#
# RE-TAKEN AT M8, 2026-08-31, load 0.88, STATIC=full, `make startup` against
# startup.rows' M7 baseline. Two new rows, and the two of them are the point.
#
#     bare int main(){return 0;}          0.555 ms   (M7: 0.546)
#     satl (opening information)          0.754 ms   share 0.199  (M7: 0.171)
#     satl --version                      0.759 ms   share 0.204  (M7: 0.186)
#     satl --words                        0.945 ms   share 0.390  (M7: 0.354)
#     satl --tokens hello_world.satl      0.856 ms   share 0.301  (M7: 0.271)
#     satl --tokens class_test.satl       1.154 ms   share 0.599  (M7: 0.573)
#     satl --limits                       1.493 ms   share 0.938  (M7: 0.909)
#     satl --resolve hello_world.satl     0.893 ms   share 0.338  (M7: 0.308)
#     satl --resolve frames.satl          1.147 ms   share 0.592  (M7: 0.565)
#     satl --number 1 + 1                 0.769 ms   share 0.214  (new at M8)
#     satl --number 1 / 3                 0.786 ms   share 0.231  (new at M8)
#
# SATL'S OWN SHARE IS 0.204 ms AGAINST M7's 0.186, AND IT IS THE IMAGE AGAIN --
# WITH ONE PIECE OF EVIDENCE M7's ATTRIBUTION DID NOT HAVE. Every row moved by
# about the same amount, and the row that moved is the BARE `satl`, which prints
# its opening information and never reaches an arm. A cost paid before a command
# is chosen cannot be that command's arithmetic. What changed is the size of the
# image: satl at STATIC=full went from 1,828,776 bytes to 1,887,016, which is
# +58,240 for the six satellite_number objects and number_command.o.
#
# AND M8's ARITHMETIC IS UNDER THE RUN-TO-RUN NOISE, WHICH THE TWO NEW ROWS ARE
# THERE TO SAY. `--number 1 + 1` is 0.010 ms over `--version`: reading two
# operands, one add with an overflow check, and five printed lines. `1 / 3` is
# 0.017 ms over that, which is long division to 34 significant digits plus the
# one allocation the answer boxes into. The harness was run three times on this
# build and the spread between runs was 0.027 ms -- LARGER than either figure --
# so what these rows establish is a bound and not a measurement, and they are
# recorded as one. DESIGN §8.2's claim that the small case never allocates is
# checked where it can be checked exactly, by counting bytes:
# tests/number_test/limbs.cpp and arithmetic.cpp both assert payload_bytes().
#
# AND IT BECAME A SHARE OF THE MACHINE THAT EVENING, WHICH COST ONE READ OF
# /proc/meminfo. The number stopped being 8 GiB and became 32 KiB of stack for
# every MiB of memory -- so every run of satl now calls mem_total_bytes() before
# it does anything else, `satl --help` included. Timed directly rather than
# inferred, because `make startup` cannot see it: 0.05 ms for the one cold read
# a run makes, 0.013 ms warm, against the 0.42 ms physical_cores() costs and
# which limits.cpp's header refuses to pay on every run. A tenth of the price
# the same file already turned down, for a number every run needs.
#
# IT COSTS NO MEMORY AND DOES NOT MULTIPLY ACROSS THE POOL, both measured. A
# stack is lazily committed, so the reservation moved VmSize by 0.0 MiB; and
# glibc fixes the default stack size for NEW threads at library init, before
# main() runs, so M6's 24 pool threads still take 8 MiB each -- VmSize is 230.3
# MiB with the raise and 230.3 without.

# RE-TAKEN AT M10, 2026-09-02, load 0.67, STATIC=full, `make startup` against
# startup.rows' M8 baseline -- AND THAT BASELINE IS THREE MILESTONES OLD, WHICH
# IS THE FIRST THING TO SAY ABOUT THE TABLE. M8.5 and M9 both landed without
# running this target, which is §9's rule going unrun again exactly the way M4,
# M4.5 and M5 did before the target existed. So every delta below covers M8.5,
# M9 AND M10 together.
#
#     bare int main(){return 0;}          0.557 ms   (M8: 0.565)
#     satl (opening information)          0.779 ms   share 0.222  (M8: 0.239)
#     satl --version                      0.784 ms   share 0.227  (M8: 0.234)
#     satl --words                        0.958 ms   share 0.401  (M8: 0.418)
#     satl --tokens hello_world.satl      0.874 ms   share 0.317  (M8: 0.327)
#     satl --tokens class_test.satl       1.169 ms   share 0.612  (M8: 0.618)
#     satl --limits                       1.508 ms   share 0.951  (M8: 0.965)
#     satl --limits example/...ini        1.519 ms   share 0.962  (M8: 0.971)
#     satl --resolve hello_world.satl     0.923 ms   share 0.366  (M8: 0.374)
#     satl --resolve frames.satl          1.191 ms   share 0.634  (M8: 0.635)
#     satl --number 1 + 1                 0.792 ms   share 0.235  (M8: 0.237)
#     satl --number 1 / 3                 0.812 ms   share 0.255  (M8: 0.259)
#     satl --compile frames.satl          1.106 ms   share 0.549  (M8: 0.546)
#     satl --call frames.satl factorial 10
#                                         1.107 ms   share 0.550  (M8: 0.521)
#
# SATL'S OWN SHARE IS 0.222 ms AGAINST M8's 0.239, AND IT WENT DOWN ACROSS THREE
# MILESTONES THAT ADDED AN EVALUATOR, A VALUE MODEL AND A CONSOLE. Every earlier
# re-measurement in this file found the share going UP by the size of the image
# -- M7 by 0.011 for six objects, M8 by 0.018 for seven -- and the attribution
# was always the same: a cost paid before a command is chosen turns up in the
# BARE `satl` row, which runs no arm at all. That row is what moved this time,
# in the other direction, by about the same amount every other row moved. The
# honest reading is that this is a quieter machine (load 0.67 against 0.88) and
# not that M8.5, M9 and M10 made satl faster; what the table establishes is a
# BOUND -- three milestones and 12 new translation units cost nothing this
# measurement can see -- and it is recorded as one, the same way M8's own two
# arithmetic rows were.
#
# THE ONE CLAIM IT DOES MAKE IS ABOUT THE CONSOLE, and it is a negative. M10
# adds a module that starts a THREAD, which is the kind of thing §4.5.1.2's 0.14
# ms of pool builder is already on this table for. It does not appear, because
# `Console::the()` is a function-local static and the printer is made inside
# start_held() -- reached from programs/run_command.cpp and from the queue, and
# from nowhere a `satl --version` can get to. If a console were being built
# before an arm was chosen, the bare `satl` row is precisely where it would
# show, and that row is the one that came DOWN.
#
# AND THE ROW FOR RUNNING A PROGRAM IS OWED RATHER THAN IMPOSSIBLE. When this
# table was taken, M10's own command had no file in `example/` it could run --
# all five programs there declare `satellite.main`'s parameter, which is M16's,
# so they answer S0720 and exit 3. The author wrote `example/console.satl` and
# `example/bare_main.satl` the same day, so what is missing now is a run of the
# harness rather than a file. startup.rows says what that row is expected to
# cost, and why writing the prediction down first is the point.
# *(The row was taken at M11, one milestone late and with the prediction
# upheld -- startup.rows' M11 section is the account.)*

# RE-TAKEN AT M11, 2026-09-03, load 0.93, STATIC=full, against the M10 baseline
# above -- ONE MILESTONE THIS TIME, which is §9's rule actually being kept.
# Every delta is inside the noise in both directions: satl's own share is 0.229
# against M10's 0.222, `--version` 0.223 against 0.227, and the widest move in
# the table is `--limits` at -0.023. What M11 added to the binary is a new
# module (satellite_scalars/, three translation units), the interrupt handler,
# and two evaluator arms -- and none of it runs before an arm is chosen, so the
# bare `satl` row stayed where it was, which is the M8 finding holding for the
# third milestone running.
#
#     bare int main(){return 0;}          0.541 ms   (M10: 0.557)
#     satl (opening information)          0.770 ms   share 0.229  (M10: 0.222)
#     satl --version                      0.764 ms   share 0.223  (M10: 0.227)
#     satl example/console.satl           1.002 ms   share 0.461  (M10: owed)
#     satl example/scalars.satl           1.352 ms   share 0.811  (M10: --)
#
# THE TWO NEW ROWS ARE THE MILESTONE'S. `example/console.satl` pays M10's debt
# and answers its prediction -- a whole run, printer thread and four-step
# shutdown included, UNDERRUNS `--call frames.satl`'s 0.564 because the program
# is smaller and the console costs about what one std::thread costs.
# `example/scalars.satl` is M11's consumer: fourteen displays, three loops,
# both module constants and a method from each family over 25 cached dispatch
# sites, at 0.811. The full table and both accounts are startup.rows'.

# The window is a separate binary (M1.5, built 2026-08-27) and, for
# satellite.window.new(), a
# dlopen'd library (M24) -- because the two-binary split cannot help a window
# opened from inside a user program, which runs in this one. PLAN_ONE.md sec 4.4.
#
# $(RESOLVE) IS SIX TRANSLATION UNITS AND READS $(CACHE), which is the one
# dependency in this list that looks backwards and is not. M4.5's
# satellite_cache/paths.cpp walks a postfix chain against the numbering, and
# DESIGN §6.3 says that walk is M7's -- so the milestone that owns the walk reads
# the file that already had it rather than writing a second one.
# name_resolver/numbers.cpp opens with the argument; paths.hpp's "THIS IS NOT
# RESOLVE AND MUST NOT BECOME IT" is about growing a receiver argument, which
# reading it is not.
#
# $(WORDS)/dump.cpp IS THE ONLY .cpp THE WORD REGISTRY HAS, and that is a
# property of the module rather than an omission. Everything else under
# satellite_words/ is constexpr data and pure functions over it, so a future
# .satc reader or disassembler can read the numbering without linking anything
# -- which was true of the first satellite's registry and is worth keeping. The
# one file that prints is the one that had to be a translation unit.
SATL_SRCS = $(PROGRAMS)/main.cpp \
            $(PROGRAMS)/arms.cpp \
            $(PROGRAMS)/opening.cpp \
            $(PROGRAMS)/window_handover.cpp \
            $(PROGRAMS)/source_file.cpp \
            $(PROGRAMS)/cache_command.cpp \
            $(PROGRAMS)/check_command.cpp \
            $(PROGRAMS)/source_report.cpp \
            $(PROGRAMS)/dump_commands.cpp \
            $(PROGRAMS)/file_commands.cpp \
            $(PROGRAMS)/limits_command.cpp \
            $(PROGRAMS)/built_program.cpp \
            $(PROGRAMS)/evaluate_commands.cpp \
            $(PROGRAMS)/run_command.cpp \
            $(PROGRAMS)/number_command.cpp \
            $(PROGRAMS)/resolve_command.cpp \
            $(DIAGNOSE)/diagnose.cpp \
            $(DIAGNOSE)/suit_cycles.cpp \
            $(ERRORS)/report.cpp \
            $(ERRORS)/suggest.cpp \
            $(ERRORS)/foreign.cpp \
            $(ERRORS)/dump.cpp \
            $(LEXER)/lexer.cpp \
            $(LEXER)/dump.cpp \
            $(PARSER)/parser.cpp \
            $(PARSER)/parser_declarations.cpp \
            $(PARSER)/parser_statements.cpp \
            $(PARSER)/parser_control_flow.cpp \
            $(PARSER)/parser_expressions.cpp \
            $(PARSER)/parser_types.cpp \
            $(RESOLVE)/resolve.cpp \
            $(RESOLVE)/scopes.cpp \
            $(RESOLVE)/walk.cpp \
            $(RESOLVE)/names.cpp \
            $(RESOLVE)/numbers.cpp \
            $(RESOLVE)/dump.cpp \
            $(CACHE)/paths.cpp \
            $(CACHE)/write.cpp \
            $(CACHE)/write_declarations.cpp \
            $(CACHE)/write_expressions.cpp \
            $(CACHE)/read.cpp \
            $(CACHE)/unnumber.cpp \
            $(CACHE)/save.cpp \
            $(CACHE)/file.cpp \
            $(LIMITS)/limits.cpp \
            $(LIMITS)/config.cpp \
            $(LIMITS)/pool.cpp \
            $(LIMITS)/watchdog.cpp \
            $(LIMITS)/dump.cpp \
            $(SYSTEM)/arguments_facts.cpp \
            $(SYSTEM)/memory_facts.cpp \
            $(SYSTEM)/firmware_facts.cpp \
            $(SYSTEM)/host_facts.cpp \
            $(SYSTEM)/stack_facts.cpp \
            $(SYSTEM)/user_facts.cpp \
            $(SYSTEM)/interrupt.cpp \
            $(NUMBER)/limbs.cpp \
            $(NUMBER)/number_core.cpp \
            $(NUMBER)/number_query.cpp \
            $(NUMBER)/number_arith.cpp \
            $(NUMBER)/render.cpp \
            $(NUMBER)/random.cpp \
            $(BITS)/bits.cpp \
            $(FLOAT)/float_value.cpp \
            $(FLOAT)/float_arith.cpp \
            $(FLOAT)/float_power.cpp \
            $(STRING)/satellite_string.cpp \
            $(VALUE)/value.cpp \
            $(VALUE)/render.cpp \
            $(CONSOLE)/console.cpp \
            $(CONSOLE)/reader.cpp \
            $(CONSOLE)/handlers.cpp \
            $(SCALARS)/handlers.cpp \
            $(SCALARS)/string_methods.cpp \
            $(SCALARS)/number_methods.cpp \
            $(SCALARS)/variant_methods.cpp \
            $(SCALARS)/bits_methods.cpp \
            $(SCALARS)/hex_methods.cpp \
            $(SCALARS)/float_methods.cpp \
            $(SCALARS)/conversions.cpp \
            $(CONTAIN)/bodies.cpp \
            $(CONTAIN)/search_score.cpp \
            $(CONTAIN)/search_walk.cpp \
            $(CONTAIN)/handlers.cpp \
            $(CONTAIN)/list_methods.cpp \
            $(CONTAIN)/list_sorting.cpp \
            $(CONTAIN)/map_methods.cpp \
            $(SYSLIB)/handlers.cpp \
            $(SYSLIB)/units.cpp \
            $(SYSLIB)/group_map.cpp \
            $(SYSLIB)/memory_methods.cpp \
            $(SYSLIB)/host_methods.cpp \
            $(ARGS)/arguments.cpp \
            $(ARGS)/rows.cpp \
            $(ARGS)/render.cpp \
            $(ARGS)/handlers.cpp \
            $(ARGS)/selectors.cpp \
            $(SATFILE)/file_handle.cpp \
            $(SATFILE)/handlers.cpp \
            $(SATFILE)/file_methods.cpp \
            $(SATFILE)/file_reading.cpp \
            $(SATFILE)/gzip.cpp \
            $(DIRECTRY)/handlers.cpp \
            $(DIRECTRY)/listing.cpp \
            $(HELP)/built.cpp \
            $(HELP)/render.cpp \
            $(HELP)/handlers.cpp \
            $(RANDOM)/random.cpp \
            $(RANDOM)/tiers.cpp \
            $(RANDOM)/handlers.cpp \
            $(RANDOM)/seeded.cpp \
            $(THREAD)/thread_handle.cpp \
            $(THREAD)/handlers.cpp \
            $(TIME)/time.cpp \
            $(TIME)/handlers.cpp \
            $(EVAL)/evaluate.cpp \
            $(EVAL)/compile.cpp \
            $(EVAL)/compile_expressions.cpp \
            $(EVAL)/compile_statements.cpp \
            $(EVAL)/machine.cpp \
            $(EVAL)/operations.cpp \
            $(EVAL)/operations_control.cpp \
            $(EVAL)/operations_dispatch.cpp \
            $(EVAL)/operations_subscript.cpp \
            $(EVAL)/dispatch.cpp \
            $(EVAL)/dump.cpp \
            $(TREE)/ast.cpp \
            $(TREE)/unparse.cpp \
            $(TREE)/unparse_declarations.cpp \
            $(TREE)/unparse_expressions.cpp \
            $(WORDS)/dump.cpp \
            $(PROMPT)/raw_mode.cpp \
            $(PROMPT)/keys.cpp \
            $(PROMPT)/history.cpp \
            $(PROMPT)/editor.cpp \
            $(PROMPT)/render.cpp \
            $(PROMPT)/line_reader.cpp \
            $(PROMPT)/block.cpp \
            $(PROMPT)/session.cpp \
            $(PROMPT)/prompt.cpp

SATL_OBJS = $(SATL_SRCS:.cpp=.o)

# THE ONE VENDORED LIBRARY THAT IS COMPILED RATHER THAN INCLUDED. pcg is headers
# and costs the build nothing; zlib is fifteen C files, and it is here because
# `satellite.file.open(path, "read_gzip")` has to inflate.
#
# VENDORED AND NOT LINKED FROM THE SYSTEM, WHICH IS FORCED BY STATIC=full. This
# machine has /usr/lib64/libz.so and no libz.a, so `-static` against the system
# zlib does not link at all -- and 048-static.mk's whole argument is that satl
# carries no dynamic dependencies. Building from source keeps that true and
# keeps it true on machines that have no zlib at all.
#
# ALL FIFTEEN AND NOT THE NINE READING NEEDS. adler32, crc32, inflate, inffast,
# inftrees, zutil, gzclose, gzlib and gzread would do for `read_gzip` today; the
# other six are deflate's side. Naming nine would mean editing this list the day
# somebody writes a `.gz`, and the difference is about 100KB against a 1.1MB
# binary -- 048-static.mk's table is what makes that trade checkable.
ZLIB_SRCS = $(ZLIB)/adler32.c $(ZLIB)/compress.c $(ZLIB)/crc32.c \
            $(ZLIB)/deflate.c $(ZLIB)/gzclose.c $(ZLIB)/gzlib.c \
            $(ZLIB)/gzread.c $(ZLIB)/gzwrite.c $(ZLIB)/infback.c \
            $(ZLIB)/inffast.c $(ZLIB)/inflate.c $(ZLIB)/inftrees.c \
            $(ZLIB)/trees.c $(ZLIB)/uncompr.c $(ZLIB)/zutil.c

ZLIB_OBJS = $(ZLIB_SRCS:.c=.o)

# Every header any object depends on. Listed rather than generated: -MMD would
# do this automatically and is the obvious answer, but it writes .d files into
# the tree and makes a from-scratch build depend on files a clean has removed.
# This list is short and stays short if it is maintained; when it stops being
# either, revisit that decision on purpose rather than by drift.
#
# help.def IS THE THIRD SUCH FILE, M18's, and it is the one that changes most
# often of the three: it is what `satellite.help` prints, it is generated from
# help_lines/, and help_text.hpp expands it twice. A build that did not depend
# on it would leave the old text in every object while the document beside it
# said something else -- which is the drift this milestone is about, arriving
# through the build system instead of through a second document.
#
# words.def IS IN THIS LIST AND IS NOT A HEADER, deliberately. It is included by
# six of the seven headers below and it is the file that actually changes when
# the language gains a word, so a build that did not depend on it would compile
# a stale numbering into every object -- silently, since the header it was
# expanded into would look untouched. errors.def is here for the same reason and
# is the second file of that kind: it is what changes when satl gains a message,
# and codes.hpp expands it five ways.
HDRS = $(SYSTEM)/version.hpp \
       $(SYSTEM)/facts.hpp \
       $(SYSTEM)/interrupt.hpp \
       $(SATFILE)/file_handle.hpp \
       $(SATFILE)/file_internal.hpp \
       $(SATFILE)/handlers.hpp \
       $(DIRECTRY)/handlers.hpp \
       $(DIRECTRY)/listing.hpp \
       $(ERRORS)/errors.def \
       $(ERRORS)/codes.hpp \
       $(ERRORS)/report.hpp \
       $(ERRORS)/suggest.hpp \
       $(ERRORS)/dump.hpp \
       $(LEXER)/lexer.hpp \
       $(LEXER)/lexer_chars.hpp \
       $(LEXER)/dump.hpp \
       $(PARSER)/parser.hpp \
       $(PARSER)/parser_internal.hpp \
       $(RESOLVE)/resolve.hpp \
       $(RESOLVE)/resolve_internal.hpp \
       $(RESOLVE)/dump.hpp \
       $(CACHE)/cache.hpp \
       $(CACHE)/paths.hpp \
       $(CACHE)/write_internal.hpp \
       $(LIMITS)/limits.hpp \
       $(LIMITS)/config_internal.hpp \
       $(LIMITS)/pool.hpp \
       $(LIMITS)/watchdog.hpp \
       $(LIMITS)/dump.hpp \
       $(TREE)/ast.hpp \
       $(TREE)/unparse.hpp \
       $(PROGRAMS)/cache_command.hpp \
       $(PROGRAMS)/check_command.hpp \
       $(PROGRAMS)/dump_commands.hpp \
       $(PROGRAMS)/file_commands.hpp \
       $(PROGRAMS)/limits_command.hpp \
       $(PROGRAMS)/evaluate_commands.hpp \
       $(PROGRAMS)/number_command.hpp \
       $(PROGRAMS)/built_program.hpp \
       $(PROGRAMS)/run_command.hpp \
       $(PROGRAMS)/resolve_command.hpp \
       $(PROGRAMS)/opening.hpp \
       $(PROGRAMS)/source_file.hpp \
       $(TERM_DIR)/menu.hpp \
       $(TERM_DIR)/tabs.hpp \
       $(TERM_DIR)/terminal.hpp \
       $(TERM_DIR)/child.hpp \
       $(TERM_DIR)/keys.hpp \
       $(PROGRAMS)/window_handover.hpp \
       $(NUMBER)/bignum.hpp \
       $(NUMBER)/bignum_bigint.hpp \
       $(NUMBER)/bignum_internal.hpp \
       $(NUMBER)/bignum_number.hpp \
       $(RANDOM)/random.hpp \
       $(RANDOM)/tiers.hpp \
       $(RANDOM)/handlers_internal.hpp \
       $(RANDOM)/handlers.hpp \
       $(THREAD)/thread_handle.hpp \
       $(THREAD)/handlers.hpp \
       $(TIME)/time.hpp \
       $(TIME)/handlers.hpp \
       $(STRING)/satellite_string.hpp \
       $(VALUE)/value.hpp \
       $(VALUE)/render.hpp \
       $(CONSOLE)/console.hpp \
       $(CONSOLE)/reader.hpp \
       $(CONSOLE)/handlers.hpp \
       $(BITS)/bits.hpp \
       $(SCALARS)/handlers.hpp \
       $(SCALARS)/methods_internal.hpp \
       $(CONTAIN)/containers.hpp \
       $(CONTAIN)/search.hpp \
       $(CONTAIN)/handlers.hpp \
       $(CONTAIN)/methods_internal.hpp \
       $(HELP)/help.def \
       $(HELP)/help_text.hpp \
       $(HELP)/built.hpp \
       $(HELP)/render.hpp \
       $(HELP)/handlers.hpp \
       $(EVAL)/closure.hpp \
       $(EVAL)/machine.hpp \
       $(EVAL)/globals.hpp \
       $(EVAL)/dispatch.hpp \
       $(EVAL)/evaluate.hpp \
       $(EVAL)/evaluator_internal.hpp \
       $(EVAL)/dump.hpp \
       $(WORDS)/words.def \
       $(WORDS)/words.hpp \
       $(WORDS)/words_nodes.hpp \
       $(WORDS)/words_numbers.hpp \
       $(WORDS)/words_spellings.hpp \
       $(WORDS)/words_walk.hpp \
       $(WORDS)/words_invariants.hpp \
       $(WORDS)/words_digest.hpp \
       $(WORDS)/words_runtime.hpp \
       $(WORDS)/dump.hpp \
       $(PROMPT)/raw_mode.hpp \
       $(PROMPT)/keys.hpp \
       $(PROMPT)/history.hpp \
       $(PROMPT)/editor.hpp \
       $(PROMPT)/render.hpp \
       $(PROMPT)/line_reader.hpp \
       $(PROMPT)/block.hpp \
       $(PROMPT)/session.hpp \
       $(PROMPT)/prompt.hpp

# Every object in the tree, which is what 060-compile.mk hangs the header
# dependency on. The haswell objects and the detector are named here rather than
# only where they are built, so that adding a header stays one edit in HDRS
# above and reaches every object rather than only the ones somebody remembered.
#
# $(SATL_HASWELL_OBJS) and $(CPU_LEVEL_OBJ) come from 045-microarchitecture.mk
# and $(TERM_OBJS) from 047-window.mk, both read after this file; recursive
# expansion is what makes that legal, and the top-level Makefile says so once
# for all the fragments. TERM_OBJS is empty on a machine with no gtk4, which is
# what keeps this line honest there rather than naming objects nothing builds.
# satellite.random -- DESIGN §11's three tiers, the 32-bit seam, and the spin.
#
# LINKED INTO `satl` SINCE M13, WHICH ENDS THE EXCEPTION THAT USED TO BE
# DOCUMENTED HERE. From M2 to M12 this paragraph said "compiled by `all` and
# linked into no binary `all` produces" -- the module landed ahead of any
# milestone that called it, `satellite.random.*` reached no milestone at all,
# and tests/number_test/draw.cpp was its only consumer, kept so a compiler
# upgrade would break the build rather than break silently months later. M13
# is the milestone the old sentence was waiting on: random.cpp's entry moved
# into SATL_SRCS above, tiers.cpp and handlers.cpp arrived beside it, and the
# consumer is the language now -- twelve rows in `handlers[path_id]`.
#
# What did NOT move: the bignum half of the draw is still
# satellite_number/random.cpp (M8's port), number_test still drives the Bits32
# seam with a splitmix32 stub, and random.o is still the ONE object that sees
# a third-party header -- 060-compile.mk's explicit -isystem rules, baseline
# and haswell, are where that is enforced.

OBJS = $(SATL_OBJS) $(SATL_HASWELL_OBJS) $(CPU_LEVEL_OBJ) $(TERM_OBJS)
