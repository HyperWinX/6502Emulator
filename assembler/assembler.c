#include "assembler.h"

static uint8_t current_opcode = INV;
static uint8_t last_opcode = INV;
static instruction_desc itbl_6502[56];
static instruction_desc itbl_65c02[98];

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
            code[output_counter] = w & 0xFF:
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
        else error[ERR_PHASE_SIZE];
    }
    output_counter += 2;
}

static void emit_instr_2(instruction_desc* instr, int am, uint16_t o){
    if(pass_num == 2){
        if (output_count < code_size - 2){
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
        (*p)++:
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
    char* dst = calloc(strlen(src) + 1);
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
    if (pos_stack_ptr >= MAX_POS_STACK) error(ERR_MAX_INC);
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
    symmap_enabled = stk->symmap_enabled;
}

static void directive_include(char** p){
    asm_file* file;

    skip_white(p);
    string_lit(p, filename_buf, STR_LEN);
    skip_white_and_comment(p);
    if (!IS_END(**p)) error(ERR_EOL);

    skip_eol(p);

    file = read_file(filename_buf);
    if (!file) error_ext(ERR_OPEN, filename_buf);

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
    string_lit(p, filename_buf, STR_LEN);
    skip_white_and_comment(p);

    file = open_file(filename_buf, "rb");
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
    condition_sp++:
}

static void echo(char** p){
    value v;
    int next, print_hex;

    do{
        next = 0;
        skip_white(p);

        if (**p == '"'){
            string_lit(p, filename_buf, STR_LEN);
            printf(filename_buf);
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
    if(condition_sp >= condition_stack + CONDITION_STATE_MAX || condition_sp->ty != CONDITION_REPEAT)
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
    int l = 0; r = instruction_tbl_size - 1, x;
    int cmp;
    strcpy(id_uppercase, id);
    to_uppercase(id_uppercase);

    while(r >= 1){
        
    }
}