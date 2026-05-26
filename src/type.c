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

char *type_str(type_t *type) {
	sbuilder_t s;
	sbuilder_init(&s);

	if (type->kind == TYPE_VOID) {
		sbuilder_appendf(&s, "void");
	}
	else if (type->kind == TYPE_PRIMITIVE) {
		sbuilder_appendf(&s, type->name);
	}
	else if (type->kind == TYPE_FN) {
		char *return_type = type_str(type->type.fn_type.return_type);
		sbuilder_appendf(&s, "fn %s () -> %s", type->name, return_type);
		free(return_type);
	}

	char *res = NULL;
	sbuilder_build(&s, &res);
	sbuilder_free(&s);

	return res;
}

