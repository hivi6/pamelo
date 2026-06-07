#ifndef AST_H
#define AST_H

#include "token.h"
#include "scope.h"

enum {
	AST_PROG,

	AST_FN_DECL,

	AST_TYPE_SPECIFIER,

	AST_BLOCK_STMT,
	AST_VAR_STMT,
	AST_RETURN_STMT,
	AST_IF_STMT,
	AST_WHILE_STMT,
	AST_EXPR_STMT,

	AST_LITERAL_EXPR,
	AST_VAR_EXPR,
	AST_CALL_EXPR,
	AST_CAST_EXPR,
	AST_MUL_EXPR,
	AST_ADD_EXPR,
	AST_EQUAL_EXPR,
};

typedef struct ast_t ast_t;
struct ast_t {
	int kind;
	const char *filepath;
	const char *source;
	pos_t start;
	pos_t end;

	scope_t *scope;
	type_t *type;
	symbol_t *symbol; // for var_expr
	int total_id; // for fn_decl

	union {
		struct {
			// vector of ast*
			vec_t decls;
		} prog;

		struct {
			token_t *fn_keyword;
			token_t *name;
			token_t *lparen;

			// vector of token_t*
			vec_t params;

			// vector of ast_t* (kind = AST_TYPE_SPECIFIER)
			vec_t param_types;

			token_t *rparen;
			ast_t *type_specifier;
			
			token_t *extern_keyword;
			token_t *semicolon;

			ast_t *block_stmt;
		} fn_decl;

		struct {
			token_t *name;
		} type_specifier;

		struct {
			token_t *lbrace;

			// vector of ast_t*
			vec_t stmts;

			token_t *rbrace;
		} block_stmt;

		struct {
			token_t *var_keyword;
			token_t *name;
			ast_t *type_specifier;
			ast_t *expr;
			token_t *semicolon;
		} var_stmt;

		struct {
			token_t *return_keyword;
			ast_t *expr;
			token_t *semicolon;
		} return_stmt;

		struct {
			token_t *if_keyword;
			token_t *lparen;
			ast_t *expr;
			token_t *rparen;
			ast_t *true_stmt;

			token_t *else_keyword;
			ast_t *false_stmt;
		} if_stmt;

		struct {
			token_t *while_keyword;
			token_t *lparen;
			ast_t *expr;
			token_t *rparen;
			ast_t *true_stmt;
		} while_stmt;

		struct {
			ast_t *expr;
			token_t *semicolon;
		} expr_stmt;

		struct {
			token_t *token;
		} literal_expr;

		struct {
			token_t *token;
		} var_expr;

		struct {
			ast_t *left;
			token_t *lparen;

			// vector of ast_t*
			vec_t args;

			token_t *rparen;
		} call_expr;

		struct {
			ast_t *left;
			token_t *as_keyword;
			ast_t *type_specifier;
		} cast_expr;

		struct {
			ast_t *left;
			token_t *op;
			ast_t *right;
		} mul_expr;

		struct {
			ast_t *left;
			token_t *op;
			ast_t *right;
		} add_expr;

		struct {
			ast_t *left;
			token_t *op;
			ast_t *right;
		} equal_expr;
	} ast;
};

ast_t *parse(vec_t tokens);
void print_ast(ast_t *ast);

#endif /* AST_H */

