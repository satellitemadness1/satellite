# satellite -- where everything this build makes ends up.
#
# AFTER 020-inherited.mk, whose source lists every name below is derived from.
# BEFORE 050-build.mk, which links the object lists into the four binaries.
#
# This fragment declares NO TARGETS, deliberately, exactly like the parent's 030
# and 040. It says where files go; 050 and 060 are what put them there.

# TWO DIRECTORIES AND NOT ONE. The binaries are the output somebody asked for
# and the objects are how they were made, so the four names a person came here
# to run sit alone in build/ rather than in a drift of forty .o files:
#
#     build/satl  build/satl.haswell  build/satl-cpu-level  build/satl-term
#     build/objects/programs/main.o ... and the rest
#
# The stamps live in build/ rather than build/objects/, because they describe
# the whole build and not the objects alone -- .ldflags-stamp is about links.
# They are dotted, so `ls build/` shows the four binaries and the objects
# directory and no stamps -- which is the point: a person looking for what they
# built sees it, and has to ask with `ls -a` to see how it was built.
BUILD = build
OBJ   = $(BUILD)/objects

# THE SOURCE-TO-OBJECT MAP, which is the one thing this build does that the
# parent does not have to.
#
# The parent derives an object from a source by substituting a suffix --
# $(SATL_SRCS:.cpp=.o) -- and that is why its objects land beside its sources:
# a suffix substitution cannot move a path, only rename its tail. Out of tree
# the directory has to change too, so the map is a patsubst with $(SRC) on one
# side and $(OBJ) on the other, and the stem carries the module directory
# across unchanged:
#
#     ../src/programs/main.cpp   ->   build/objects/programs/main.o
#     ../src/satellite_words/dump.cpp -> build/objects/satellite_words/dump.o
#
# A FUNCTION AND NOT FIVE PATSUBSTS. There are five object lists below and they
# differ only in which sources they map, so the transformation is written once
# and called five times; a sixth list is one more call and not one more chance
# to spell $(OBJ) wrong.
#
# $(SRC) IS ../src BY THE TIME THIS IS CALLED, which is what makes the pattern
# match. 020-inherited.mk is one fragment away and is where that happens.
object_for  = $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(1))
haswell_for = $(patsubst $(SRC)/%.cpp,$(OBJ)/%.haswell.o,$(1))

# THE FIVE LISTS, EACH OVERRIDING A PARENT VARIABLE OF THE SAME NAME, and the
# names are kept rather than prefixed on purpose. $(OBJS) in 040-sources.mk is
#
#     OBJS = $(SATL_OBJS) $(SATL_HASWELL_OBJS) $(CPU_LEVEL_OBJ) $(TERM_OBJS) \
#            $(RANDOM_OBJS)
#
# and it is recursive, so redefining its five members redefines it: $(OBJS)
# below means this build's objects without a line here saying so, and the
# header dependency 060-compile.mk hangs on it is the parent's line unchanged.
# Renaming these to DEB_SATL_OBJS would have cost a sixth override and a reason
# to remember it.
#
# .haswell.o RATHER THAN A SECOND DIRECTORY, which is the parent's choice in
# 045-microarchitecture.mk and is kept for its reason: the pattern rules in
# 060-compile.mk stay one line each. It matters slightly more here, because a
# second tree of directories would double what 070-clean.mk has to name.
SATL_OBJS         = $(call object_for,$(SATL_SRCS))
SATL_HASWELL_OBJS = $(call haswell_for,$(SATL_SRCS))
CPU_LEVEL_OBJ     = $(call object_for,$(CPU_LEVEL_SRC))
TERM_OBJS         = $(call object_for,$(TERM_SRCS))
RANDOM_OBJS       = $(call object_for,$(RANDOM_SRCS))

# The four binaries, spelled once. 050-build.mk links them and 070-clean.mk
# removes them, and neither writes `build/satl` itself.
SATL         = $(BUILD)/satl
SATL_HASWELL = $(BUILD)/satl.haswell
CPU_LEVEL    = $(BUILD)/satl-cpu-level
SATL_TERM    = $(BUILD)/satl-term

# The three stamps, in build/ for the reason given at the top of this file.
# 040-static.mk writes the first and 060-compile.mk the other two; both are
# named here so that 070-clean.mk can remove them without knowing which
# fragment wrote which.
LDFLAGS_STAMP         = $(BUILD)/.ldflags-stamp
CXXFLAGS_STAMP        = $(BUILD)/.cxxflags-stamp
CXXFLAGS_STAMP_HASWELL = $(BUILD)/.cxxflags-stamp-haswell

# EVERY DIRECTORY THAT HAS TO EXIST, DERIVED AND NEVER LISTED. $(OBJS) already
# names every object this build can make, so the set of directories they live in
# is $(dir) over it -- which means adding a module directory to the language
# needs no edit here at all. Spelling them out would be a list that goes stale
# the first time src/ grows, and the failure would be a compiler reporting that
# it cannot open an output file in a directory nobody created.
#
# $(sort) both deduplicates and orders; $(dir) leaves a trailing slash, which
# patsubst takes off so that these are ordinary directory names and not
# `build/objects/programs/`. make will happily create a target with a trailing
# slash and then fail to match it against the same name written without one.
OBJ_DIRS = $(patsubst %/,%,$(sort $(dir $(OBJS))))
