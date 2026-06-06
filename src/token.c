#include "token.h"
#include "common.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

typedef struct lexer_t lexer_t;
struct lexer_t {
	const char *filepath;
	const char *source;
	int source_length;
	pos_t prev;
	pos_t cur;
	vec_t tokens;
};

static void init(lexer_t *lexer, const char *filepath, const char *source);
static char eof(lexer_t *lexer);
static void generate_token(lexer_t *lexer);
static void append_token(lexer_t *lexer, int kind);
static char char_at(lexer_t *lexer, int offset);
static void char_skip(lexer_t *lexer, int skip);
static int int_literal_skip(lexer_t *lexer);
static int keyword_skip(lexer_t *lexer);

static char is_whitespace(char ch);
static char is_octal(char ch);
static char is_hexadecimal(char ch);

// ========================================
// token.h - definition
// ========================================

vec_t generate_tokens(const char *filepath, const char *source) {
	lexer_t lexer = {0};
	init(&lexer, filepath, source);
	while (!eof(&lexer)) {
		generate_token(&lexer);
	}
	lexer.prev = lexer.cur;
	append_token(&lexer, TOKEN_EOF);
	return lexer.tokens;
}

char *token_type(token_t token) {
	char *res = NULL;

	sbuilder_t s;
	sbuilder_init(&s);

	if (token.kind == TOKEN_EOF) 
		sbuilder_appendf(&s, "EOF");
	else if (token.kind == TOKEN_LBRACE) 
		sbuilder_appendf(&s, "LBRACE");
	else if (token.kind == TOKEN_LPAREN) 
		sbuilder_appendf(&s, "LPAREN");
	else if (token.kind == TOKEN_RBRACE) 
		sbuilder_appendf(&s, "RBRACE");
	else if (token.kind == TOKEN_RPAREN) 
		sbuilder_appendf(&s, "RPAREN");
	else if (token.kind == TOKEN_SEMICOLON) 
		sbuilder_appendf(&s, "SEMICOLON");
	else if (token.kind == TOKEN_EQUAL) 
		sbuilder_appendf(&s, "EQUAL");
	else if (token.kind == TOKEN_PLUS)
		sbuilder_appendf(&s, "PLUS");
	else if (token.kind == TOKEN_MINUS)
		sbuilder_appendf(&s, "MINUS");
	else if (token.kind == TOKEN_COMMA)
		sbuilder_appendf(&s, "COMMA");
	else if (token.kind == TOKEN_STAR)
		sbuilder_appendf(&s, "STAR");
	else if (token.kind == TOKEN_FSLASH)
		sbuilder_appendf(&s, "FSLASH");
	else if (token.kind == TOKEN_MOD)
		sbuilder_appendf(&s, "MOD");
	else if (token.kind == TOKEN_INT_LITERAL) 
		sbuilder_appendf(&s, "INT_LITERAL");
	else if (token.kind == TOKEN_ID)
		sbuilder_appendf(&s, "ID");
	else if (token.kind == TOKEN_FN_KEYWORD) 
		sbuilder_appendf(&s, "FN_KEYWORD");
	else if (token.kind == TOKEN_AS_KEYWORD) 
		sbuilder_appendf(&s, "AS_KEYWORD");
	else if (token.kind == TOKEN_VAR_KEYWORD) 
		sbuilder_appendf(&s, "VAR_KEYWORD");
	else if (token.kind == TOKEN_RETURN_KEYWORD) 
		sbuilder_appendf(&s, "RETURN_KEYWORD");
	else if (token.kind == TOKEN_EXTERN_KEYWORD)
		sbuilder_appendf(&s, "EXTERN_KEYWORD");
	else if (token.kind == TOKEN_IF_KEYWORD)
		sbuilder_appendf(&s, "IF_KEYWORD");
	else if (token.kind == TOKEN_ELSE_KEYWORD)
		sbuilder_appendf(&s, "ELSE_KEYWORD");
	else {
		eprintf(token.filepath, token.source, token.start, token.end,
			"What is this token type?");
		exit(1);
	}

	sbuilder_build(&s, &res);
	sbuilder_free(&s);

	return res;
}

char *token_lexical(token_t token) {
	char *res = NULL;
	
	sbuilder_t s;
	sbuilder_init(&s);

	for (int i = token.start.index; i < token.end.index; i++) {
		sbuilder_appendf(&s, "%c", token.source[i]);
	}

	sbuilder_build(&s, &res);
	sbuilder_free(&s);

	return res;
}

void print_tokens(vec_t tokens) {
	assert(tokens.len > 0);

	const char *filepath = ((token_t*) tokens.elems[0])->filepath;
	printf("file: %s\n", filepath);
	for (int index = 0; index < tokens.len; index++) {
		token_t *head = tokens.elems[index];
		char *lexical = token_lexical(*head);
		char *type = token_type(*head);
		printf("%s(%s)\n", type, lexical);
	}
	printf("\n");
}

// ========================================
// helper declaration
// ========================================

static void init(lexer_t *lexer, const char *filepath, const char *source) {
	lexer->filepath = filepath;
	lexer->source = source;
	lexer->source_length = strlen(source);
	lexer->prev = lexer->cur = POS_INIT();
	vec_init(&lexer->tokens);
}

static char eof(lexer_t *lexer) {
	return lexer->cur.index >= lexer->source_length;
}

static void generate_token(lexer_t *lexer) {
	lexer->prev = lexer->cur;

	if (is_whitespace(char_at(lexer, 0))) {
		char_skip(lexer, 1);
		return;
	}

	int skip = 1;
	int kind = TOKEN_EOF;
	if (char_at(lexer, 0) == '{') kind = TOKEN_LBRACE;
	else if (char_at(lexer, 0) == '(') kind = TOKEN_LPAREN;
	else if (char_at(lexer, 0) == '}') kind = TOKEN_RBRACE;
	else if (char_at(lexer, 0) == ')') kind = TOKEN_RPAREN;
	else if (char_at(lexer, 0) == ';') kind = TOKEN_SEMICOLON;
	else if (char_at(lexer, 0) == '+') kind = TOKEN_PLUS;
	else if (char_at(lexer, 0) == '-') kind = TOKEN_MINUS;
	else if (char_at(lexer, 0) == '=') kind = TOKEN_EQUAL;
	else if (char_at(lexer, 0) == ',') kind = TOKEN_COMMA;
	else if (char_at(lexer, 0) == '*') kind = TOKEN_STAR;
	else if (char_at(lexer, 0) == '/') kind = TOKEN_FSLASH;
	else if (char_at(lexer, 0) == '%') kind = TOKEN_MOD;
	else if (isdigit(char_at(lexer, 0))) {
		kind = int_literal_skip(lexer);
		skip = 0;
	}
	else if (isalpha(char_at(lexer, 0)) || char_at(lexer, 0) == '_') {
		kind = keyword_skip(lexer);
		skip = 0;
	}

	if (kind == TOKEN_EOF) {
		char_skip(lexer, 1);
		eprintf(lexer->filepath, lexer->source, lexer->prev, 
			lexer->cur, "Invalid token!");
		exit(1);
	}

	char_skip(lexer, skip);
	append_token(lexer, kind);
}

static void append_token(lexer_t *lexer, int kind) {
	token_t *res = calloc(sizeof(token_t), 1);
	res->kind = kind;
	res->filepath = lexer->filepath;
	res->source = lexer->source;
	res->start = lexer->prev;
	res->end = lexer->cur;

	vec_append(&lexer->tokens, res);
}

static char char_at(lexer_t *lexer, int offset) {
	if (offset + lexer->cur.index >= lexer->source_length) return 0;
	return lexer->source[lexer->cur.index + offset];
}

static void char_skip(lexer_t *lexer, int skip) {
	for (int i = 0; i < skip; i++) {
		char ch = char_at(lexer, 0);
		if (!ch) break;
		
		lexer->cur.index++;
		lexer->cur.column++;
		if (ch == '\n') {
			lexer->cur.line++;
			lexer->cur.column = 1;
		}
	}
}

static int int_literal_skip(lexer_t *lexer) {
	int incomplete = 0;
	if (char_at(lexer, 0) == '0' && tolower(char_at(lexer, 1)) == 'x') {
		char_skip(lexer, 2);
		int err = 1;
		while (is_hexadecimal(char_at(lexer, 0))) {
			err = 0;
			char_skip(lexer, 1);
		}
		incomplete = err;
	}
	else if (char_at(lexer, 0) == '0' && 
		tolower(char_at(lexer, 1)) == 'b') {
		char_skip(lexer, 2);
		int err = 1;
		while (char_at(lexer, 0) == '0' || char_at(lexer, 0) == '1') {
			err = 0;
			char_skip(lexer, 1);
		}
		incomplete = err;
	}
	else if (char_at(lexer, 0) == '0' && 
		is_octal(char_at(lexer, 1))) {
		char_skip(lexer, 2);
		while (is_octal(char_at(lexer, 0)))
			char_skip(lexer, 1);
	}
	else if (char_at(lexer, 0) == '0') {
		char_skip(lexer, 1);
	}
	else {
		while (isdigit(char_at(lexer, 0)))
			char_skip(lexer, 1);
	}

	int invalid = 0;
	while (isalnum(char_at(lexer, 0)) || char_at(lexer, 0) == '_') {
		invalid = 1;
		char_skip(lexer, 1);
	}

	if (invalid || incomplete) {
		eprintf(lexer->filepath, lexer->source, lexer->prev, 
			lexer->cur, "Invalid int literal");
		exit(1);
	}

	return TOKEN_INT_LITERAL;
}

static int keyword_skip(lexer_t *lexer) {
	sbuilder_t s;
	sbuilder_init(&s);

	while (isalnum(char_at(lexer, 0)) || char_at(lexer, 0) == '_') {
		sbuilder_appendf(&s, "%c", char_at(lexer, 0));
		char_skip(lexer, 1);
	}

	char *res = NULL;
	sbuilder_build(&s, &res);
	sbuilder_free(&s);

	int kind = TOKEN_ID;
	if (strcmp(res, "fn") == 0) kind = TOKEN_FN_KEYWORD;
	if (strcmp(res, "as") == 0) kind = TOKEN_AS_KEYWORD;
	if (strcmp(res, "var") == 0) kind = TOKEN_VAR_KEYWORD;
	if (strcmp(res, "return") == 0) kind = TOKEN_RETURN_KEYWORD;
	if (strcmp(res, "extern") == 0) kind = TOKEN_EXTERN_KEYWORD;
	if (strcmp(res, "if") == 0) kind = TOKEN_IF_KEYWORD;
	if (strcmp(res, "else") == 0) kind = TOKEN_ELSE_KEYWORD;

	free(res);

	return kind;
}

static char is_whitespace(char ch) {
	return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
}

static char is_octal(char ch) {
	return '0' <= ch && ch <= '7';
}

static char is_hexadecimal(char ch) {
	return isdigit(ch) || ('a' <= ch && ch <= 'f') 
		|| ('A' <= ch && ch <= 'F');
}
