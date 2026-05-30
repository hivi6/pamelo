#include "ir.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static ir_fn_t ***g_list;
static int *g_len;
static int g_temp_id = 0;
static type_t *g_current_fn_type = NULL;
static ir_fn_t *g_current_ir_fn = NULL;
static int g_current_return_temp = 0;

static void print_inst(ir_inst_t inst);
static void init(ir_fn_t ***list, int *len);
static void match(ast_t *ast, int kind, const char *message);
static symbol_t *get_symbol_from_token(scope_t *scope, token_t *token);
static int create_temp_id();
static void set_temp_id(int id);
static ir_fn_t *create_ir_fn(int id, const char *name);
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
static int cast_expr(ast_t *ast);

// ========================================
// ir.h - definition
// ========================================

void print_ir(ir_fn_t **list, int len) {
	for (int i = 0; i < len; i++) {
		printf("$%d: # %s\n", list[i]->id, list[i]->name);
		printf("@function_start\n");
		for (int j = 0; j < list[i]->len; j++) {
			print_inst(list[i]->list[j]);
		}
		printf("@function_end\n\n");
	}
}

void generate_ir(ast_t *ast, ir_fn_t ***list, int *len) {
	init(list, len);

	prog(ast);
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
	default:
		printf("WHAT IS THIS INST\n");
		exit(1);
	}
	
	printf("\n");
}

static void init(ir_fn_t ***list, int *len) {
	g_list = list;
	g_len = len;
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

static ir_fn_t *create_ir_fn(int id, const char *name) {
	ir_fn_t *res = calloc(sizeof(ir_fn_t), 1);
	res->id = id;
	res->name = sbuildf("%s", name);
	return res;
}

static void append(ir_fn_t *ir_fn) {
	*g_len += 1;
	*g_list = realloc(*g_list, *g_len * sizeof(ir_fn_t*));
	(*g_list)[*g_len - 1] = ir_fn;
}

static void emit(int kind, word_t arg1, word_t arg2, word_t arg3, word_t arg4) {
	g_current_ir_fn->len += 1;
	g_current_ir_fn->list = realloc(g_current_ir_fn->list,
		g_current_ir_fn->len * sizeof(ir_inst_t));
	g_current_ir_fn->list[g_current_ir_fn->len - 1] = (ir_inst_t) {
		.kind = kind,
		.arg1 = arg1,
		.arg2 = arg2,
		.arg3 = arg3,
		.arg4 = arg4,
	};
}

static void emitReturn() {
	if (g_current_ir_fn->len <= 0 || 
		g_current_ir_fn->list[g_current_ir_fn->len - 1].kind != IR_INST_RETURN) {
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
		res = res * base + (lexical[index] - '0');
		index++;
	}

	return res;
}

static void prog(ast_t *ast) {
	match(ast, AST_PROG, "Expected AST_PROG");

	for (int i = 0; i < ast->ast.prog.decls_len; i++) {
		decl(ast->ast.prog.decls[i]);
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
	ir_fn_t *ir_fn = create_ir_fn(s->id, s->name);

	g_current_fn_type = s->type;
	g_current_ir_fn = ir_fn;
	if (s->type->type.fn_type.return_type->kind != TYPE_VOID) {
		g_current_return_temp = create_temp_id();
		emit(IR_INST_GET_RETURN_ADDR, g_current_return_temp, 0, 0, 0);
	}
	stmt(ast->ast.fn_decl.block_stmt);
	emitReturn();

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

	for (int i = 0; i < ast->ast.block_stmt.stmts_len; i++) {
		stmt(ast->ast.block_stmt.stmts[i]);
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
	
	int id = create_temp_id();
	emit(IR_INST_LOAD, id, s->id, ast->type->size, 0);
	return id;
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


