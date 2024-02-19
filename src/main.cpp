#include <cstdio>
#include <cstdlib>
#include "cpu.hpp"
#include "help.h"

typedef struct asm_file {
    char* filename;
    char* text;
} asm_file;

extern "C"{
	char* output_filename;
	uint16_t output_counter;
	uint8_t* code;
    char* assembler_entry_point(char**);
    int genrom(char*, char*);
	void buf_genrom(char* asm_bin, int len, uint8_t* target);
	int save_code(const char*, const uint8_t*, int);
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
		save_code(output_filename, code, output_counter);
        return EXIT_SUCCESS;
    }
	else if (argc == 3 && !strcmp(argv[1], "execute")){
		char* fake_argv[] = {(char*)"emu6502", (char*)"assemble", argv[2], (char*)"t.o"};
		char* data = assembler_entry_point(fake_argv);
		if (!data){
			printf("Failed to assemble ROM!");
			return EXIT_FAILURE;
		}
		uint8_t rom[0xFFFF];
		memset(rom, 0x00, sizeof(rom));
		buf_genrom(data, output_counter, rom);
		EMU6502::CPU cpu;
		cpu.Reset();
		cpu.SetROM((uint8_t*)&rom);
		cpu.Execute(0xFFFF, 0);
	}
	else if (argc == 3 && !strcmp(argv[1], "new")){
		FILE* prj = fopen(argv[2], "w");
		if (!prj){
			printf("Failed to get file descriptor!");
			return 1;
		}
		char init[] = ".org $0200\n.word $0002 ; Reset vector\n.word $0000 ; Interrupt vector\n\nstart:\n\tlda #0\n ";
		fwrite(init, sizeof(char), sizeof(init) - 1, prj);
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
