# satellite 004 -- removing what a build made.
#
# THE WHOLE build/ FOLDER, as the old Makefile did: 004 writes every object,
# binary, library, stamp and log there and nothing else (.gitignore: "Nothing here
# is source"), which is exactly why 060-compile.mk puts objects under it rather
# than beside their sources the way 003 does.
#
# .satellite_build STAYS. It is the last build number used, and `make clean`
# followed by `make` is the same build, not a new one (build_number.py).
clean:
	rm -rf $(BUILD)

.PHONY: clean
