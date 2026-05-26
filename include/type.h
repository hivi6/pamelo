#ifndef TYPE_H
#define TYPE_H

enum {
	TYPE_VOID,
	TYPE_PRIMITIVE,
	TYPE_FN,
};

typedef struct type_t type_t;
struct type_t {
	int kind;
	const char *name;
	int size;

	union {
		struct {
			type_t *return_type;
		} fn_type;
	} type;
};

type_t *create_type(int kind, const char *name, int size);
void append_type(type_t ***list, int *len, type_t *type);
char *type_str(type_t *type);

#endif /* TYPE_H */

