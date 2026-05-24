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
		g_global_declaration = calloc(sizeof(scope_t), 1);
	}
	return g_global_declaration;
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

