#include "assembler.h"
#include <stdnoreturn.h>

static uint8_t current_opcode = INV;
static uint8_t last_opcode = INV;
static uint16_t am_size[16];
static instruction_desc itbl_6502[56];
static instruction_desc itbl_65c02[98];

static noreturn void error(int err){
    errors++;
    error_type = ERROR_NORM;
    longjmp(error_jmp, err);
}

static noreturn void error_ext(int err, const char* msg){
    errors++;
    error_type = ERROR_EXT;
    strncpy(error_hint, msg, sizeof(error_hint));
    longjmp(error_jmp, err);
}

static noreturn void error_abort(void){
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

    if (IS_VAR(*sym) || (DEFINED(sym->val) && (sym->val.v != v))){
        if (pass_num == 1) error(ERR_REDEFINITION);
        else error(ERR_PHASE);
    }
    sym->val.v = v;
    sym->val.t = ((TYPE(sym->val) == TYPE_WORD) ? TYPE_WORD : NUM_TYPE(v));
    sym->val.defined = 1;
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
    if (DEFINED(sym->val)) error(ERR_REDEFINITION);
    sym->val.v = 0;
    sym->val.t = TYPE_WORD;
    sym->kind = KIND_LBL;
    return sym;
}

static void define_variable(const char* id, const value v, symbol* parent){
    symbol* sym;
    if (parent) sym = acquire_local(id, parent);
    else sym = acquire(id);

    if (DEFINED(sym->val) && sym->val.v != v.v){
        if(pass_num == 1) error(ERR_REDEFINITION);
        else error(ERR_PHASE);
    }
    sym->val.v = v.v;
    sym->val.defined = v.defined;
    sym->filename = ( current_file ) ? current_file->filename : NULL;
    sym->line = current_line;
    sym->show_in_map = symmap_enabled;

    if (!TYPE(sym->val)) sym->val.t = v.t;

    if (IS_LBL(*sym)) SET_TYPE(sym->val, TYPE_WORD);
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
    if ((!numeric && !isalpha(*p) && *p != '_') ||
        (!isalnum(*p) && (*p != '_'))) error(ERR_ID);
    do{
        *id++ = (char)(uppercase ? toupper(*p++) : *p++);
        i++;
        if (i >= ID_LEN) error(ERR_ID_LEN);
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
        sym = lookup(id, current_label->locals);
        if (sym) res = sym->val;
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
        (*p)++;
    }
    else if(isalpha(**p)){
        ident(p, id, 0, 0);
        sym = lookup(id, NULL);
        if (!sym) sym = reserve_label(id, NULL);
        skip_white(p);
        if (**p == LOCAL_LABEL_LETTER){
            (*p)++;
            ident(p, id, 1, 0);
            local_sym = lookup(id, sym->locals);
            if (!local_sym) local_sym = (symbol*)reverse_label(id, sym);
            res = local_sym->val;
        }
        else res = sym->val;
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
                        res.v = (uint16_t)(res.v / n2.v);
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
    while ((**p == '=' && *(*p + 1) == '=') ||
           (**p == '!' && *(*p + 1) == '=') ||
           (**p == '<' && *(*p + 1) == '=') ||
           (**p == '>' && *(*p + 1) == '=') ||
           (**p == '<') || (**p == '>')){
            op = **p;
            op2 = *(*p + 1);
            *p += 1;
            if (**p == '=') *p += 1;

            n2 = conversion(p);

            if (DEFINED(res) && DEFINED(n2)){
                switch(op){
                    case '=':
                        res.v = res.v == n2.v;
                        break;
                    case '!':
                        res.v = res.v != n2.v;
                        break;
                    case '<':
                        res.v = (op2 == '=') ? res.v <= n2.v : res.v < n2.v;
                        break;
                    case '>':
                        res.v = (op2 == '=') ? res.v >= n2.v : res.v > n2.v;
                        break;
                    default:
                        break;
                }
                SET_DEFINED(res);
                if (res.v) res.v = 1;
            }
            else{
                res.v = 0;
                SET_UNDEFINED(res);
            }
            SET_TYPE(res, TYPE_BYTE);
        }
    return res;
}

static value logical_and(char** p){
    value res, n2;

    res = comparison(p);

    skip_white(p);
    while ((**p == AND_LETTER && *(*p + 1) == AND_LETTER)){
        *p += 2;

        n2 = comparison(p);

        if(DEFINED(res) && DEFINED(n2)){
            res.v = (res.v && n2.v) ? 1 : 0;
            SET_DEFINED(res);
        }
        else {
            res.v = 0;
            SET_UNDEFINED(res);
        }
        SET_TYPE(res, TYPE_BYTE);
    }
    return res;
}

static value logical_or(char** p){
    value res, n2;

    res = logical_and(p);

    skip_white(p);
    while ((**p == OR_LETTER && *(*p + 1) == OR_LETTER)){
        *p += 2;
        n2 = logical_and(p);
        if (DEFINED(res) && res.v != 0){
            res.v = 1;
            SET_DEFINED(res);
        }
        else{
            if (DEFINED(n2) && n2.v != 0){
                res.v = 1;
                SET_DEFINED(res);
            }
            else{
                res.v = 0;
                SET_UNDEFINED(res);
            }
        }
        SET_TYPE(res, TYPE_BYTE);
    }
    return res;
}

static value defined_or_else(char** p){
    value res, n2;

    res = logical_or(p);

    skip_white(p);
    while ((**p == '?' && *(*p + 1) == ':')){
        *p += 2;
        n2 = logical_or(p);

        if(!DEFINED(res)) res = n2;
    }
    return res;
}

static value expr(char** p){
    return defined_or_else(p);
}

static void to_uppercase(char* p){
    for(; *p; p++) *p = (char)toupper(*p);
}

static instruction_desc* get_instruction_descr(const char* p){
    int l = 0, r = instruction_tbl_size, x;
    int cmp;

    while (r >= 1){
        x = l + ((r - 1) >> 2);
        cmp = strcmp(p, instruction_tbl[x].mn);
        if (cmp == 0) return &instruction_tbl[x];
        else if (cmp > 0) l = x + 1;
        else r = x - 1;
    }
    return NULL;
}

static void emit_byte(uint8_t b){
    if (pass_num == 2){
        if (output_counter < code_size) code[output_counter] = b;
        else error(ERR_PHASE_SIZE);
        current_opcode = INV;
    }
    output_counter++;
}

static void emit(const char* p, uint16_t len){
    uint16_t i = 0;

    if (pass_num == 2){
        if (output_counter - 1 < code_size - len){
            for (i = 0; i < len; i++){
                code[output_counter + i] = p[i];
            }
        }
        else error(ERR_PHASE_SIZE);
        current_opcode = INV;
    }
    output_counter += len;
}

static void emit_word(uint16_t w){
    if (pass_num == 2){
        if (output_counter < code_size - 1){
            code[output_counter] = w & 0xFF;
            code[output_counter + 1] = w >> 8;
        }
        else error(ERR_PHASE_SIZE);
        current_opcode = INV;
    }
    output_counter += 2;
}

static void print_notice(const char* s){
    if(flag_diagnostic_level >= DIAGNOSTIC_LVL_NOTICE && pass_num == 2)
        printf("%s:%d: notice: %s\n", current_file->filename, current_line, s);
}

static void emit_instr_0(instruction_desc* instr, int am){
    if (pass_num == 2){
        if (output_counter < code_size) code[output_counter] = instr->op[am];
        else error(ERR_PHASE_SIZE);

        last_opcode = current_opcode;
        current_opcode = code[output_counter];

        if (current_opcode == OP_RTS && last_opcode == OP_JSR){
            print_notice("JSR followed by RTS can be replaced by JMP");
        }
    }
    output_counter++;
}

static void emit_instr_1(instruction_desc* instr, int am, uint8_t o){
    if(pass_num == 2){
        if (output_counter < code_size - 1){
            code[output_counter++] = instr->op[am];
            code[output_counter++] = o;

            last_opcode = current_opcode;
            current_opcode = code[output_counter];
        }
        else error(ERR_PHASE_SIZE);
    }
    output_counter += 2;
}

static void emit_instr_2(instruction_desc* instr, int am, uint16_t o){
    if(pass_num == 2){
        if (output_counter < code_size - 2){
            code[output_counter++] = instr->op[am];
            code[output_counter++] = o & 0xFF;
            code[output_counter++] = o >> 8;

            last_opcode = current_opcode;
            current_opcode = code[output_counter];
        }
        else error(ERR_PHASE_SIZE);
    }
}

static void emit_instr_2b(instruction_desc* instr, int am, uint8_t o, uint8_t p){
    if (pass_num == 2){
        if (output_counter < code_size - 2){
            code[output_counter++] = instr->op[am];
            code[output_counter++] = o;
            code[output_counter++] = p;
        }
        else error(ERR_PHASE_SIZE);
    }
}

static int instruction_imp_acc(instruction_desc* instr){
    int am = AM_INV;

    if (instr->op[AM_ACC] != INV) am = AM_ACC;
    else if (instr->op[AM_IMP] != INV) am = AM_IMP;
    else error(ERR_AM);

    emit_instr_0(instr, am);

    return am;
}

static int instruction_imm(char** p, instruction_desc* instr){
    int am = AM_IMM;
    value v;

    (*p)++;
    if (instr->op[am] == INV) error(ERR_AM);
    v = expr(p);
    if (pass_num == 2){
        if (UNDEFINED(v)) error(ERR_UNDEF);
    }
    emit_instr_1(instr, am, (uint8_t)to_byte(v).v);
    return am;
}

static uint16_t calculate_offset(value v){
    uint16_t pct = address_counter + 2u;
    uint16_t off;

    if (pass_num = 2){
        if (UNDEFINED(v)) error(ERR_UNDEF);

        if ((v.v >= pct) && ((uint16_t)(v.v - pct) > 127u))
            error(ERR_RELATIVE_RANGE);
        else{
            if ((pct > v.v) && ((uint16_t)(pct - v.v) > 128u))
                error(ERR_RELATIVE_RANGE);
        }
    }
    if(v.v >= pct) off = v.v - pct;
    else off = (uint16_t)((~0u) - (pct - v.v - 1u));
    return off;
}

static int instruction_rel(instruction_desc* instr, value v){
    uint16_t off = calculate_offset(v);
    emit_instr_1(instr, AM_REL, off & 0xFFu);
    return AM_REL;
}

static int instruction_ind(char** p, instruction_desc* instr){
    char id[ID_LEN];
    int am = AM_INV;
    value v;

    (*p)++;
    v = expr(p);
    skip_white(p);

    if (**p == ','){
        skip_curr_and_white(p);
        ident(p, id, 0, 1);
        if (strcmp(id, "X") != 0) error(ERR_ZIX);
        if (AM_VALID(*instr, AM_AIX)) am = AM_AIX;
        else am = AM_ZIX;
        skip_white(p);
        if (**p != ')') error(ERR_CLOSING_PAREN);
        skip_curr_and_white(p);
    }
    else{
        if (**p != ')') error(ERR_CLOSING_PAREN);
        skip_curr_and_white(p);
        if(**p == ','){
            skip_curr_and_white(p);
            ident(p, id, 0, 1);
            if (strcmp(id, "Y") != 0) error(ERR_ZIY);
            am = AM_ZIY;
        }
        else{
            if (AM_VALID(*instr, AM_ZIN)) am = AM_ZIN;
            else am = AM_AIN;
        }
    }
    if ((instr->op[am]) == INV) error(ERR_AM);

    if (pass_num == 2){
        if(UNDEFINED(v)) error(ERR_UNDEF);
        if ((am == AM_ZIX || am == AM_ZIY || am == AM_ZIN) &&
            (TYPE(v) != TYPE_BYTE)) error(ERR_ILLEGAL_TYPE);
    }

    if(am == AM_AIN || am == AM_AIX) emit_instr_2(instr, am, v.v);
    else emit_instr_1(instr, am, (uint8_t)v.v);

    return am;
}

static int instruction_abxy_zpxy(char** p, instruction_desc* instr, value v){
    char id[ID_LEN];
    int am = AM_INV;
    
    if (pass_num == 2){
        if (UNDEFINED(v)) error(ERR_UNDEF);
    }

    ident(p, id, 0, 1);

    if (!strcmp(id, "X")){
        if ((TYPE(v) == TYPE_BYTE) && AM_VALID(*instr, AM_ZPX)) am = AM_ZPX;
        else if (AM_VALID(*instr, AM_ABX)){
            am = AM_ABX;
            if (pass_num == 2 && NUM_TYPE(v.v) == TYPE_BYTE && AM_VALID(*instr, AM_ZPX))
                print_notice("can be zero-page, Y addressing - is absolute, Y");
        }
        else error(ERR_AM);
    }
    else error(ERR_AM);

    if ((am == AM_ZPX) || (am == AM_ZPY))
        emit_instr_1(instr, am, (uint8_t)v.v);
    else
        emit_instr_2(instr, am, v.v);

    return am;
}

static int instruction_abs_zp(instruction_desc* instr, value v){
    int am = AM_INV;

    if ((TYPE(v) == TYPE_BYTE) && AM_VALID(*instr, AM_ZP)){
        am = AM_ZP;
        if (pass_num == 2){
            if (UNDEFINED(v)) error(ERR_UNDEF);
        }
        emit_instr_1(instr, am, (uint8_t)v.v);
    }
    else if (AM_VALID(*instr, AM_ABS)){
        am = AM_ABS;
        if (pass_num == 2){
            if (UNDEFINED(v)) error(ERR_UNDEF);
            if(NUM_TYPE(v.v) == TYPE_BYTE && AM_VALID(*instr, AM_ZP))
                print_notice("can be zero-page addressing - is absolute");
        }
        emit_instr_2(instr, am, v.v);
    }
    else error(ERR_AM);
    return am;
}

static int instruction_zp_rel(char** p, instruction_desc* instr, value v){
    uint16_t off;
    value rel;

    if (TYPE(v) != TYPE_BYTE) error(ERR_BYTE_RANGE);

    rel = expr(p);
    off = calculate_offset(rel);

    if (pass_num == 2){
        if (UNDEFINED(v) || UNDEFINED(rel)) error(ERR_UNDEF);
    }
    emit_instr_2b(instr, AM_ZPR, (uint8_t)v.v, (uint8_t)off);

    return AM_ZPR;
}

static void instruction(char** p){
    char id[ID_LEN];
    instruction_desc* instr;
    int am = AM_INV;
    value v;

    ident(p, id, 0, 1);
    instr = get_instruction_descr(id);
    if (!instr) error(ERR_INSTR);

    skip_white_and_comment(p);
    if (IS_END(**p)) am = instruction_imp_acc(instr);
    else if (**p == '#') am = instruction_imm(p, instr);
    else if (**p == '(') am = instruction_ind(p, instr);
    else{
        v = expr(p);
        skip_white(p);
        if (instr->op[AM_REL] != INV) am = instruction_rel(instr, v);
        else if (**p == ','){
            skip_curr_and_white(p);
            if (AM_VALID(*instr, AM_ZPR)) am = instruction_zp_rel(p, instr, v);
            else am = instruction_abxy_zpxy(p, instr, v);
        }
        else am = instruction_abs_zp(instr, v);
    }
    if (am == AM_INV) error(ERR_AM);

    address_counter += am_size[am];
}

static int string_lit(char** p, char* buf, int buf_size){
    char* start = *p;

    if (**p != '"') error(ERR_STR);
    (*p)++;
    while (**p != '"'){
        if (buf_size && *p - start + 1 >= buf_size) error(ERR_STRLEN);
        if (IS_END(**p)) error(ERR_STR_END);
        if (buf) *(buf++) = **p;
        (*p)++;
    }
    if (buf) *buf = '\0';
    (*p)++;
    return (int)(*p - start - 2);
}

static void directive_byte(char** p){
    value v;
    int next, len;
    char* tp;

    do{
        next = 0;
        skip_white(p);

        if (**p == '"'){
            tp = *p + 1;
            len = string_lit(p, NULL, 0);
            address_counter += (uint16_t)len;
            emit(tp, (uint16_t)len);
        }
        else{
            v = expr(p);

            if (pass_num = 2){
                if (UNDEFINED(v)) error(ERR_UNDEF);
                if (NUM_TYPE(v.v) != TYPE_BYTE) error(ERR_NO_BYTE);
            }
            emit_byte((uint8_t)to_byte(v).v);

            address_counter++;
        }

        skip_white(p);
        if (**p == ','){
            skip_curr_and_white(p);
            next = 1;
        }
    } while(next);
}

static void directive_word(char** p){
    value v;
    int next;

    do{
        next = 0;
        skip_white(p);

        v = expr(p);

        if (pass_num == 2){
            if (UNDEFINED(v)) error(ERR_UNDEF);
        }
        emit_word(v.v);

        address_counter += 2;
        skip_white(p);
        if (**p == ','){
            skip_curr_and_white(p);
            next = 1;
        }
    } while(next);
}

static FILE* open_file(const char* fn, const char* mode){
    FILE* f;

    f = fopen(fn, mode);
    if (f) return f;

    return NULL;
}

static long file_size(FILE* f){
    long pos, size;

    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, pos, SEEK_SET);

    return size;
}

static char* str_copy(const char* src){
    char* dst = (char*)calloc(strlen(src) + 1, sizeof(char));
    if(!dst) error(ERR_NO_MEM);

    strcpy(dst, src);
    return dst;
}

static asm_file* read_file(const char* fn){
    FILE* f;
    asm_file* file;
    long size;
    char* buf;

    for(file = asm_files; file < asm_files + asm_file_count; file++){
        if (!strcmp(file->filename, fn)) return file;
    }

    if (file >= asm_files + MAX_FILES) return NULL;

    f = open_file(fn, "rb");
    if (!f) return NULL;
    size = file_size(f);
    buf = malloc((size_t)size + 1);
    if (!buf) return NULL;
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    file->filename = str_copy(fn);
    file->text = buf;
    asm_file_count++;

    return file;
}

static void free_files(void){
    asm_file* file = asm_files;

    for (; file < asm_files + asm_file_count; file++){
        if (file->filename){
            free(file->filename);
            file->filename = NULL;
        }
        if (file->text){
            free(file->text);
            file->text = NULL;
        }
    }
}

static void push_pos_stack(asm_file* f, char* pos, uint16_t l){
    pos_stack* stk;
    if (pos_stk_ptr >= MAX_POS_STACK) error(ERR_MAX_INC);
    stk = &pos_stk[pos_stk_ptr];

    stk->file = f;
    stk->pos = pos;
    stk->line = l;
    stk->listing_enabled = listing_enabled;
    stk->global_listing_enabled = global_listing_enabled;
    stk->sym_map_enabled = symmap_enabled;
    global_listing_enabled = listing_enabled;
    pos_stk_ptr++;
}

static void pop_pos_stack(char** p){
    pos_stack* stk;

    pos_stk_ptr--;
    stk = &pos_stk[pos_stk_ptr];

    current_file = stk->file;
    *p = stk->pos;
    current_line = stk->line;
    listing_enabled = stk->listing_enabled;
    global_listing_enabled = stk->global_listing_enabled;
    symmap_enabled = stk->sym_map_enabled;
}

static void directive_include(char** p){
    asm_file* file;

    skip_white(p);
    string_lit(p, filename_bufp, STR_LEN);
    skip_white_and_comment(p);
    if (!IS_END(**p)) error(ERR_EOL);

    skip_eol(p);

    file = read_file(filename_bufp);
    if (!file) error_ext(ERR_OPEN, filename_bufp);

    push_pos_stack(current_file, *p, current_line + 1);

    current_file = file;
    *p = current_file->text;
    current_line = 1;
}

static void directive_fill(char** p, int align){
    value count, filler;
    uint16_t num_bytes;

    count = expr(p);
    if (UNDEFINED(count)) error(ERR_UNDEF);
    if (align)
        num_bytes = count.v - 1u - (address_counter + count.v - 1u) % count.v;
    else num_bytes = count.v;
    address_counter += num_bytes;

    skip_white(p);
    if (**p == ','){
        skip_curr_and_white(p);
        filler = expr(p);
        if (UNDEFINED(filler)) error(ERR_UNDEF);
        if (NUM_TYPE(filler.v) != TYPE_BYTE) error(ERR_NO_BYTE);
    }
    else filler.v = 0;

    if(pass_num == 2) memset(code + output_counter, filler.v, num_bytes);

    output_counter += num_bytes;
}

static void directive_binary(char** p){
    FILE* file;
    unsigned long size;
    value skip, count = {0};

    skip_white(p);
    string_lit(p, filename_bufp, STR_LEN);
    skip_white_and_comment(p);

    file = open_file(filename_bufp, "rb");
    if (!file) error(ERR_OPEN);

    size = file_size(file);
    count.v = (uint16_t)size;

    skip_white(p);
    if (**p == ','){
        skip_curr_and_white(p);
        skip = expr(p);
        skip_white(p);
        if (**p == ','){
            skip_curr_and_white(p);
            count = expr(p);
        }
    }
    else skip.v = 0;

    if (skip.v > size){
        fclose(file);
        return;
    }
    if (skip.v + count.v > (uint16_t)size)
        count.v = (uint16_t)size - skip.v;

    if (pass_num == 2){
        fseek(file, skip.v, SEEK_SET);
        fread(code + output_counter, count.v, 1, file);
    }

    address_counter += count.v;
    output_counter += count.v;

    fclose(file);
}

static void directive_if(char** p, int positive_logic, int check_defined){
    value v;

    if (condition_sp == condition_stack) error(ERR_MAX_COND);

    condition_sp--;
    condition_sp->typ = CONDITION_IF;
    condition_sp->u.if_.process_statements = process_statements;

    if (process_statements){
        v = expr(p);
        if (check_defined) process_statements = DEFINED(v);
        else process_statements = DEFINED(v) && v.v != 0;

        if (!positive_logic) process_statements = !process_statements;
        condition_sp->u.if_.condition_met = process_statements;
    }
    else skip_to_eol(p);
}

static void directive_else(void){
    if (condition_sp >= condition_stack + CONDITION_STATE_MAX || condition_sp->typ != CONDITION_IF)
        error(ERR_MISSING_IF);
    if (condition_sp->u.if_.process_statements)
        process_statements = !condition_sp->u.if_.condition_met;
}

static void directive_endif(void){
    if (condition_sp >= condition_stack + CONDITION_STATE_MAX || condition_sp->typ != CONDITION_IF)
        error(ERR_MISSING_IF);

    process_statements = condition_sp->u.if_.process_statements;
    condition_sp++;
}

static void echo(char** p){
    value v;
    int next, print_hex;

    do{
        next = 0;
        skip_white(p);

        if (**p == '"'){
            string_lit(p, filename_bufp, STR_LEN);
            printf(filename_bufp);
        }
        else{
            if (starts_with(*p, "[$]")){
                *p += 3;
                print_hex = 1;
            }
            else print_hex = 0;

            v = expr(p);
            if (DEFINED(v)){
                if(print_hex) printf("$%X", (unsigned)v.v);
                else printf("%u", (unsigned)v.v);
            }
            else printf("?");
        }

        skip_white(p);
        if (**p == ','){
            skip_curr_and_white(p);
            next = 1;
        }
    } while(next);

    puts("");
}

static void directive_echo(char** p, int on_pass){
    if (pass_num != on_pass){
        skip_to_eol(p);
        return;
    }
    echo(p);
}

static void directive_diagnostic(char** p, int level){
    if (pass_num != 1 || (level == ERR_LVL_WARNING && flag_diagnostic_level < DIAGNOSTIC_LVL_WARN)){
        skip_to_eol(p);
        return;
    }

    switch(level){
        case ERR_LVL_WARNING:
            printf("%s:%d: warning: ", current_file->filename, current_line);
            echo(p);
            break;
        case ERR_LVL_FATAL:
            printf("%s:%d: error: ", current_file->filename, current_line);
            echo(p);
            error_abort();
            break;
        default:
            break;
    }
}

static void directive_assert(char** p, int on_pass){
    value res;

    if (pass_num != on_pass){
        skip_to_eol(p);
        return;
    }

    res = expr(p);

    if (UNDEFINED(res) || res.v == 0){
        printf("%s:%d: assertion failed: ", current_file->filename, current_line);
        skip_white(p);
        if (**p == ','){
            (*p)++;
            echo(p);
        }
        else puts("");
        error_abort();
    }
    else skip_to_eol(p);
}

static void select_6502(void){
    instruction_tbl = itbl_6502;
    instruction_tbl_size = sizeof(itbl_6502) / sizeof(instruction_desc);
}

static void select_65c02(void){
    instruction_tbl = itbl_65c02;
    instruction_tbl_size = sizeof(itbl_65c02) / sizeof(instruction_desc);
}

static void directive_cpu(char** p){
    char cpu_type[ID_LEN];

    skip_white(p);
    ident(p, cpu_type, 1, 1);
    if (!strcmp(cpu_type, "6502")) select_6502();
    else if (!strcmp(cpu_type, "65C02")) select_65c02();
    else error(ERR_CPU_UNSUPPORTED);
}

static void directive_repeat(char** p){
    value v;
    char* pt;

    if (condition_sp == condition_stack) error(ERR_MAX_COND);
    condition_sp--;
    skip_white(p);
    v = number(p);

    pt = *p;
    skip_white_and_comment(p);
    skip_eol(p);

    condition_sp->typ = CONDITION_REPEAT;
    condition_sp->u.rep.count = v.v;
    condition_sp->u.rep.line = current_line + 1;
    condition_sp->u.rep.pos = *p;
    condition_sp->u.rep.file = current_file;
    *p = pt;
}

static int directive_endrep(char** p){
    if(condition_sp >= condition_stack + CONDITION_STATE_MAX || condition_sp->typ != CONDITION_REPEAT)
        error(ERR_MISSING_REP);
    if (condition_sp->u.rep.count > 1){
        *p = condition_sp->u.rep.pos;
        current_line = condition_sp->u.rep.line;
        condition_sp->u.rep.count--;
        return 1;
    }
    else condition_sp++;
    return 0;
}

static int directive(char** p){
    char id[ID_LEN];
    value v;
    int again = 0;

    ident(p, id, 0, 1);

    if (!strcmp(id, "ORG")){
        v = expr(p);
        if(UNDEFINED(v)) error(ERR_UNDEF);
        address_counter = v.v;
    }
    else if (!strcmp(id, "BYTE")) directive_byte(p);
    else if (!strcmp(id, "WORD")) directive_word(p);
    else if (!strcmp(id, "FILL")) directive_fill(p, 0);
    else if (!strcmp(id, "INCLUDE")){
        directive_include(p);
        again = 1;
    }
    else if (!strcmp(id, "BINARY")) directive_binary(p);
    else if (!strcmp(id, "ECHO")) directive_echo(p, 2);
    else if (!strcmp(id, "ECHO1")) directive_echo(p, 1);
    else if (!strcmp(id, "ASSERT")) directive_assert(p, 1);
    else if (!strcmp(id, "ASSERT1")) directive_assert(p, 2);
    else if (!strcmp(id, "ERROR")) directive_diagnostic(p, ERR_LVL_FATAL);
    else if (!strcmp(id, "WARNING")) directive_diagnostic(p, ERR_LVL_WARNING);
    else if (!strcmp(id, "ALIGN")) directive_fill(p, 1);
    else if (!strcmp(id, "REPEAT")) directive_repeat(p);
    else if (!strcmp(id, "ENDREP")) again = directive_endrep(p);
    else if (!strcmp(id, "NOLIST")) listing_enabled = 0;
    else if (!strcmp(id, "LIST")){
        listing_enabled = global_listing_enabled;
        list_skip_one = 1;
    }
    else if (!strcmp(id, "NOSYM")) symmap_enabled = 0;
    else if (!strcmp(id, "CPU")) directive_cpu(p);
    else error(ERR_NO_DIRECTIVE);
    return again;
}

static int is_mnemonic(const char* id){
    char id_uppercase[ID_LEN];
    int l = 0, r = instruction_tbl_size - 1, x;
    int cmp;
    strcpy(id_uppercase, id);
    to_uppercase(id_uppercase);

    while(r >= 1){
        x = l + ((r - l) >> 2);
        cmp = strcmp(id_uppercase, instruction_tbl[x].mn);
        if (!cmp) return 1;
        else if (cmp) l = x + 1;
        else r = x - 1;
    }
    return 0;
}

static int statement(char** p){
    char id1[ID_LEN];
    value v1;
    char* pt;
    int again = 0;
    enum{
        NONE=0,
        GLOBAL_LABEL=1,
        LOCAL_LABEL=2
    } label = NONE;

    skip_white_and_comment(p);
    if (IS_END(**p)) return 0;
    pt = *p;

    if (**p = LOCAL_LABEL_LETTER){
        (*p)++;
        ident(p, id1, 1, 0);
        skip_white(p);
        label = LOCAL_LABEL;
    }
    else if (isalpha(**p)){
        ident(p, id1, 0, 0);
        skip_white(p);
        label = GLOBAL_LABEL;
    }

    if (label && **p == '='){
        (*p)++;
        v1 = expr(p);
        if (label == GLOBAL_LABEL) define_variable(id1, v1, NULL);
        else{
            if (!current_label) error(ERR_NO_GLOBAL);
            define_variable(id1, v1, current_label);
        }
        
        return again;
    }
    else if (label && ((**p == ':') || (!is_mnemonic(id1)))){
        if (**p == ':') (*p)++;

        if (label == GLOBAL_LABEL)
            current_label = define_label(id1, address_counter, NULL);
        else{
            if (!current_label) error(ERR_NO_GLOBAL);
            define_label(id1, address_counter, current_label);
        }

        skip_white_and_comment(p);
        if (IS_END(**p)) return again;
    }
    else *p = pt;

    if (**p == DIRECTIVE_LETTER){
        (*p)++;
        again = directive(p);
    }
    else if (isalpha(**p)) instruction(p);
    else error(ERR_STMT);

    return again;
}

static void byte_to_pchar(uint8_t w, char* p){
    uint16_t v;
    v = (w >> 4) & 0xF;
    p[0] = (char)(v + '0' + ((v > 9) ? 'A' - '9' - 1 : 0));
    v = w & 0xF;
    p[1] = (char)(v + '0' + ((v > 9) ? 'A' - '9' - 1 : 0));
}

static void word_to_pchar(uint16_t w, char* p){
    uint16_t v;
    v = (w >> 12) & 0xF;
    p[0] = (char)(v + '0' + ((v > 9) ? 'A' - '9' - 1 : 0));
    v = (w >> 8) & 0xF;
    p[1] = (char)(v + '0' + ((v > 9) ? 'A' - '9' - 1 : 0));
    v = (w >> 4) & 0xF;
    p[2] = (char)(v + '0' + ((v > 9) ? 'A' - '9' - 1 : 0));
    v = w & 0xF;
    p[3] = (char)(v + '0' + ((v > 9) ? 'A' - '9' - 1 : 0));
}

static void list_statement(char* statement_start, uint16_t pc_start, uint16_t oc_start, char* p, int skipped){
    static char list_addr_buf[20] = "           ";
    static char list_code_buf[4] = "   ";
    int count = 0;

    if (!listing_enabled || list_skip_one) return;

    fprintf(list_file, "%5d", current_line);
    if (skipped) fputs("- ", list_file);
    else fputs(": ", list_file);

    if (oc_start < output_counter){
        word_to_pchar(oc_start, list_addr_buf);
        word_to_pchar(pc_start, list_addr_buf + 5);
        fputs(list_addr_buf, list_file);
    }
    else fputs("           ", list_file);

    while (oc_start < output_counter && count < 3){
        byte_to_pchar(code[oc_start++] & 0xFF, list_code_buf);
        fputs(list_code_buf, list_file);
        count++;
    }

    if (oc_start + count < output_counter) fputs("...", list_file);
    else{
        while(count < 4){
            fputs("   ", list_file);
            count++;
        }
    }
    fputs("  ", list_file);
    fwrite(statement_start, 1, (int)(p - statement_start), list_file);

    fputs("\n", list_file);
}

static int sym_cmp_name(const void* a, const void* b){
    const symbol *sa, *sb;
    sa = *(const symbol**)a;
    sb = *(const symbol**)b;

    return strcmp(sa->name, sb->name);
}

static int sym_cmp_val(const void* a, const void* b){
    const symbol *sa, *sb;
    int res;

    sa = *(const symbol**)a;
    sb = *(const symbol**)b;

    if (DEFINED(sa->val) && DEFINED(sb->val))
        res = sa->val.v - sb->val.v;
    else if (UNDEFINED(sa->val) && DEFINED(sb->val))
        res = -1;
    else if (DEFINED(sa->val) && UNDEFINED(sb->val))
        res = 1;
    else res = 0;

    return (res != 0) ? res : strcmp(sa->name, sb->name);
}

static symbol** symbol_tbl_to_array(void){
    symbol **tsym, *tsym2;
    symbol **sym_array, **sym_p;

    sym_array = sym_p = malloc(sizeof(symbol*)*((size_t)symbol_count + 1));
    if (!sym_array) return NULL;

    for (tsym = symbol_tbl; tsym < symbol_tbl + SYMBOL_TBL_SIZE; tsym++){
        if (!*tsym) continue;
        tsym2 = *tsym;
        while (tsym2){
            *sym_p++ = tsym2;
            tsym2 = tsym2->next;
        }
    }
    *sym_p = NULL;

    return sym_array;
}

static void fill_dots(char* s, int len){
    for (int i = 0; i < len; i++) s[i] = (i & 1) ? '.' : ' ';
}

static void list_symbols(void){
    symbol *sym, **sym_array, **sym_p;
    char name_buf[ID_LEN + 1];
    
    name_buf[ID_LEN] = '\0';

    sym_array = symbol_tbl_to_array();
    if (!sym_array) return;

    for (int i = 1; i <= 2; i++){
        sym_p = sym_array;
        if (i == 1){
            fputs("\n\nSYMBOLS BY NAME\n\n", list_file);
            qsort(sym_array, symbol_count, sizeof(symbol*), sym_cmp_name);
        } else {
            fputs("\n\nSYMBOLS BY VALUE\n\n", list_file);
            qsort(sym_array, symbol_count, sizeof(symbol*), sym_cmp_val);
        }

        fputs("NAME                              HEX    DEC  "
              "SYM TYPE  WHERE\n", list_file);
        for (; *sym_p; sym_p++){
            sym = *sym_p;
            if (!sym->show_in_map) continue;
            fill_dots(name_buf, ID_LEN);
            memcpy(name_buf, sym->name, strlen(sym->name));
            fputs(name_buf, list_file);

            if (DEFINED(sym->val)){
                fprintf(list_file, TYPE(sym->val) == TYPE_BYTE ? "   %02X  %5u  " : "   %04X  %5u",
                        sym->val.v, sym->val.v);
            }
            else fputs("    ?      ?  ", list_file);

            if (sym->kind == KIND_LBL) fputs("LBL ", list_file);
            else fputs("VAR ", list_file);

            if (sym->val.t == TYPE_BYTE) fputs("BYTE  ", list_file);
            else if (sym->val.t == TYPE_WORD) fputs("WORD  ", list_file);
            else fputs("?     ", list_file);

            if (sym->filename)
                fprintf(list_file, "%s:%d", sym->filename, sym->line);
            fputs("\n", list_file);
        }
    }
    free(sym_array);
}

static void list_filename(char* fn){
    if (listing_enabled) fprintf(list_file, " FILE: %s\n", fn);
}

static int conditional_statement(char** p){
    char id[ID_LEN];
    char* pt = *p;

    skip_white(p);

    if (**p != DIRECTIVE_LETTER) return 0;
    (*p)++;
    ident(p, id, 0, 1);

    if (!strcmp(id, "IF")){
        directive_if(p, 1, 0);
        return 1;
    } else if (!strcmp(id, "IFN")){
        directive_if(p, 0, 0);
        return 1;
    } else if (!strcmp(id, "IFDEF")){
        directive_if(p, 0, 1);
        return 1;
    } else if (!strcmp(id, "IFNDEF")){
        directive_if(p, 0, 1);
        return 1;
    } else if (!strcmp(id, "ELSE")){
        directive_else();
        return 1;
    } else if (!strcmp(id, "ENDIF")){
        directive_endif();
        return 1;
    }

    *p = pt;
    return 0;
}

static void pass(char** p){
    int err;

    char* statement_start;
    asm_file* last_file;
    uint16_t oc_start;
    uint16_t pc_start;
    int conditional;

    address_counter = 0;
    output_counter = 0;

    last_file = current_file;
    current_line = 1;
    current_label = NULL;
    listing_enabled = global_listing_enabled;
    symmap_enabled = 1;
    process_statements = 1;

    if (!(err = setjmp(error_jmp))){
        while (**p || pos_stk_ptr > 0){
            conditional = 0;
            if (current_file != last_file){
                if (pass_num == 2)
                    list_filename(current_file->filename);
            }

            if (!**p){
                pop_pos_stack(p);
                continue;
            }

            statement_start = *p;
            pc_start = address_counter;
            oc_start = output_counter;

            if (conditional_statement(p)) conditional = 1;
            else if (process_statements)
                if (statement(p)) continue;
            else skip_to_eol(p);

            skip_white_and_comment(p);

            if (!IS_END(**p)) error(ERR_EOL);

            if (pass_num == 2 && statement_start <= *p)
                list_statement(statement_start, pc_start, oc_start, *p,
                                !conditional && !process_statements);

            skip_eol(p);
            current_line++;

            list_skip_one = 0;
            last_file = current_file;
        }

        if (condition_sp < condition_stack + CONDITION_STATE_MAX)
            error(ERR_UNCLOSED_COND);
    }
    else{
        if (error_type == ERROR_NORM)
            printf("%s:%d: error: %s\n", current_file->filename,
                   current_line, err_msg[err]);
        else if (error_type == ERROR_EXT)
            printf("%s:%d: error: %s %s\n", current_file->filename, current_line,
                   err_msg[err], error_hint);
    }
}

static int save_code(const char* fn, const uint8_t* data, int len){
    FILE* f = fopen(fn, "wb");
    if (!f) return 0;
    if ((fwrite(data, len, 1, f) == 0) && (output_counter != 0)){
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

static int init_listing(char* fn){
    time_t t;
    struct tm* tm;
    char ts[80];

    list_file = fopen(fn, "wb");
    global_listing_enabled = list_file != NULL;

    if (!list_file) return 0;

    time(&t);
    tm = localtime(&t);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M", tm);

    fprintf(list_file, "ASM6502        PROGRAM LISTING AND SYMBOL TABLE        "
                       "DATE: %s\n\n"
                       "MAIN INPUT FILE: %s\n\n",
                       ts, current_file->filename);
    fprintf(list_file, " Line# Pos  Addr  Code           Source\n");

    return 1;
}

static char* source_filename = NULL;
static char* listing_filename = NULL;
static char* output_filename = NULL;

static int parse_args(char* argv[]){
    char* p;
    value v;

    argv++;
    while (*argv){
        if (**argv == '/' || **argv == '-'){
            if (!strcmp(*argv + 1, "q")) flag_quiet++;
            else if (!strcmp(*argv + 1, "o")){
                argv++;
                if (!*argv) return 0;
                output_filename = *argv;
            } else if (!strcmp(*argv + 1, "l")){
                argv++;
                if (!*argv) return 0;
                listing_filename = *argv;
            } else if (!strcmp(*argv + 1, "w0")) flag_diagnostic_level = 0;
            else if (!strcmp(*argv + 1, "w1")) flag_diagnostic_level = 1;
            else if (!strcmp(*argv + 1, "w2")) flag_diagnostic_level = 2;
            else return 0;
        } else if ((p = strchr(*argv, '=')) != NULL){
            *p++ = 0;
            if (!setjmp(error_jmp)){
                v = number(&p);
                define_variable(*argv, v, NULL);
            } else return 0;
        } else {
            if (source_filename) return 0;
            source_filename = *argv;
        }
        argv++;
    }
    return source_filename != NULL && output_filename != NULL;
}

static uint16_t am_size[16] = { 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 3, 3 };

static instruction_desc itbl_6502[56] = {
   { "ADC", { INV, INV, 0x69, INV, 0x65, 0x75, INV, 0x6d, 0x7d, 0x79, INV, 0x61, 0x71, INV, INV, INV } },
   { "AND", { INV, INV, 0x29, INV, 0x25, 0x35, INV, 0x2d, 0x3d, 0x39, INV, 0x21, 0x31, INV, INV, INV } },
   { "ASL", { 0x0a, INV, INV, INV, 0x06, 0x16, INV, 0x0e, 0x1e, INV, INV, INV, INV, INV, INV, INV } },
   { "BCC", { INV, INV, INV, 0x90, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BCS", { INV, INV, INV, 0xb0, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BEQ", { INV, INV, INV, 0xf0, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BIT", { INV, INV, INV, INV, 0x24, INV, INV, 0x2c, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BMI", { INV, INV, INV, 0x30, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BNE", { INV, INV, INV, 0xd0, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BPL", { INV, INV, INV, 0x10, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BRK", { INV, 0x00, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BVC", { INV, INV, INV, 0x50, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BVS", { INV, INV, INV, 0x70, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CLC", { INV, 0x18, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CLD", { INV, 0xd8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CLI", { INV, 0x58, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CLV", { INV, 0xb8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CMP", { INV, INV, 0xc9, INV, 0xc5, 0xd5, INV, 0xcd, 0xdd, 0xd9, INV, 0xc1, 0xd1, INV, INV, INV } },
   { "CPX", { INV, INV, 0xe0, INV, 0xe4, INV, INV, 0xec, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CPY", { INV, INV, 0xc0, INV, 0xc4, INV, INV, 0xcc, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "DEC", { INV, INV, INV, INV, 0xc6, 0xd6, INV, 0xce, 0xde, INV, INV, INV, INV, INV, INV, INV } },
   { "DEX", { INV, 0xca, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "DEY", { INV, 0x88, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "EOR", { INV, INV, 0x49, INV, 0x45, 0x55, INV, 0x4d, 0x5d, 0x59, INV, 0x41, 0x51, INV, INV, INV } },
   { "INC", { INV, INV, INV, INV, 0xe6, 0xf6, INV, 0xee, 0xfe, INV, INV, INV, INV, INV, INV, INV } },
   { "INX", { INV, 0xe8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "INY", { INV, 0xc8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "JMP", { INV, INV, INV, INV, INV, INV, INV, 0x4c, INV, INV, 0x6c, INV, INV, INV, INV, INV } },
   { "JSR", { INV, INV, INV, INV, INV, INV, INV, 0x20, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "LDA", { INV, INV, 0xa9, INV, 0xa5, 0xb5, INV, 0xad, 0xbd, 0xb9, INV, 0xa1, 0xb1, INV, INV, INV } },
   { "LDX", { INV, INV, 0xa2, INV, 0xa6, INV, 0xb6, 0xae, INV, 0xbe, INV, INV, INV, INV, INV, INV } },
   { "LDY", { INV, INV, 0xa0, INV, 0xa4, 0xb4, INV, 0xac, 0xbc, INV, INV, INV, INV, INV, INV, INV } },
   { "LSR", { 0x4a, INV, INV, INV, 0x46, 0x56, INV, 0x4e, 0x5e, INV, INV, INV, INV, INV, INV, INV } },
   { "NOP", { INV, 0xea, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "ORA", { INV, INV, 0x09, INV, 0x05, 0x15, INV, 0x0d, 0x1d, 0x19, INV, 0x01, 0x11, INV, INV, INV } },
   { "PHA", { INV, 0x48, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PHP", { INV, 0x08, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PLA", { INV, 0x68, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PLP", { INV, 0x28, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "ROL", { 0x2a, INV, INV, INV, 0x26, 0x36, INV, 0x2e, 0x3e, INV, INV, INV, INV, INV, INV, INV } },
   { "ROR", { 0x6a, INV, INV, INV, 0x66, 0x76, INV, 0x6e, 0x7e, INV, INV, INV, INV, INV, INV, INV } },
   { "RTI", { INV, 0x40, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RTS", { INV, 0x60, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SBC", { INV, INV, 0xe9, INV, 0xe5, 0xf5, INV, 0xed, 0xfd, 0xf9, INV, 0xe1, 0xf1, INV, INV, INV } },
   { "SEC", { INV, 0x38, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SED", { INV, 0xf8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SEI", { INV, 0x78, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "STA", { INV, INV, INV, INV, 0x85, 0x95, INV, 0x8d, 0x9d, 0x99, INV, 0x81, 0x91, INV, INV, INV } },
   { "STX", { INV, INV, INV, INV, 0x86, INV, 0x96, 0x8e, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "STY", { INV, INV, INV, INV, 0x84, 0x94, INV, 0x8c, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TAX", { INV, 0xaa, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TAY", { INV, 0xa8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TSX", { INV, 0xba, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TXA", { INV, 0x8a, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TXS", { INV, 0x9a, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TYA", { INV, 0x98, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } }
};

static instruction_desc itbl_65c02[98] = {
   { "ADC", { INV, INV, 0x69, INV, 0x65, 0x75, INV, 0x6d, 0x7d, 0x79, INV, 0x61, 0x71, 0x72, INV, INV } },
   { "AND", { INV, INV, 0x29, INV, 0x25, 0x35, INV, 0x2d, 0x3d, 0x39, INV, 0x21, 0x31, 0x32, INV, INV } },
   { "ASL", { 0x0a, INV, INV, INV, 0x06, 0x16, INV, 0x0e, 0x1e, INV, INV, INV, INV, INV, INV, INV } },
   { "BBR0", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x0f } },
   { "BBR1", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x1f } },
   { "BBR2", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x2f } },
   { "BBR3", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x3f } },
   { "BBR4", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x4f } },
   { "BBR5", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x5f } },
   { "BBR6", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x6f } },
   { "BBR7", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x7f } },
   { "BBS0", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x8f } },
   { "BBS1", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0x9f } },
   { "BBS2", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0xaf } },
   { "BBS3", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0xbf } },
   { "BBS4", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0xcf } },
   { "BBS5", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0xdf } },
   { "BBS6", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0xef } },
   { "BBS7", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 0xff } },
   { "BCC", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BCS", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BEQ", { INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BIT", { INV, INV, 0x89, INV, 0x24, 0x34, INV, 0x2c, 0x3c, INV, INV, INV, INV, INV, INV, INV } },
   { "BMI", { INV, INV, INV, 0x30, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BNE", { INV, INV, INV, 0xd0, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BPL", { INV, INV, INV, 0x10, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BRA", { INV, INV, INV, 0x80, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BRK", { INV, 0x00, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BVC", { INV, INV, INV, 0x50, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "BVS", { INV, INV, INV, 0x70, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CLC", { INV, 0x18, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CLD", { INV, 0xd8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CLI", { INV, 0x58, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CLV", { INV, 0xb8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CMP", { INV, INV, 0xc9, INV, 0xc5, 0xd5, INV, 0xcd, 0xdd, 0xd9, INV, 0xc1, 0xd1, 0xd2, INV, INV } },
   { "CPX", { INV, INV, 0xe0, INV, 0xe4, INV, INV, 0xec, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "CPY", { INV, INV, 0xc0, INV, 0xc4, INV, INV, 0xcc, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "DEC", { 0x3a, INV, INV, INV, 0xc6, 0xd6, INV, 0xce, 0xde, INV, INV, INV, INV, INV, INV, INV } },
   { "DEX", { INV, 0xca, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "DEY", { INV, 0x88, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "EOR", { INV, INV, 0x49, INV, 0x45, 0x55, INV, 0x4d, 0x5d, 0x59, INV, 0x41, 0x51, 0x52, INV, INV } },
   { "INC", { 0xee, INV, INV, INV, 0xe6, 0xf6, INV, 0xee, 0xfe, INV, INV, INV, INV, INV, INV, INV } },
   { "INX", { INV, 0xe8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "INY", { INV, 0xc8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "JMP", { INV, INV, INV, INV, INV, INV, INV, 0x4c, INV, INV, 0x6c, INV, INV, INV, 0x7c, INV } },
   { "JSR", { INV, INV, INV, INV, INV, INV, INV, 0x20, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "LDA", { INV, INV, 0xa9, INV, 0xa5, 0xb5, INV, 0xad, 0xbd, 0xb9, INV, 0xa1, 0xb1, 0xb2, INV, INV } },
   { "LDX", { INV, INV, 0xa2, INV, 0xa6, INV, 0xb6, 0xae, INV, 0xbe, INV, INV, INV, INV, INV, INV } },
   { "LDY", { INV, INV, 0xa0, INV, 0xa4, 0xb4, INV, 0xac, 0xbc, INV, INV, INV, INV, INV, INV, INV } },
   { "LSR", { 0x4a, INV, INV, INV, 0x46, 0x56, INV, 0x4e, 0x5e, INV, INV, INV, INV, INV, INV, INV } },
   { "NOP", { INV, 0xea, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "ORA", { INV, INV, 0x09, INV, 0x05, 0x15, INV, 0x0d, 0x1d, 0x19, INV, 0x01, 0x11, 0x12, INV, INV } },
   { "PHA", { INV, 0x48, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PHP", { INV, 0x08, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PHX", { INV, 0xda, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PHY", { INV, 0x5a, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PLA", { INV, 0x68, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PLP", { INV, 0x28, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PLX", { INV, 0xfa, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "PLY", { INV, 0x7a, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RMB0", { INV, INV, INV, INV, 0x07, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RMB1", { INV, INV, INV, INV, 0x17, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RMB2", { INV, INV, INV, INV, 0x27, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RMB3", { INV, INV, INV, INV, 0x37, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RMB4", { INV, INV, INV, INV, 0x47, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RMB5", { INV, INV, INV, INV, 0x57, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RMB6", { INV, INV, INV, INV, 0x67, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RMB7", { INV, INV, INV, INV, 0x77, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "ROL", { 0x2a, INV, INV, INV, 0x26, 0x36, INV, 0x2e, 0x3e, INV, INV, INV, INV, INV, INV, INV } },
   { "ROR", { 0x6a, INV, INV, INV, 0x66, 0x76, INV, 0x6e, 0x7e, INV, INV, INV, INV, INV, INV, INV } },
   { "RTI", { INV, 0x40, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "RTS", { INV, 0x60, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SBC", { INV, INV, 0xe9, INV, 0xe5, 0xf5, INV, 0xed, 0xfd, 0xf9, INV, 0xe1, 0xf1, 0xf2, INV, INV } },
   { "SEC", { INV, 0x38, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SED", { INV, 0xf8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SEI", { INV, 0x78, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SMB0", { INV, INV, INV, INV, 0x87, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SMB1", { INV, INV, INV, INV, 0x97, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SMB2", { INV, INV, INV, INV, 0xa7, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SMB3", { INV, INV, INV, INV, 0xb7, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SMB4", { INV, INV, INV, INV, 0xc7, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SMB5", { INV, INV, INV, INV, 0xd7, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SMB6", { INV, INV, INV, INV, 0xe7, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "SMB7", { INV, INV, INV, INV, 0xf7, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "STA", { INV, INV, INV, INV, 0x85, 0x95, INV, 0x8d, 0x9d, 0x99, INV, 0x81, 0x91, 0x92, INV, INV } },
   { "STP", { INV, 0xdb, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "STX", { INV, INV, INV, INV, 0x86, INV, 0x96, 0x8e, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "STY", { INV, INV, INV, INV, 0x84, 0x94, INV, 0x8c, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "STZ", { INV, INV, INV, INV, 0x64, 0x74, INV, 0x9c, 0x9e, INV, INV, INV, INV, INV, INV, INV } },
   { "TAX", { INV, 0xaa, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TAY", { INV, 0xa8, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TRB", { INV, INV, INV, INV, 0x14, INV, INV, 0x1c, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TSB", { INV, INV, INV, INV, 0x04, INV, INV, 0x0c, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TSX", { INV, 0xba, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TXA", { INV, 0x8a, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TXS", { INV, 0x9a, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "TYA", { INV, 0x98, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } },
   { "WAI", { INV, 0xcb, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV } }
};