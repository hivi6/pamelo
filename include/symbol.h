#ifndef SYMBOL_H
#define SYMBOL_H

#include "type.h"

typedef struct symbol_t symbol_t;
struct symbol_t {
	int id;
	const char *name;
	type_t *type;
};

symbol_t *create_symbol(int id, const char *name, type_t *type);
void append_symbol(symbol_t ***list, int *len, symbol_t *symbol);
char *symbol_str(symbol_t *symbol);

#endif /* SYMBOL_H */

