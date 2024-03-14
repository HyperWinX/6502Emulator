#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define FILESZ 65536

FILE* validate_existance(char* target){
	FILE* fd = fopen(target, "rb");
	if (fd) return fd;

	puts("Validation failed! ROM file does not exist.");
	return 0;
}

int validate_size(FILE* target){
	fseek(target, 0, SEEK_END);
	uint64_t size = ftell(target);
	rewind(target);
	return size == 0xFFFF;
}



int genrom(char* bin, char* out){
	char rom[0xFFFF];
	memset(rom, 0x00, sizeof(rom));
	FILE* romfile = fopen(out, "wb");
	FILE* assembly = fopen(bin, "rb");
	fseek(assembly, 0, SEEK_END);
	size_t len = ftell(assembly);
	rewind(assembly);
	if (len > FILESZ - 0x01FF - 4) return 1;
	char bin_asm[len];
	fread(bin_asm, sizeof(char), len, assembly);
	memcpy(&rom[0x0200], bin_asm + 4, len - 4);
	memcpy(rom + 0xFFFB, bin_asm, 4);
	fwrite(rom, sizeof(char), 0xFFFF, romfile);
	fclose(romfile);
	fclose(assembly);
	if (!(romfile = validate_existance(out))) return 1;
	if (!(validate_size(romfile))) return 1;
	fclose(romfile);
	return 0;
}

void buf_genrom(char* asm_bin, int len, char* target){
	memcpy(target + 0x0200, asm_bin + 4, len - 4);
    memcpy(target + 0xFFFF - 4, asm_bin, 4);
}
