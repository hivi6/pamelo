#include "scope.h"
#include "common.h"

// ========================================
// helper declaration
// ========================================

static scope_t *g_global_declaration = NULL;

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

