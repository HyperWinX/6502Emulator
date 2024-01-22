#include "cpu.hpp"
#include <cstdio>

void display_help(){
    printf("Simple 6502 CPU emulator!\nEmulator can take following arguments:\n");
    printf("\tassemble <asm_file> <output_file> - assemble assembly code for 6502 CPU using integrated assembler\n");
    printf("\tgenrom <bin_file> <output_file> - generate ROM using binary file, which will be exactly 64KB\n");
    printf("\texecute <rom_file> - execute generated ROM file\n");
    exit(0);
}

int main(int argc, char* argv[]){
    if (argc == 1) display_help();
}