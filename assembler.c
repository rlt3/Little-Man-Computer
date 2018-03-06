#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "arg.h"


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

static struct Token token[MAX_TOKENS] = {0};
static int max_tokens = 0;

/*
 * hash("FOO", -1)
 * hash(&line[start], end)
 */
static inline int
hash (const char *str, const int end)
{
    int i = 0, h = 0;
    while (str[i] != '\0') {
        h += ((int) str[i]) + i;
        i++;
        if (end > -1 && i > end)
            break;
    }
    return h + i;
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
    if (*part == LABEL) {
        //printf("%d LABEL %.*s => %d\n", hsh, end + 1, str, line_num);
        add_token(LABEL, hsh, line_num);
    }

next:
    (*part)++;
}

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
        memory[line_num] = tok->val;
        //printf("%d | %d", line_num, tok->val);
        goto next;
    }

    /* if we're on the DATA part and ran into a token, get its value */
    if (*part == DATA && tok) {
        memory[line_num] += tok->val;
        //printf(" %d", tok->val);
        goto next;
    }

    /* if were on the DATA but no token, must be raw numbers */
    if (*part == DATA && !tok) {
        memory[line_num] += atoi(str);
        //printf(" %d", atoi(str));
        goto next;
    }

next:
    (*part)++;
}

void
parse_line (const char *line, const int line_num, LineFunction f)
{
    int i, start = -1;
    /* lines are divided into parts like: [label] OP [loc] */
    int part = 0;

    for (i = 0; line[i] != '\0'; i++) {

        /* if we encounter whitespace */
        if (line[i] == ' ' || line[i] == '\n' || line[i] == '\t') {
            /* and we have the start of a string, i - 1 must be the end of it */
            if (start != -1) {
                f(&line[start], i - 1 - start, line_num, &part);
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
    add_token(OP, hash("HLT", -1),   0);
    add_token(OP, hash("ADD", -1), 100); 
    add_token(OP, hash("SUB", -1), 200); 
    add_token(OP, hash("STA", -1), 300); 
    add_token(OP, hash("LDA", -1), 500); 
    add_token(OP, hash("BRA", -1), 600);
    add_token(OP, hash("BRZ", -1), 700); 
    add_token(OP, hash("BRP", -1), 800); 
    add_token(OP, hash("INP", -1), 901); 
    add_token(OP, hash("OUT", -1), 902); 
    add_token(OP, hash("PRT", -1), 903);
    add_token(OP, hash("DAT", -1), 0);
}

static inline void
parse_file (FILE *fp, LineFunction f)
{
    char *curr_line = NULL;
    int line_num = 0;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&curr_line, &len, fp)) != -1 && ++line_num)
        parse_line(curr_line, line_num, f);

    if (curr_line)
        free(curr_line);
}

int
assemble (const char *file);
{
    FILE *fp = fopen(file, "r");

    if (!fp) {
        fprintf(stderr, "No such file: %s\n", file);
        return 1;
    }

    set_default_tokens();

    parse_file(fp, tokenize_line);
    rewind(fp);
    parse_file(fp, assemble_line);
    
    fclose(fp);
    return 0;
}
