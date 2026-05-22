#include "util.h"
#include "common.h"

// ========================================
// util.h - definition
// ========================================

// ++++++++++++++++++++++++++++++++++++++++ string builder

void sbuilder_init(sbuilder_t *self) {
	self->elems = NULL;
	self->len = self->cap = 0;
}

void sbuilder_reserve(sbuilder_t *self, int new_cap) {
	if (new_cap <= self->cap) return;

	self->elems = realloc(self->elems, new_cap * sizeof(char));
	self->cap = new_cap;
}

void sbuilder_appendvf(sbuilder_t *self, const char *format, va_list args) {
	int len = vsnprintf(NULL, 0, format, args);
	sbuilder_reserve(self, self->cap + len + 2);
	vsprintf(self->elems + self->len, format, args);
	self->len += len;
}

void sbuilder_appendf(sbuilder_t *self, const char *format, ...) {
	va_list args;
	va_start(args, format);
	sbuilder_appendvf(self, format, args);
	va_end(args);
}

void sbuilder_build(sbuilder_t *self, char **out) {
	char *res = malloc((self->len + 1) * sizeof(char));
	sprintf(res, "%s", self->elems);
	*out = res;
}

void sbuilder_free(sbuilder_t *self) {
	free(self->elems);
	sbuilder_init(self);
}

// ++++++++++++++++++++++++++++++++++++++++ error printer

void eprintf(const char *filepath, const char *source, pos_t start, pos_t end,
	const char *format, ...) {

	sbuilder_t s;
	sbuilder_init(&s);

	// append the filepath, start and end position
	sbuilder_appendf(&s, "%s:[%d:%d]-[%d:%d]: ", filepath, start.line, 
		start.column, end.line, end.column);

	// append the message
	va_list args;
	va_start(args, format);
	sbuilder_appendvf(&s, format, args);
	va_end(args);

	// append a newline
	sbuilder_appendf(&s, "\n");

	// find the starting index of the start line
	int index = start.index;
	while (index > 0 && source[index-1] != '\n') index--;

	// find the max value of number padding
	int padding_size = 2, end_line = end.line + 1;
	while (end_line) {
		padding_size++;
		end_line /= 10;
	}

	// add the following in the first line of the error
	// add something like below:
	//                 vvvvvvvvvvvvvvvvv
	// This is a line. This is the error
	sbuilder_appendf(&s, "%*c | ", padding_size, ' ');
	int temp = index;
	while (source[temp] && source[temp] != '\n') {
		char ch = ' ';
		if (start.index <= temp && temp < end.index) ch = 'v';
		sbuilder_appendf(&s, "%c", ch);
		temp++;
	}
	sbuilder_appendf(&s, "\n");

	// print each line in the source code
	temp = index;
	for (int line = start.line; line <= end.line; line++) {
		sbuilder_appendf(&s, "%*d > ", padding_size, line);
		while (source[temp] && source[temp] != '\n') {
			sbuilder_appendf(&s, "%c", source[temp]);
			temp++;
		}
		sbuilder_appendf(&s, "\n");
		if (!source[temp]) break;
		temp++;
	}
	sbuilder_appendf(&s, "%*c | \n", padding_size, ' ');

	char *res = NULL;
	sbuilder_build(&s, &res);
	
	fprintf(stderr, "%s", res);

	sbuilder_free(&s);
	free(res);
}

