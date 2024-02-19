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

clean:
	@rm -rf build

rebuild:
	$(MAKE) clean
	$(MAKE) build
