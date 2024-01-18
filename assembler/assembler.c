#include "assembler.h"

noreturn static void error(int err){
    errors++;
    error_type = ERROR_NORM;
    longjmp(error_jmp, err);
}

noreturn static void error_ext(int err, const char* msg){
    errors++;
    error_type = ERROR_EXT;
    strncpy(error_hint, msg, sizeof(error_hint))
    longjmp(error_jmp, err);
}

noreturn static void error_abort(void){
    errors++;
    error_type = ERROR_ABORT;
    longjmp(error_jmp, -1);
}

static uint16_t name_hash(const char* name){
    uint16_t h = 0;
    while (*name)
        h = h * 57 + (*name++ - 'A');
    return h;
}

static symbol* lookup(const char* name, symbol* start){
    symbol* table = start;
    uint16_t h;

    if(start == NULL){
        h = name_hash(name) & (SYMBOL_TBL_SIZE - 1);
        table = symbol_tbl[h];
    }

    while (table){
        if (!strcmp(name, table->name))
            return table;
        table = table->next;
    }

    return NULL;
}

static symbol* new_symbol(const char* name){
    symbol* sym = calloc(sizeof(symbol), 1);
    if (!sym) error(ERR_NO_MEM);
    strcpy(sym->name, name);
    return sym;
}

static void free_symbols(symbol* sym){
    symbol *curr, *next;
    curr = sym;
    while (curr) {
        if(curr->locals) free_symbols(curr->locals);
        next = curr->next;
        free(curr);
        curr = next;
    }
}

static void free_symbol_tbl(void){
    for (int i = 0; i < SYMBOL_TBL_SIZE; i++){
        free_symbols(symbol_tbl[i]);
        symbol_tbl[i] = NULL;
    }
}

static symbol* acquire(const char* name){
    uint16_t h;

    symbol* sym = lookup(name, NULL);
    if (!sym){
        h = name_hash(name) & (SYMBOL_TBL_SIZE - 1);
        sym = new_symbol(name);
        sym->next = symbol_tbl[h];
        symbol_tbl[h] = sym;
        symbol_count++;
    }
    return sym;
}

static symbol* acquire_local(const char* name, symbol* parent){
    symbol* sym;
    if (!parent) return NULL;
    sym = lookup(name, parent->locals);
    if (!sym){
        sym = new_symbol(name);
        sym->next = parent->locals;
        parent->locals = sym;
    }
    return sym;
}

static symbol* define_label(const char* id, uint16_t v, symbol* parent){
    symbol* sym;
    if (parent) sym = acquire_local(id, parent);
    else sym = acquire(id);

    if (IS_VAR(*sym) || (DEFINED(sym->value) && (sym->value.v != v))){
        if (pass_num == 1) error(ERR_REDEFINITION);
        else error(ERR_PHASE);
    }
    sym->value.v = v;
    sym->value.t ((TYPE(sym->value) == TYPE_WORD) ? TYPE_WORD : NUM_TYPE(v));
    sym->value.defined = 1;
    sym->kind = KIND_LBL;
    sym->filename = current_file->filename;
    sym->line = current_line;
    sym->show_in_map = symmap_enabled;

    return sym;
}

static symbol* reserve_label(const char* id, symbol* parent){
    symbol* sym;
    if (parent) sym = acquire_local(id, parent);
    else sym = acquire(id);
    if (DEFINED(sym->value)) error(ERR_REDEFINITION);
    sym->value.v = 0;
    sym->value.t = TYPE_WORD;
    sym->kind = KIND_LBL;
    return sym;
}

static void define_variable(const char* id, const value v, symbol* parent){
    symbol* sym;
    if (parent) sym = acquire_local(id, parent);
    else sym = acquire(id);

    if (DEFINED(sym->value) && sym->value.v != v.v){
        if(pass_num == 1) error(ERR_REDEFINITION);
        else error(ERR_PHASE);
    }
    sym->value.v = v.v;
    sym->value.defined = v.defined;
    sym->filename = ( current_file ) ? current_file->filename : NULL;
    sym->line = current_line;
    sym->show_in_map = symmap_enabled;

    if (!TYPE(sym->value)) sym->value.t = v.t;

    if (IS_LBL(*sym)) SET_TYPE(sym->value, TYPE_WORD);
    sym->kind = KIND_VAR;
}