# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/hyperwin/projects/6502Emulator

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hyperwin/projects/6502Emulator/tmp

# Include any dependencies generated for this target.
include CMakeFiles/romgen.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/romgen.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/romgen.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/romgen.dir/flags.make

CMakeFiles/romgen.dir/genrom.c.o: CMakeFiles/romgen.dir/flags.make
CMakeFiles/romgen.dir/genrom.c.o: /home/hyperwin/projects/6502Emulator/genrom.c
CMakeFiles/romgen.dir/genrom.c.o: CMakeFiles/romgen.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/hyperwin/projects/6502Emulator/tmp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/romgen.dir/genrom.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/romgen.dir/genrom.c.o -MF CMakeFiles/romgen.dir/genrom.c.o.d -o CMakeFiles/romgen.dir/genrom.c.o -c /home/hyperwin/projects/6502Emulator/genrom.c

CMakeFiles/romgen.dir/genrom.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/romgen.dir/genrom.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/hyperwin/projects/6502Emulator/genrom.c > CMakeFiles/romgen.dir/genrom.c.i

CMakeFiles/romgen.dir/genrom.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/romgen.dir/genrom.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/hyperwin/projects/6502Emulator/genrom.c -o CMakeFiles/romgen.dir/genrom.c.s

# Object files for target romgen
romgen_OBJECTS = \
"CMakeFiles/romgen.dir/genrom.c.o"

# External object files for target romgen
romgen_EXTERNAL_OBJECTS =

romgen: CMakeFiles/romgen.dir/genrom.c.o
romgen: CMakeFiles/romgen.dir/build.make
romgen: CMakeFiles/romgen.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/hyperwin/projects/6502Emulator/tmp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable romgen"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/romgen.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/romgen.dir/build: romgen
.PHONY : CMakeFiles/romgen.dir/build

CMakeFiles/romgen.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/romgen.dir/cmake_clean.cmake
.PHONY : CMakeFiles/romgen.dir/clean

CMakeFiles/romgen.dir/depend:
	cd /home/hyperwin/projects/6502Emulator/tmp && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hyperwin/projects/6502Emulator /home/hyperwin/projects/6502Emulator /home/hyperwin/projects/6502Emulator/tmp /home/hyperwin/projects/6502Emulator/tmp /home/hyperwin/projects/6502Emulator/tmp/CMakeFiles/romgen.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/romgen.dir/depend

