### variables.mk
### Common variables used throughout the project.
###
### Author: Nathan Campos <nathan@innoveworkshop.com>

# Project
PROJECT = groundlift

# Environment
PLATFORM := $(shell uname -s)

# Directories and Paths
SRCDIR   := src
BUILDDIR := build
TARGET   := $(BUILDDIR)/$(PROJECT)

# Tools
CC    = gcc
AR    = ar
GDB   = gdb --args
RM    = rm -f
MKDIR = mkdir -p
TOUCH = touch

# Handle OS X-specific tools.
ifeq ($(PLATFORM), Darwin)
	CC  = clang
	GDB = lldb
endif

# Flags
CFLAGS  = -Wall -Wno-psabi --std=c89
LDFLAGS =
