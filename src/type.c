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

char *type_str(type_t *type) {
	if (type == NULL) return NULL;

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
		sbuilder_appendf(&s, "fn %s (", type->name);

		for (int i = 0; i < type->type.fn_type.param_types.len; i++) {
			char *param = type_str(
				type->type.fn_type.param_types.elems[i]);
			sbuilder_appendf(&s, "%s", param);
			free(param);

			if (i < type->type.fn_type.param_types.len-1)
				sbuilder_appendf(&s, ", ");
		}

		sbuilder_appendf(&s, ") -> %s", return_type);
		free(return_type);
	}

	char *res = NULL;
	sbuilder_build(&s, &res);
	sbuilder_free(&s);

	return res;
}

