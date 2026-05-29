#ifndef IR_H
#define IR_H

#include "ast.h"

typedef unsigned long long word_t;

enum {
	IR_INST_ADD,

	// desc: allocate space from stack and store the addr to dest
	// arg1 = destination temp
	// arg2 = size in bytes
	IR_INST_ALLOCATE,

	IR_INST_BEGIN_CALL,

	IR_INST_CALL,

	IR_INST_CONST,

	IR_INST_END_CALL,

	// desc: get the return address value and store the addr to dest
	// arg1 = destination temp
	IR_INST_GET_RETURN_ADDR,

	IR_INST_LOAD,

	// desc: return from the function
	// No arguments
	IR_INST_RETURN,

	IR_INST_SET_RETURN_ADDR,

	IR_INST_STORE,

	IR_INST_SUB,
};

typedef struct ir_inst_t ir_inst_t;
struct ir_inst_t {
	int kind;
	word_t arg1;
	word_t arg2;
	word_t arg3;
	word_t arg4;
};

typedef struct ir_fn_t ir_fn_t;
struct ir_fn_t {
	int id;
	const char *name;

	ir_inst_t *list;
	int len;
};

void print_ir(ir_fn_t **list, int len);
void generate_ir(ast_t *ast, ir_fn_t ***list, int *len);

#endif /* IR_H */

