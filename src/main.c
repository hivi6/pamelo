#include "common.h"
#include "pos.h"

int main() {
	pos_t start = POS_INIT();

	printf("Hello, World! start: %d | line: %d | column: %d\n", 
		start.index, start.line, start.column);
	return 0;
}

