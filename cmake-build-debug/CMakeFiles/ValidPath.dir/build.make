# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.14

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

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "D:\Program Files\CLion 2019.2.1\bin\cmake\win\bin\cmake.exe"

# The command to remove a file.
RM = "D:\Program Files\CLion 2019.2.1\bin\cmake\win\bin\cmake.exe" -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\Users\88453\CLionProjects\ValidPath

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\Users\88453\CLionProjects\ValidPath\cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/ValidPath.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ValidPath.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ValidPath.dir/flags.make

CMakeFiles/ValidPath.dir/main.cpp.obj: CMakeFiles/ValidPath.dir/flags.make
CMakeFiles/ValidPath.dir/main.cpp.obj: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=C:\Users\88453\CLionProjects\ValidPath\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ValidPath.dir/main.cpp.obj"
	C:\PROGRA~1\MINGW-~1\X86_64~1.0-W\mingw64\bin\G__~1.EXE  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles\ValidPath.dir\main.cpp.obj -c C:\Users\88453\CLionProjects\ValidPath\main.cpp

CMakeFiles/ValidPath.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ValidPath.dir/main.cpp.i"
	C:\PROGRA~1\MINGW-~1\X86_64~1.0-W\mingw64\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E C:\Users\88453\CLionProjects\ValidPath\main.cpp > CMakeFiles\ValidPath.dir\main.cpp.i

CMakeFiles/ValidPath.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ValidPath.dir/main.cpp.s"
	C:\PROGRA~1\MINGW-~1\X86_64~1.0-W\mingw64\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S C:\Users\88453\CLionProjects\ValidPath\main.cpp -o CMakeFiles\ValidPath.dir\main.cpp.s

# Object files for target ValidPath
ValidPath_OBJECTS = \
"CMakeFiles/ValidPath.dir/main.cpp.obj"

# External object files for target ValidPath
ValidPath_EXTERNAL_OBJECTS =

ValidPath.exe: CMakeFiles/ValidPath.dir/main.cpp.obj
ValidPath.exe: CMakeFiles/ValidPath.dir/build.make
ValidPath.exe: CMakeFiles/ValidPath.dir/linklibs.rsp
ValidPath.exe: CMakeFiles/ValidPath.dir/objects1.rsp
ValidPath.exe: CMakeFiles/ValidPath.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=C:\Users\88453\CLionProjects\ValidPath\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ValidPath.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\ValidPath.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ValidPath.dir/build: ValidPath.exe

.PHONY : CMakeFiles/ValidPath.dir/build

CMakeFiles/ValidPath.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\ValidPath.dir\cmake_clean.cmake
.PHONY : CMakeFiles/ValidPath.dir/clean

CMakeFiles/ValidPath.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\Users\88453\CLionProjects\ValidPath C:\Users\88453\CLionProjects\ValidPath C:\Users\88453\CLionProjects\ValidPath\cmake-build-debug C:\Users\88453\CLionProjects\ValidPath\cmake-build-debug C:\Users\88453\CLionProjects\ValidPath\cmake-build-debug\CMakeFiles\ValidPath.dir\DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ValidPath.dir/depend
