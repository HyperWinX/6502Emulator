SRCS := main.cpp \
	cpu.cpp

all:
	@g++ $(SRCS) -O2 -o emulator

tests:
	@g++ tests.cpp cpu.cpp -lgtest_main -lgtest -pthread
	@./a.out
	@rm -f a.out
