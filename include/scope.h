#ifndef SCOPE_H
#define SCOPE_H

#include "type.h"

typedef struct scope_t scope_t;
struct scope_t {
	scope_t *parent_scope;

	type_t **types;
	int types_len;
};

scope_t *get_global_scope();
scope_t *create_scope(scope_t *parent_scope);
int add_type(scope_t *scope, type_t *type);
type_t *get_type(scope_t *scope, const char *name);

#endif /* SCOPE_H */

