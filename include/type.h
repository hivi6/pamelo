#ifndef TYPE_H
#define TYPE_H

#include "util.h"

enum {
	TYPE_VOID,
	TYPE_PRIMITIVE,
	TYPE_FN,
	TYPE_POINTER,
};

typedef struct type_t type_t;
struct type_t {
	int kind;
	const char *name;
	int size;

	union {
		struct {
			// vector of type_t*
			vec_t param_types;

			type_t *return_type;
		} fn_type;

		struct {
			type_t *base_type;
		} pointer_type;
	} type;
};

type_t *create_type(int kind, const char *name, int size);
char *type_str(type_t *type);

#endif /* TYPE_H */

