#include <cstdio>
#include <cstdlib>
#include "cpu.hpp"

extern "C"{
    char* source_filename;
    char* listing_filename;
    char* output_filename;

    asm_file* read_file(const char*);
    int genrom(char*, char*);
    int parse_args(char*);
    void select_6502(void);
    void pass(char**);
    int init_listing(char*);
    void list_symbols(void);
    int save_code(const char*, const uint8_t*, int);
    void free_files(void);
    void free_symbol_tbl(void);
}

#include "assembler/assembler.h"

void display_help(){
    printf("Simple 6502 CPU emulator!\nEmulator can take following arguments:\n");
    printf("\tassemble <asm_file> <output_file> - assemble assembly code for 6502 CPU using integrated assembler\n");
    printf("\tgenrom <bin_file> <output_file> - generate ROM using binary file, which will be exactly 64KB\n");
    printf("\texecute <rom_file> - execute generated ROM file\n");
    exit(0);
}

int main(int argc, char** argv){
    char* source;
    if (argc == 4 && !strcmp(argv[1], "genrom")){
        FILE* bin = fopen(argv[2], "rb");
        if (!bin) return EXIT_FAILURE;
        genrom(argv[1], argv[2]);
    }
    else if (argc >= 2 && !strcmp(argv[1], "assemble")){
        if (!parse_args(*(++argv))){
            display_help();
            return (argc > 1) ? EXIT_FAILURE : EXIT_SUCCESS;
        }
        if (!strcmp(source_filename, output_filename)){
            puts("Sources may be overwritten, exiting");
            return EXIT_FAILURE;
        }
        if (listing_filename && (!strcmp(source_filename, listing_filename) || 
                                 !strcmp(output_filename, listing_filename))){
            puts("Sources may be overwritten, exiting");
            return EXIT_FAILURE;
        }

        if (!(current_file = read_file(source_filename))){
            puts("Cannot load file!");
            errors = 1;
            goto ret0;
        }

        select_6502();

        pass_num = 1;
        source = current_file->text;
        pass(&source);
        code_size = output_counter;
        if (errors) goto ret1;

        if (listing_filename){
            if (!init_listing(listing_filename)){
                puts("Cannot open listing file!");
                errors = 1;
                goto ret1;
            }
        }

        pass_num++;
        source = current_file->text;
        code = (uint8_t*)malloc(code_size);
        pass(&source);

        if (output_counter != code_size && !errors){
            puts("Pass code sizes differ!");
            errors++;
        }

        if (errors) goto ret2;

        if (listing_filename) list_symbols();

        if (!flag_quiet){
            printf("Output file %s, %d bytes written.\n", output_filename, output_counter);
            if (listing_filename) printf("Listing written to %s\n", listing_filename);
        }

        if (!save_code(output_filename, code, output_counter)){
            puts("Cannot save file!");
            errors = 1;
            goto ret2;
        }

ret2:
        if (list_file) fclose(list_file);
        free(code);
ret1:
        free_files();
ret0:
        free_symbol_tbl();
        if (errors) return EXIT_FAILURE;
        else return EXIT_SUCCESS;
    }
}