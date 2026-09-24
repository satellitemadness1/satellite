# satellite 004 -- one variable per directory.
#
# Every path in every later fragment is spelled through one of these, so a
# directory that moves is one edit here. The interpreter's sources are under
# satellite/, one folder a subject (the author, 2026-09-15).

# PGO_STAGE is the training build's, a make inside the ordinary one (045-optimise.mk).
BUILD      = $(if $(CPU),build/cpu/$(CPU),$(if $(PGO_STAGE),build/$(PGO_STAGE),build))
OBJECTS    = $(BUILD)/objects
SATELLITE  = satellite
NUMBERS    = satellite-numbers
MACHINE    = $(SATELLITE)/machine
ARGUMENTS  = $(SATELLITE)/arguments
BYTECODE   = $(SATELLITE)/bytecode
OBJECT     = $(SATELLITE)/satellite_object
NUMBER     = $(SATELLITE)/satellite_variable_number
STRING16   = $(SATELLITE)/satellite_variable_string
BINARY     = $(SATELLITE)/satellite_variable_binary
PERCENTAGE = $(SATELLITE)/satellite_variable_percentage
FLOAT_DIR  = $(SATELLITE)/satellite_variable_float
HEX_DIR    = $(SATELLITE)/satellite_variable_hex
COLOR_DIR  = $(SATELLITE)/satellite_variable_color
FRACTION_DIR = $(SATELLITE)/satellite_variable_fraction
INFINITY   = $(SATELLITE)/satellite_variable_infinity
WINDOW_DIR = $(SATELLITE)/satellite_variable_window
VERSION_DIR = $(SATELLITE)/version
PROMPT     = $(SATELLITE)/prompt
STRINGS32  = strings
