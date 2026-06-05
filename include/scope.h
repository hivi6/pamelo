#ifndef SCOPE_H
#define SCOPE_H

#include "type.h"
#include "symbol.h"

typedef struct scope_t scope_t;
struct scope_t {
	scope_t *parent_scope;

	// vector of type_t*
	vec_t types;

	// vector of symbol_t*
	vec_t symbols;
};

scope_t *get_global_scope();
scope_t *create_scope(scope_t *parent_scope);
int add_type(scope_t *scope, type_t *type);
type_t *get_type(scope_t *scope, const char *name);
type_t *get_type_in_chain(scope_t *scope, const char *name);
int add_symbol(scope_t *scope, symbol_t *symbol);
symbol_t *get_symbol(scope_t *scope, const char *name);
symbol_t *get_symbol_in_chain(scope_t *scope, const char *name);
vec_t get_scope_list();
void print_scope();

#endif /* SCOPE_H */

