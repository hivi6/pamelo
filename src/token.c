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

// ========================================
// token.h - definition
// ========================================

token_t *generate_tokens(const char *filepath, const char *source) {
	init(filepath, source);
	while (!eof()) {
		generate_token();
	}
	append_token(TOKEN_EOF);
	return g_head;
}

char *token_type(token_t token) {
	char *res = NULL;

	sbuilder_t s;
	sbuilder_init(&s);

	if (token.kind == TOKEN_LBRACE) 
		sbuilder_appendf(&s, "LBRACE");
	else if (token.kind == TOKEN_LPAREN) 
		sbuilder_appendf(&s, "LPAREN");
	else if (token.kind == TOKEN_RBRACE) 
		sbuilder_appendf(&s, "RBRACE");
	else if (token.kind == TOKEN_RPAREN) 
		sbuilder_appendf(&s, "RPAREN");
	else if (token.kind == TOKEN_SEMICOLON) 
		sbuilder_appendf(&s, "SEMICOLON");
	else if (token.kind == TOKEN_INT_LITERAL) 
		sbuilder_appendf(&s, "INT_LITERAL");
	else if (token.kind == TOKEN_FN_KEYWORD) 
		sbuilder_appendf(&s, "FN_KEYWORD");

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

	if (kind == TOKEN_EOF) {
		printf("Invalid token!\n");
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

