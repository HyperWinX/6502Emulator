build:
	@mkdir build
	@cmake -S . -B build
	@cmake --build build

tests:
	@mkdir build
	@cmake -S . -B build
	@cmake --build build --target tests
	$(MAKE) clean

emulator:
	@mkdir build
	@cmake -S . -B build
	@cmake --build build --target emulator

clean:
	@rm -rf build

rebuild:
	$(MAKE) clean
	$(MAKE) build
