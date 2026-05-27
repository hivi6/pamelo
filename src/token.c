#include "token.h"
#include "common.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static const char *g_filepath;
static const char *g_source;
static int g_source_length;
static token_t *g_head;
static token_t *g_tail;
static pos_t g_prev;
static pos_t g_cur;

static void init(const char *filepath, const char *source);
static char eof();
static void generate_token();
static void append_token(int kind);
static char char_at(int offset);
static char is_whitespace(char ch);
static void char_skip(int skip);
static char is_octal(char ch);
static char is_hexadecimal(char ch);
static int int_literal_skip();
static int keyword_skip();

// ========================================
// token.h - definition
// ========================================

token_t *generate_tokens(const char *filepath, const char *source) {
	init(filepath, source);
	while (!eof()) {
		generate_token();
	}
	g_prev = g_cur;
	append_token(TOKEN_EOF);
	return g_head;
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

// ========================================
// helper declaration
// ========================================

static void init(const char *filepath, const char *source) {
	g_filepath = filepath;
	g_source = source;
	g_source_length = strlen(source);
	g_head = g_tail = NULL;
	g_prev = g_cur = POS_INIT();
}

static char eof() {
	return g_cur.index >= g_source_length;
}

static void generate_token() {
	g_prev = g_cur;

	if (is_whitespace(char_at(0))) {
		char_skip(1);
		return;
	}

	int skip = 1;
	int kind = TOKEN_EOF;
	if (char_at(0) == '{') kind = TOKEN_LBRACE;
	else if (char_at(0) == '(') kind = TOKEN_LPAREN;
	else if (char_at(0) == '}') kind = TOKEN_RBRACE;
	else if (char_at(0) == ')') kind = TOKEN_RPAREN;
	else if (char_at(0) == ';') kind = TOKEN_SEMICOLON;
	else if (char_at(0) == '+') kind = TOKEN_PLUS;
	else if (char_at(0) == '-') kind = TOKEN_MINUS;
	else if (char_at(0) == '=') kind = TOKEN_EQUAL;
	else if (isdigit(char_at(0))) {
		kind = int_literal_skip();
		skip = 0;
	}
	else if (isalpha(char_at(0)) || char_at(0) == '_') {
		kind = keyword_skip();
		skip = 0;
	}

	if (kind == TOKEN_EOF) {
		char_skip(1);
		eprintf(g_filepath, g_source, g_prev, g_cur, "Invalid token!");
		exit(1);
	}

	char_skip(skip);
	append_token(kind);
}

static void append_token(int kind) {
	token_t *res = calloc(sizeof(token_t), 1);
	res->kind = kind;
	res->filepath = g_filepath;
	res->source = g_source;
	res->start = g_prev;
	res->end = g_cur;

	if (g_head == NULL) g_head = res;
	else g_tail->next = res;
	g_tail = res;
}

static char char_at(int offset) {
	if (offset + g_cur.index >= g_source_length) return 0;
	return g_source[g_cur.index + offset];
}

static char is_whitespace(char ch) {
	return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
}

static void char_skip(int skip) {
	for (int i = 0; i < skip; i++) {
		char ch = char_at(0);
		if (!ch) break;
		
		g_cur.index++;
		g_cur.column++;
		if (ch == '\n') {
			g_cur.line++;
			g_cur.column = 1;
		}
	}
}

static char is_octal(char ch) {
	return '0' <= ch && ch <= '7';
}

static char is_hexadecimal(char ch) {
	return isdigit(ch) || ('a' <= ch && ch <= 'f') 
		|| ('A' <= ch && ch <= 'F');
}

static int int_literal_skip() {
	int incomplete = 0;
	if (char_at(0) == '0' && tolower(char_at(1)) == 'x') {
		char_skip(2);
		int err = 1;
		while (is_hexadecimal(char_at(0))) {
			err = 0;
			char_skip(1);
		}
		incomplete = err;
	}
	else if (char_at(0) == '0' && tolower(char_at(1)) == 'b') {
		char_skip(2);
		int err = 1;
		while (char_at(0) == '0' || char_at(0) == '1') {
			err = 0;
			char_skip(1);
		}
		incomplete = err;
	}
	else if (char_at(0) == '0' && is_octal(char_at(1))) {
		char_skip(2);
		while (is_octal(char_at(0)))
			char_skip(1);
	}
	else if (char_at(0) == '0') {
		char_skip(1);
	}
	else {
		while (isdigit(char_at(0)))
			char_skip(1);
	}

	int invalid = 0;
	while (isalnum(char_at(0)) || char_at(0) == '_') {
		invalid = 1;
		char_skip(1);
	}

	if (invalid || incomplete) {
		eprintf(g_filepath, g_source, g_prev, g_cur,
			"Invalid int literal");
		exit(1);
	}

	return TOKEN_INT_LITERAL;
}

static int keyword_skip() {
	sbuilder_t s;
	sbuilder_init(&s);

	while (isalnum(char_at(0)) || char_at(0) == '_') {
		sbuilder_appendf(&s, "%c", char_at(0));
		char_skip(1);
	}

	char *res = NULL;
	sbuilder_build(&s, &res);
	sbuilder_free(&s);

	int kind = TOKEN_ID;
	if (strcmp(res, "fn") == 0) kind = TOKEN_FN_KEYWORD;
	if (strcmp(res, "as") == 0) kind = TOKEN_AS_KEYWORD;
	if (strcmp(res, "var") == 0) kind = TOKEN_VAR_KEYWORD;
	if (strcmp(res, "return") == 0) kind = TOKEN_RETURN_KEYWORD;

	free(res);

	return kind;
}

