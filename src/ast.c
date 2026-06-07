#include "ast.h"
#include "common.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

typedef struct parser_t parser_t;
struct parser_t {
	int cur;
	vec_t tokens;
};

static void print_ast_helper(ast_t *ast, char *indent, int depth, 
	const char *extra);
static char *token_str(token_t *token);

static void init(parser_t *parser, vec_t tokens);
static token_t *token_at(parser_t *parser, int offset);
static char check(parser_t *parser, int offset, int token_kind);
static token_t *match(parser_t *parser, int token_kind, const char *message);
static void skip(parser_t *parser, int inc);

static ast_t *malloc_ast(int kind, const char *filepath, const char *source,
	pos_t start, pos_t end);
static ast_t *malloc_ast_prog();
static ast_t *malloc_ast_fn_decl(token_t *fn_keyword, token_t *name,
	token_t *lparen, token_t *rparen, ast_t *t, token_t *extern_keyword,
	token_t *semicolon, ast_t *block_stmt);
static ast_t *malloc_ast_type_specifier(token_t *id);
static ast_t *malloc_ast_block_stmt(token_t *lbrace);
static ast_t *malloc_ast_var_stmt(token_t *var_keyword, token_t *name,
	ast_t *type_specifier, ast_t *expr, token_t *semicolon);
static ast_t *malloc_ast_return_stmt(token_t *return_keyword, ast_t *expr,
	token_t *semicolon);
static ast_t *malloc_ast_if_stmt(token_t *if_keyword, token_t *lparen,
	ast_t *expr, token_t *rparen, ast_t *true_stmt, token_t *else_keyword,
	ast_t *false_stmt);
static ast_t *malloc_ast_while_stmt(token_t *if_keyword, token_t *lparen,
	ast_t *expr, token_t *rparen, ast_t *true_stmt);
static ast_t *malloc_ast_expr_stmt(ast_t *expr, token_t *semicolon);
static ast_t *malloc_ast_literal_expr(token_t *token);
static ast_t *malloc_ast_var_expr(token_t *token);
static ast_t *malloc_ast_call_expr(ast_t *left, token_t *lparen, 
	token_t *rparen);
static ast_t *malloc_ast_cast_expr(ast_t *left, token_t *as_keyword, 
	ast_t *type_specifier);
static ast_t *malloc_ast_mul_expr(ast_t *left, token_t *op, ast_t *right);
static ast_t *malloc_ast_add_expr(ast_t *left, token_t *op, ast_t *right);
static ast_t *malloc_ast_equal_expr(ast_t *left, token_t *op, ast_t *right);

static ast_t *prog(parser_t *parser);
static ast_t *decl(parser_t *parser);
static ast_t *fn_decl(parser_t *parser);
static ast_t *type_specifier(parser_t *parser);
static ast_t *stmt(parser_t *parser);
static ast_t *block_stmt(parser_t *parser);
static ast_t *var_stmt(parser_t *parser);
static ast_t *return_stmt(parser_t *parser);
static ast_t *if_stmt(parser_t *parser);
static ast_t *while_stmt(parser_t *parser);
static ast_t *expr_stmt(parser_t *parser);
static ast_t *expr(parser_t *parser);
static ast_t *literal_expr(parser_t *parser);
static ast_t *var_expr(parser_t *parser);
static ast_t *primary_expr(parser_t *parser);
static ast_t *postfix_expr(parser_t *parser);
static ast_t *call_expr(parser_t *parser, ast_t *left);
static ast_t *cast_expr(parser_t *parser);
static ast_t *mul_expr(parser_t *parser);
static ast_t *add_expr(parser_t *parser);
static ast_t *equal_expr(parser_t *parser);

// ========================================
// ast.h - definition
// ========================================

ast_t *parse(vec_t tokens) {
	parser_t parser = {0};
	init(&parser, tokens);
	return prog(&parser);
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
		for (int i = 0; i < ast->ast.prog.decls.len; i++) {
			if (i == ast->ast.prog.decls.len-1) 
				indent[depth+1] = 0;
			print_ast_helper(ast->ast.prog.decls.elems[i], indent, 
				depth+1, NULL);
		}
		break;
	}
	case AST_FN_DECL: {
		char *str = token_str(ast->ast.fn_decl.name);
		printf("AST_FN_DECL(%s)", str);
		free(str);

		if (ast->ast.fn_decl.extern_keyword) {
			printf(" EXTERN");
		}
		printf("\n");
		

		print_ast_helper(ast->ast.fn_decl.type_specifier, indent, 
			depth+1, "RETURN TYPE");

		for (int i = 0; i < ast->ast.fn_decl.params.len; i++) {
			token_t *param_token = ast->ast.fn_decl.params.elems[i];
			char *param = token_lexical(*param_token);
			print_ast_helper(ast->ast.fn_decl.param_types.elems[i], 
				indent, depth+1, param);
			free(param);
		}

		indent[depth+1] = 0;

		if (ast->ast.fn_decl.block_stmt) {
			print_ast_helper(ast->ast.fn_decl.block_stmt, indent, 
				depth+1, "FUNCTION BODY");
		}
		break;
	}
	case AST_TYPE_SPECIFIER: {
		char *type_info = type_str(ast->type);
		char *str = token_str(ast->ast.type_specifier.name);
		printf("AST_TYPE_SPECIFIER(%s) [%s]\n", str, type_info);
		free(str);
		free(type_info);
		break;
	}
	case AST_BLOCK_STMT: {
		printf("AST_BLOCK_STMT\n");
		vec_t stmts = ast->ast.block_stmt.stmts;
		int stmts_len = ast->ast.block_stmt.stmts.len;
		for (int i = 0; i < stmts_len; i++) {
			if (i == stmts_len-1) indent[depth+1] = 0;
			print_ast_helper(stmts.elems[i], indent, depth+1, NULL);
		}
		break;
	}
	case AST_VAR_STMT: {
		char *type_info = type_str(ast->type);
		char *name = token_str(ast->ast.var_stmt.name);
		printf("AST_VAR_STMT(%s) [%s]\n", name, type_info);
		free(type_info);
		free(name);
		if (ast->ast.var_stmt.type_specifier) {
			if (ast->ast.var_stmt.expr == NULL) 
				indent[depth+1] = 0;
			print_ast_helper(ast->ast.var_stmt.type_specifier, 
				indent, depth+1, NULL);
		}
		indent[depth+1] = 0;
		if (ast->ast.var_stmt.expr) {
			print_ast_helper(ast->ast.var_stmt.expr, indent, 
				depth+1, NULL);
		}
		break;
	}
	case AST_RETURN_STMT: {
		printf("AST_RETURN_STMT\n");
		indent[depth+1] = 0;
		if (ast->ast.return_stmt.expr) {
			print_ast_helper(ast->ast.return_stmt.expr, indent,
				depth+1, NULL);
		}
		break;
	}
	case AST_IF_STMT: {
		printf("AST_IF_STMT\n");

		print_ast_helper(ast->ast.if_stmt.expr, indent, depth+1,
			"IF CONDITION");

		if (ast->ast.if_stmt.false_stmt == NULL) {
			indent[depth+1] = 0;
		}
		print_ast_helper(ast->ast.if_stmt.true_stmt, indent, depth+1,
			"TRUE STATEMENT");
		
		if (ast->ast.if_stmt.false_stmt) {
			indent[depth+1] = 0;
			print_ast_helper(ast->ast.if_stmt.false_stmt, indent,
				depth+1, "FALSE STATEMENT");
		}

		break;
	}
	case AST_WHILE_STMT: {
		printf("AST_WHILE_STMT\n");

		print_ast_helper(ast->ast.while_stmt.expr, indent, depth+1,
			"WHILE CONDITION");

		indent[depth+1] = 0;
		print_ast_helper(ast->ast.while_stmt.true_stmt, indent, depth+1,
			"WHILE STATEMENT");

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
	case AST_VAR_EXPR: {
		char *type_info = type_str(ast->type);
		char *str = token_str(ast->ast.var_expr.token);
		printf("AST_VAR_EXPR(%s) [%s]\n", str, type_info);
		free(str);
		free(type_info);
		break;
	}
	case AST_CALL_EXPR: {
		char *type_info = type_str(ast->type);
		printf("AST_CALL_EXPR [%s]\n", type_info);
		free(type_info);

		if (ast->ast.call_expr.args.len <= 0) indent[depth+1] = 0;
		print_ast_helper(ast->ast.call_expr.left, indent, depth+1, 
			NULL);

		for (int i = 0; i < ast->ast.call_expr.args.len; i++) {
			if (i == ast->ast.call_expr.args.len-1)
				indent[depth+1] = 0;
			char *arg_name = sbuildf("ARG %d", i+1);
			print_ast_helper(ast->ast.call_expr.args.elems[i], 
				indent, depth+1, arg_name);
			free(arg_name);
		}

		break;
	}
	case AST_CAST_EXPR: {
		char *type_info = type_str(ast->type);
		printf("AST_CAST_EXPR [%s]\n", type_info);
		free(type_info);
		print_ast_helper(ast->ast.cast_expr.left, indent, depth+1, 
			"CASTING EXPR");
		indent[depth+1] = 0;
		print_ast_helper(ast->ast.cast_expr.type_specifier, indent,
			depth+1, "CASTING TYPE");
		break;
	}
	case AST_MUL_EXPR: {
		char *type_info = type_str(ast->type);
		char *op = token_str(ast->ast.mul_expr.op);
		printf("AST_MUL_EXPR(%s) [%s]\n", op, type_info);
		free(op);
		free(type_info);
		print_ast_helper(ast->ast.mul_expr.left, indent, depth+1, 
			NULL);
		indent[depth+1] = 0;
		print_ast_helper(ast->ast.mul_expr.right, indent, depth+1, 
			NULL);
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
	case AST_EQUAL_EXPR: {
		char *type_info = type_str(ast->type);
		char *op = token_str(ast->ast.equal_expr.op);
		printf("AST_EQUAL_EXPR(%s) [%s]\n", op, type_info);
		free(op);
		free(type_info);
		print_ast_helper(ast->ast.equal_expr.left, indent, depth+1, 
			NULL);
		indent[depth+1] = 0;
		print_ast_helper(ast->ast.equal_expr.right, indent, depth+1, 
			NULL);
		break;
	}
	default: {
		eprintf(ast->filepath, ast->source, ast->start, ast->end,
			"What is this ast?");
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

static void init(parser_t *parser, vec_t tokens) {
	parser->tokens = tokens;
}

static token_t *token_at(parser_t *parser, int offset) {
	token_t *cur = NULL;
	vec_get(&parser->tokens, parser->cur + offset, (void*) &cur);
	return cur;
}

static char check(parser_t *parser, int offset, int token_kind) {
	token_t *token = token_at(parser, offset);
	return token->kind == token_kind;
}

static token_t *match(parser_t *parser, int token_kind, const char *message) {
	token_t *token = token_at(parser, 0);
	if (token->kind != token_kind) {
		eprintf(token->filepath, token->source, token->start, 
			token->end, message);
		exit(1);
	}
	skip(parser, 1);
	return token;
}

static void skip(parser_t *parser, int inc) {
	while (inc > 0) {
		token_t *token = token_at(parser, 0);
		if (token->kind == TOKEN_EOF) break;
		inc--;
		parser->cur += 1;
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
	token_t *lparen, token_t *rparen, ast_t *t, 
	token_t *extern_keyword, token_t *semicolon, ast_t *block_stmt) {

	assert(semicolon != NULL || block_stmt != NULL);

	pos_t end = (semicolon ? semicolon->end : block_stmt->end);
	ast_t *res = malloc_ast(AST_FN_DECL, fn_keyword->filepath, 
		fn_keyword->source, fn_keyword->start, end);
	res->ast.fn_decl.fn_keyword = fn_keyword;
	res->ast.fn_decl.name = name;
	res->ast.fn_decl.lparen = lparen;
	res->ast.fn_decl.rparen = rparen;
	res->ast.fn_decl.type_specifier = t;
	res->ast.fn_decl.extern_keyword = extern_keyword;
	res->ast.fn_decl.semicolon = semicolon;
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

static ast_t *malloc_ast_var_stmt(token_t *var_keyword, token_t *name,
	ast_t *type_specifier, ast_t *expr, token_t *semicolon) {
	ast_t *res = malloc_ast(AST_VAR_STMT, var_keyword->filepath,
		var_keyword->source, var_keyword->start, semicolon->end);
	res->ast.var_stmt.var_keyword = var_keyword;
	res->ast.var_stmt.name = name;
	res->ast.var_stmt.type_specifier = type_specifier;
	res->ast.var_stmt.expr = expr;
	res->ast.var_stmt.semicolon = semicolon;
	return res;
}

static ast_t *malloc_ast_return_stmt(token_t *return_keyword, ast_t *expr,
	token_t *semicolon) {
	ast_t *res = malloc_ast(AST_RETURN_STMT, return_keyword->filepath,
		return_keyword->source, return_keyword->start, semicolon->end);
	res->ast.return_stmt.return_keyword = return_keyword;
	res->ast.return_stmt.expr = expr;
	res->ast.return_stmt.semicolon = semicolon;
	return res;
}

static ast_t *malloc_ast_if_stmt(token_t *if_keyword, token_t *lparen,
	ast_t *expr, token_t *rparen, ast_t *true_stmt, token_t *else_keyword,
	ast_t *false_stmt) {
	
	pos_t end = (false_stmt ? false_stmt->end : true_stmt->end);
	ast_t *res = malloc_ast(AST_IF_STMT, if_keyword->filepath,
		if_keyword->source, if_keyword->start, end);
	res->ast.if_stmt.if_keyword = if_keyword;
	res->ast.if_stmt.lparen = lparen;
	res->ast.if_stmt.expr = expr;
	res->ast.if_stmt.rparen = rparen;
	res->ast.if_stmt.true_stmt = true_stmt;
	res->ast.if_stmt.else_keyword = else_keyword;
	res->ast.if_stmt.false_stmt = false_stmt;
	return res;
}

static ast_t *malloc_ast_while_stmt(token_t *while_keyword, token_t *lparen,
	ast_t *expr, token_t *rparen, ast_t *true_stmt) {
	ast_t *res = malloc_ast(AST_WHILE_STMT, while_keyword->filepath,
		while_keyword->source, while_keyword->start, true_stmt->end);
	res->ast.while_stmt.while_keyword = while_keyword;
	res->ast.while_stmt.lparen = lparen;
	res->ast.while_stmt.expr = expr;
	res->ast.while_stmt.rparen = rparen;
	res->ast.while_stmt.true_stmt = true_stmt;
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

static ast_t *malloc_ast_var_expr(token_t *token) {
	ast_t *res = malloc_ast(AST_VAR_EXPR, token->filepath, token->source,
		token->start, token->end);
	res->ast.var_expr.token = token;
	return res;
}

static ast_t *malloc_ast_call_expr(ast_t *left, token_t *lparen, 
	token_t *rparen) {
	ast_t *res = malloc_ast(AST_CALL_EXPR, left->filepath, left->source,
		left->start, rparen->end);
	res->ast.call_expr.left = left;
	res->ast.call_expr.lparen = lparen;
	res->ast.call_expr.rparen = rparen;
	return res;
}

static ast_t *malloc_ast_cast_expr(ast_t *left, token_t *as_keyword, 
	ast_t *type_specifier) {
	ast_t *res = malloc_ast(AST_CAST_EXPR, left->filepath, left->source,
		left->start, type_specifier->end);
	res->ast.cast_expr.left = left;
	res->ast.cast_expr.as_keyword = as_keyword;
	res->ast.cast_expr.type_specifier = type_specifier;
	return res;
}

static ast_t *malloc_ast_mul_expr(ast_t *left, token_t *op, ast_t *right) {
	ast_t *res = malloc_ast(AST_MUL_EXPR, left->filepath, left->source,
		left->start, right->end);
	res->ast.mul_expr.left = left;
	res->ast.mul_expr.op = op;
	res->ast.mul_expr.right = right;
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

static ast_t *malloc_ast_equal_expr(ast_t *left, token_t *op, ast_t *right) {
	ast_t *res = malloc_ast(AST_EQUAL_EXPR, left->filepath, left->source,
		left->start, right->end);
	res->ast.equal_expr.left = left;
	res->ast.equal_expr.op = op;
	res->ast.equal_expr.right = right;
	return res;
}

static ast_t *prog(parser_t *parser) {
	ast_t *ast = malloc_ast_prog();
	while (!check(parser, 0, TOKEN_EOF)) {
		ast_t *d = decl(parser);
		vec_append(&ast->ast.prog.decls, d);
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

static ast_t *decl(parser_t *parser) {
	return fn_decl(parser);
}

static ast_t *fn_decl(parser_t *parser) {
	// vector of token_t*
	vec_t params;
	vec_init(&params);

	// vector of ast_t*
	vec_t param_types;
	vec_init(&param_types);

	token_t *fn_keyword = match(parser, TOKEN_FN_KEYWORD, 
		"Expected fn keyword");
	token_t *name = match(parser, TOKEN_ID, "Expected name for function");
	token_t *lparen = match(parser, TOKEN_LPAREN, "Expected (");

	while (!check(parser, 0, TOKEN_RPAREN)) {
		token_t *id = match(parser, TOKEN_ID, "Expected param name");
		ast_t *type = type_specifier(parser);

		vec_append(&params, id);
		vec_append(&param_types, type);

		if (!check(parser, 0, TOKEN_COMMA)) break;
		match(parser, TOKEN_COMMA, "Expected ','");
	}

	token_t *rparen = match(parser, TOKEN_RPAREN, "Expected )");
	ast_t *t = type_specifier(parser);
	token_t *extern_keyword = NULL;
	token_t *semicolon = NULL;
	ast_t *s = NULL;;

	if (check(parser, 0, TOKEN_EXTERN_KEYWORD)) {
		extern_keyword = match(parser, TOKEN_EXTERN_KEYWORD, 
			"Expected extern keyword");
		semicolon = match(parser, TOKEN_SEMICOLON, "Expected ';'");
	}
	else {
		s = block_stmt(parser);
	}

	ast_t *res = malloc_ast_fn_decl(fn_keyword, name, lparen, rparen, t, 
		extern_keyword, semicolon, s);
	res->ast.fn_decl.params = params;
	res->ast.fn_decl.param_types = param_types;
	return res;
}

static ast_t *type_specifier(parser_t *parser) {
	token_t *id = match(parser, TOKEN_ID, "Expected type id");
	return malloc_ast_type_specifier(id);
}

static ast_t *stmt(parser_t *parser) {
	if (check(parser, 0, TOKEN_LBRACE)) return block_stmt(parser);
	if (check(parser, 0, TOKEN_VAR_KEYWORD)) return var_stmt(parser);
	if (check(parser, 0, TOKEN_RETURN_KEYWORD)) return return_stmt(parser);
	if (check(parser, 0, TOKEN_IF_KEYWORD)) return if_stmt(parser);
	if (check(parser, 0, TOKEN_WHILE_KEYWORD)) return while_stmt(parser);
	return expr_stmt(parser);
}

static ast_t *block_stmt(parser_t *parser) {
	token_t *lbrace = match(parser, TOKEN_LBRACE, "Expected {");
	ast_t *ast = malloc_ast_block_stmt(lbrace);
	while (!check(parser, 0, TOKEN_RBRACE)) {
		ast_t *s = stmt(parser);
		vec_append(&ast->ast.block_stmt.stmts, s);
	}
	token_t *rbrace = match(parser, TOKEN_RBRACE, "Expected }");

	ast->ast.block_stmt.rbrace = rbrace;
	ast->end = rbrace->end;

	return ast;
}

static ast_t *var_stmt(parser_t *parser) {
	token_t *var_keyword = match(parser, TOKEN_VAR_KEYWORD, 
		"Expected 'var' keyword");
	token_t *name = match(parser, TOKEN_ID, "Expected var name");
	ast_t *t = NULL;
	ast_t *e = NULL;

	if (!check(parser, 0, TOKEN_SEMICOLON) && 
		!check(parser, 0, TOKEN_EQUAL)) {
		t = type_specifier(parser);
	}

	if (check(parser, 0, TOKEN_EQUAL)) {
		skip(parser, 1);
		e = expr(parser);
	}

	token_t *semicolon = match(parser, TOKEN_SEMICOLON, 
		"Expected ';' at the end of var statement");

	return malloc_ast_var_stmt(var_keyword, name, t, e, semicolon);
}	

static ast_t *return_stmt(parser_t *parser) {
	token_t *return_keyword = match(parser, TOKEN_RETURN_KEYWORD,
		"Expected 'return' keyword");
	ast_t *e = NULL;
	if (!check(parser, 0, TOKEN_SEMICOLON)) {
		e = expr(parser);
	}
	token_t *semicolon = match(parser, TOKEN_SEMICOLON, 
		"Expected ';' at the end of return statement");
	return malloc_ast_return_stmt(return_keyword, e, semicolon);
}

static ast_t *if_stmt(parser_t *parser) {
	token_t *if_keyword = match(parser, TOKEN_IF_KEYWORD, 
		"Expected 'if' keyword");
	token_t *lparen = match(parser, TOKEN_LPAREN, 
		"Expected '(' after if keyword");
	ast_t *e = expr(parser);
	token_t *rparen = match(parser, TOKEN_RPAREN,
		"Expected ')' at the end of if condition");
	ast_t *true_stmt = stmt(parser);

	token_t *else_keyword = NULL;
	ast_t *false_stmt = NULL;

	if (check(parser, 0, TOKEN_ELSE_KEYWORD)) {
		else_keyword = match(parser, TOKEN_ELSE_KEYWORD,
			"Expected 'else'");
		false_stmt = stmt(parser);
	}

	return malloc_ast_if_stmt(if_keyword, lparen, e, rparen, true_stmt,
		else_keyword, false_stmt);
}

static ast_t *while_stmt(parser_t *parser) {
	token_t *while_keyword = match(parser, TOKEN_WHILE_KEYWORD,
		"Expected while keyword");
	token_t *lparen = match(parser, TOKEN_LPAREN, 
		"Expected '(' after while keyword");
	ast_t *e = expr(parser);
	token_t *rparen = match(parser, TOKEN_RPAREN,
		"Expected ')' after while condition");
	ast_t *true_stmt = stmt(parser);

	return malloc_ast_while_stmt(while_keyword, lparen, e, rparen, 
		true_stmt);
}

static ast_t *expr_stmt(parser_t *parser) {
	ast_t *e = expr(parser);
	token_t *semicolon = match(parser, TOKEN_SEMICOLON, "Expected ;");
	return malloc_ast_expr_stmt(e, semicolon);
}

static ast_t *expr(parser_t *parser) {
	return equal_expr(parser);
}

static ast_t *literal_expr(parser_t *parser) {
	token_t *token = match(parser, TOKEN_INT_LITERAL, 
		"Expected int literal");
	return malloc_ast_literal_expr(token);
}

static ast_t *var_expr(parser_t *parser) {
	token_t *token = match(parser, TOKEN_ID, "Expected var literal");
	return malloc_ast_var_expr(token);
}

static ast_t *primary_expr(parser_t *parser) {
	if (check(parser, 0, TOKEN_INT_LITERAL)) return literal_expr(parser);
	if (check(parser, 0, TOKEN_ID)) return var_expr(parser);
	
	token_t *tok = token_at(parser, 0);
	eprintf(tok->filepath, tok->source, tok->start, tok->end,
		"What is this primary expr?");
	exit(1);
}

static ast_t *postfix_expr(parser_t *parser) {
	ast_t *left = primary_expr(parser);
	while (check(parser, 0, TOKEN_LPAREN)) {
		if (check(parser, 0, TOKEN_LPAREN)) 
			left = call_expr(parser, left);
	}

	return left;
}

static ast_t *call_expr(parser_t *parser, ast_t *left) {
	// vector of ast_t*
	vec_t args;
	vec_init(&args);

	token_t *lparen = match(parser, TOKEN_LPAREN, "Expected '('");
	
	while (!check(parser, 0, TOKEN_RPAREN)) {
		ast_t *arg = expr(parser);

		vec_append(&args, arg);
		
		if (!check(parser, 0, TOKEN_COMMA)) break;
		match(parser, TOKEN_COMMA, "Expected ','");
	}

	token_t *rparen = match(parser, TOKEN_RPAREN, "Expected ')'");

	ast_t *res = malloc_ast_call_expr(left, lparen, rparen);
	res->ast.call_expr.args = args;
	return res;
}

static ast_t *cast_expr(parser_t *parser) {
	ast_t *left = postfix_expr(parser);
	if (check(parser, 0, TOKEN_AS_KEYWORD)) {
		token_t *as_keyword = token_at(parser, 0);
		skip(parser, 1);
		ast_t *t = type_specifier(parser);
		left = malloc_ast_cast_expr(left, as_keyword, t);
	}
	return left;
}

static ast_t *mul_expr(parser_t *parser) {
	ast_t *left = cast_expr(parser);
	while (check(parser, 0, TOKEN_STAR) || check(parser, 0, TOKEN_FSLASH) || 
		check(parser, 0, TOKEN_MOD)) {
		token_t *op = token_at(parser, 0);
		skip(parser, 1);
		ast_t *right = cast_expr(parser);
		left = malloc_ast_mul_expr(left, op, right);
	}
	return left;
}

static ast_t *add_expr(parser_t *parser) {
	ast_t *left = mul_expr(parser);
	while (check(parser, 0, TOKEN_PLUS) || check(parser, 0, TOKEN_MINUS)) {
		token_t *op = token_at(parser, 0);
		skip(parser, 1);
		ast_t *right = mul_expr(parser);
		left = malloc_ast_add_expr(left, op, right);
	}
	return left;
}

static ast_t *equal_expr(parser_t *parser) {
	ast_t *left = add_expr(parser);
	while (check(parser, 0, TOKEN_EQUAL_EQUAL) || 
		check(parser, 0, TOKEN_BANG_EQUAL)) {
		token_t *op = token_at(parser, 0);
		skip(parser, 1);
		ast_t *right = add_expr(parser);
		left = malloc_ast_equal_expr(left, op, right);
	}
	return left;
}

