
.DEFAULT_GOAL := all
clean:
	[ -f build/CMakeFiles/Makefile2 ] && cd build/ && make clean || true
	/bin/rm -rf build/CMakeCache.txt build/CMakeFiles/
%:
	cd build/ && make $@
