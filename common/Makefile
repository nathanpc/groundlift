include ../variables.mk

# Source file names.
SRCNAMES = bits.c utf16.c filesystem.c threads.c logging.c capabilities.c

# Sources and Objects
PROJECT  = libglcommon
TARGET   = $(BUILDDIR)/$(PROJECT)/$(PROJECT).a
SOURCES += $(addprefix utils/, $(SRCNAMES))
OBJECTS := $(patsubst utils/%.c, $(BUILDDIR)/$(PROJECT)/%.o, $(SOURCES))

.PHONY: all compile debug memcheck clean
all: compile

compile: $(BUILDDIR)/$(PROJECT)/stamp $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $^

$(BUILDDIR)/$(PROJECT)/%.o: utils/%.c
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
