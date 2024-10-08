#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "parsta.h"

int printnum(int a) {
    printf("%d\n", a);
    return a;
}

int print(char * a, char * b, char * c, char * d, char * e, char * f) {
    printf("%s", a); if (b == NULL) return 0;
    printf("%s", b); if (c == NULL) return 0;
    printf("%s", c); if (d == NULL) return 0;
    printf("%s", d); if (e == NULL) return 0;
    printf("%s", e); if (f == NULL) return 0;
    printf("%s", f);
    return 0;
}

// Var support
typedef struct Variable {
    char * name;
   union {
        intptr_t num;
        void * ptr;
    } value;
} Variable;

#define NUM_VARS 256
Variable * variables;
Variable * top_variables; // In assembly, this is easier than a counter
Variable * end_variables;

void init() {
    variables = malloc(sizeof(Variable) * NUM_VARS);
    top_variables = variables;
    end_variables = variables + (sizeof(Variable) * NUM_VARS);
    //printf("sizeof Variable: %ld\n", sizeof(Variable));
}

Variable * slot(char * name) {
    // printf("Find var %s in %p %p\n", name, top_variables, variables);
    for (Variable * var = top_variables-1; var >= variables; var--) {
        if (var->name == name) { // exact same string pointer
            // printf("Found it!\n");
            return var;
        }
    }
    printf("Runtime error: var %s not found\n", name);
    exit(-1);
//    return NULL;
}

intptr_t define(char * name, intptr_t val) {
    // printf("Defining %s as %ld\n", name, val);
    Variable * var = top_variables++;
    var->name = name;
    var->value.num = val;
    return val;
}

intptr_t get(char * name) {
    Variable * var = slot(name);
    if (var == NULL) return 0;
    return var->value.num;
}

intptr_t set(char * name, intptr_t val) {
    //printf("Setting %s to %d\n", name, val);
    Variable * var = slot(name);
    if (var == NULL) return 0;
    var->value.num = val;
    return val;
}

char buffer[25];

char * dollar(intptr_t val, intptr_t base, intptr_t positions) {
    if (base == 0) { base = 10; positions = 0; } // if varargs stopped at base, then space may have garbage
    if (positions == 0) {
        // Decide positions based on val
        uint16_t val2 = val;
        while (val2 != 0) { val2 = val2 / base; positions++; }
        if (positions == 0) positions = 1;
    }

    int divider = 1;
    while (positions > 1) { divider *= base; positions--; }

    int i = 0;
    while (divider != 0) {
        uint8_t digit = (val / divider) % base;
        uint8_t start = digit < 10 ? '0' : 'a' - 10;
        buffer[i++] = start + ((val / divider) % base);
        divider /= base;
    }

    buffer[i++] = '\0';
    return buffer;
}
