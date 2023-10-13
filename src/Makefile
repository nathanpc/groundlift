include ../variables.mk

# Source file names.
SRCNAMES = sockets.c obex.c server.c client.c conf.c error.c

# Sources and Objects
PROJECT  = libglproto
TARGET   = $(BUILDDIR)/$(PROJECT)/$(PROJECT).a
SOURCES += $(addprefix groundlift/, $(SRCNAMES))
OBJECTS := $(patsubst groundlift/%.c, $(BUILDDIR)/$(PROJECT)/%.o, $(SOURCES))

.PHONY: all compile debug memcheck clean
all: compile

compile: $(BUILDDIR)/$(PROJECT)/stamp $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $^

$(BUILDDIR)/$(PROJECT)/%.o: groundlift/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(PROJECT)/stamp:
	$(MKDIR) $(@D)
	$(TOUCH) $@

run: test

debug: CFLAGS += -g3 -DDEBUG
debug: clean all

memcheck: CFLAGS += -g3 -DDEBUG -DMEMCHECK
memcheck: compile

clean:
	$(RM) -r $(BUILDDIR)/$(PROJECT)
