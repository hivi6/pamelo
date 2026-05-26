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
static void create_fn(ast_t *ast, scope_t *scope);

static void prog(ast_t *ast, scope_t *scope);

static type_t *type_specifier(ast_t *ast, scope_t *scope);

static void fn_decl(ast_t *ast, scope_t *scope);

static void stmt(ast_t *ast, scope_t *scope);
static void block_stmt(ast_t *ast, scope_t *scope);
static void expr_stmt(ast_t *ast, scope_t *scope);

static type_t *expr(ast_t *ast, scope_t *scope);
static type_t *literal_expr(ast_t *ast, scope_t *scope);

// ========================================
// semantic.h - definition
// ========================================

void semantic_analyse(ast_t *ast) {
	init(ast);
	prog(ast, get_global_scope());
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

static void create_fn(ast_t *ast, scope_t *scope) {
	match(ast, AST_FN_DECL, "Expected AST_FN_DECL; create_fn(ast_t*)");

	scope_t *fn_scope = create_scope(scope);
	ast->scope = fn_scope;

	token_t *tok = ast->ast.fn_decl.name;
	char *name = token_lexical(*tok);

	type_t *return_type = type_specifier(ast->ast.fn_decl.type_specifier,
		scope);

	// Add the function type in the parent scope
	type_t *fn_type = create_type(TYPE_FN, name, 0);
	if (!add_type(scope, fn_type)) {
		eprintf(tok->filepath, tok->source, tok->start, tok->end,
			"Function already defined");
		exit(1);
	}
	fn_type->type.fn_type.return_type = return_type;

	free(name);
}

static void prog(ast_t *ast, scope_t *scope) {
	match(ast, AST_PROG, "Expected AST_PROG; prog(ast_t*)");

	scope_t *new_scope = create_scope(scope);
	ast->scope = new_scope;

	// first go through all the function declaration and 
	// create the function
	for (int i = 0; i < ast->ast.prog.decls_len; i++) {
		ast_t *decl = ast->ast.prog.decls[i];
		if (decl->kind == AST_FN_DECL) {
			create_fn(decl, new_scope);
		}
	}

	// first go through all the function declaration
	for (int i = 0; i < ast->ast.prog.decls_len; i++) {
		ast_t *decl = ast->ast.prog.decls[i];
		if (decl->kind == AST_FN_DECL) {
			fn_decl(decl, new_scope);
		}
	}
}

static void fn_decl(ast_t *ast, scope_t *scope) {
	match(ast, AST_FN_DECL, "Expected AST_FN_DECL");
	block_stmt(ast->ast.fn_decl.block_stmt, ast->scope);
}

static type_t *type_specifier(ast_t *ast, scope_t *scope) {
	match(ast, AST_TYPE_SPECIFIER, "Expected AST_TYPE_SPECIFIER");

	token_t *tok = ast->ast.type_specifier.name;
	char *name = token_lexical(*tok);

	type_t *t = get_type_in_chain(scope, name);
	if (t == NULL) {
		eprintf(tok->filepath, tok->source, tok->start, tok->end,
			"Type not defined");
		exit(1);
	}
	if (t->kind == TYPE_FN) {
		eprintf(tok->filepath, tok->source, tok->start, tok->end,
			"Cannot be a function type");
		exit(1);
	}

	free(name);

	return t;
}

static void stmt(ast_t *ast, scope_t *scope) {
	ast->scope = scope;

	if (ast->kind == AST_BLOCK_STMT) {
		block_stmt(ast, scope);
	}
	else if (ast->kind == AST_EXPR_STMT) {
		expr_stmt(ast, scope);
	}
	else {
		eprintf(ast->filepath, ast->source, ast->start, ast->end,
			"What is this statement?");
		exit(1);
	}
}

static void block_stmt(ast_t *ast, scope_t *scope) {
	match(ast, AST_BLOCK_STMT, "Expected AST_BLOCK_STMT");

	scope_t *new_scope = create_scope(scope);
	ast->scope = new_scope;

	for (int i = 0; i < ast->ast.block_stmt.stmts_len; i++) {
		stmt(ast->ast.block_stmt.stmts[i], new_scope);
	}
}

static void expr_stmt(ast_t *ast, scope_t *scope) {
	match(ast, AST_EXPR_STMT, "Expected AST_EXPR_STMT");
	ast->scope = scope;
	expr(ast->ast.expr_stmt.expr, scope);
}

static type_t *expr(ast_t *ast, scope_t *scope) {
	ast->scope = scope;

	type_t *type = NULL;
	if (ast->kind == AST_LITERAL_EXPR) {
		type = literal_expr(ast, scope);
	}

	if (type == NULL) {
		eprintf(ast->filepath, ast->source, ast->start, ast->end,
			"What is this ast(kind: %d)?", ast->kind);
		exit(1);
	}

	return ast->type = type;
}

static type_t *literal_expr(ast_t *ast, scope_t *scope) {
	match(ast, AST_LITERAL_EXPR, "Expected AST_LITERAL_EXPR");

	token_t *tok = ast->ast.literal_expr.token;
	type_t *type = NULL;
	if (tok->kind == TOKEN_INT_LITERAL) {
		type = get_type_in_chain(scope, "u32");
	}

	if (type == NULL) {
		eprintf(tok->filepath, tok->source, tok->start, tok->end,
			"Invalid token literal type");
		exit(1);
	}

	return type;
}

