#include <cstdio>
#include <cstdlib>
#include "cpu.hpp"
#include "help.h"

typedef struct asm_file {
    char* filename;
    char* text;
} asm_file;

extern "C"{
    void assembler_entry_point(char**);
    int genrom(char*, char*);
}
void display_help(){
    printf("Simple 6502 CPU emulator!\nEmulator can take following arguments:\n");
    printf("\tassemble <asm_file> <output_file> - assemble assembly code for 6502 CPU using integrated assembler\n");
    printf("\tgenrom <bin_file> <output_file> - generate ROM using binary file, which will be exactly 64KB\n");
    printf("\texecute <rom_file> - execute generated ROM file\n");
    exit(0);
}

int main(int argc, char** argv){
    if (argc == 4 && !strcmp(argv[1], "genrom")){
        FILE* bin = fopen(argv[2], "rb");
        if (!bin) return EXIT_FAILURE;
        genrom(argv[2], argv[3]);
    }
    else if (argc == 4 && !strcmp(argv[1], "assemble")){
        assembler_entry_point(argv);
        return EXIT_SUCCESS;
    }
    else{
        printf("argc: %d, second argv: %s", argc, argv[1]);
    }
}