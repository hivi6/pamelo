#include "common.h"
#include "pos.h"

// ========================================
// helper declaration
// ========================================

static void usage(FILE *f);

// ========================================
// main
// ========================================

int main(int argc, const char **argv) {
	if (argc == 1) {
		usage(stderr);
		return 1;
	}

	pos_t start = POS_INIT();

	printf("Hello, World! start: %d | line: %d | column: %d\n", 
		start.index, start.line, start.column);
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

