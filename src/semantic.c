#include "semantic.h"
#include "util.h"
#include "type.h"
#include "scope.h"

// ========================================
// helper declaration
// ========================================

static int g_first_time = 1;

static void init(ast_t *ast);
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
	init(ast);
	prog(ast);
}

// ========================================
// helper definition
// ========================================

static void init(ast_t *ast) {
	if (g_first_time) {
		type_t *v = create_type(TYPE_VOID, "void", 0);
		type_t *u8 = create_type(TYPE_PRIMITIVE, "u8", 8);
		type_t *u16 = create_type(TYPE_PRIMITIVE, "u16", 16);
		type_t *u32 = create_type(TYPE_PRIMITIVE, "u32", 32);
		type_t *u64 = create_type(TYPE_PRIMITIVE, "u64", 64);
		scope_t *global_scope = get_global_scope();
		add_type(global_scope, v);
		add_type(global_scope, u8);
		add_type(global_scope, u16);
		add_type(global_scope, u32);
		add_type(global_scope, u64);
	}
	g_first_time = 0;
}

static void match(ast_t *ast, int kind, const char *message) {
	if (ast->kind == kind) return;
	eprintf(ast->filepath, ast->source, ast->start, ast->end,
		message);
	exit(1);
}

static void create_fn(ast_t *ast) {
	match(ast, AST_FN_DECL, "Expected AST_FN_DECL; create_fn(ast_t*)");
}

static void prog(ast_t *ast) {
	match(ast, AST_PROG, "Expected AST_PROG; prog(ast_t*)");

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
	match(ast, AST_BLOCK_STMT, "Expected AST_BLOCK_STMT");
}

