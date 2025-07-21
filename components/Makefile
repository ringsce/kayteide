# Makefile to compile Kayte Lang components with dynamic libraries

# Free Pascal Compiler executable
FPC = fpc

# Output directory for compiled units, object files, and the final executable/libraries
OUTDIR = build

# --- OS Detection (DO NOT MODIFY) ---
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    LIB_EXT = so
    TARGET_OS_FLAG = -Tlinux
else ifeq ($(UNAME_S),Darwin)
    LIB_EXT = dylib
    TARGET_OS_FLAG = -Tdarwin
else
    # Default for other OS, or error out if strict
    LIB_EXT = dll
    TARGET_OS_FLAG = -Twin32 # Placeholder for Windows, adjust if needed
    # $(error Unsupported OS: $(UNAME_S). Please add specific rules for it.)
endif
# --- End OS Detection ---


# List of directories containing your Pascal unit source files (.pas)
SOURCE_DIRS = source components

# The main program file (.lpr) that will be compiled into an executable
MAIN_PROGRAM = components.lpr

# Units that should be compiled into dynamic libraries
# These are the .pas files that will become lib<name>.so or lib<name>.dylib
LIB_UNIT_SOURCES = components/InputBox.pas components/MessageBox.pas components/ConsoleListView.pas

# Names of the dynamic libraries (without 'lib' prefix or extension)
# These names are used for linking (e.g., -lInputBox)
LIB_NAMES = InputBox MessageBox ConsoleListView

# --- Internal Variables (Do not modify below this line unless you know what you're doing) ---

# Find all .pas files in the specified source directories
ALL_PAS_SOURCES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.pas))

# Identify regular units (all .pas sources minus those designated as libraries)
REGULAR_UNIT_SOURCES = $(filter-out $(LIB_UNIT_SOURCES),$(ALL_PAS_SOURCES))

# Convert all .pas sources (regular and library) to their expected .ppu output paths in OUTDIR
ALL_PPU_TARGETS = $(patsubst %.pas,$(OUTDIR)/%.ppu,$(ALL_PAS_SOURCES))

# Convert library .pas sources to their expected dynamic library output paths in OUTDIR
ALL_LIB_TARGETS = $(patsubst components/%.pas,$(OUTDIR)/lib%.$(LIB_EXT),$(LIB_UNIT_SOURCES))

# FPC unit search paths (-Fu flags)
# -Fu.: Add the current directory to the unit search path (for components.lpr)
# -Fu$(OUTDIR): Add the main output directory (where all .ppu files and libraries will be collected)
# -Fu$(dir): Add each source directory to the unit search path. This is crucial
#            for FPC to find the source .pas files when resolving dependencies,
#            especially if a .ppu is missing or outdated.
UNIT_PATHS = -Fu. -Fu$(OUTDIR) $(foreach dir,$(SOURCE_DIRS),-Fu$(dir))

# Common FPC compilation flags for units and main program
# Includes OS-specific target flag
# -MObjFPC: Objective Pascal mode (standard for modern FPC)
# -O2: Optimization level 2
# -g: Generate debug information
# -gl: Generate line info for debugger
# -l: Generate list file (assembly output)
# -vewnhi: Display verbose messages (errors, warnings, notes, hints, info)
# -FE$(OUTDIR): Specify the output directory for the executable
# -FU$(OUTDIR): Specify the output directory for compiled units (.ppu, .o)
FPC_COMMON_FLAGS = -MObjFPC $(TARGET_OS_FLAG) -O2 -g -gl -l -vewnhi -FE$(OUTDIR) -FU$(OUTDIR)

# Linker flags for dynamic libraries
# -k-L$(OUTDIR): Add OUTDIR to the linker's library search path
# -k-l<name>: Link with library <name> (e.g., -lInputBox for libInputBox.so/dylib)
LINK_LIBS = $(foreach lib,$(LIB_NAMES),-k-l$(lib))
LINK_FLAGS = -k-L$(OUTDIR) $(LINK_LIBS)

.PHONY: all clean

# Default target: builds the output directory, all libraries, and the main executable
all: $(OUTDIR) $(ALL_LIB_TARGETS) $(OUTDIR)/$(MAIN_PROGRAM:.lpr=)

# Rule to create the output directory if it doesn't exist
$(OUTDIR):
	mkdir -p $(OUTDIR)

# Generic rule to compile any regular .pas unit into its corresponding .ppu and .o files in OUTDIR.
# This is needed for units that are 'used' by other units or the main program.
$(OUTDIR)/%.ppu: %.pas
	@echo "Compiling regular unit $<..."
	$(FPC) $(FPC_COMMON_FLAGS) $(UNIT_PATHS) $<

# Rule to compile a .pas file into a dynamic library.
# This also implicitly generates the .ppu and .o files for the unit.
# The -o $@ ensures the output is named correctly (e.g., libInputBox.so)
$(OUTDIR)/lib%.$(LIB_EXT): components/%.pas
	@echo "Compiling dynamic library $< -> $@..."
	$(FPC) $(FPC_COMMON_FLAGS) $(UNIT_PATHS) -CDYNAMICLIB $< -o $@

# Rule for the main program executable.
# Prerequisites: The main .lpr file itself, all compiled .ppu files, and all dynamic libraries.
# Make will ensure all prerequisites are up-to-date before compiling the main program.
$(OUTDIR)/$(MAIN_PROGRAM:.lpr=): $(MAIN_PROGRAM) $(ALL_PPU_TARGETS) $(ALL_LIB_TARGETS)
	@echo "Compiling main program $< and linking libraries..."
	$(FPC) $(FPC_COMMON_FLAGS) $(UNIT_PATHS) $(LINK_FLAGS) $<

# Clean target: removes the entire build directory
clean:
	@echo "Cleaning build directory..."
	rm -rf $(OUTDIR)
