# Set include directory
include_directories(${6502Emulator_SOURCE_DIR}/include)

# Set sources for different targets
set(EMULATOR_SRCS cpu.cpp
                  main.cpp
                  genrom.c
                  assembler/assembler.c)

set(TESTS_SRCS    cpu.cpp
                  tests.cpp)

add_executable(tests ${TESTS_SRCS})
target_link_libraries(tests gtest_main gtest pthread)
add_custom_command(TARGET tests POST_BUILD COMMAND ../tests)
set_target_properties(tests PROPERTIES COMPILE_FLAGS "-Wall -Werror -static -O2")

add_executable(emulator ${EMULATOR_SRCS})
set_target_properties(emulator PROPERTIES COMPILE_FLAGS "-Wall -Werror -g")
