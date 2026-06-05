#ifndef IR_H
#define IR_H

#include "ast.h"

typedef unsigned long long word_t;

enum {
	// desc: add two temp value and store in another temp, size casted
	// arg1 = destination temp
	// arg2 = left temp
	// arg3 = right temp
	// arg4 = size of result
	IR_INST_ADD,

	// desc: allocate space from stack and store the addr to dest
	// arg1 = destination temp
	// arg2 = size in bytes
	IR_INST_ALLOCATE,

	// desc: deallocate space from stack of given size
	// arg1 = size in bytes
	IR_INST_DEALLOCATE,

	// desc: Start the call process
	IR_INST_BEGIN_CALL,

	// desc: Call a function
	// arg1 = function id
	IR_INST_CALL,

	// desc: store the value of a constant to a temp
	// arg1 = destination temp
	// arg2 = constant value
	IR_INST_CONST,

	// desc: End the call process
	IR_INST_END_CALL,

	// desc: get the return address value and store the addr to dest
	// arg1 = destination temp
	IR_INST_GET_RETURN_ADDR,

	// desc: get the param address
	// arg1 = destination temp
	// arg2 = param index
	IR_INST_GET_PARAM_ADDR,

	// desc: load the content of a address to a temp
	// arg1 = destination temp
	// arg2 = temp with address
	// arg3 = content size in the address
	IR_INST_LOAD,

	// desc: return from the function
	// No arguments
	IR_INST_RETURN,

	// desc: Set the return address
	// arg1 = temp with address value
	IR_INST_SET_RETURN_ADDR,

	// desc: get the param address
	// arg1 = param index
	// arg2 = temporary (with address value)
	IR_INST_SET_PARAM_ADDR,

	// desc: load the content of a temp to a destination address
	// arg1 = destination address
	// arg2 = temp value
	// arg3 = size of the destination address
	IR_INST_STORE,

	// desc: subtract two temp value and store in another temp, size casted
	// arg1 = destination temp
	// arg2 = left temp
	// arg3 = right temp
	// arg4 = size of result
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
	int is_extern;

	// vector of ir_inst_t*
	vec_t insts;
};

typedef struct ir_t ir_t;
struct ir_t {
	// vector of ir_fn_t*
	vec_t fn_list;
};

void print_ir(ir_t ir);
ir_t generate_ir(ast_t *ast);

#endif /* IR_H */

