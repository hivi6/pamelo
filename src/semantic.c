#include "semantic.h"
#include "util.h"
#include "type.h"
#include "scope.h"

// ========================================
// helper declaration
// ========================================

static int g_first_time = 1;
static type_t *g_void = NULL;
static type_t *g_u8 = NULL;
static type_t *g_u16 = NULL;
static type_t *g_u32 = NULL;
static type_t *g_u64 = NULL;
static type_t *g_current_return_type = NULL;
static int g_check_return_stmt = 0;

static void init(ast_t *ast);
static void match(ast_t *ast, int kind, const char *message);
static void create_fn(ast_t *ast, scope_t *scope);
static char is_numeric(type_t *type);
static char is_castable(type_t *out, type_t *in);

static void prog(ast_t *ast, scope_t *scope);

static type_t *type_specifier(ast_t *ast, scope_t *scope);

static void fn_decl(ast_t *ast, scope_t *scope);

static void stmt(ast_t *ast, scope_t *scope);
static void block_stmt(ast_t *ast, scope_t *scope);
static void var_stmt(ast_t *ast, scope_t *scope);
static void return_stmt(ast_t *ast, scope_t *scope);
static void expr_stmt(ast_t *ast, scope_t *scope);

static type_t *expr(ast_t *ast, scope_t *scope);
static type_t *literal_expr(ast_t *ast, scope_t *scope);
static type_t *var_expr(ast_t *ast, scope_t *scope);
static type_t *call_expr(ast_t *ast, scope_t *scope);
static type_t *cast_expr(ast_t *ast, scope_t *scope);
static type_t *add_expr(ast_t *ast, scope_t *scope);

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
		g_void = v;
		g_u8 = u8;
		g_u16 = u16;
		g_u32 = u32;
		g_u64 = u64;
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
			"Function type already defined");
		exit(1);
	}
	fn_type->type.fn_type.return_type = return_type;
	ast->type = fn_type;

	// add a function symbol
	symbol_t *s = create_symbol(name, fn_type);
	if (!add_symbol(scope, s)) {
		eprintf(tok->filepath, tok->source, tok->start, tok->end,
			"Function symbol already defined");
		exit(1);
	}

	free(name);
}

static char is_numeric(type_t *type) {
	return type == g_u8 || type == g_u16 || type == g_u32 || type == g_u64;
}

static char is_castable(type_t *out, type_t *in) {
	return is_numeric(out) && is_numeric(in);
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
	g_current_return_type = ast->type->type.fn_type.return_type;
	g_check_return_stmt = (g_current_return_type != g_void);
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

	return ast->type = t;
}

static void stmt(ast_t *ast, scope_t *scope) {
	ast->scope = scope;

	if (ast->kind == AST_BLOCK_STMT) {
		block_stmt(ast, scope);
	}
	else if (ast->kind == AST_VAR_STMT) {
		var_stmt(ast, scope);
	}
	else if (ast->kind == AST_EXPR_STMT) {
		expr_stmt(ast, scope);
	}
	else if (ast->kind == AST_RETURN_STMT) {
		return_stmt(ast, scope);
	}
	else {
		eprintf(ast->filepath, ast->source, ast->start, ast->end,
			"What is this statement?");
		exit(1);
	}
}

static void block_stmt(ast_t *ast, scope_t *scope) {
	match(ast, AST_BLOCK_STMT, "Expected AST_BLOCK_STMT");

	if (g_check_return_stmt) {
		int len = ast->ast.block_stmt.stmts_len;
		int invalid = 0;
		ast_t *err = ast;

		if (len > 0) {
			ast_t *last_stmt = ast->ast.block_stmt.stmts[len-1];
			if (last_stmt->kind != AST_RETURN_STMT) invalid = 1;
			err = last_stmt;
		}
		else invalid = 1;


		if (invalid) {
			eprintf(err->filepath, err->source, err->start, 
				err->end, 
				"Expected return statement at the end");
			exit(1);
		}
	}

	g_check_return_stmt = 0;
	scope_t *new_scope = create_scope(scope);
	ast->scope = new_scope;


	for (int i = 0; i < ast->ast.block_stmt.stmts_len; i++) {
		stmt(ast->ast.block_stmt.stmts[i], new_scope);
	}
}

static void var_stmt(ast_t *ast, scope_t *scope) {
	match(ast, AST_VAR_STMT, "Expected AST_VAR_STMT");

	type_t *t1 = NULL, *t2 = NULL;
	if (ast->ast.var_stmt.type_specifier) {
		t1 = type_specifier(ast->ast.var_stmt.type_specifier, scope);
		if (t1 && t1->kind == TYPE_VOID) {
			ast_t *t = ast->ast.var_stmt.type_specifier;
			eprintf(t->filepath, t->source, t->start, t->end, 
				"Cannot be void type in var statement");
			exit(1);
		}
	}
	if (ast->ast.var_stmt.expr) {
		t2 = expr(ast->ast.var_stmt.expr, scope);
	}

	if (t1 == NULL && t2 == NULL) {
		token_t *name = ast->ast.var_stmt.name;
		eprintf(name->filepath, name->source, name->start, name->end,
			"Cannot infer type as type info and "
			"expression is missing");
		exit(1);
	}

	if (t2 == g_void) {
		ast_t *expr = ast->ast.var_stmt.expr;
		eprintf(expr->filepath, expr->source, expr->start, expr->end,
			"Cannot have void expression");
		exit(1);
	}
	
	type_t *final_type = (t1 ? t1 : t2);
	token_t *tok = ast->ast.var_stmt.name;
	char *name = token_lexical(*tok);
	symbol_t *var_symbol = create_symbol(name, final_type);
	if (!add_symbol(scope, var_symbol)) {
		eprintf(tok->filepath, tok->source, tok->start, tok->end,
			"Symbol already defined");
		exit(1);
	}

	free(name);

	ast->type = final_type;
}

static void return_stmt(ast_t *ast, scope_t *scope) {
	match(ast, AST_RETURN_STMT, "Expected AST_RETURN_STMT");

	type_t *return_type = g_void;
	if (ast->ast.return_stmt.expr) {
		return_type = expr(ast->ast.return_stmt.expr, scope);
	}

	if (return_type == g_void && ast->ast.return_stmt.expr) {
		eprintf(ast->filepath, ast->source, ast->start, ast->end,
			"Void cannot be in return statements");
		exit(1);
	}

	if (g_current_return_type == return_type) {
		return;
	}

	if (!is_castable(g_current_return_type, return_type)) {
		eprintf(ast->filepath, ast->source, ast->start, ast->end,
			"Incompatible return expression and function return type");
		exit(1);
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
	else if (ast->kind == AST_VAR_EXPR) {
		type = var_expr(ast, scope);
	}
	else if (ast->kind == AST_CALL_EXPR) {
		type = call_expr(ast, scope);
	}
	else if (ast->kind == AST_ADD_EXPR) {
		type = add_expr(ast, scope);
	}
	else if (ast->kind == AST_CAST_EXPR) {
		type = cast_expr(ast, scope);
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

static type_t *var_expr(ast_t *ast, scope_t *scope) {
	match(ast, AST_VAR_EXPR, "Expected AST_VAR_EXPR");
	
	token_t *tok = ast->ast.var_expr.token;
	char *name = token_lexical(*tok);
	symbol_t *s = get_symbol_in_chain(scope, name);
	if (s == NULL) {
		eprintf(tok->filepath, tok->source, tok->start, tok->end,
			"No such variable defined");
		exit(1);
	}
	
	return s->type;
}

static type_t *call_expr(ast_t *ast, scope_t *scope) {
	match(ast, AST_CALL_EXPR, "Expected AST_CALL_EXPR");

	ast_t *left = ast->ast.call_expr.left;
	type_t *type = expr(left, scope);
	if (type->kind != TYPE_FN) {
		eprintf(left->filepath, left->source, left->start, left->end,
			"Expected function type");
		exit(1);
	}

	return type->type.fn_type.return_type;
}

static type_t *cast_expr(ast_t *ast, scope_t *scope) {
	match(ast, AST_CAST_EXPR, "Expected AST_CAST_EXPR");

	type_t *left = expr(ast->ast.cast_expr.left, scope);
	type_t *t = type_specifier(ast->ast.cast_expr.type_specifier, scope);
	
	ast_t *err = NULL;
	if (!is_numeric(t)) {
		err = ast->ast.cast_expr.type_specifier;
	}
	if (!is_numeric(left)) {
		err = ast->ast.cast_expr.left;
	}

	if (err) {
		eprintf(err->filepath, err->source, err->start, err->end,
			"Expected numeric type");
		exit(1);
	}

	return t;
}

static type_t *add_expr(ast_t *ast, scope_t *scope) {
	match(ast, AST_ADD_EXPR, "Expected AST_ADD_EXPR");
	
	type_t *left = expr(ast->ast.add_expr.left, scope);
	type_t *right = expr(ast->ast.add_expr.right, scope);
	ast_t *err_ast = NULL;
	if (!is_numeric(left)) {
		err_ast = ast->ast.add_expr.left;
	}
	if (!is_numeric(right)) {
		err_ast = ast->ast.add_expr.right;
	}
	if (err_ast) {
		eprintf(err_ast->filepath, err_ast->source, err_ast->start,
			err_ast->end, "Expected a numeric type");
		exit(1);
	}

	if (left->size > right->size) return left;
	return right;
}

