# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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
CMAKE_SOURCE_DIR = /root/dev/resourse/yaml-cpp-0.8.0

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/dev/resourse/yaml-cpp-0.8.0/out

# Include any dependencies generated for this target.
include util/CMakeFiles/yaml-cpp-read.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include util/CMakeFiles/yaml-cpp-read.dir/compiler_depend.make

# Include the progress variables for this target.
include util/CMakeFiles/yaml-cpp-read.dir/progress.make

# Include the compile flags for this target's objects.
include util/CMakeFiles/yaml-cpp-read.dir/flags.make

util/CMakeFiles/yaml-cpp-read.dir/read.cpp.o: util/CMakeFiles/yaml-cpp-read.dir/flags.make
util/CMakeFiles/yaml-cpp-read.dir/read.cpp.o: ../util/read.cpp
util/CMakeFiles/yaml-cpp-read.dir/read.cpp.o: util/CMakeFiles/yaml-cpp-read.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/dev/resourse/yaml-cpp-0.8.0/out/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object util/CMakeFiles/yaml-cpp-read.dir/read.cpp.o"
	cd /root/dev/resourse/yaml-cpp-0.8.0/out/util && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT util/CMakeFiles/yaml-cpp-read.dir/read.cpp.o -MF CMakeFiles/yaml-cpp-read.dir/read.cpp.o.d -o CMakeFiles/yaml-cpp-read.dir/read.cpp.o -c /root/dev/resourse/yaml-cpp-0.8.0/util/read.cpp

util/CMakeFiles/yaml-cpp-read.dir/read.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/yaml-cpp-read.dir/read.cpp.i"
	cd /root/dev/resourse/yaml-cpp-0.8.0/out/util && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/dev/resourse/yaml-cpp-0.8.0/util/read.cpp > CMakeFiles/yaml-cpp-read.dir/read.cpp.i

util/CMakeFiles/yaml-cpp-read.dir/read.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/yaml-cpp-read.dir/read.cpp.s"
	cd /root/dev/resourse/yaml-cpp-0.8.0/out/util && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/dev/resourse/yaml-cpp-0.8.0/util/read.cpp -o CMakeFiles/yaml-cpp-read.dir/read.cpp.s

# Object files for target yaml-cpp-read
yaml__cpp__read_OBJECTS = \
"CMakeFiles/yaml-cpp-read.dir/read.cpp.o"

# External object files for target yaml-cpp-read
yaml__cpp__read_EXTERNAL_OBJECTS =

util/read: util/CMakeFiles/yaml-cpp-read.dir/read.cpp.o
util/read: util/CMakeFiles/yaml-cpp-read.dir/build.make
util/read: libyaml-cpp.a
util/read: util/CMakeFiles/yaml-cpp-read.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/dev/resourse/yaml-cpp-0.8.0/out/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable read"
	cd /root/dev/resourse/yaml-cpp-0.8.0/out/util && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/yaml-cpp-read.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
util/CMakeFiles/yaml-cpp-read.dir/build: util/read
.PHONY : util/CMakeFiles/yaml-cpp-read.dir/build

util/CMakeFiles/yaml-cpp-read.dir/clean:
	cd /root/dev/resourse/yaml-cpp-0.8.0/out/util && $(CMAKE_COMMAND) -P CMakeFiles/yaml-cpp-read.dir/cmake_clean.cmake
.PHONY : util/CMakeFiles/yaml-cpp-read.dir/clean

util/CMakeFiles/yaml-cpp-read.dir/depend:
	cd /root/dev/resourse/yaml-cpp-0.8.0/out && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/dev/resourse/yaml-cpp-0.8.0 /root/dev/resourse/yaml-cpp-0.8.0/util /root/dev/resourse/yaml-cpp-0.8.0/out /root/dev/resourse/yaml-cpp-0.8.0/out/util /root/dev/resourse/yaml-cpp-0.8.0/out/util/CMakeFiles/yaml-cpp-read.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : util/CMakeFiles/yaml-cpp-read.dir/depend

