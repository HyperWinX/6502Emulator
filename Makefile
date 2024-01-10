SRCS := main.cpp

all:
	@g++ $(SRCS) -O2 -o emulator

tests:
	@g++ tests.cpp -lgtest_main -lgtest -pthread
	@./a.out
	@rm -f a.out