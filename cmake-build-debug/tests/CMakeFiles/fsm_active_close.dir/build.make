# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /tmp/tmp.cQ6D6jqgKo

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /tmp/tmp.cQ6D6jqgKo/cmake-build-debug

# Include any dependencies generated for this target.
include tests/CMakeFiles/fsm_active_close.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/fsm_active_close.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/fsm_active_close.dir/flags.make

tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o: tests/CMakeFiles/fsm_active_close.dir/flags.make
tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o: ../tests/fsm_active_close.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/tmp/tmp.cQ6D6jqgKo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o"
	cd /tmp/tmp.cQ6D6jqgKo/cmake-build-debug/tests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o -c /tmp/tmp.cQ6D6jqgKo/tests/fsm_active_close.cc

tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.i"
	cd /tmp/tmp.cQ6D6jqgKo/cmake-build-debug/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /tmp/tmp.cQ6D6jqgKo/tests/fsm_active_close.cc > CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.i

tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.s"
	cd /tmp/tmp.cQ6D6jqgKo/cmake-build-debug/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /tmp/tmp.cQ6D6jqgKo/tests/fsm_active_close.cc -o CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.s

tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o.requires:

.PHONY : tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o.requires

tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o.provides: tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o.requires
	$(MAKE) -f tests/CMakeFiles/fsm_active_close.dir/build.make tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o.provides.build
.PHONY : tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o.provides

tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o.provides.build: tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o


# Object files for target fsm_active_close
fsm_active_close_OBJECTS = \
"CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o"

# External object files for target fsm_active_close
fsm_active_close_EXTERNAL_OBJECTS =

tests/fsm_active_close: tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o
tests/fsm_active_close: tests/CMakeFiles/fsm_active_close.dir/build.make
tests/fsm_active_close: tests/libspongechecks.a
tests/fsm_active_close: libsponge/libsponge.a
tests/fsm_active_close: tests/CMakeFiles/fsm_active_close.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/tmp/tmp.cQ6D6jqgKo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable fsm_active_close"
	cd /tmp/tmp.cQ6D6jqgKo/cmake-build-debug/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fsm_active_close.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/fsm_active_close.dir/build: tests/fsm_active_close

.PHONY : tests/CMakeFiles/fsm_active_close.dir/build

tests/CMakeFiles/fsm_active_close.dir/requires: tests/CMakeFiles/fsm_active_close.dir/fsm_active_close.cc.o.requires

.PHONY : tests/CMakeFiles/fsm_active_close.dir/requires

tests/CMakeFiles/fsm_active_close.dir/clean:
	cd /tmp/tmp.cQ6D6jqgKo/cmake-build-debug/tests && $(CMAKE_COMMAND) -P CMakeFiles/fsm_active_close.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/fsm_active_close.dir/clean

tests/CMakeFiles/fsm_active_close.dir/depend:
	cd /tmp/tmp.cQ6D6jqgKo/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /tmp/tmp.cQ6D6jqgKo /tmp/tmp.cQ6D6jqgKo/tests /tmp/tmp.cQ6D6jqgKo/cmake-build-debug /tmp/tmp.cQ6D6jqgKo/cmake-build-debug/tests /tmp/tmp.cQ6D6jqgKo/cmake-build-debug/tests/CMakeFiles/fsm_active_close.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/fsm_active_close.dir/depend

