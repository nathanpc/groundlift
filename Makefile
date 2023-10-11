### Makefile
### Automates the build and everything else of the project.
###
### Author: Nathan Campos <nathan@innoveworkshop.com>

include variables.mk

.PHONY: all compiledb cli gtk2 debug memcheck clean
all: $(BUILDDIR)/stamp cli gtk2

$(BUILDDIR)/stamp:
	$(MKDIR) $(@D)
	$(MKDIR) $(BUILDDIR)/bin
	$(TOUCH) $@

compiledb: clean
	bear --output .vscode/compile_commands.json -- make all

run: test

debug: CFLAGS += -g3 -DDEBUG
debug: clean compile
	$(GDB) $(TESTCMD) s

memcheck: CFLAGS += -g3 -DDEBUG -DMEMCHECK
memcheck: clean compile
	valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=all \
		--track-origins=yes --log-file=$(BUILDDIR)/valgrind.log $(TESTCMD) s
	cat $(BUILDDIR)/valgrind.log

cli:
	cd cli/ && $(MAKE)

gtk2:
	cd linux/gtk2/ && $(MAKE)

clean:
	$(RM) -r $(BUILDDIR)
