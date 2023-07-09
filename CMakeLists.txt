cmake_minimum_required(VERSION 3.10)

project(groundlift VERSION 0.1.0
		DESCRIPTION "A cross-platform AirDrop alternative"
		LANGUAGES C)

# Ensure we can easily debug things.
#set(CMAKE_VERBOSE_MAKEFILE on)

# Ensure we know if we are running under Linux.
if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

# Generate the compile commands for SonarLint.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_HEADERS_IN_COMPILE_COMMANDS ON)

# Ensure we are using the most compatible standard of the C language.
set(CMAKE_C_STANDARD 90)

# Setup some standard compilation flgas across the board.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wno-psabi")

# Add the groundlift library and utilities subdirectory.
add_subdirectory(common)
add_subdirectory(src)

# Add the included server/client command-line utility.
add_subdirectory(cli)

# Add the D-Bus service if running Linux.
if(LINUX)
	#add_subdirectory(linux/dbus)
	add_subdirectory(linux/gtk2)
endif()
