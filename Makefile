# .PHONY is needed so that make reruns the command. Without .PHONY, make will assume the
# command name corresponds to a file name and if the file/dir exists will not rerun the command.
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html

EXECUTABLE=build/src/pgdb
PROG_TO_DEBUG=build/src/samples/hello.tsk

.PHONY: build
build:
	mkdir build || cd build && cmake .. && cmake --build .

.PHONY: debug
debug:
	$(EXECUTABLE) $(PROG_TO_DEBUG)

.PHONY: gdb
gdb:
	gdb --args $(EXECUTABLE) $(PROG_TO_DEBUG)

.PHONY: clean
clean:
	rm -rf build