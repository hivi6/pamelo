#include "type.h"
#include "common.h"
#include "util.h"

// ========================================
// type.h - definition
// ========================================

type_t *create_type(int kind, const char *name, int size) {
	type_t *type = calloc(sizeof(type_t), 1);
	type->kind = kind;
	type->name = sbuildf("%s", name);
	type->size = size;
	return type;
}

void append_type(type_t ***list, int *len, type_t *type) {
	*len += 1;
	*list = realloc(*list, *len * sizeof(type_t*));
	(*list)[*len-1] = type;
}

