#include "scope.h"
#include "common.h"

// ========================================
// helper declaration
// ========================================

static scope_t *g_global_declaration = NULL;
static vec_t g_scope_list = {}; // vector of scope_t*

// ========================================
// scope.h - definition
// ========================================

scope_t *get_global_scope() {
	if (g_global_declaration == NULL) {
		g_global_declaration = create_scope(NULL);
	}
	return g_global_declaration;
}

scope_t *create_scope(scope_t *parent_scope) {
	scope_t *scope = calloc(sizeof(scope_t), 1);
	scope->parent_scope = parent_scope;
	vec_append(&g_scope_list, scope);
	return scope;
}

int add_type(scope_t *scope, type_t *type) {
	if (get_type(scope, type->name)) {
		return 0;
	}
	vec_append(&scope->types, type);
	return 1; // success
}

type_t *get_type(scope_t *scope, const char *name) {
	for (int i = 0; i < scope->types.len; i++) {
		type_t *type = scope->types.elems[i];
		if (strcmp(type->name, name) == 0) {
			return type;
		}
	}
	return NULL;
}

type_t *get_type_in_chain(scope_t *scope, const char *name) {
	for (scope_t *cur = scope; cur; cur = cur->parent_scope) {
		type_t *t = get_type(cur, name);
		if (t) return t;
	}
	return NULL;
}

int add_symbol(scope_t *scope, symbol_t *symbol) {
	if (get_symbol(scope, symbol->name)) {
		return 0;
	}
	vec_append(&scope->symbols, symbol);
	return 1;
}

symbol_t *get_symbol(scope_t *scope, const char *name) {
	for (int i = 0; i < scope->symbols.len; i++) {
		symbol_t *symbol = scope->symbols.elems[i];
		if (strcmp(symbol->name, name) == 0) {
			return symbol;
		}
	}
	return NULL;
}

symbol_t *get_symbol_in_chain(scope_t *scope, const char *name) {
	for (scope_t *cur = scope; cur; cur = cur->parent_scope) {
		symbol_t *s = get_symbol(cur, name);
		if (s) return s;
	}
	return NULL;
}

vec_t get_scope_list() {
	return g_scope_list;
}

void print_scope() {
	vec_t scope_list = {};
	scope_list = get_scope_list();
	for (int i = 0; i < scope_list.len; i++) {
		scope_t *scope = scope_list.elems[i];

		printf("id: %p\n", scope);
		printf("parent-id: %p\n", scope->parent_scope);

		printf("types:\n");
		for (int j = 0; j < scope->types.len; j++) {
			char *name = type_str(scope->types.elems[j]);
			printf("    %d. %s\n", j+1, name);
			free(name);
		}

		printf("symbols:\n");
		for (int j = 0; j < scope->symbols.len; j++) {
			symbol_t *symbol = scope->symbols.elems[j];
			char *name = symbol_str(symbol);
			printf("    %d. %s (%d)\n", j+1, name, symbol->id);
			free(name);
		}
		printf("\n");
	}
}

