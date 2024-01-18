#define DIRECTIVE_LETTER '.'
#define LOCAL_LABEL_LETTER '@'
#define PROGRAM_COUNTER_LETTER '@'

#define AND_LETTER '&'
#define OR_LETTER '|'
#define EOR_LETTER '^'

#define ID_LEN 32
#define STR_LEN 255
#define SYMBOL_TBL_SIZE 1024

#define MAX_FILES 64
#define MAX_POS_STACK 32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <stdint.h>

static int flag_quiet = 0;

static int pass_num;

static uint16_t address_counter = 0;
static uint16_t output_counter = 0;

static uint8_t* code = NULL;
static uint16_t code_size;

static uint16_t current_line;

typedef struct asm_file {
    char* filename;
    char* text;
} asm_file;

typedef struct pos_stack {
    asm_file* file;
    char* pos;
    uint16_t line;
    int listing_enabled;
    int global_listing_enabled;
    int sym_map_enabled;
} pos_stack;

static asm_file asm_files[MAX_FILES];
static int asm_file_count = 0;
static asm_file* current_file;
static char filename_bufp[STR_LEN];

static pos_stack pos_stk[MAX_POS_STACK];
static int pos_stk_ptr = 0;

static int global_listing_enabled = 0;
static int listing_enabled = 0;
static int list_skip_one = 0;

static int symmap_enabled = 0;
static FILE* list_file;

static int process_statements = 1;

typedef struct value {
    uint16_t value;
    uint8_t type;
    uint8_t defined;
} value;

enum {
    TYPE_BYTE=1,
    TYPE_WORD=2
};

typedef struct symbol{
    char name[ID_LEN];
    value value;
    uint8_t kind;
    struct symbol* next;
    struct symbol* locals;
    char* filename;
    uint16_t line;
    int show_in_map;
} symbol;

enum{
    KIND_LBL=1,
    KIND_VAR=2
};

symbol* symbol_tbl[SYMBOL_TBL_SIZE];

static int symbol_count = 1;
static symbol* current_label = NULL;

enum{
    AM_ACC=0,
    AM_IMP,
    AM_IMM,
    AM_REL,
    AM_ZP,
    AM_ZPX,
    AM_ZPY,
    AM_ABS,
    AM_ABX,
    AM_ABY,
    AM_AIN,
    AM_ZIX,
    AM_ZIY,
    AM_ZIN,
    AM_AIX,
    AM_ZPR,
    AM_INV
};

enum{
    OP_RTS=0x60,
    OP_JSR=0x20,
    INV=0xFC
};

typedef struct instruction_desc{
    char mn[5];
    uint8_t op[16];
} instruction_desc;

static instruction_desc* instruction_tbl;
static int instruction_tbl_size;
static uint16_t am_size[16];

#define AM_VALID(instr, am) ((instr).op[am] != INV)
#define MAXINT(a, b) (((b) >= (a)) ? (b) : (a))

#define IS_LBL(x) ((x).kind == KIND_LBL)
#define IS_VAR(x) ((x).kind == KIND_VAR)

#define DEFINED(x) (((x).defined) != 0)
#define UNDEFINED(x) (((x).defined) == 0)
#define SET_DEFINED(v) ((v).defined = 1)
#define SET_UNDEFINED(v) ((v).defined = 0)
#define INFER_DEFINED(a, b) (a).defined = DEFINED(a) || DEFINED(b)

#define TYPE(v) ((v).t)
#define SET_TYPE(v, u) ((v).t = (u))

#define NUM_TYPE(x) (((x) < 0x100) ? TYPE_BYTE : TYPE_WORD)

#define INFER_TYPE(a, b) \
    (((a).v >= 0x100) || ((b).v >= 0x1000)) \
    ? SET_TYPE((a), TYPE_WORD) \
    : SET_TYPE((a), MAXINT(TYPE(a), (TYPE(b))))

typedef enum{
    CONDITION_IF,
    CONDITION_REPEAT
} condition_type;

typedef struct condition_state{
    condition_type typ;
    union{
        struct{
            uint8_t process_statements;
            uint8_t condition_met;
        } if_;
        struct{
            asm_file* file;
            char* pos;
            uint16_t line;
            uint16_t count;
        } rep;
    } u;
} condition_state;

#define CONDITION_STATE_MAX 32
static condition_state condition_stack[CONDITION_STATE_MAX];
static condition_state* condition_sp = condition_stack + CONDITION_STATE_MAX;

enum{
    ERR_LVL_WARNING,
    ERR_LVL_FATAL
};

enum{
    DIAGNOSTIC_LVL_WARN=1,
    DIAGNOSTIC_LVL_NOTICE
};

static int flag_diagnostic_level = DIAGNOSTIC_LVL_WARN;

enum{
    ERR_NUM=1,
    ERR_UNBALANCED,
    ERR_ID,
    ERR_ID_LEN,
    ERR_STMT,
    ERR_EOL,
    ERR_REDEFINITION,
    ERR_INSTR,
    ERR_AM,
    ERR_CLOSING_PAREN,
    ERR_ZIX,
    ERR_ZIY,
    ERR_NO_DIRECTIVE,
    ERR_UNDEF,
    ERR_ILLEGAL_TYPE,
    ERR_RELATIVE_RANGE,
    ERR_STR_END,
    ERR_BYTE_RANGE,
    ERR_NO_GLOBAL,
    ERR_CHR,
    ERR_STRLEN,
    ERR_STR,
    ERR_OPEN,
    ERR_MAX_INC,
    ERR_NO_BYTE,
    ERR_NO_MEM,
    ERR_MISSING_IF,
    ERR_UNCLOSED_COND,
    ERR_MAX_COND,
    ERR_PHASE,
    ERR_PHASE_SIZE,
    ERR_DIV_BY_ZERO,
    ERR_CPU_UNSUPPORTED,
    ERR_MISSING_REP
}/

static char* err_msg[] = {
   "",
   "value expected",
   "unbalanced parentheses",
   "identifier expected",
   "identifier length exceeded",
   "illegal statement",
   "end of line expected",
   "illegal redefinition",
   "unknown instruction mnemonic",
   "invalid addressing mode for instruction",
   "missing closing brace",
   "malformed indirect X addressing",
   "malformed indirect Y addressing",
   "unknown directive",
   "undefined value",
   "illegal type",
   "relative jump target out of range",
   "string not terminated",
   "byte value out of range",
   "no scope for local definition",
   "malformed character constant",
   "string too long",
   "string expected",
   "can not read file",
   "maximum number of files exceeded",
   "byte sized value expected",
   "insufficient memory",
   "missing .IF",
   "unclosed conditional",
   "condition stack exhausted",
   "symbol value mismatch between pass one and two",
   "pass two code size greater than pass one code size",
   "division by zero",
   "CPU not supported",
   "missing .REPEAT",
};

enum{
    ERROR_NORM,
    ERROR_EXT,
    ERROR_ABORT
};

static char error_hint[128];
static int errors = 0;
static int error_type = 0;

jmp_buf error_jmp;