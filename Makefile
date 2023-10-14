### Makefile
### Automates the build and everything else of the project.
###
### Author: Nathan Campos <nathan@innoveworkshop.com>

include variables.mk

.PHONY: all compiledb cli gtk2 debug memcheck clean
all: $(BUILDDIR)/stamp cli

$(BUILDDIR)/stamp:
	$(MKDIR) $(@D)
	$(MKDIR) $(BUILDDIR)/bin
	$(TOUCH) $@

compiledb: clean
	bear --output .vscode/compile_commands.json -- make debug

debug: CFLAGS += -g3 -DDEBUG
debug: clean cli

memcheck: CFLAGS += -g3 -DDEBUG -DMEMCHECK
memcheck: clean cli
	valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=all \
		--track-origins=yes --log-file=$(BUILDDIR)/valgrind.log ./build/bin/gl s
	cat $(BUILDDIR)/valgrind.log

cli: $(BUILDDIR)/stamp
	cd cli/ && $(MAKE)

gtk2: $(BUILDDIR)/stamp
	cd linux/gtk2/ && $(MAKE)

clean:
	$(RM) -r $(BUILDDIR)
