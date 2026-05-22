#ifndef UTIL_H
#define UTIL_H

#include "common.h"

typedef struct sbuilder_t sbuilder_t;
struct sbuilder_t {
	char *elems;
	int len;
	int cap;
};

void sbuilder_init(sbuilder_t *self);
void sbuilder_reserve(sbuilder_t *self, int new_cap);
void sbuilder_appendvf(sbuilder_t *self, const char *format, va_list args);
void sbuilder_appendf(sbuilder_t *self, const char *format, ...);
void sbuilder_build(sbuilder_t *self, char **out);
void sbuilder_free(sbuilder_t *self);

#endif /* UTIL_H */

