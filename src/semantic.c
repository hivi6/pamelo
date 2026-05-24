#include "semantic.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static void match(ast_t *ast, int kind, const char *message);
static void create_fn(ast_t *ast);

static void prog(ast_t *ast);

static void fn_decl(ast_t *ast);

static void stmt(ast_t *ast);
static void block_stmt(ast_t *ast);

// ========================================
// semantic.h - definition
// ========================================

void semantic_analyse(ast_t *ast) {
	prog(ast);
}

// ========================================
// helper definition
// ========================================

static void match(ast_t *ast, int kind, const char *message) {
	if (ast->kind == kind) return;
	eprintf(ast->filepath, ast->source, ast->start, ast->end,
		message);
	exit(1);
}

static void create_fn(ast_t *ast) {
	// TODO
}

static void prog(ast_t *ast) {
	match(ast, AST_PROG, "Expected AST_PROG");

	// first go through all the function declaration
	for (int i = 0; i < ast->ast.prog.decls_len; i++) {
		ast_t *decl = ast->ast.prog.decls[i];
		if (decl->kind == AST_FN_DECL) {
			fn_decl(decl);
		}
	}
}

static void fn_decl(ast_t *ast) {
	match(ast, AST_FN_DECL, "Expected AST_FN_DECL");

	create_fn(ast);
	block_stmt(ast->ast.fn_decl.block_stmt);
}

static void stmt(ast_t *ast) {
	// TODO
}

static void block_stmt(ast_t *ast) {
	// TODO
}

