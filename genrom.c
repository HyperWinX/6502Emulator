#include <stdlib.h>
#include <stdio.h>

#define FILESZ 65536

int genrom(char* bin, char* out){
	char rom[0xFFFF];
	memset(rom, 0x00, sizeof(rom));
	FILE* romfile = fopen(bin, "wb");
	FILE* assembly = fopen(out, "rb");
	fseek(assembly, 0, SEEK_END);
	size_t len = ftell(assembly);
	rewind(assembly);
	if (len > FILESZ - 0x01FF - 4) return 1;
	fread(rom + 0x0200, sizeof(char), len, assembly);
	char fend[4] = {0x00, 0x02, 0x00, 0x00};
	memcpy(rom + 0xFFFF - 4, fend, 4);
	fwrite(rom, sizeof(char), 0xFFFF, romfile);
	fclose(romfile);
	fclose(assembly);
	puts("ROM generated!");
}
