# .PHONY is needed so that make reruns the command. Without .PHONY, make will assume the
# command name corresponds to a file name and if the file/dir exists will not rerun the command.
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html

.PHONY: build
build:
	cd build && cmake .. && cmake --build .

debug:
	build/pgdb build/hello.tsk

gdb:
	gdb --args build/pgdb build/hello.tsk

clean:
	rm -rf build