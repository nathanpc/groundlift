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
	bear --output $(ROOT)/compile_commands.json -- make CC=clang debug

debug: CFLAGS += -g3 -DDEBUG
debug: clean $(BUILDDIR)/stamp
	cd cli/ && $(MAKE) debug

memcheck: CFLAGS += -g3 -DDEBUG -DMEMCHECK
memcheck: clean $(BUILDDIR)/stamp
	cd cli/ && $(MAKE) memcheck

cli: $(BUILDDIR)/stamp
	cd cli/ && $(MAKE)

gtk2: $(BUILDDIR)/stamp
	cd linux/gtk2/ && $(MAKE)

run-server:
	cd cli/ && $(MAKE) run-server

run-client:
	cd cli/ && $(MAKE) run-client

debug-server: debug run-server

debug-client: debug run-client

clean:
	$(RM) -r $(BUILDDIR)
