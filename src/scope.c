#include "scope.h"
#include "common.h"

// ========================================
// helper declaration
// ========================================

static scope_t *g_global_declaration = NULL;
static scope_t **g_scope_list = NULL;
static int g_scope_list_len = 0;

static void append_scope(scope_t ***list, int *len, scope_t *scope);

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
	append_scope(&g_scope_list, &g_scope_list_len, scope);
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
	append_symbol(&scope->symbols, &scope->symbols_len, symbol);
	return 1;
}

symbol_t *get_symbol(scope_t *scope, const char *name) {
	for (int i = 0; i < scope->symbols_len; i++) {
		if (strcmp(scope->symbols[i]->name, name) == 0) {
			return scope->symbols[i];
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

void get_scope_list(scope_t ***scope_list, int *scope_list_len) {
	*scope_list = g_scope_list;
	*scope_list_len = g_scope_list_len;
}

void print_scope() {
	scope_t **scope_list = NULL;
	int scope_list_len = 0;
	get_scope_list(&scope_list, &scope_list_len);
	for (int i = 0; i < scope_list_len; i++) {
		printf("id: %p\n", scope_list[i]);
		printf("parent-id: %p\n", scope_list[i]->parent_scope);
		printf("types:\n");
		for (int j = 0; j < scope_list[i]->types.len; j++) {
			char *name = type_str(scope_list[i]->types.elems[j]);
			printf("    %d. %s\n", j+1, name);
			free(name);
		}
		printf("symbols:\n");
		for (int j = 0; j < scope_list[i]->symbols_len; j++) {
			char *name = symbol_str(scope_list[i]->symbols[j]);
			printf("    %d. %s (%d)\n", j+1, name, 
				scope_list[i]->symbols[j]->id);
			free(name);
		}
		printf("\n");
	}
}

// ========================================
// helper definition
// ========================================

static void append_scope(scope_t ***list, int *len, scope_t *scope) {
	*len += 1;
	*list = realloc(*list, *len * sizeof(scope_t*));
	(*list)[*len-1] = scope;
}

