#include "vm.h"
#include "common.h"

// ========================================
// helper declaration
// ========================================

typedef struct vm_fn_state_t vm_fn_state_t;
struct vm_fn_state_t {
	int fn_id;
	int ip;

	word_t *temps;
	int temps_len;
};

static ir_fn_t **g_list;
static int g_len;
static int g_main_fn;
static int g_is_running = 1;
static vm_fn_state_t *g_fn_states;
static int g_fn_states_len;
static int g_current_fn_state;

static void init(ir_fn_t **list, int len);
static void run();
static ir_inst_t current_inst();
static vm_fn_state_t *current_state();
static void push_fn_state(int fn_id);
static void next_fn_state();
static void pop_fn_state();
static word_t get(int temp_id);
static void set(int temp_id, word_t value);
static void next_ip();
static word_t cast(word_t v, int bytes);
static void run_inst();

static void inst_add(ir_inst_t inst);

// ========================================
// vm.h - definition
// ========================================

void run_vm(ir_fn_t **list, int len) {
	init(list, len);
	run();
}

// ========================================
// helper definition
// ========================================

static void init(ir_fn_t **list, int len) {
	g_list = list;
	g_len = len;
	g_main_fn = -1;
	g_is_running = 1;
	g_fn_states = NULL;
	g_fn_states_len = 0;
	g_current_fn_state = -1;

	for (int i = 0; i < len; i++) {
		if (strcmp(list[i]->name, "main") == 0) {
			g_main_fn = list[i]->id;
		}
	}

	if (g_main_fn == -1) {
		printf("No main function found!\n");
		exit(1);
	}
}

static void run() {
	push_fn_state(g_main_fn);
	next_fn_state();
	while (g_is_running) {
		run_inst();
	}
	pop_fn_state();
}

static ir_inst_t current_inst() {
	vm_fn_state_t *state = current_state();
	return g_list[state->fn_id]->list[state->ip];
}

static vm_fn_state_t *current_state() {
	return &g_fn_states[g_current_fn_state];
}

static void push_fn_state(int fn_id) {
	g_fn_states_len += 1;
	g_fn_states = realloc(g_fn_states, g_fn_states_len * sizeof(vm_fn_state_t));
	g_fn_states[g_fn_states_len-1] = (vm_fn_state_t) {
		.fn_id=fn_id, 
		.ip=0,
		.temps=NULL,
		.temps_len=0,
	};
}

static void next_fn_state() {
	g_current_fn_state += 1;
}

static void pop_fn_state() {
	g_fn_states_len -= 1;
	g_fn_states = realloc(g_fn_states, g_fn_states_len * sizeof(vm_fn_state_t));
	g_current_fn_state -= 1;
}

static word_t get(int temp_id) {
	vm_fn_state_t *state = current_state();

	if (state->temps_len <= temp_id) {
		state->temps_len = 2 * temp_id + 100;
		state->temps = realloc(state->temps, 
			state->temps_len * sizeof(word_t));
	}

	return state->temps[temp_id];
}

static void set(int temp_id, word_t value) {
	vm_fn_state_t *state = current_state();

	if (state->temps_len <= temp_id) {
		state->temps_len = 2 * temp_id + 100;
		state->temps = realloc(state->temps, 
			state->temps_len * sizeof(word_t));
	}

	state->temps[temp_id] = value;
}

static void next_ip() {
	vm_fn_state_t *state = current_state();
	state->ip += 1;
}

static word_t cast(word_t v, int bytes) {
	assert(bytes <= 8);
	int shift = 8 * (8 - bytes);
	return (v << shift) >> shift;
}

static void run_inst() {
	ir_inst_t inst = current_inst();
	switch (inst.kind) {
	case IR_INST_ADD: {
		inst_add(inst);
		break;
	}
	default:
		printf("What is this inst?");
		exit(1);
	}
}

static void inst_add(ir_inst_t inst) {
	word_t v1 = get(inst.arg2);
	word_t v2 = get(inst.arg3);
	word_t res = cast(v1 + v2, inst.arg4);
	set(inst.arg1, res);
	next_ip();
}

