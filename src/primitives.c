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

#ifdef LEXICAL_SCOPING

char * CLOSURE; // Reference to the unique string used for a closure variable
char * PARENT;  // Reference to the unique string used to point to a closure for the parent scope

#endif

void init() {
    variables = malloc(sizeof(Variable) * NUM_VARS);
    top_variables = variables;
    end_variables = variables + (sizeof(Variable) * NUM_VARS);
    printf("sizeof Variable: %d\n", (int) sizeof(Variable));
//    printf("sizeof intptr: %ld void *: %ld\n", sizeof(intptr_t), sizeof(void *));
//    printf("value offset: %ld\n", (char **) &(variables->value.ptr) - &(variables->name));

#ifdef LEXICAL_SCOPING
    CLOSURE = "(closure)"; // this string only exists at runtime
    PARENT = NULL; // simplest value to share between C and asm
#endif
}

Variable * slot(char * name) {
    // printf("Find var %s in %p %p\n", name, top_variables, variables);
    for (Variable * var = top_variables-1; var >= variables; var--) {

#ifdef LEXICAL_SCOPING
        if(var->name == PARENT) {
            // printf("Skipping to parent %p\n", var->value.ptr);
            //if (var <= var->value.ptr) { printf("Forward reference detected!\n"); exit(-1); }
            var = (Variable *) var->value.ptr; // skip to parent
            continue;
        }
#endif

        if (var->name == name) { // exact same string pointer
            // printf("Var %s = %ld %p\n", name, var->value.num, var);
            return var;
        }
//        else printf("Not %s %p\n", var->name, var);
    }
    printf("Runtime error: var %s not found\n", name);
    exit(-1);
}

intptr_t define(char * name, intptr_t val) {
    if (top_variables >= end_variables) printf("Var stack is full\n");
    Variable * var = top_variables++;
    // printf("Defining %s as %ld %p\n", name, val, var);
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

#ifdef LEXICAL_SCOPING

// As in regular pasta, we 'abuse' the var stack to store the reference to
// an anonymous function at its point of definition, which precisely marks its lexical scope.
Variable * bind(void * val) {
    Variable * var = top_variables++;
    // printf("Binding a block: %p to %p\n", var, val);
    var->name = CLOSURE;
    var->value.ptr = val;
    return var;
}

typedef intptr_t (*GenericFunction) (void*, void*, void*, void*, void*);

/*
intptr_t funcall(GenericFunction func, char * a, char * b, char * c, char * d, char * e) {
    define(PARENT, (intptr_t) func);
    func = ((Variable *) func)->value.ptr;
    intptr_t result = func(a, b, c, d, e);
    top_variables--; // remove parent ref
    return result;
}
*/
#endif

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
