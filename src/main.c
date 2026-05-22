#include "common.h"
#include "pos.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

static void usage(FILE *f);
static char *read_file(const char *filepath);

// ========================================
// main
// ========================================

int main(int argc, const char **argv) {
	if (argc == 1) {
		usage(stderr);
		return 1;
	}

	const char *filepath = argv[1];
	const char *source = read_file(filepath);
	printf("FILEPATH: %s\n\n%s\n", filepath, source);

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
		"    --help, -h    This screen\n"
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

