#include "ast.h"
#include "common.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static token_t *g_head;
static token_t *g_cur;

static void init(token_t *tokens);
static token_t *token_at(int offset);
static char check(int offset, int token_kind);
static token_t *match(int token_kind, const char *message);
static void skip(int inc);

static ast_t *malloc_ast(int kind, const char *filepath, const char *source,
	pos_t start, pos_t end);
static ast_t *malloc_ast_fn_decl(token_t *fn_keyword, token_t *lparen, 
	token_t *rparen, ast_t *block_stmt);
static ast_t *malloc_ast_block_stmt(token_t *lbrace);
static ast_t *malloc_ast_expr_stmt(ast_t *expr, token_t *semicolon);
static ast_t *malloc_ast_literal_expr(token_t *token);

static ast_t *prog();
static ast_t *decl();
static ast_t *fn_decl();
static ast_t *stmt();
static ast_t *block_stmt();
static ast_t *expr_stmt();
static ast_t *expr();
static ast_t *literal_expr();

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
	*list[*len-1] = ast;
}

// ========================================
// helper definition
// ========================================

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

static ast_t *malloc_ast_fn_decl(token_t *fn_keyword, token_t *lparen, 
	token_t *rparen, ast_t *block_stmt) {
	ast_t *res = malloc_ast(AST_FN_DECL, fn_keyword->filepath, 
		fn_keyword->source, fn_keyword->start, block_stmt->end);
	res->ast.fn_decl.fn_keyword = fn_keyword;
	res->ast.fn_decl.lparen = lparen;
	res->ast.fn_decl.rparen = rparen;
	res->ast.fn_decl.block_stmt = block_stmt;
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

static ast_t *prog() {
	return decl();
}

static ast_t *decl() {
	return fn_decl();
}

static ast_t *fn_decl() {
	token_t *fn_keyword = match(TOKEN_FN_KEYWORD, "Expected fn keyword");
	token_t *lparen = match(TOKEN_LPAREN, "Expected (");
	token_t *rparen = match(TOKEN_RPAREN, "Expected )");
	ast_t *s = block_stmt();
	return malloc_ast_fn_decl(fn_keyword, lparen, rparen, s);
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
	return literal_expr();
}

static ast_t *literal_expr() {
	token_t *token = match(TOKEN_INT_LITERAL, "Expected int literal");
	return malloc_ast_literal_expr(token);
}

