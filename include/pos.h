#ifndef POS_H
#define POS_H

typedef struct pos_t pos_t;
struct pos_t {
	int index;
	int line;
	int column;
};

#define POS_INIT() ((pos_t) {.index=0, .line=1, .column=1})

#endif /* POS_H */

