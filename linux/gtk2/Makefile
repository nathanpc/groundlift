include ../../variables.mk

# Source file names.
SRCNAMES = main.c server.c comdlg.c windows/sendfile.c windows/transfer.c \
	windows/recvprogress.c windows/sendprogress.c

# Sources and Objects
PROJECT     = gl-gtk
TARGET      = $(BUILDDIR)/bin/$(PROJECT)
SOURCES    += $(SRCNAMES)
OBJECTS    := $(patsubst %.c, $(BUILDDIR)/$(PROJECT)/%.o, $(SOURCES))
STATICLIBS := $(BUILDDIR)/libglproto/libglproto.a \
	$(BUILDDIR)/libglcommon/libglcommon.a

# Flags
CFLAGS += $(shell pkg-config --cflags gtk+-2.0 glib-2.0)
LIBS   += $(shell pkg-config --libs gtk+-2.0 glib-2.0)

.PHONY: all compile test run-server run-client debug memcheck clean
all: compile

compile: $(BUILDDIR)/$(PROJECT)/stamp $(BUILDDIR)/$(PROJECT)/windows/stamp $(TARGET)

$(TARGET): $(OBJECTS) $(STATICLIBS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BUILDDIR)/$(PROJECT)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/libglproto/libglproto.a:
	cd $(ROOT)/src && $(MAKE) $(MAKECMDGOALS)

$(BUILDDIR)/libglcommon/libglcommon.a:
	cd $(ROOT)/common && $(MAKE) $(MAKECMDGOALS)

$(BUILDDIR)/$(PROJECT)/stamp:
	$(MKDIR) $(@D)
	$(TOUCH) $@

$(BUILDDIR)/$(PROJECT)/windows/stamp: $(BUILDDIR)/$(PROJECT)/stamp
	$(MKDIR) $(@D)
	$(TOUCH) $@

run: test

debug: CFLAGS += -g3 -DDEBUG
debug: clean compile
	$(GDB) $(TARGET) s

memcheck: CFLAGS += -g3 -DDEBUG -DMEMCHECK
memcheck: clean compile
	valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=all \
		--track-origins=yes --log-file=$(BUILDDIR)/valgrind.log \
		$(TARGET) s
	cat $(BUILDDIR)/valgrind.log

test: run-server

clean:
	$(RM) -r $(BUILDDIR)/$(PROJECT)

run-server: compile
	$(TARGET) s

run-client: compile
	$(TARGET) c $(TESTSRVADDR) $(TESTSRVPORT)
