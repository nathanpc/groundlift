### variables.mk
### Common variables used throughout the project.
###
### Author: Nathan Campos <nathan@innoveworkshop.com>

# Project
PROJECT = groundlift

# Environment
PLATFORM := $(shell uname -s)

# Directories and Paths
ROOT     := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
BUILDDIR := build

# Tools
CC    = gcc
GDB   = gdb --args
RM    = rm -f
MKDIR = mkdir -p
TOUCH = touch
MAKE  = make

# Handle OS X-specific tools.
ifeq ($(PLATFORM), Darwin)
	CC  = clang
	GDB = lldb
endif

# Flags
CFLAGS  = -Wall -Wno-psabi --std=gnu89 -pthread
LDFLAGS = -pthread
LIBS    =
