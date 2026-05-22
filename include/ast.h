#ifndef AST_H
#define AST_H

#include "token.h"

enum {
	AST_FN_DECL,

	AST_TYPE_SPECIFIER,

	AST_BLOCK_STMT,
	AST_EXPR_STMT,
	
	AST_LITERAL_EXPR,
};

typedef struct ast_t ast_t;
struct ast_t {
	int kind;
	const char *filepath;
	const char *source;
	pos_t start;
	pos_t end;

	union {
		struct {
			token_t *fn_keyword;
			token_t *name;
			token_t *lparen;
			token_t *rparen;
			ast_t *type_specifier;
			ast_t *block_stmt;
		} fn_decl;

		struct {
			token_t *name;
		} type_specifier;

		struct {
			token_t *lbrace;

			ast_t **stmts;
			int stmts_len;

			token_t *rbrace;
		} block_stmt;

		struct {
			ast_t *expr;
			token_t *semicolon;
		} expr_stmt;

		struct {
			token_t *token;
		} literal_expr;
	} ast;
};

ast_t *parse(token_t *tokens);
void append_ast(ast_t ***list, int *len, ast_t *ast);
void print_ast(ast_t *ast);

#endif /* AST_H */

