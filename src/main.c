#include "common.h"
#include "pos.h"
#include "util.h"
#include "token.h"
#include "ast.h"
#include "semantic.h"
#include "ir.h"

// ========================================
// helper declaration
// ========================================

static int g_help_flag = 0;
static int g_print_token_flag = 0;
static int g_print_ast_flag = 0;
static int g_print_scope_flag = 0;

static void usage(FILE *f);
static char *read_file(const char *filepath);
static int get_opts(int argc, const char **argv);

// ========================================
// main
// ========================================

int main(int argc, const char **argv) {
	int index = get_opts(argc, argv);
	if (g_help_flag) {
		usage(stdout);
		return 0;
	}

	if (index >= argc) {
		usage(stderr);
		return 1;
	}

	const char *filepath = argv[index];
	const char *source = read_file(filepath);
	token_t *tokens = generate_tokens(filepath, source);
	ast_t *ast = parse(tokens);
	semantic_analyse(ast);

	ir_fn_t **ir_list = NULL;
	int ir_list_len = 0;
	generate_ir(ast, &ir_list, &ir_list_len);

	if (g_print_token_flag) {
		printf("file: %s\n", filepath);
		for (token_t *head = tokens; head; head = head->next) {
			char *lexical = token_lexical(*head);
			char *type = token_type(*head);
			printf("%s(%s)\n", type, lexical);
		}
		printf("\n");
	}

	if (g_print_ast_flag) {
		printf("file: %s\n", filepath);
		print_ast(ast);
		printf("\n");
	}

	if (g_print_scope_flag) {
		scope_t **scope_list = NULL;
		int scope_list_len = 0;
		get_scope_list(&scope_list, &scope_list_len);
		for (int i = 0; i < scope_list_len; i++) {
			printf("id: %p\n", scope_list[i]);
			printf("parent-id: %p\n", scope_list[i]->parent_scope);
			printf("types:\n");
			for (int j = 0; j < scope_list[i]->types_len; j++) {
				char *name = type_str(scope_list[i]->types[j]);
				printf("    %d. %s\n", j+1, name);
				free(name);
			}
			printf("symbols:\n");
			for (int j = 0; j < scope_list[i]->symbols_len; j++) {
				char *name = symbol_str(scope_list[i]->symbols[j]);
				printf("    %d. %s\n", j+1, name);
				free(name);
			}
			printf("\n");
		}
	}

	return 0;
}

// ========================================
// helper definition
// ========================================

static void usage(FILE *f) {
	fprintf(f, 
		"USAGE: pamelo [OPTIONS] <file>\n"
		"\n"
		"ABOUT:\n"
		"    A Programming Language\n"
		"\n"
		"OPTIONS:\n"
		"    --help, -h       This screen\n"
		"    --print-token    Print the token to the screen\n"
		"    --print-ast      Print the ast to the screen\n"
		"    --print-scope    Print the scope info to the screen\n"
		"\n"
		"HINTS:\n"
		"    1. If you want to read from stdin, then make filepath == '-'\n"
		"       echo hello | orange -; this should read hello from the stdin\n"
		"\n"
	);
}

static char *read_file(const char *filepath) {
	char *res = NULL;
	sbuilder_t s;
	sbuilder_init(&s);

	FILE *f = stdin;
	if (strcmp(filepath, "-") != 0) f = fopen(filepath, "r");
	if (f == NULL) {
		perror(filepath);
		exit(1);
	}

	char ch = 0;
	while ((ch = fgetc(f)) != EOF) {
		sbuilder_appendf(&s, "%c", ch);
	}

	sbuilder_build(&s, &res);
	sbuilder_free(&s);

	if (strcmp(filepath, "-") != 0) fclose(f);

	return res;
}

static int get_opts(int argc, const char **argv) {
	int i = 1;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0 
			|| strcmp(argv[i], "-h") == 0) 
			g_help_flag = 1;
		else if (strcmp(argv[i], "--print-token") == 0)
			g_print_token_flag = 1;
		else if (strcmp(argv[i], "--print-ast") == 0)
			g_print_ast_flag = 1;
		else if (strcmp(argv[i], "--print-scope") == 0)
			g_print_scope_flag = 1;
		else
			break;
	}
	return i;
}

