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
	for (int i = 0; i < scope->types_len; i++) {
		if (strcmp(scope->types[i]->name, type->name) == 0) {
			return 0; // fail; type already exists in the scope
		}
	}
	append_type(&scope->types, &scope->types_len, type);
	return 1; // success
}

type_t *get_type(scope_t *scope, const char *name) {
	for (int i = 0; i < scope->types_len; i++) {
		if (strcmp(scope->types[i]->name, name) == 0) {
			return scope->types[i];
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

void get_scope_list(scope_t ***scope_list, int *scope_list_len) {
	*scope_list = g_scope_list;
	*scope_list_len = g_scope_list_len;
}

// ========================================
// helper definition
// ========================================

static void append_scope(scope_t ***list, int *len, scope_t *scope) {
	*len += 1;
	*list = realloc(*list, *len * sizeof(scope_t*));
	(*list)[*len-1] = scope;
}

