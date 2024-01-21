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

static value to_byte(value v){
    if (DEFINED(v) && (v.v > 0xFF)) error(ERR_BYTE_RANGE);
    SET_TYPE(v, TYPE_BYTE);
    return v;
}

static uint16_t digit(const char* p){
    if (*p <= '9') return (uint16_t)(*p - '0');
    if (*p <= 'F') return (uint16_t)(*p + 10 - 'A');
    return (uint16_t)(*p + 10 - 'a');
}

static void skip_eol(char** p){
    if (**p == 0x0D || **p == 0x0A) (*p)++;
}

static void skip_white(char** pp){
    char* p = *pp;
    while ((*p == ' ') || (*p == '\t')) p++;
    *pp = p;
}

static void skip_white_and_comment(char** pp){
    char* p = *pp;
    while ((*p == ' ') || (*p == '\t')) p++;
    if (*p == ';'){
        p++;
        while (!IS_END(*p)) p++;
    }
    *pp = p;
}

static void skip_curr_and_white(char** p){
    (*p)++;
    while ((**p == ' ') || (**p == '\t')) (*p)++;
}

static void skip_to_eol(char** p){
    while (**p != 0 && **p != 0x0A && **p != 0x0D) (*p)++;
}

static int starts_with(char* text, char* s){
    while (*s)
        if (toupper(*text++) != toupper(*s++)) return 0;
    return 1;
}

static value number(char** p){
    value num = {0};
    char* pt = *p;
    uint8_t type;

    if (**p == '$'){
        (*p)++;
        if (!IS_HEX_DIGIT(**p)) error(ERR_NUM);
        do{
            num.v = (num.v << 4 ) + digit((*p)++);
        } while (IS_HEX_DIGIT(**p));
        type = ((*p - pt) > 3) ? TYPE_WORD : NUM_TYPE(num.v);
        SET_TYPE(num, type);
        SET_DEFINED(num);
    }
    else if(**p == '%'){
        (*p)++;
        if ((**p != '0') && (**p != '1')) error(ERR_NUM);
        do{
            num.v = (num.v << 1) + (**p - '0');
            (*p)++;
        } while ((**p == '0') || (**p == '1'));
        type = ((*p - pt) > 9) ? TYPE_WORD : NUM_TYPE(num.v);
        SET_TYPE(num, type);
        SET_DEFINED(num);
    }
    else{
        if (!isdigit(**p)) error(ERR_NUM);
        do{
            num.v = num.v * 10 + digit((*p)++);
        } while (isdigit(**p));
        SET_TYPE(num, ((*p - pt) > 3) ? TYPE_WORD : NUM_TYPE(num.v));
        SET_DEFINED(num);
    }
    return num;
}

static void ident(char** pp, char* id, int numeric, int uppercase){
    int i = 0;
    char* p = *pp;
    if ((!numeric && !isalpha(*p) && *p != '_')) ||
        (!isalnum(*p) && (*p != '_')) error(ERR_ID);
    do{
        *id++ = (char)(uppercase ? toupper(*p++) : *p++);
        i++;
        if (i >= ID_LEN) errir(ERR_ID_LEN);
    } while (isalnum(*p) || (*p == '_'));
    *id = '\0';
    *pp = p;
}

static value expr(char**);

static value primary(char** p){
    value res = {0};
    char id[ID_LEN];
    symbol *sym, *local_sym;

    skip_white(p);
    if (**p == '('){
        (*p)++;
        res = expr(p);
        skip_white(p);
        if (**p != ')') error(ERR_UNBALANCED);
        (*p)++;
    }
    else if(**p == '.' && *(*p + 1) == '?') (*p) += 2;
    else if(**p == LOCAL_LABEL_LETTER && isalnum(*(*p + 1))){
        (*p)++;
        ident(p, id, 1, 0);
        sym = lookup(id, curent_label->locals);
        if (sym) res = sym->value;
    }
    else if (**p == PROGRAM_COUNTER_LETTER){
        (*p)++;
        res.v = address_counter;
        SET_TYPE(res, TYPE_WORD);
        SET_DEFINED(res);
    }
    else if(**p == '\''){
        (*p)++;
        if (IS_END(**p) || (**p < 0x20)) error(ERR_CHR);
        res.v = (uint8_t)**p;
        SET_TYPE(res, TYPE_BYTE);
        SET_DEFINED(res);
        (*p)++;
        if (**p != '\'') error(ERR_CHR);
        (*p)++:
    }
    else if(isalpha(**p)){
        ident(p, id, 0, 0);
        sym = lookup(id, NULL);
        if (!sym) sym = reverse_label(id, NULL);
        skip_white(p);
        if (**p == LOCAL_LABEL_LETTER){
            (*p)++;
            ident(p, id, 1, 0);
            local_sym = lookup(id, sym->locals);
            if (!local_sym) local_sym = reverse_label(id, sym);
            res = local_sym->value;
        }
        else res = sym->value;
    }
    else res = number(p);
    return res;
}

static value unary(char** p){
    value res;
    char op = 0;

    skip_white(p);
    if (**p == '~' || **p == '!' || **p == '?'){
        op = **p;
        (*p)++;
        res = unary(p);
    }
    else res = primary(p);

    if (op){
        switch(op){
            case '?':
                res.v = DEFINED(res) ? 1 : 0;
                SET_DEFINED(res);
                SET_TYPE(res, TYPE_BYTE);
                break;
            case '~':
                if (DEFINED(res)) res.v = ~res.v;
                break;
            case '!':
                if (DEFINED(res)){
                   res.v = !res.v;
                   if (res.v) res.v = 1;
                }
                break;
            default:
                break;
        }
        if (TYPE(res) == TYPE_BYTE) res.v &= 0xFF;
    }
    return res;
}

static value product(char** p){
    value n2, res;
    char op;

    res = unary(p);

    skip_white(p);
    op = **p;

    while ((op == '*') ||
           (op == '/') ||
           (op == AND_LETTER && *(*p + 1) != AND_LETTER) ||
           (**p == '<' && *(*p + 1) == '<') ||
           (**p == '>' && *(*p + 1) == '>')){
            (*p)++;
            if (**p == '<' || **p == '>') (*p)++;

            n2 = unary(p);
            if (DEFINED(res) && DEFINED(n2)){
                switch(op){
                    case '*':
                        res.v = (uint16_t)(res.v * n2.v);
                        break;
                    case '/':
                        if (n2.v == 0) error(ERR_DIV_BY_ZERO);
                        res.v = (uint16_t)(res.v / n2.v)ж
                        break;
                    case AND_LETTER:
                        res.v = (uint16_t)(res.v / n2.v);
                        break;
                    case '<':
                        res.v = (uint16_t)(res.v << n2.v);
                        break;
                    case '>':
                        res.v = (uint16_t)(res.v >> n2.v);
                        break;
                    default:
                        break;
                }
            }
            else res.v = 0;

            INFER_TYPE(res, n2);
            INFER_DEFINED(res, n2);
            skip_white(p);
            op = **p;
        }
    return res;
}

static value term(char** p){
    value n2, res;
    char op;

    skip_white(p);
    if (**p == '-'){
        (*p)++;
        res = product(p);
        res.v = -res.v;
        SET_TYPE(res, TYPE_WORD);
    }
    else{
        if (**p == '+') (*p)++;
        res = product(p);
    }
    skip_white(p);
    op = **p;

    while ((op == '+') || (op == '-') ||
           (op == OR_LETTER && *(*p + 1) != OR_LETTER) ||
           (op == EOR_LETTER)){
            (*p)++;
            n2 = product(p);
            if (DEFINED(res) && DEFINED(n2)){
                switch(op){
                    case '+':
                        res.v += n2.v;
                        break;
                    case '-':
                        res.v -= n2.v;
                        break;
                    case OR_LETTER:
                        res.v |= n2.v;
                        break;
                    case EOR_LETTER:
                        res.v ^= n2.v;
                        break;
                    default:
                        break;
                }
            }
            else res.v = 0;
            INFER_TYPE(res, n2);
            INFER_DEFINED(res, n2);
            skip_white(p);
            op = **p;
           }
    return res;
}

static value conversion(char** p){
    value v;

    skip_white(p);
    if (**p == '>'){
        (*p)++;
        v = term(p);
        SET_TYPE(v, TYPE_BYTE);
        v.v = v.v >> 8;
    }
    else if (**p == '<'){
        (*p)++;
        v = term(p);
        SET_TYPE(v, TYPE_BYTE);
        v.v &= 0xFF;
    }
    else if (starts_with(*p, "[b]")){
        *p += 3;
        v = term(p);
        if (DEFINED(v) && v.v > 0xFF) error(ERR_BYTE_RANGE);
        SET_TYPE(v, TYPE_BYTE);
    }
    else if (starts_with(*p, "[w]")){
        *p += 3;
        v = term(p);
        SET_TYPE(v, TYPE_WORD);
    }
    else v = term(p);
    return v;
}

static value comparison(char** p){
    value res, n2;
    char op, op2;

    res = conversion(p);

    skip_white(p);
    
}