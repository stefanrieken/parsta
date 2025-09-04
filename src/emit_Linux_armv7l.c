#include <stdio.h>

#include "parsta.h"

/**
 * This Aarch32 port is the first ABI forcing us to implement stack based arguments.
 * This now works reasonably well, up to the point where the changes may serve as a
 * template for the other ports, to at least review their use of calling convention.
 * 'Stashing' subexpr results still solely relies on registers; when also using the
 * stack here we have to be very careful not to clash with stack arguments, although
 * in theory these two call phases should not mix.
 *
 * To make a generic executable, GCC assumes an Aarch32 without udiv / sdiv support.
 * Telling GCC to build for this specific host using -march=native fixes this.
 */
char * cmdnames[] = { "add", "sub", "and", "orr", "eor", "not", "mul", "udiv", "remainder", "equals", "lt", "gt", "lte", "gte", "lnot", "land", "lor", "dollar"};
char * regnames[] = { "r6", "r0", "r1", "r2", "r3" } ; // NOTE: first reg in this list is for function pointer (if needed)
char * resnames[] = { "r4", "r5" }; // Reserved, i.e. for other purposes than args or stashed return values
char * retnames[] = { "r7", "r8", "r9", "r10" } ; // Stashed return values: return value appears in r0; then we copy them to one of these.

const int NUM_ARG_REGS = 4; // After r0-r3, other args should go onto the stack
const int NUM_BUILTINS = 5; // Number of CPU-native binary operators in 'cmdnames'
const int NUM_COMMANDS = sizeof(cmdnames) / sizeof(char *); // Total number of commands with translations to primitive names in 'cmdnames'
int num_retnames = sizeof(retnames) / sizeof(char *);

#ifdef LEXICAL_SCOPING
// TODO: choose register to pass closure to be independent from those used for return values
#define CLOSURE_REG "r10"
#endif

void emit_start(FILE * out) {
    // Improvement suggestions:
    // 1) only emit asm utility functions that are actually referenced (e.g. at end instead of start)
    // 2) clang: define built-in-support functions without underscore, so as not to interfere with C primitives
    for (int i=0; i<5;i++) {
        fprintf(out, ".align 4\n");
        fprintf(out, "%s:\n", cmdnames[i]);
        fprintf(out, "    %s r0, %s, %s      /* direct %s into return reg     */\n", cmdnames[i], regnames[1], regnames[2], cmdnames[i]);
        fprintf(out, "    mov pc, lr         /* return */\n");
    }
    fprintf(out, ".align 4\n");
    fprintf(out, "not:\n");
    fprintf(out, "    mvn %s, %s        /* direct not                      */\n", regnames[1], regnames[1]);
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, "mul:\n");
    fprintf(out, "    mul r0, %s, %s      /* direct mul into return reg     */\n", regnames[1], regnames[2]);
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "udiv:\n");
#ifdef ARM_HAS_DIV
    fprintf(out, "    udiv r0, %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov pc, lr        /* return */\n");
#else
    fprintf(out, "    b __aeabi_uidiv\n");
#endif
    fprintf(out, ".align 4\n");
    fprintf(out, "remainder:             /* e.g. 13 %% 4 (no arm native %%)  */\n");
#ifdef ARM_HAS_DIV
    fprintf(out, "    udiv r3, %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mul r3, r3, %s\n", regnames[2]);
    fprintf(out, "    sub r0, %s, r3\n", regnames[1]);
    fprintf(out, "    mov pc, lr        /* return */\n");
#else
    fprintf(out, "    push { fp, lr }      /* save fp, lr for bl */\n");
    fprintf(out, "    mov %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    bl __aeabi_uidivmod\n");
    fprintf(out, "    pop { fp, pc }        /* return */\n");
#endif
    fprintf(out, ".align 4\n");
    fprintf(out, "equals:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bne 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "lt:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bge 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "gt:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bls 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, "lte:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bgt 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "gte:\n");
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    blt 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "lnot:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    mov %s, #0    /* assume false */\n", regnames[1]);
    fprintf(out, "    bne 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "land:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    mov %s, #0    /* assume false */ \n", regnames[1]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    cmp %s, #0\n", regnames[2]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "lor:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    mov %s, #0    /* assume false */ \n", regnames[1]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, "0:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[2]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    mov %s, #1\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "if:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    beq 0f\n");
#ifdef LEXICAL_SCOPING
    fprintf(out, "    mov %s, %s      /* pass original closure in %s */\n", CLOSURE_REG, regnames[2], CLOSURE_REG);
    fprintf(out, "    ldr %s, [%s, #4]   /* dereference function closure */\n", regnames[2], regnames[2]);
#endif
    fprintf(out, "    mov pc, %s          /* let target return to caller    */\n", regnames[2]);
    fprintf(out, "0:\n");
    fprintf(out, "    cmp %s, #0     /* have else block? */\n", regnames[3]);
    fprintf(out, "    beq 0f\n");
#ifdef LEXICAL_SCOPING
    fprintf(out, "    mov %s, %s      /* pass original closure in %s */\n", CLOSURE_REG, regnames[3], CLOSURE_REG);
    fprintf(out, "    ldr %s, [%s, #4]   /* dereference function closure */\n", regnames[3], regnames[3]);
#endif
    fprintf(out, "    mov pc, %s          /* let target return to caller    */\n", regnames[3]);
    fprintf(out, "0:\n");
    fprintf(out, "    mov %s, #0      /* return false if no else */\n", regnames[1]);
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "loop:\n");
    fprintf(out, "    push { fp, lr }      /* save fp, lr for bl */\n");
    fprintf(out, "    push { %s }        /* save block arg to stack */\n", regnames[1]);
    fprintf(out, "0:\n");
#ifdef LEXICAL_SCOPING
    fprintf(out, "    ldr %s, [sp]      /* pass original closure in %s */\n", CLOSURE_REG, CLOSURE_REG);
    fprintf(out, "    ldr %s, [%s, #4]   /* dereference function closure */\n", regnames[1], CLOSURE_REG);
    fprintf(out, "    blx %s          /* call block    */\n", regnames[1]);
#else
    fprintf(out, "    ldr %s, [sp]     /* recall block arg */\n", regnames[1]);
    fprintf(out, "    blx %s           /* call block    */\n", regnames[1]);
#endif
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    bne 0b\n");
    fprintf(out, "    add sp, sp, #4     /* remove block arg from stack */\n");
    fprintf(out, "    pop { fp, pc }        /* return */\n");
    fprintf(out, "funcall:               /* (demo) function ptr support    */\n"); // 'funcal' _is_ a 'C primitive'
    // TODO: work out how to take further args from stack
    for (int i=1; i<=NUM_ARG_REGS; i++) {
        fprintf(out, "    mov %s, %s\n", regnames[i-1], regnames[i]);
    }
#ifdef LEXICAL_SCOPING
    fprintf(out, "    mov %s, %s      /* pass original closure in %s */\n", CLOSURE_REG, regnames[0], CLOSURE_REG);
    fprintf(out, "    ldr %s, [%s, #4]   /* dereference function closure */\n", regnames[0], regnames[0]);
#endif
    fprintf(out, "    mov pc, %s               /* let target return to caller     */\n", regnames[0]);
    fprintf(out, ".align 4\n");
    fprintf(out, "args:\n");
    fprintf(out, "    ldr r5, =top_variables\n");
    fprintf(out, "    ldr r6, [r5]\n");
    // TODO: work out how to take further args from stack
    for (int i=1;i<=NUM_ARG_REGS; i++) {
        fprintf(out, "    cmp %s, #0        /* have arg %d?                    */ \n", regnames[i], i);
        fprintf(out, "    beq %df           /* else done                      */\n", i == 1 ? 1 : 0);
        fprintf(out, "    str %s, [r6, #0]  /* store name %d                  */\n", regnames[i], i);
        fprintf(out, "    ldr %s, [sp, #%d] /* find value %d on stack pointer */\n", regnames[i], 8*(i-1), i);
        fprintf(out, "    str %s, [r6, #4]  /* store value %d */\n", regnames[i], i);
        fprintf(out, "    add r6, r6, #8    /* top_variables++                */\n");
    }
    fprintf(out, "0:\n");
    fprintf(out, "    str r6, [r5] /* and save */\n");
    fprintf(out, "1:\n");
    fprintf(out, "    mov pc, lr        /* return */\n");
    fprintf(out, ".align 4\n");
    fprintf(out, ".pool\n");
    fprintf(out, "return:\n");
    fprintf(out, "    mov pc, lr        /* return; arg0 = result */\n");
    fprintf(out, ".globl main\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "main:\n");
    fprintf(out, "    push { fp, lr }      /* save fp, lr for bl */\n");
    fprintf(out, "    push { r4, r5, r6, r7, r8, r9, r10 }\n");
    fprintf(out, "    bl init\n");
}

// NOTE: our use of 'varargs' has not been checked against EABI!
void emit_end_of_varargs(FILE * out, int n_args) {
    if (n_args <= NUM_ARG_REGS) {
        fprintf(out, "    mov %s, #0         /* mark end of potential varargs */\n", regnames[n_args]);
    } else {
        fprintf(out, "    mov r5, #0         /* mark end of potential varargs... */\n");
        fprintf(out, "    str r5, [sp, #%d]  /* ...on stack */\n", ((n_args-1)-NUM_ARG_REGS)*4);
    }
}

void emit_int_arg(FILE * out, int num, int n_arg) {
    if (n_arg <= NUM_ARG_REGS) {
        fprintf(out, "    ldr %s, =%d\n", regnames[n_arg], num);
    } else {
        fprintf(out, "    ldr r5, =%d\n", num);
        fprintf(out, "    str r5, [sp, #%d]          /* Pass arg on stack */\n", (n_arg - NUM_ARG_REGS) * 4);
    }
}

void emit_string_arg(FILE * out, int idx, int n_arg) {
    if (n_arg <= NUM_ARG_REGS) {
        fprintf(out, "    ldr %s, =str%d\n", regnames[n_arg], idx);
    } else {
        fprintf(out, "    ldr r5, =str%d\n", idx);
        fprintf(out, "    str r5, [sp, #%d]          /* Pass arg on stack */\n", (n_arg - NUM_ARG_REGS) * 4);
    }
}

void emit_builtin(FILE * out, char * cmdname, int n_arg, int n_args) {
    if (n_arg == 0) {
        // if function is invoked directly, use processor's math operations
        for (int i=2; i<n_args; i++) {
            fprintf (out, "    %s r0, r0, %s      /* %s argument # %d               */ \n", cmdname, regnames[i], cmdname, i);
        }
    } else {
        // if this is a function pointer argument, point to a real function (which by the way takes at most two args)
        fprintf (out, "    adr %s, %s\n", regnames[n_arg], cmdname);
    }
}

void emit_func_arg(FILE * out, char * cname, char * pname, int n_arg, int n_args) {
    if (n_arg == 0) { // function position
        emit_end_of_varargs(out, n_args);
        // if function is invoked directly, call it
        fprintf(out, "    bl %s              /* call '%s'                     */\n", cname, pname);
    } else {
        // if this is a function pointer argument, just supply the pointer
        fprintf (out, "    adr %s, %s\n", regnames[n_arg], cname);
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
        fprintf(out, "    mov %s, %s         /* pass original closure in %s */\n", CLOSURE_REG, regnames[0], CLOSURE_REG);
        fprintf(out, "    ldr %s, [%s, #4]   /* dereference function closure */\n", regnames[0], regnames[0]);
    }
#endif
    // Skip sub-expression since it was already written
    return skip_until_close(stack, from)-1;
}

int emit_block(FILE * out, ParseStack * stack, int from, int n_arg) {
    block_depth++;

    fprintf(out, "    ldr %s, =0f          /* load start of block as arg     */\n", regnames[n_arg]);
    fprintf(out, "    b %df                /* jump over the block            */\n", block_depth);
    fprintf(out, ".align 4\n");
    fprintf(out, "0:                      /* start of block                 */\n");
    fprintf(out, "    push { fp, lr }      /* save fp, lr for bl */\n");
    fprintf(out, "    ldr r5, =top_variables\n");
    fprintf(out, "    ldr r6, [r5]\n");
    fprintf(out, "    push { r6 }\n");

#ifdef LEXICAL_SCOPING
    // Setup parent pointer
    fprintf(out, "    mov r5, #0\n"); 
    fprintf(out, "    str r5, [r6]       /* setup parent pointer; name = nil */\n");
    fprintf(out, "    str %s, [r6, #4]   /* value = pos of closure */\n", CLOSURE_REG);
    fprintf(out, "    add r6, r6, #8       /* top_variables++                */\n");
    fprintf(out, "    ldr r5, =top_variables\n");
    fprintf(out, "    str r6, [r5]      /* and save */\n");
#endif

    int n_args2 = num_args(stack, from+1); // = -1 if no 'args'
    if (n_args2 != -1) {
        for (int i=n_args2-1;i>=0;i--) {
            fprintf(out, "    str %s, [sp, #-8]!     /* store arg %d for 'args'       */\n", regnames[i+1], i+1);
        }
        if (n_args2 <= NUM_ARG_REGS) {
            fprintf(out, "    mov %s, #0     /* mark end of varargs to 'args' */\n", regnames[n_args2+1]);
        }
    }

    from = emit_code(out, stack, from+1, '}')-1;

    if (n_args2 > 0) {
        fprintf(out, "    add sp, sp, #%d     /* remove args from stack */\n", n_args2*8);
    }
    fprintf(out, "    pop { r6 }  /* restore top of variables to before call */\n");
    fprintf(out, "    ldr r5, =top_variables\n");
    fprintf(out, "    str r6, [r5]\n");
    fprintf(out, "    pop { fp, pc }    /* restore fp, and lr as pc to return */\n");
    fprintf(out, ".pool /* tell gasm it can store literals here (used by 'ldr' instructions) */ \n");
    fprintf(out, "%d:\n", block_depth);

    block_depth--;
    return from;
}	

// TODO: move this func to emit.c, and just keep all arch-dependent calls here (do this for all ports)
int emit_entry(FILE * out, ParseStack * stack, int from, int n_arg, int n_args, int * stashbase) {
    ParseStackEntry * entry = &(stack->entries[from]);
    int idx;
    switch(entry->type) {
        case PT_INT:
            emit_int_arg(out, entry->value.num, n_arg);
            break;
        case PT_STR:
            idx = 0;
            StringEntry * e = unique_strings;
            while(e != NULL) { if (e->str == entry->value.str) break;  e = e->next; idx++; }
	    emit_string_arg(out, idx, n_arg);
            break;
        case PT_FUN:
            if (entry->value.num < NUM_BUILTINS) { // operators directly supported by CPU: '+', '-', '&', '|', '^'
                emit_builtin(out, cmdnames[entry->value.num], n_arg, n_args);
	    } else if (entry->value.num < NUM_COMMANDS) { // other operators whose cmdnames are translated to different primitive names ('~' ... '$')
                emit_func_arg(out, cmdnames[entry->value.num], primitive_names[entry->value.num], n_arg, n_args);
            } else { // other primitives: "funcall", "printnum", "print", ...
                emit_func_arg(out, primitive_names[entry->value.num], primitive_names[entry->value.num], n_arg, n_args);
            }
            break;
//        case PT_REF:
//            break;
        case PT_OPN:
            if (entry->value.num == '(' && n_arg <= NUM_ARG_REGS) { // other subexpr results are already placed on stack
                return emit_subexpr(out, stack, from, n_arg, stashbase);
            } else { // '{': block
                return emit_block(out, stack, from, n_arg);
            }
	    break;
        case PT_CLS:
            // Expecting caller to halt expression at CLS
            // without calling us (even in case of ';')
            printf("Error: bracket mismatch\n");
            break;
    }

    return from;
}

void emit_save_retval(FILE * out, int n_arg) {
    if (n_arg <= NUM_ARG_REGS) {
        if (stashptr >= sizeof(retnames) / sizeof(char *)) { printf("Too many return values.\n"); return; }
        fprintf(out, "    mov %s, r0          /* stash return value             */\n", retnames[stashptr++]);
    } else {
        fprintf(out, "    str r0, [sp, #%d]          /* Pass outcome on stack */\n", (n_arg - NUM_ARG_REGS) * 4);
    }
}

void emit_move_retval(FILE * out, int n_arg) {
    if (n_arg <= NUM_ARG_REGS) {
        if (n_arg == 1) fprintf(out, "                        /* retval reg r0 == arg reg r0    */\n");
        else fprintf(out, "    mov %s, r0          /* transfer return value to arg   */\n", regnames[n_arg]);
    } else {
        fprintf(out, "    str r0, [sp, #%d]          /* Pass outcome on stack */\n", (n_arg - NUM_ARG_REGS) * 4);
    }
}

void emit_call_subexpr(FILE * out) {
    fprintf(out, "    blx %s          /* call subexpr function          */ \n", regnames[0]);
}

void emit_reserve_stack(FILE * out, int n) {
    fprintf(out, "    sub sp, sp, #%d      /* Reserve space for %d stack args */\n", n*4, n);
}

void emit_restore_stack(FILE * out, int n) {
    fprintf(out, "    add sp, sp, #%d      /* Remove %d args from stack */\n", n*4, n);
}

void emit_end(FILE * out) {
    fprintf(out, "    pop { r4, r5, r6, r7, r8, r9, r10 }\n");
    fprintf(out, "    pop { fp, pc }    /* restore fp, and lr as pc to return */\n");
    fprintf(out, ".section	.note.GNU-stack,\"\",%%progbits\n"); // Make GNU linker happy about something.
}
