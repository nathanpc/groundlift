### Makefile
### Automates the build and everything else of the project.
###
### Author: Nathan Campos <nathan@innoveworkshop.com>

include variables.mk

# Source file names.
SRCNAMES = main.c tcp.c
ifeq ($(PLATFORM), Linux)
	SRCNAMES += avahi.c
endif

# Sources and Objects
SOURCES  += $(addprefix $(SRCDIR)/, $(SRCNAMES))
OBJECTS  := $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

# Test executable command line.
TESTCMD := $(TARGET)

.PHONY: all compile compiledb test server client debug memcheck clean
all: compile

compile: $(BUILDDIR)/stamp $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/stamp:
	$(MKDIR) $(@D)
	$(TOUCH) $@

compiledb: clean
	bear --output .vscode/compile_commands.json -- make compile

run: test

debug: CFLAGS += -g3 -DDEBUG
debug: clean compile
	$(GDB) $(TESTCMD)

memcheck: CFLAGS += -g3 -DDEBUG -DMEMCHECK
memcheck: clean compile
	valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=all \
		--track-origins=yes --log-file=$(BUILDDIR)/valgrind.log $(TESTCMD)
	cat $(BUILDDIR)/valgrind.log

test: server

server: compile
	$(TESTCMD) s

client: compile
	$(TESTCMD) c

clean:
	$(RM) -r $(BUILDDIR)
