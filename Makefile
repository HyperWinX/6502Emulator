build: clean
	@mkdir build
	@cmake -S . -B build
	@cmake --build build

tests: clean
	@mkdir build
	@cmake -S . -B build
	@cmake --build build --target tests
	$(MAKE) clean

emulator: clean
	@mkdir build
	@cmake -S . -B build
	@cmake --build build --target emulator

emulator-dbg: clean
	@mkdir build
	@cmake -S . -B build
	@cmake --build build --target emulator-dbg

clean:
	@rm -rf build

rebuild:
	$(MAKE) clean
	$(MAKE) build
