#include "ast.h"
#include "common.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static token_t *g_head;
static token_t *g_cur;

static void print_ast_helper(ast_t *ast, char *indent, int depth, 
	const char *extra);
static char *token_str(token_t *token);

static void init(token_t *tokens);
static token_t *token_at(int offset);
static char check(int offset, int token_kind);
static token_t *match(int token_kind, const char *message);
static void skip(int inc);

static ast_t *malloc_ast(int kind, const char *filepath, const char *source,
	pos_t start, pos_t end);
static ast_t *malloc_ast_prog();
static ast_t *malloc_ast_fn_decl(token_t *fn_keyword, token_t *name,
	token_t *lparen, token_t *rparen, ast_t *t, ast_t *block_stmt);
static ast_t *malloc_ast_type_specifier(token_t *id);
static ast_t *malloc_ast_block_stmt(token_t *lbrace);
static ast_t *malloc_ast_expr_stmt(ast_t *expr, token_t *semicolon);
static ast_t *malloc_ast_literal_expr(token_t *token);
static ast_t *malloc_ast_add_expr(ast_t *left, token_t *op, ast_t *right);

static ast_t *prog();
static ast_t *decl();
static ast_t *fn_decl();
static ast_t *type_specifier();
static ast_t *stmt();
static ast_t *block_stmt();
static ast_t *expr_stmt();
static ast_t *expr();
static ast_t *literal_expr();
static ast_t *add_expr();

// ========================================
// ast.h - definition
// ========================================

ast_t *parse(token_t *tokens) {
	init(tokens);
	return prog();
}

void append_ast(ast_t ***list, int *len, ast_t *ast) {
	*len += 1;
	*list = realloc(*list, *len * sizeof(ast_t*));
	(*list)[*len-1] = ast;
}

void print_ast(ast_t *ast) {
	char indent[1024] = {};
	print_ast_helper(ast, indent, 0, NULL);
}

// ========================================
// helper definition
// ========================================

static void print_ast_helper(ast_t *ast, char *indent, int depth,
	const char *extra) {
	if (depth+1 >= 1024) {
		printf("[...] Too deep\n");
		return;
	}

	for (int i = 0; i < depth; i++) {
		if (indent[i]) printf("|  ");
		else printf("   ");
	}

	indent[depth+1] = 1;
	if (extra) printf("+- %s: ", extra);
	else printf("+- ");
	switch (ast->kind) {
	case AST_PROG: {
		printf("AST_PROG\n");
		for (int i = 0; i < ast->ast.prog.decls_len; i++) {
			if (i == ast->ast.prog.decls_len-1) 
				indent[depth+1] = 0;
			print_ast_helper(ast->ast.prog.decls[i], indent, 
				depth+1, NULL);
		}
		break;
	}
	case AST_FN_DECL: {
		char *str = token_str(ast->ast.fn_decl.name);
		printf("AST_FN_DECL(%s)\n", str);
		free(str);
		print_ast_helper(ast->ast.fn_decl.type_specifier, indent, 
			depth+1, "RETURN TYPE");
		indent[depth+1] = 0;
		print_ast_helper(ast->ast.fn_decl.block_stmt, indent, depth+1, 
			"FUNCTION BODY");
		break;
	}
	case AST_TYPE_SPECIFIER: {
		char *str = token_str(ast->ast.type_specifier.name);
		printf("AST_TYPE_SPECIFIER(%s)\n", str);
		free(str);
		break;
	}
	case AST_BLOCK_STMT: {
		printf("AST_BLOCK_STMT\n");
		ast_t **stmts = ast->ast.block_stmt.stmts;
		int stmts_len = ast->ast.block_stmt.stmts_len;
		for (int i = 0; i < stmts_len; i++) {
			if (i == stmts_len-1) indent[depth+1] = 0;
			print_ast_helper(stmts[i], indent, depth+1, NULL);
		}
		break;
	}
	case AST_EXPR_STMT: {
		printf("AST_EXPR_STMT\n");
		indent[depth+1] = 0;
		print_ast_helper(ast->ast.expr_stmt.expr, indent, 
			depth+1, NULL);
		break;
	}
	case AST_LITERAL_EXPR: {
		char *type_info = type_str(ast->type);
		char *str = token_str(ast->ast.literal_expr.token);
		printf("AST_LITERAL_EXPR(%s) [%s]\n", str, type_info);
		free(str);
		free(type_info);
		break;
	}
	case AST_ADD_EXPR: {
		char *type_info = type_str(ast->type);
		char *op = token_str(ast->ast.add_expr.op);
		printf("AST_ADD_EXPR(%s) [%s]\n", op, type_info);
		free(op);
		free(type_info);
		print_ast_helper(ast->ast.add_expr.left, indent, depth+1, 
			NULL);
		indent[depth+1] = 0;
		print_ast_helper(ast->ast.add_expr.right, indent, depth+1, 
			NULL);
		break;
	}
	default: {
		printf("WHAT IS THIS AST?\n");
		exit(1);
	}
	}
}

static char *token_str(token_t *token) {
	char *lexical = token_lexical(*token);
	char *type = token_type(*token);
	char *res = sbuildf("type: %s | lexical: %s", type, lexical);
	free(lexical);
	free(type);
	return res;
}

static void init(token_t *tokens) {
	g_head = g_cur = tokens;
}

static token_t *token_at(int offset) {
	token_t *cur = g_cur;
	while (offset && cur->kind != TOKEN_EOF) {
		cur = cur->next;
		offset--;
	}
	return cur;
}

static char check(int offset, int token_kind) {
	token_t *token = token_at(offset);
	return token->kind == token_kind;
}

static token_t *match(int token_kind, const char *message) {
	token_t *token = token_at(0);
	if (token->kind != token_kind) {
		eprintf(token->filepath, token->source, token->start, 
			token->end, message);
		exit(1);
	}
	skip(1);
	return token;
}

static void skip(int inc) {
	while (g_cur->kind != TOKEN_EOF && inc) {
		inc--;
		g_cur = g_cur->next;
	}
}

static ast_t *malloc_ast(int kind, const char *filepath, const char *source,
	pos_t start, pos_t end) {
	ast_t *res = calloc(sizeof(ast_t), 1);
	res->kind = kind;
	res->filepath = filepath;
	res->source = source;
	res->start = start;
	res->end = end;
	return res;
}

static ast_t *malloc_ast_prog() {
	ast_t *res = malloc_ast(AST_PROG, NULL, NULL, POS_INIT(), POS_INIT());
	return res;
}

static ast_t *malloc_ast_fn_decl(token_t *fn_keyword, token_t *name,
	token_t *lparen, token_t *rparen, ast_t *t, ast_t *block_stmt) {
	ast_t *res = malloc_ast(AST_FN_DECL, fn_keyword->filepath, 
		fn_keyword->source, fn_keyword->start, block_stmt->end);
	res->ast.fn_decl.fn_keyword = fn_keyword;
	res->ast.fn_decl.name = name;
	res->ast.fn_decl.lparen = lparen;
	res->ast.fn_decl.rparen = rparen;
	res->ast.fn_decl.type_specifier = t;
	res->ast.fn_decl.block_stmt = block_stmt;
	return res;
}

static ast_t *malloc_ast_type_specifier(token_t *id) {
	ast_t *res = malloc_ast(AST_TYPE_SPECIFIER, id->filepath,
		id->source, id->start, id->end);
	res->ast.type_specifier.name = id;
	return res;
}

static ast_t *malloc_ast_block_stmt(token_t *lbrace) {
	ast_t *res = malloc_ast(AST_BLOCK_STMT, lbrace->filepath, 
		lbrace->source, lbrace->start, lbrace->end);
	res->ast.block_stmt.lbrace = lbrace;
	return res;
}

static ast_t *malloc_ast_expr_stmt(ast_t *expr, token_t *semicolon) {
	ast_t *res = malloc_ast(AST_EXPR_STMT, expr->filepath,
		expr->source, expr->start, semicolon->end);
	res->ast.expr_stmt.expr = expr;
	res->ast.expr_stmt.semicolon = semicolon;
	return res;
}

static ast_t *malloc_ast_literal_expr(token_t *token) {
	ast_t *res = malloc_ast(AST_LITERAL_EXPR, token->filepath,
		token->source, token->start, token->end);
	res->ast.literal_expr.token = token;
	return res;
}

static ast_t *malloc_ast_add_expr(ast_t *left, token_t *op, ast_t *right) {
	ast_t *res = malloc_ast(AST_ADD_EXPR, left->filepath, left->source,
		left->start, right->end);
	res->ast.add_expr.left = left;
	res->ast.add_expr.op = op;
	res->ast.add_expr.right = right;
	return res;
}

static ast_t *prog() {
	ast_t *ast = malloc_ast_prog();
	while (!check(0, TOKEN_EOF)) {
		ast_t *d = decl();
		append_ast(&ast->ast.prog.decls, &ast->ast.prog.decls_len, d);
		if (!ast->filepath) {
			ast->filepath = d->filepath;
			ast->source = d->source;
			ast->start = d->start;
			ast->end = d->end;
		}
		ast->end = d->end;
	}
	return ast;
}

static ast_t *decl() {
	return fn_decl();
}

static ast_t *fn_decl() {
	token_t *fn_keyword = match(TOKEN_FN_KEYWORD, "Expected fn keyword");
	token_t *name = match(TOKEN_ID, "Expected name for function");
	token_t *lparen = match(TOKEN_LPAREN, "Expected (");
	token_t *rparen = match(TOKEN_RPAREN, "Expected )");
	ast_t *t = type_specifier();
	ast_t *s = block_stmt();
	return malloc_ast_fn_decl(fn_keyword, name, lparen, rparen, t, s);
}

static ast_t *type_specifier() {
	token_t *id = match(TOKEN_ID, "Expected type id");
	return malloc_ast_type_specifier(id);
}

static ast_t *stmt() {
	if (check(0, TOKEN_LBRACE)) return block_stmt();
	return expr_stmt();
}

static ast_t *block_stmt() {
	token_t *lbrace = match(TOKEN_LBRACE, "Expected {");
	ast_t *ast = malloc_ast_block_stmt(lbrace);
	while (!check(0, TOKEN_RBRACE)) {
		ast_t *s = stmt();
		append_ast(&ast->ast.block_stmt.stmts, 
			&ast->ast.block_stmt.stmts_len, s);
	}
	token_t *rbrace = match(TOKEN_RBRACE, "Expected }");

	ast->ast.block_stmt.rbrace = rbrace;
	ast->end = rbrace->end;

	return ast;
}

static ast_t *expr_stmt() {
	ast_t *e = expr();
	token_t *semicolon = match(TOKEN_SEMICOLON, "Expected ;");
	return malloc_ast_expr_stmt(e, semicolon);
}

static ast_t *expr() {
	return add_expr();
}

static ast_t *literal_expr() {
	token_t *token = match(TOKEN_INT_LITERAL, "Expected int literal");
	return malloc_ast_literal_expr(token);
}

static ast_t *add_expr() {
	ast_t *left = literal_expr();
	while (check(0, TOKEN_PLUS) || check(0, TOKEN_MINUS)) {
		token_t *op = token_at(0);
		skip(1);
		ast_t *right = literal_expr();
		left = malloc_ast_add_expr(left, op, right);
	}
	return left;
}

