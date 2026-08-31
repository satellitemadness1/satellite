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
# the kernel for an 8 GiB stack at startup (machine_limits/limits.hpp's
# kWantedStackBytes) because `ulimit -s`'s 8 MiB is a shell's soft DEFAULT with
# an unlimited hard limit behind it -- not a kernel wall, and raisable without
# root. Measured the same way, STATIC=full:
#
#     satl --version      0.734 ms, share 0.186   (was 0.179)
#
# +0.007 ms, which is inside the run-to-run noise on a machine at load 1.5 and is
# one setrlimit(2). What it buys: 500,000 nested brackets parse, unparse and
# write a `.satc` where 19,000 used to segfault.
#
# IT COSTS NO MEMORY AND DOES NOT MULTIPLY ACROSS THE POOL, both measured. A
# stack is lazily committed, so the reservation moved VmSize by 0.0 MiB; and
# glibc fixes the default stack size for NEW threads at library init, before
# main() runs, so M6's 24 pool threads still take 8 MiB each -- VmSize is 230.3
# MiB with the raise and 230.3 without.

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
            $(PROGRAMS)/opening.cpp \
            $(PROGRAMS)/window_handover.cpp \
            $(PROGRAMS)/source_file.cpp \
            $(PROGRAMS)/cache_command.cpp \
            $(PROGRAMS)/check_command.cpp \
            $(PROGRAMS)/dump_commands.cpp \
            $(PROGRAMS)/file_commands.cpp \
            $(PROGRAMS)/limits_command.cpp \
            $(PROGRAMS)/resolve_command.cpp \
            $(ERRORS)/report.cpp \
            $(ERRORS)/suggest.cpp \
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
            $(SYSTEM)/memory_facts.cpp \
            $(SYSTEM)/host_facts.cpp \
            $(SYSTEM)/stack_facts.cpp \
            $(STRING)/satellite_string.cpp \
            $(TREE)/ast.cpp \
            $(TREE)/unparse.cpp \
            $(WORDS)/dump.cpp

SATL_OBJS = $(SATL_SRCS:.cpp=.o)

# Every header any object depends on. Listed rather than generated: -MMD would
# do this automatically and is the obvious answer, but it writes .d files into
# the tree and makes a from-scratch build depend on files a clean has removed.
# This list is short and stays short if it is maintained; when it stops being
# either, revisit that decision on purpose rather than by drift.
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
       $(PROGRAMS)/resolve_command.hpp \
       $(PROGRAMS)/opening.hpp \
       $(PROGRAMS)/source_file.hpp \
       $(PROGRAMS)/terminal.hpp \
       $(PROGRAMS)/window_handover.hpp \
       $(RANDOM)/random.hpp \
       $(STRING)/satellite_string.hpp \
       $(WORDS)/words.def \
       $(WORDS)/words.hpp \
       $(WORDS)/words_nodes.hpp \
       $(WORDS)/words_numbers.hpp \
       $(WORDS)/words_spellings.hpp \
       $(WORDS)/words_walk.hpp \
       $(WORDS)/words_invariants.hpp \
       $(WORDS)/words_digest.hpp \
       $(WORDS)/words_runtime.hpp \
       $(WORDS)/dump.hpp

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
# COMPILED BY `all` AND LINKED INTO NOTHING, which is a deliberate exception to
# this project's own rule and is written here rather than left to be discovered.
# PLAN M2 says the registry gets a consumer in the milestone that writes it,
# because the first satellite shipped three commits where it had none. This
# module has no consumer for a different reason: the thing that would call it is
# `satellite.random.*`, which reaches no milestone at all (SCRATCH.md/MILESTONE.md
# §0.1 counts its 16 paths), and the thing it would FEED -- drawing an N-digit
# number -- needs the arbitrary-precision half that M8 has not ported yet.
#
# Compiling it under `all` is the cheapest thing that stops it rotting: a header
# change or a compiler upgrade breaks the build rather than breaking silently
# months later.
#
# THE HARNESS LANDED AT M2 AND THIS MODULE STILL HAS NO TEST. This paragraph
# used to end "there is no test target in this tree, and porting the first
# satellite's harness is the job that would give this module a genuine
# consumer" -- 065-tests.mk is that harness and TESTNAMES is three fragments
# away, so the reason is now simply that nobody has written a random_test.
# Said plainly, because the old sentence deferred the work behind a blocker
# that no longer exists and a reader would have believed it.
RANDOM_SRCS = $(RANDOM)/random.cpp

RANDOM_OBJS = $(RANDOM_SRCS:.cpp=.o)

OBJS = $(SATL_OBJS) $(SATL_HASWELL_OBJS) $(CPU_LEVEL_OBJ) $(TERM_OBJS) $(RANDOM_OBJS)
