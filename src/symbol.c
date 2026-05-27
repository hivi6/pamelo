#include "symbol.h"
#include "util.h"

// ========================================
// symbol.h - definition
// ========================================

symbol_t *create_symbol(const char *name, type_t *type) {
	symbol_t *res = calloc(sizeof(symbol_t), 1);
	res->name = sbuildf("%s", name);
	res->type = type;
	return res;
}

void append_symbol(symbol_t ***list, int *len, symbol_t *symbol) {
	*len += 1;
	*list = realloc(*list, *len * sizeof(symbol_t*));
	(*list)[*len-1] = symbol;
}

char *symbol_str(symbol_t *symbol) {
	char *type_info = type_str(symbol->type);
	char *res = sbuildf("name: %s | type: %s", symbol->name, type_info);
	free(type_info);
	return res;
}

