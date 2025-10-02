#include <stdio.h>

#include "parsta.h"

/**
 * This Darwin (Mac OS) Aarch64 port targets the LLVM assembler.
 * I'm not sure whether this adds further dialect differences to the Linux Aarch32 port.
 * Another small platform difference is that C functions are prepended with an underscore.
 */
//char * cmdnames[] = { "add", "sub", "mul", "div", "remainder", "equals"};
const char * cmdnames[] = { "add", "sub", "and", "orr", "eor", "not", "mul", "udiv", "remainder", "equals", "lt", "gt", "lte", "gte", "lnot", "land", "lor", "dollar"};
char * regnames[] = { "x8", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"} ; // NOTE: first reg in this list is for function pointer (if needed)
//char * retnames[] = { "x9", "x10", "x11", "x12", "x13", "x14", "x15" } ; // these are all caller saved and at our disposal on aarch64; but retnames must be callee saved
char * retnames[] = { "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26" } ; // these are callee saved, so we use these for stashing return values


const int NUM_ARG_REGS = 7;
const int NUM_BUILTINS = 5; // Number of CPU-native binary operators in 'cmdnames'
const int NUM_COMMANDS = sizeof(cmdnames) / sizeof(char *); // Total number of commands with translations to primitive names in 'cmdnames'
int num_regnames = sizeof(regnames) / sizeof(char *);
int num_retnames = sizeof(retnames) / sizeof(char *);

#ifdef LEXICAL_SCOPING
// TODO: choose register to pass closure to be independent from those used for return values
#define CLOSURE_REG "x26"
#endif

void emit_start(FILE * out) {
    // Improvement suggestions:
    // 1) only emit asm utility functions that are actually referenced (e.g. at end instead of start)
    // 2) clang: define built-in-support functions without underscore, so as not to interfere with C primitives
    for (int i=0; i<5;i++) {
        fprintf(out, ".align 4\n");
        fprintf(out, "%s:\n", cmdnames[i]);
        fprintf(out, "    %s x0, %s, %s      /* direct %s into return reg     */\n", cmdnames[i], regnames[1], regnames[2], cmdnames[i]);
        fprintf(out, "    ret\n");
    }
    fprintf(out, ".align 4\n");
    fprintf(out, "not:\n");
    fprintf(out, "    mvn %s, %s        /* direct not                      */\n", regnames[1], regnames[1]);
    fprintf(out, "    ret\n");
    fprintf(out, "mul:\n");
    fprintf(out, "    mul x0, %s, %s      /* direct mul into return reg     */\n", regnames[1], regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "udiv:\n");
    fprintf(out, "    udiv x0, %s, %s      /* direct udiv into return reg     */\n", regnames[1], regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "remainder:             /* e.g. 13 %% 4 (no arm native %%)  */\n");
    fprintf(out, "    udiv %s, %s, %s     /* tmp = 13 / 4                   */\n", regnames[3], regnames[1], regnames[2]);
    fprintf(out, "    msub %s, %s, %s, %s /* result = 13 - (tmp * 4)        */\n", regnames[1], regnames[3], regnames[2], regnames[1]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "equals:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bne 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "lt:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bge 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "gt:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bls 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "lte:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bgt 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "gte:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    blt 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "lnot:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bne 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "land:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    mov %s, #0    /* assume false */ \n", regnames[1]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    cmp %s, #0\n", regnames[2]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "lor:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    mov %s, #0    /* assume false */ \n", regnames[1]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "    ret\n");
    fprintf(out, "0:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[2]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "_if:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    beq 0f\n");
#ifdef LEXICAL_SCOPING
    fprintf(out, "    mov %s, %s      /* pass original closure in %s */\n", CLOSURE_REG, regnames[2], CLOSURE_REG);
    fprintf(out, "    ldr %s, [%s, #8]   /* dereference function closure */\n", regnames[2], regnames[2]);
#endif
    fprintf(out, "    br %s           /* let target return to caller    */\n", regnames[2]);
    fprintf(out, "0:\n");
    fprintf(out, "    cmp %s, #0     /* have else block? */\n", regnames[3]);
    fprintf(out, "    beq 0f\n");
#ifdef LEXICAL_SCOPING
    fprintf(out, "    mov %s, %s      /* pass original closure in %s */\n", CLOSURE_REG, regnames[3], CLOSURE_REG);
    fprintf(out, "    ldr %s, [%s, #8]   /* dereference function closure */\n", regnames[3], regnames[3]);
#endif
    fprintf(out, "    br %s           /* let target return to caller    */\n", regnames[3]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov %s, #0      /* return false if no else */\n", regnames[1]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "_loop:\n");
    fprintf(out, "    stp fp, lr, [sp, #-0x10]!       /* save fp, lr for bl */\n");
    fprintf(out, "    str %s, [sp, #-16]!         /* save block arg to stack */\n", regnames[1]);
    fprintf(out, "0:\n");
#ifdef LEXICAL_SCOPING
    fprintf(out, "    ldr %s, [sp]      /* pass original closure in %s */\n", CLOSURE_REG, CLOSURE_REG);
    fprintf(out, "    ldr %s, [%s, #8]   /* dereference function closure */\n", regnames[1], CLOSURE_REG);
    fprintf(out, "    blr %s           /* call block    */\n", regnames[1]);
#else
    fprintf(out, "    ldr %s, [sp]     /* recall block arg */\n", regnames[1]);
    fprintf(out, "    blr %s           /* call block    */\n", regnames[1]);
#endif
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    bne 0b\n");
    fprintf(out, "    add sp, sp, #16     /* remove block arg from stack */\n");
    fprintf(out, "    ldp fp, lr, [sp], #0x10    /* restore fp, lr after bl */\n");
    fprintf(out, "    ret\n");
    fprintf(out, "_funcall:               /* (demo) function ptr support    */\n"); // 'funcal' _is_ a 'C primitive'
    for (int i=1; i<num_regnames; i++) {
        fprintf(out, "    mov %s, %s\n", regnames[i-1], regnames[i]);
    }
#ifdef LEXICAL_SCOPING
    fprintf(out, "    mov %s, %s      /* pass original closure in %s */\n", CLOSURE_REG, regnames[0], CLOSURE_REG);
    fprintf(out, "    ldr %s, [%s, #8]   /* dereference function closure */\n", regnames[0], regnames[0]);
#endif
    fprintf(out, "    br %s               /* let target return to caller     */\n", regnames[0]);
    fprintf(out, ".align 4\n");
    fprintf(out, "_args:\n");
    fprintf(out, "    adrp x7, _top_variables@PAGE\n"); // TODO hijacking an arg register here.
    fprintf(out, "    add x7, x7, _top_variables@PAGEOFF\n"); // TODO hijacking an arg register here.
    fprintf(out, "    ldr x8, [x7]\n");
    for (int i=1;i<num_regnames; i++) {
        fprintf(out, "    cmp %s, #0        /* have arg %d?                    */ \n", regnames[i], i);
        fprintf(out, "    beq %df               /* else done                      */\n", i == 1 ? 1 : 0);
        fprintf(out, "    str %s, [x8, #0]  /* store name %d                  */\n", regnames[i], i);
        fprintf(out, "    ldr %s, [sp, #%d]   /* find value %d on stack pointer */\n", regnames[i], 16*(i-1), i);
        fprintf(out, "    str %s, [x8, #8]   /* store value %d */\n", regnames[i], i);
        fprintf(out, "    add x8, x8, #16       /* top_variables++                */\n");
    }
    fprintf(out, "0:\n");
    fprintf(out, "    str x8, [x7] /* and save */\n");
    fprintf(out, "1:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "_return:\n");
    fprintf(out, "    ret      /* arg0 == result */\n");
    fprintf(out, ".globl _main\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "_main:\n");
    fprintf(out, "    stp x19, x20, [sp, #-0x10]!\n");
    fprintf(out, "    stp x21, x22, [sp, #-0x10]!\n");
    fprintf(out, "    stp x23, x24, [sp, #-0x10]!\n");
    fprintf(out, "    stp x25, x26, [sp, #-0x10]!\n");
    fprintf(out, "    stp fp, lr, [sp, #-0x10]!       /* save fp, lr for bl */\n");
    fprintf(out, "    bl _init\n");
}

void emit_int_arg(FILE * out, int num, int n_arg) {
    fprintf(out, "    mov %s, %d\n", regnames[n_arg], entry->value.num);
}

void emit_string_arg(FILE * out, int idx, int n_arg) {
    fprintf(out, "    adr %s, str%d\n", regnames[n_arg], idx);
}

void emit_builtin(FILE * out, char * cmdname, int n_arg, int n_args) {
    if (n_arg == 0) {
        // if function is invoked directly, call it
        for (int i=2; i<n_args; i++) {
            fprintf (out, "    %s x0, x0, %s      /* %s argument # %d               */ \n", cmdnames[entry->value.num], regnames[i], cmdnames[entry->value.num], i);
        }
    } else {
        // if this is a function pointer argument, point to a real function (which by the way takes at most two args)
        fprintf (out, "    adr %s, %s\n", regnames[n_arg], cmdnames[entry->value.num]);
    }
}

void emit_func_arg(FILE * out, char * cname, char * pname, int n_arg, int n_args) {
    if (n_arg == 0) { // that's the function position; in any other position, function == common argument
        if (n_args < num_regnames) fprintf(out, "    mov %s, #0 /* mark end of potential varargs */\n", regnames[n_args]);
        fprintf (out, "    bl _%s\n", primitive_names[entry->value.num]);
    } else {
        // if this is a function pointer argument, just supply the pointer
        fprintf (out, "    adr %s, _%s\n", regnames[n_arg], primitive_names[entry->value.num]); // That's for function pointers
    }
}

int emit_subexpr(FILE * out, ParseStack * stack, int from, int n_arg, int * stashbase) {
   if (*stashbase != stashptr) // Last subexpr was not stashed
        fprintf(out, "    mov %s, %s          /* unstash return value           */\n", regnames[n_arg], retnames[(*stashbase)++]);
    // As we generate subexprs in written order to facilitate stash / unstash,
    // we cannot immediately invoke any function subexpr as we meet it; leave this with our caller
    //if (n_arg == 0) { // expression at function position!
    //    fprintf(out, "    bl %s%s\n", regnames[0]);
    //}

#ifdef LEXICAL_SCOPING
    if (n_arg == 0) { // subexpr at function position; assume it is "get" or at least yields a closure; resolve closure
        fprintf(out, "    mov %s, %s      /* pass original closure in %s */\n", CLOSURE_REG, regnames[0], CLOSURE_REG);
        fprintf(out, "    ldr %s, [%s, #8]   /* dereference function closure */\n", regnames[0], regnames[0]);
    }
#endif
    // Skip sub-expression since it was already written
    return skip_until_close(stack, from)-1;
}

int emit_block(FILE * out, ParseStack * stack, int from, int n_arg) {
        block_depth++;

        fprintf(out, "    adr %s, 0f          /* load start of block as arg     */\n", regnames[n_arg]);
        fprintf(out, "    b %df                /* jump over the block            */\n", block_depth);
        fprintf(out, ".align 4\n");
        fprintf(out, "0:                      /* start of block                 */\n");
        fprintf(out, "    stp fp, lr, [sp, #-0x10]!       /* save fp, lr for bl */\n");
        fprintf(out, "    adrp x7, _top_variables@PAGE\n"); // TODO hijacking an arg register here.
        fprintf(out, "    add x7, x7, _top_variables@PAGEOFF\n"); // TODO hijacking an arg register here.
        fprintf(out, "    ldr x8, [x7]\n");
        fprintf(out, "    str x8, [sp, #-16]!\n");

#ifdef LEXICAL_SCOPING
        // Setup parent pointer
        fprintf(out, "    str wzr, [x8]  /* setup parent pointer; name = nil */\n");
        fprintf(out, "    str %s, [x8, #8]   /* value = pos of closure */\n", CLOSURE_REG);
        fprintf(out, "    add x8, x8, #16       /* top_variables++                */\n");
        fprintf(out, "    str x8, [x7]      /* and save */\n");
#endif

        int n_args2 = num_args(stack, from+1); // = -1 if no 'args'
        if (n_args2 != -1) {
            for (int i=n_args2-1;i>=0;i--) {
                fprintf(out, "    str %s, [sp, #-16]!     /* store arg %d for 'args'       */\n", regnames[i+1], i+1);
            }
            if (n_args2 < num_regnames-1) {
                fprintf(out, "    mov %s, #0     /* mark end of varargs to 'args' */\n", regnames[n_args2+1]);
            }
        }

        from = emit_code(out, stack, from+1, '}')-1;

        if (n_args2 > 0) {
            fprintf(out, "    add sp, sp, #%d     /* remove args from stack */\n", n_args2*16);
        }
        fprintf(out, "    ldr x8, [sp], #16  /* restore top of variables to before call */\n");
        fprintf(out, "    adrp x7, _top_variables@PAGE\n"); // TODO hijacking an arg register here.
        fprintf(out, "    add x7, x7, _top_variables@PAGEOFF\n"); // TODO hijacking an arg register here.
        fprintf(out, "    str x8, [x7]\n");
        fprintf(out, "    ldp fp, lr, [sp], #0x10    /* restore fp, lr after bl */\n");
        fprintf(out, "    ret                 /* return from block              */\n");
        fprintf(out, "%d:\n", block_depth);

        block_depth--;
        return from;
}

void emit_save_retval(FILE * out) {
    if (stashptr >= sizeof(retnames) / sizeof(char *)) { printf("Too many return values.\n"); return; }
    fprintf(out, "    mov %s, x0          /* stash return value             */\n", retnames[stashptr++]);
}

void emit_move_retval(FILE * out, int n_arg) {
    if (n_arg == 1) fprintf(out, "                        /* retval reg x0 == arg reg x0    */\n");
    else fprintf(out, "    mov %s, x0          /* transfer return value to arg   */\n", regnames[n_arg]);
}

void emit_call_subexpr(FILE * out) {
    fprintf(out, "    blr %s          /* call subexpr function          */ \n", regnames[0]);
}

void emit_end(FILE * out) {
    fprintf(out, "    ldp fp, lr, [sp], #0x10    /* restore fp, lr after bl */\n");
    fprintf(out, "    ldp x25, x26, [sp], #0x10\n");
    fprintf(out, "    ldp x23, x24, [sp], #0x10\n");
    fprintf(out, "    ldp x21, x22, [sp], #0x10\n");
    fprintf(out, "    ldp x19, x20, [sp], #0x10\n");
    fprintf(out, "    ret\n");
}
