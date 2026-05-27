#ifndef SCOPE_H
#define SCOPE_H

#include "type.h"
#include "symbol.h"

typedef struct scope_t scope_t;
struct scope_t {
	scope_t *parent_scope;

	type_t **types;
	int types_len;

	symbol_t **symbols;
	int symbols_len;
};

scope_t *get_global_scope();
scope_t *create_scope(scope_t *parent_scope);
int add_type(scope_t *scope, type_t *type);
type_t *get_type(scope_t *scope, const char *name);
type_t *get_type_in_chain(scope_t *scope, const char *name);
int add_symbol(scope_t *scope, symbol_t *symbol);
symbol_t *get_symbol(scope_t *scope, const char *name);
symbol_t *get_symbol_in_chain(scope_t *scope, const char *name);
void get_scope_list(scope_t ***scope_list, int *scope_list_len);

#endif /* SCOPE_H */

