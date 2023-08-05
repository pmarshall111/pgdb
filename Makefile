build:
	make clean && mkdir build && cd build && cmake .. && cmake --build .

debug:
	build/pgdb build/hello.tsk

clean:
	rm -rf build