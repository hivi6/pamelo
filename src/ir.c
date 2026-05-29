#include "ir.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static ir_fn_t ***g_list;
static int *g_len;

static void init(ir_fn_t ***list, int *len);
static void match(ast_t *ast, int kind, const char *message);

static void prog(ast_t *ast);

static void decl(ast_t *ast);
static void fn_decl(ast_t *ast);

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
}

