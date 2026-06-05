#include "ir.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static vec_t g_list; // vector of ir_fn_t*
static int g_temp_id = 0;
static type_t *g_current_fn_type = NULL;
static ir_fn_t *g_current_ir_fn = NULL;
static int g_current_return_temp = 0;

static void print_inst(ir_inst_t inst);
static void init();
static void match(ast_t *ast, int kind, const char *message);
static symbol_t *get_symbol_from_token(scope_t *scope, token_t *token);
static int create_temp_id();
static void set_temp_id(int id);
static ir_fn_t *create_ir_fn(int id, const char *name, int is_extern, 
	type_t *type);
static void append(ir_fn_t *ir_fn);
static void emit(int kind, word_t arg1, word_t arg2, word_t arg3, word_t arg4);
static void emitReturn();
static word_t get_int_literal(const char *lexical);

static void prog(ast_t *ast);

static void decl(ast_t *ast);
static void fn_decl(ast_t *ast);

static void stmt(ast_t *ast);
static void block_stmt(ast_t *ast);
static void var_stmt(ast_t *ast);
static void return_stmt(ast_t *ast);
static void expr_stmt(ast_t *ast);

static int expr(ast_t *ast);
static int literal_expr(ast_t *ast);
static int var_expr(ast_t *ast);
static int call_expr(ast_t *ast);
static int cast_expr(ast_t *ast);
static int add_expr(ast_t *ast);

// ========================================
// ir.h - definition
// ========================================

void print_ir(ir_t ir) {
	vec_t list = ir.fn_list;

	for (int i = 0; i < list.len; i++) {
		ir_fn_t *ir_fn = list.elems[i];
		char *str = type_str(ir_fn->type);
		printf("$%d: # %s\n", ir_fn->id, str);
		free(str);

		if (ir_fn->is_extern) {
			printf("extern\n");
		}
		else {
			printf("@function_start\n");

			for (int j = 0; j < ir_fn->insts.len; j++) {
				ir_inst_t *inst = ir_fn->insts.elems[j];
				print_inst(*inst);
			}

			printf("@function_end\n");
		}

		printf("\n");
	}
}

ir_t generate_ir(ast_t *ast) {
	init();
	prog(ast);
	ir_t ir;
	ir.fn_list = g_list;
	return ir;
}

// ========================================
// ir.h - definition
// ========================================

static void print_inst(ir_inst_t inst) {
	printf("    ");

	switch (inst.kind) {
	case IR_INST_GET_RETURN_ADDR:
		printf("%%%llu := GET_RETURN_ADDR", inst.arg1);
		break;
	case IR_INST_SET_RETURN_ADDR:
		printf("SET_RETURN_ADDR %%%llu", inst.arg1);
		break;
	case IR_INST_GET_PARAM_ADDR:
		printf("%%%llu := GET_PARAM_ADDR %llu", inst.arg1, inst.arg2);
		break;
	case IR_INST_SET_PARAM_ADDR:
		printf("SET_PARAM_ADDR %llu %%%llu", inst.arg1, inst.arg2);
		break;
	case IR_INST_RETURN:
		printf("RETURN");
		break;
	case IR_INST_ALLOCATE:
		printf("%%%llu := ALLOCATE %llu", inst.arg1, inst.arg2);
		break;
	case IR_INST_STORE:
		printf("STORE %%%llu %llu := %%%llu", inst.arg1, inst.arg3, 
			inst.arg2);
		break;
	case IR_INST_CONST:
		printf("%%%llu := CONST %llu", inst.arg1, inst.arg2);
		break;
	case IR_INST_LOAD:
		printf("%%%llu := LOAD %%%llu %llu", inst.arg1, inst.arg2, 
			inst.arg3);
		break;
	case IR_INST_ADD:
		printf("%%%llu := ADD %%%llu %%%llu %llu", inst.arg1, inst.arg2, 
			inst.arg3, inst.arg4);
		break;
	case IR_INST_SUB:
		printf("%%%llu := SUB %%%llu %%%llu %llu", inst.arg1, inst.arg2, 
			inst.arg3, inst.arg4);
		break;
	case IR_INST_CALL:
		printf("CALL $%llu", inst.arg1);
		break;
	case IR_INST_BEGIN_CALL:
		printf("BEGIN_CALL");
		break;
	case IR_INST_END_CALL:
		printf("END_CALL");
		break;
	case IR_INST_DEALLOCATE:
		printf("DEALLOCATE %llu", inst.arg1);
		break;
	default:
		printf("WHAT IS THIS INST\n");
		exit(1);
	}
	
	printf("\n");
}

static void init() {
	vec_init(&g_list);
}

static void match(ast_t *ast, int kind, const char *message) {
	if (ast->kind != kind) {
		eprintf(ast->filepath, ast->source, ast->start, ast->end,
			message);
		exit(1);
	}
}

static symbol_t *get_symbol_from_token(scope_t *scope, token_t *token) {
	char *name = token_lexical(*token);
	symbol_t *res = get_symbol_in_chain(scope, name);
	free(name);
	return res;
}

static int create_temp_id() {
	return ++g_temp_id;
}

static void set_temp_id(int id) {
	g_temp_id = id;
}

static ir_fn_t *create_ir_fn(int id, const char *name, int is_extern, 
	type_t *type) {
	ir_fn_t *res = calloc(sizeof(ir_fn_t), 1);
	res->id = id;
	res->name = sbuildf("%s", name);
	res->is_extern = is_extern;
	res->type = type;
	return res;
}

static void append(ir_fn_t *ir_fn) {
	vec_append(&g_list, ir_fn);
}

static void emit(int kind, word_t arg1, word_t arg2, word_t arg3, word_t arg4) {
	ir_inst_t *inst = calloc(sizeof(ir_inst_t), 1);
	inst->kind = kind;
	inst->arg1 = arg1;
	inst->arg2 = arg2;
	inst->arg3 = arg3;
	inst->arg4 = arg4;
	vec_append(&g_current_ir_fn->insts, inst);
}

static void emitReturn() {
	if (g_current_ir_fn->insts.len <= 0) {
		emit(IR_INST_RETURN, 0, 0, 0, 0);
		return;
	}

	int len = g_current_ir_fn->insts.len;
	ir_inst_t *inst = g_current_ir_fn->insts.elems[len-1];
	if (inst->kind != IR_INST_RETURN) {
		emit(IR_INST_RETURN, 0, 0, 0, 0);
	}
}

static word_t get_int_literal(const char *lexical) {
	int len = strlen(lexical);

	int base = 10;
	word_t res = 0;
	int index = 0;
	if (lexical[0] == '0') base = 8;
	if (len >= 2) {
		if (lexical[0] == '0' && tolower(lexical[1]) == 'b') {
			base = 2;
			index = 2;
		}
		if (lexical[0] == '0' && tolower(lexical[1]) == 'x') {
			base = 16;
			index = 2;
		}
	}

	while (index < len) {
		int value = lexical[index] - '0';
		if (tolower(lexical[index]) >= 'a') 
			value = 10 + tolower(lexical[index]) - 'a';

		res = res * base + value;
		index++;
	}

	return res;
}

static void prog(ast_t *ast) {
	match(ast, AST_PROG, "Expected AST_PROG");

	for (int i = 0; i < ast->ast.prog.decls.len; i++) {
		decl(ast->ast.prog.decls.elems[i]);
	}
}

static void decl(ast_t *ast) {
	if (ast->kind == AST_FN_DECL) fn_decl(ast);
	else {
		match(ast, AST_FN_DECL, "What is this declaration kind?");
	}
}

static void fn_decl(ast_t *ast) {
	match(ast, AST_FN_DECL, "Expected AST_FN_DECL");

	set_temp_id(ast->total_id);

	symbol_t *s = get_symbol_from_token(ast->scope, ast->ast.fn_decl.name);
	assert(s->type->kind == TYPE_FN);

	int is_extern = (ast->ast.fn_decl.block_stmt ? 0 : 1);
	ir_fn_t *ir_fn = create_ir_fn(s->id, s->name, is_extern, s->type);
	g_current_fn_type = s->type;
	g_current_ir_fn = ir_fn;

	if (!is_extern) {
		for (int i = 0; i < ast->ast.fn_decl.params.len; i++) {
			token_t *param = ast->ast.fn_decl.params.elems[i];
			symbol_t *s = get_symbol_from_token(ast->scope, param);
			emit(IR_INST_GET_PARAM_ADDR, i, s->id, 0, 0);
		}
		if (s->type->type.fn_type.return_type->kind != TYPE_VOID) {
			g_current_return_temp = create_temp_id();
			emit(IR_INST_GET_RETURN_ADDR, 
				g_current_return_temp, 0, 0, 0);
		}
		stmt(ast->ast.fn_decl.block_stmt);
		emitReturn();
	}

	append(ir_fn);
}

static void stmt(ast_t *ast) {
	if (ast->kind == AST_BLOCK_STMT) block_stmt(ast);
	else if (ast->kind == AST_VAR_STMT) var_stmt(ast);
	else if (ast->kind == AST_RETURN_STMT) return_stmt(ast);
	else if (ast->kind == AST_EXPR_STMT) expr_stmt(ast);
	else {
		eprintf(ast->filepath, ast->source, ast->start, ast->end,
			"What is this statement kind?");
		exit(1);
	}
}

static void block_stmt(ast_t *ast) {
	match(ast, AST_BLOCK_STMT, "Expected AST_BLOCK_STMT");

	int allocated_size = 0;

	for (int i = 0; i < ast->ast.block_stmt.stmts.len; i++) {
		ast_t *s = ast->ast.block_stmt.stmts.elems[i];
		stmt(s);
		if (s->kind == AST_VAR_STMT) {
			allocated_size += s->type->size;
		}
	}

	if (allocated_size) {
		emit(IR_INST_DEALLOCATE, allocated_size, 0, 0, 0);
	}
}

static void var_stmt(ast_t *ast) {
	match(ast, AST_VAR_STMT, "Expected AST_VAR_STMT");

	symbol_t *s = get_symbol_from_token(ast->scope, ast->ast.var_stmt.name);
	emit(IR_INST_ALLOCATE, s->id, s->type->size, 0, 0);

	if (ast->ast.var_stmt.expr) {
		word_t temp = expr(ast->ast.var_stmt.expr);
		emit(IR_INST_STORE, s->id, temp, s->type->size, 0);
	}
}

static void return_stmt(ast_t *ast) {
	match(ast, AST_RETURN_STMT, "Expected AST_RETURN_STMT");

	if (ast->ast.return_stmt.expr) {
		word_t temp = expr(ast->ast.return_stmt.expr);
		emit(IR_INST_STORE, g_current_return_temp, temp, 
			g_current_fn_type->type.fn_type.return_type->size, 0);
	}

	emitReturn();
}

static void expr_stmt(ast_t *ast) {
	match(ast, AST_EXPR_STMT, "Expected AST_EXPR_STMT");
	expr(ast->ast.expr_stmt.expr);
}

static int expr(ast_t *ast) {
	if (ast->kind == AST_LITERAL_EXPR) return literal_expr(ast);
	if (ast->kind == AST_VAR_EXPR) return var_expr(ast);
	if (ast->kind == AST_CAST_EXPR) return cast_expr(ast);
	if (ast->kind == AST_ADD_EXPR) return add_expr(ast);
	if (ast->kind == AST_CALL_EXPR) return call_expr(ast);

	eprintf(ast->filepath, ast->source, ast->start, ast->end,
		"Invalid expr kind");
	exit(1);
}

static int literal_expr(ast_t *ast) {
	match(ast, AST_LITERAL_EXPR, "Expected AST_LITERAL_EXPR");

	token_t *token = ast->ast.literal_expr.token;
	if (token->kind != TOKEN_INT_LITERAL) {
		eprintf(token->filepath, token->source, token->start, 
			token->end, "Expected an integer literal");
	}

	char *lexical = token_lexical(*token);
	word_t literal = get_int_literal(lexical);
	free(lexical);

	int id = create_temp_id();
	emit(IR_INST_CONST, id, literal, 0, 0);
	return id;
}

static int var_expr(ast_t *ast) {
	match(ast, AST_VAR_EXPR, "Expected AST_VAR_EXPR");

	token_t *token = ast->ast.var_expr.token;
	symbol_t *s = get_symbol_from_token(ast->scope, token);
	
	if (s->type->kind == TYPE_FN) return s->id;

	int id = create_temp_id();
	emit(IR_INST_LOAD, id, s->id, ast->type->size, 0);
	return id;
}

static int call_expr(ast_t *ast) {
	match(ast, AST_CALL_EXPR, "Expected AST_CALL_EXPR");

	int fn_id = expr(ast->ast.call_expr.left);
	int return_id = -1;
	int res_id = -1;
	int allocated_size = 0;

	emit(IR_INST_BEGIN_CALL, 0, 0, 0, 0);

	for (int i = 0; i < ast->ast.call_expr.args.len; i++) {
		type_t *fn_type = ast->ast.call_expr.left->type;
		ast_t *arg = ast->ast.call_expr.args.elems[i];
		int temp = expr(arg);
		int arg_temp = create_temp_id();

		if (fn_type->kind != TYPE_FN) {
			fprintf(stderr, "Not possible!!!\n");
			exit(1);
		}

		type_t *type = fn_type->type.fn_type.param_types.elems[i];
		int size = type->size;

		emit(IR_INST_ALLOCATE, arg_temp, size, 0, 0);
		emit(IR_INST_STORE, arg_temp, temp, size, 0);
		emit(IR_INST_SET_PARAM_ADDR, i, arg_temp, 0, 0);

		allocated_size += size;
	}

	if (ast->type->kind != TYPE_VOID) {
		return_id = create_temp_id();
		emit(IR_INST_ALLOCATE, return_id, ast->type->size, 0, 0);
		emit(IR_INST_SET_RETURN_ADDR, return_id, 0, 0, 0);

		allocated_size += ast->type->size;
	}
	emit(IR_INST_CALL, fn_id, 0, 0, 0);
	if (ast->type->kind != TYPE_VOID) {
		res_id = create_temp_id();
		emit(IR_INST_LOAD, res_id, return_id, ast->type->size, 0);
	}
	emit(IR_INST_DEALLOCATE, allocated_size, 0, 0, 0);
	emit(IR_INST_END_CALL, 0, 0, 0, 0);

	return res_id;
}

static int cast_expr(ast_t *ast) {
	match(ast, AST_CAST_EXPR, "Expected AST_CASE_EXPR");

	int e = expr(ast->ast.cast_expr.left);
	int dest_id = create_temp_id();
	int zero_id = create_temp_id();
	emit(IR_INST_CONST, zero_id, 0, 0, 0);
	emit(IR_INST_ADD, dest_id, e, zero_id, ast->type->size);
	return dest_id;
}

static int add_expr(ast_t *ast) {
	match(ast, AST_ADD_EXPR, "Expected AST_ADD_EXPR");

	int left = expr(ast->ast.add_expr.left);
	int right = expr(ast->ast.add_expr.right);
	int kind = IR_INST_ADD;
	if (ast->ast.add_expr.op->kind == TOKEN_MINUS) kind = IR_INST_SUB;

	int id = create_temp_id();
	emit(kind, id, left, right, ast->type->size);
	return id;
}

