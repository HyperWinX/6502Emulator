#include <stdlib.h>

#define FILESZ 65536

int main(int argc, char* argv[]){
	if (argc == 1) return 1;
	char rom[0xFFFF];
	memset(rom, 0x00, sizeof(rom));
	FILE* romfile = fopen("rom.bin", "wb");
	FILE* assembly = fopen(argv[1], "rb");
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
}
