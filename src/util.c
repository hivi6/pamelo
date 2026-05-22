#include "util.h"
#include "common.h"

// ========================================
// util.h - definition
// ========================================

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

