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
	fread(rom + 0x0200, sizeof(char), len, assembly);
	char fend[4] = {0x02, 0x00, 0x00, 0x00};
	memcpy(rom + 0xFFFF - 4, fend, 4);
	fwrite(rom, sizeof(char), 0xFFFF, romfile);
	fclose(romfile);
	fclose(assembly);
	puts("ROM generated! Running validation...");
	if (!(romfile = validate_existance(out))) return 1;
	puts("File exists.");
	if (!(validate_size(romfile))) return 1;
	puts("ROM size is valid.");
	fclose(romfile);
	puts("Validation complete.");
	return 0;
}
