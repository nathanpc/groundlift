cmake_minimum_required(VERSION 3.10)

project(groundlift VERSION 0.1.0
		DESCRIPTION "A cross-platform AirDrop alternative"
		LANGUAGES C)

# Ensure we can easily debug things.
#set(CMAKE_VERBOSE_MAKEFILE on)

# Generate the compile commands for SonarLint.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_HEADERS_IN_COMPILE_COMMANDS ON)

# Ensure we are using the most compatible standard of the C language.
set(CMAKE_C_STANDARD 90)

# Setup some standard compilation flgas across the board.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-psabi")

# Add the groundlift library subdirectory.
add_subdirectory(src)

# Add the included server/client command-line utility.
add_subdirectory(cli)
