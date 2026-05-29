#include "ir.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static ir_fn_t ***g_list;
static int *g_len;
static int g_temp_id = 0;
static ir_fn_t *g_current_ir_fn = NULL;
static int g_current_return_temp = 0;

static void init(ir_fn_t ***list, int *len);
static void match(ast_t *ast, int kind, const char *message);
static symbol_t *get_symbol_from_token(scope_t *scope, token_t *token);
static int create_temp_id();
static void set_temp_id(int id);
static ir_fn_t *create_ir_fn(int id, const char *name);
static void append(ir_fn_t *ir_fn);
static void emit(int kind, word_t arg1, word_t arg2, word_t arg3, word_t arg4);

static void prog(ast_t *ast);

static void decl(ast_t *ast);
static void fn_decl(ast_t *ast);

static void stmt(ast_t *ast);
static void block_stmt(ast_t *ast);

// ========================================
// ir.h - definition
// ========================================

void generate_ir(ast_t *ast, ir_fn_t ***list, int *len) {
	init(list, len);

	prog(ast);
}

// ========================================
// ir.h - definition
// ========================================

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

	g_current_ir_fn = ir_fn;
	if (s->type->type.fn_type.return_type->kind != TYPE_VOID) {
		g_current_return_temp = create_temp_id();
		emit(IR_INST_GET_RETURN_ADDR, g_current_return_temp, 0, 0, 0);
	}
	stmt(ast);
	emit(IR_INST_RETURN, 0, 0, 0, 0);

	append(ir_fn);
}

static void stmt(ast_t *ast) {
	if (ast->kind == AST_BLOCK_STMT) block_stmt(ast);
}

static void block_stmt(ast_t *ast) {
	match(ast, AST_BLOCK_STMT, "Expected AST_BLOCK_STMT");

	for (int i = 0; i < ast->ast.block_stmt.stmts_len; i++) {
		stmt(ast->ast.block_stmt.stmts[i]);
	}
}

