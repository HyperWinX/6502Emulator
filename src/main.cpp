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
	printf("\tnew <filename.asm> - create project template\n");
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
	else if (argc == 3 && !strcmp(argv[1], "execute")){
		EMU6502::CPU cpu;
		cpu.Reset();
		cpu.LoadROM(argv[2]);
		cpu.Execute(0xFFFF, 0);
	}
	else if (argc == 3 && !strcmp(argv[1], "new")){
		FILE* prj = fopen(argv[2], "w");
		if (!prj){
			printf("Failed to get file descriptor!");
			return 1;
		}
		char init[] = ".org $0200\n.word $0002 ; Reset vector\n.word $0000 ; Interrupt vector\n\nstart:\n\tlda #0\n";
		fwrite(init, sizeof(char), sizeof(init), prj);
		fclose(prj);
		printf("Project generated!\n");
	}
	else if (argc == 4 && !strcmp(argv[1], "execute") && !strcmp(argv[2], "debug")){
		EMU6502::CPU cpu;
        cpu.Reset();
        cpu.LoadROM(argv[3]);
        cpu.Execute(0xFFFF, 1);
	}
    else{
        printf("argc: %d, second argv: %s\n", argc, argv[1]);
    }
}
