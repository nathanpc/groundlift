### Makefile
### Automates the build and everything else of the project.
###
### Author: Nathan Campos <nathan@innoveworkshop.com>

include variables.mk

# User definitions.
PREFIX  ?= $(BUILDDIR)/dist

# Internal project definitions.
COMMONSRC   = main.c client.c server.c
SOURCES    := $(addprefix src/, $(COMMONSRC))
OBJECTS    := $(patsubst src/%.c, $(BUILDDIR)/%.o, $(SOURCES))
APPSRC      = glsend.c #glrecvd.c glscan.c
APPSOURCES := $(addprefix src/, $(APPSRC))
APPOBJECTS := $(patsubst src/%.c, $(BUILDDIR)/%.o, $(APPSOURCES))
TARGETS    := $(OBJECTS) $(APPOBJECTS) $(BUILDDIR)/glsend #$(BUILDDIR)/glrecvd $(BUILDDIR)/glscan

.PHONY: all compiledb compile debug memcheck clean

all: compile

$(BUILDDIR)/stamp:
	$(MKDIR) $(@D)
	$(TOUCH) $@

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/glsend: $(OBJECTS) $(BUILDDIR)/glsend.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

compiledb: clean
	bear --output $(ROOT)/compile_commands.json -- make CC=clang debug

compile: $(BUILDDIR)/stamp $(TARGETS)

debug: CFLAGS += -g3 -DDEBUG
debug: compile

memcheck: CFLAGS += -g3 -DDEBUG -DMEMCHECK
memcheck: compile

clean:
	$(RM) -r $(BUILDDIR)
