### Makefile
### Automates the build and everything else of the project.
###
### Author: Nathan Campos <nathan@innoveworkshop.com>

include variables.mk

# User definitions.
PREFIX  ?= $(BUILDDIR)/dist

# Internal project definitions.
COMMONSRC   = sockets.c request.c logging.c
OBJECTS    := $(patsubst %.c, $(BUILDDIR)/%.o, $(COMMONSRC))
APPSRC      = glrecvd.c #glsend.c glscan.c
APPOBJECTS := $(patsubst %.c, $(BUILDDIR)/%.o, $(APPSRC))
TARGETS    := $(OBJECTS) $(APPOBJECTS) $(BUILDDIR)/glrecvd #$(BUILDDIR)/glsend $(BUILDDIR)/glscan

.PHONY: all compiledb compile debug memcheck clean

all: compile

$(BUILDDIR)/stamp:
	$(MKDIR) $(@D)
	$(TOUCH) $@

$(BUILDDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/glrecvd: $(OBJECTS) $(BUILDDIR)/glrecvd.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

compiledb: clean
	bear --output $(ROOT)/compile_commands.json -- make CC=clang debug

compile: $(BUILDDIR)/stamp $(TARGETS)

debug: CFLAGS += -g3 -D_DEBUG
debug: compile

memcheck: CFLAGS += -g3 -D_DEBUG -DMEMCHECK
memcheck: compile

clean:
	$(RM) -r $(BUILDDIR)
