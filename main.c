#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "arg.h"

/*
 * CPU
 */

#define MEM_SIZE 100

/*
 * All Instructions are like XYY where X is the OP code and YY is the loc in
 * memory being modified. The exception is IO where YY is the type of IO being
 * performed.
 */
enum OP {
    HLT = 0,    /* HLT: halt the program */
    ADD = 100,  /* ADD [loc]: add value at location to register value */
    SUB = 200,  /* SUB [loc]: subtract value at location from register value */
    STA = 300,  /* STA [loc]: store register value at memory location */
    LDD = 400,  /* LDD [loc]: dereference value at loc and load to register */
    LDA = 500,  /* LDA [loc]: load value from memory to register */
    BRA = 600,  /* BRA [loc]: set pc to value */
    BRZ = 700,  /* BRZ [loc]: set pc to value if register is 0 */
    BRP = 800,  /* BRP [loc]: set pc to value if positive */
    IO  = 900   /* Handle IO */
};

enum IO_OP {
    INP = 1,  /* 901: INP: get next input to the register */
    OUT = 2,  /* 902: OUT: output register */
    PRT = 3   /* 903: PRT: print register as ASCII */
};

static int memory[MEM_SIZE] = {0};
static int program_counter = 0;
static int registr = 0;
static int input_index = 2;


/*
 * Get the instruction's loc
 */
static inline int
inst_loc (int instruction)
{
    return (instruction % MEM_SIZE);
}

/*
 * Get the instruction's OP code
 */
static inline int
inst_op (int instruction)
{
    return instruction - inst_loc(instruction);
}

/*
 * Get input from the args and advance the index.
 * Returns 0 if there is no input.
 */
static inline int
get_input (int argc, char **argv)
{
    if (input_index >= argc)
        return 0;

    return atoi(argv[input_index++]);
}

void
handle_instruction (int *run, int argc, char **argv, int is_debug)
{
    int inst = memory[program_counter];
    int op   = inst_op(inst);
    int loc  = inst_loc(inst);

    if (is_debug)
        printf("%d> REG: %d | OP: %d | LOC: %d | MEM[LOC]: %d\n",
                program_counter, registr, op, loc, memory[loc]);

    program_counter++;

    switch (op) {
    case HLT:
        *run = 0;
        break;

    case ADD:
        registr += memory[loc];
        break;

    case SUB:
        registr -= memory[loc];
        break;

    case STA:
        memory[loc] = registr;
        break;

    case LDD:
        registr = memory[memory[loc]];
        break;

    case LDA:
        registr = memory[loc];
        break;

    case BRA:
        program_counter = loc;
        break;

    case BRZ:
        if (registr == 0)
            program_counter = loc;
        break;

    case BRP:
        if (registr > 0)
            program_counter = loc;
        break;

    case IO:
        switch (loc) {
        case INP:
            registr = get_input(argc, argv);
            if (is_debug)
                printf("INP: %d\n", registr);
            break;

        case OUT:
            if (is_debug)
                printf("OUT: %d\n", registr);
            else
                printf("%d", registr);
            break;

        case PRT:
            if (is_debug)
                printf("PRT: %c\n", registr);
            else
                printf("%c", registr);
            break;

        default: break;
        }
        break;

    default: break;
    }
}

/*
 * ASSEMBLER
 */

#define MAX_TOKENS 256

enum TokenType {
    LABEL = 0, /* string labels to data values -- loop, end, etc */
    OP,        /* the mnemonic OP codes -- HLT, BRZ, INP, etc. */
    DATA,      /* numeric location or values -- 1, 8, 9, etc. */
};

struct Token {
    enum TokenType type;
    int hash;
    int val;
};

static struct Token token[MAX_TOKENS] = {};
static int max_tokens = 0;

/*
 * hash(&line[start], end)
 * hash("FOO", 3)
 *
 * A *very* simple rotating hash function (with in-built prime) for producing
 * 32-bit integers.
 */
static inline int 
hash (const char *str, int len)
{
    int h, i;
    for (h = len, i = 0; i < len; i++)
        h = (h<<4)^(h>>28)^str[i];
    return h % 4294967291;
}

static inline struct Token*
get_token (const int hash)
{
    int i;
    for (i = 0; i < max_tokens; i++)
        if (token[i].hash == hash)
            return &token[i];
    return NULL;
}

static inline void
add_token (enum TokenType type, const int hsh, const int val)
{
    token[max_tokens].hash = hsh;
    token[max_tokens].type = type;
    token[max_tokens].val  = val;
    max_tokens++;
}

typedef void (*LineFunction)(const char*, const int, const int, int*);

/*
 * Find labels on each line and tokenize them so that when we assemble each
 * line we have a reference to that token.
 */
void
tokenize_line (const char *str, const int end, const int line_num, int *part)
{
    const int hsh = hash(str, end);
    const struct Token *tok = get_token(hsh);

    if (tok) {
    /* if the token exists as an OP that means we've skipped the label part */
        if (tok->type == OP)
            *part = 1;
        goto next;
    }

    /* only add a token if it is labeling some part (not being used as ref) */
    if (*part == LABEL)
        add_token(LABEL, hsh, line_num);

next:
    (*part)++;
}

/*
 * Using the global `memory` location, assemble the instructions from the 
 * tokens on each line.
 */
void
assemble_line (const char *str, const int end, const int line_num, int *part)
{
    const struct Token *tok = get_token(hash(str, end));

    /* if we're on an actual LABEL, skip it (we already tokenized it) */
    if (*part == LABEL && tok && tok->type == LABEL)
        goto next;

    /* if we're on LABEL but we've got an OP, we must be on OP */
    if (*part == LABEL && tok && tok->type == OP)
        *part = OP;

    if (*part == OP && tok && tok->type == OP) {
        memory[line_num] += tok->val;
        goto next;
    }

    /* if we're on the DATA part and ran into a token, get its value */
    if (*part == DATA && tok) {
        memory[line_num] += tok->val;
        goto next;
    }

    /* if were on the DATA but no token, must be raw numbers */
    if (*part == DATA && !tok) {
        memory[line_num] += atoi(str);
        goto next;
    }

next:
    (*part)++;
}

/*
 * Find specific words in a line and pass the beginning of that word and the
 * length for it to the LineFunction.
 */
void
parse_line (const char *line, const int line_num, LineFunction f)
{
    int i, start = -1;
    /* lines are divided into parts like: [label] OP [loc] */
    int part = 0;

    for (i = 0; line[i] != '\0'; i++) {

        /* if we encounter whitespace */
        if (line[i] == ' ' || line[i] == '\n' || line[i] == '\t') {
            /* and we have the start of a string, i - start must be the end */
            if (start != -1) {
                f(&line[start], i - start, line_num, &part);
                start = -1;
            }
            continue;
        }

        if (start == -1)
            start = i;
    }
}

static inline void
set_default_tokens ()
{
    add_token(OP, hash("HLT", 3),   0);
    add_token(OP, hash("ADD", 3), 100); 
    add_token(OP, hash("SUB", 3), 200); 
    add_token(OP, hash("STA", 3), 300); 
    add_token(OP, hash("LDD", 3), 400); 
    add_token(OP, hash("LDA", 3), 500); 
    add_token(OP, hash("BRA", 3), 600);
    add_token(OP, hash("BRZ", 3), 700); 
    add_token(OP, hash("BRP", 3), 800); 
    add_token(OP, hash("INP", 3), 901); 
    add_token(OP, hash("OUT", 3), 902); 
    add_token(OP, hash("PRT", 3), 903);
    add_token(OP, hash("DAT", 3), 0);
}

static inline int
parse_file (FILE *fp, LineFunction f)
{
    char *curr_line = NULL;
    int line_num = 0;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&curr_line, &len, fp)) != -1)
        parse_line(curr_line, line_num++, f);

    if (curr_line)
        free(curr_line);

    return line_num;
}

void
assemble (const char *file, int is_debug)
{
    FILE *fp = fopen(file, "r");
    int i, lines;

    if (!fp) {
        fprintf(stderr, "No such file: %s\n", file);
        exit(1);
    }

    set_default_tokens();

    lines = parse_file(fp, tokenize_line);
    rewind(fp);
    parse_file(fp, assemble_line);
    
    fclose(fp);

    if (is_debug) {
        puts("Assembler Tokens:");
        for (i = 0; token[i].hash != 0; i++)
            printf("%d => %d\n", token[i].hash, token[i].val);
        putchar('\n');

        puts("Memory Layout:");
        for (i = 0; i < lines; i++)
            printf("%d> %d\n", i, memory[i]);
        putchar('\n');
    }
}

int
main (int argc, char **argv)
{
    int is_debug = 0;
    int running = 1;

    if (argc <= 1) {
        fprintf(stderr, 
                "%s [-debug] <ASSEMBLY-FILE> [input] [input]\n", 
                argv[0]);
        exit(1);
    }

    if (strncmp("-debug", argv[1], 2) == 0) {
        is_debug = 1;
        input_index++; /* input to the program comes after the assembly file */
    }
    
    assemble(argv[input_index - 1], is_debug);

    while (running)
        handle_instruction(&running, argc, argv, is_debug);

    return 0;
}
