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

	char *stack;
	int stack_size;

	char *return_addr;
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
static void push_fn_state();
static void next_fn_state(int fn_id);
static void prev_fn_state();
static void pop_fn_state();
static word_t get(int temp_id);
static void set(int temp_id, word_t value);
static void next_ip();
static word_t cast(word_t v, int bytes);
static word_t allocate(int size);
static void deallocate(int size);
static void store(char *addr, word_t value, int bytes);
static word_t load(char *addr, int bytes);
static void set_return_addr(char *addr);
static word_t get_return_addr();
static void run_inst();

static void inst_add(ir_inst_t inst);
static void inst_allocate(ir_inst_t inst);
static void inst_deallocate(ir_inst_t inst);
static void inst_begin_call(ir_inst_t inst);
static void inst_call(ir_inst_t inst);
static void inst_const(ir_inst_t inst);
static void inst_end_call(ir_inst_t inst);
static void inst_get_return_addr(ir_inst_t inst);
static void inst_load(ir_inst_t inst);
static void inst_return(ir_inst_t inst);
static void inst_set_return_addr(ir_inst_t inst);
static void inst_store(ir_inst_t inst);
static void inst_sub(ir_inst_t inst);

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
	push_fn_state();
	next_fn_state(g_main_fn);
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

static void push_fn_state() {
	g_fn_states_len += 1;
	g_fn_states = realloc(g_fn_states, g_fn_states_len * sizeof(vm_fn_state_t));
	g_fn_states[g_fn_states_len-1] = (vm_fn_state_t) {
		.fn_id=-1, 
		.ip=0,
		.temps=NULL,
		.temps_len=0,
		.stack=NULL,
		.stack_size=0,
		.return_addr=NULL,
	};
}

static void next_fn_state(int fn_id) {
	g_current_fn_state += 1;
	g_fn_states[g_current_fn_state].fn_id = fn_id;
}

static void prev_fn_state() {
	g_current_fn_state -= 1;
}

static void pop_fn_state() {
	vm_fn_state_t *state = current_state();
	free(state->temps);
	free(state->stack);
	g_fn_states_len -= 1;
	g_fn_states = realloc(g_fn_states, g_fn_states_len * sizeof(vm_fn_state_t));
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

static word_t allocate(int size) {
	vm_fn_state_t *state = current_state();
	state->stack_size += size;
	state->stack = realloc(state->stack, state->stack_size * sizeof(char));
	return (word_t) &(state->stack[state->stack_size - size]);
}

static void deallocate(int size) {
	vm_fn_state_t *state = current_state();
	state->stack_size -= size;
	state->stack = realloc(state->stack, state->stack_size * sizeof(char));
}

static void store(char *addr, word_t value, int bytes) {
	assert(bytes <= 8);
	char *src = (char*) &value;
	for (int i = 0; i < bytes; i++) addr[i] = src[i];
}

static word_t load(char *addr, int bytes) {
	assert(bytes <= 8);
	word_t res = 0;
	char *dest = (char*) &res;
	for (int i = 0; i < bytes; i++) dest[i] = addr[i];
	return res;
}

static void set_return_addr(char *addr) {
	vm_fn_state_t *state = current_state() + 1;
	state->return_addr = addr;
}

static word_t get_return_addr() {
	vm_fn_state_t *state = current_state();
	return (word_t) state->return_addr;
}

static void run_inst() {
	ir_inst_t inst = current_inst();
	switch (inst.kind) {
	case IR_INST_ADD: {
		inst_add(inst);
		break;
	}
	case IR_INST_ALLOCATE: {
		inst_allocate(inst);
		break;
	}
	case IR_INST_DEALLOCATE: {
		inst_deallocate(inst);
		break;
	}
	case IR_INST_BEGIN_CALL: {
		inst_begin_call(inst);
		break;
	}
	case IR_INST_CALL: {
		inst_call(inst);
		break;
	}
	case IR_INST_CONST: {
		inst_const(inst);
		break;
	}
	case IR_INST_END_CALL: {
		inst_end_call(inst);
		break;
	}
	case IR_INST_GET_RETURN_ADDR: {
		inst_get_return_addr(inst);
		break;
	}
	case IR_INST_LOAD: {
		inst_load(inst);
		break;
	}
	case IR_INST_RETURN: {
		inst_return(inst);
		break;
	}
	case IR_INST_SET_RETURN_ADDR: {
		inst_set_return_addr(inst);
		break;
	}
	case IR_INST_STORE: {
		inst_store(inst);
		break;
	}
	case IR_INST_SUB: {
		inst_sub(inst);
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

static void inst_allocate(ir_inst_t inst) {
	word_t addr = allocate(inst.arg2);
	set(inst.arg1, addr);
	next_ip();
}

static void inst_deallocate(ir_inst_t inst) {
	deallocate(inst.arg1);
	next_ip();
}

static void inst_begin_call(ir_inst_t inst) { 
	push_fn_state();
	next_ip();
}

static void inst_call(ir_inst_t inst) { 
	next_fn_state(inst.arg1);
}

static void inst_const(ir_inst_t inst) {
	set(inst.arg1, inst.arg2);
	next_ip();
}

static void inst_end_call(ir_inst_t inst) {
	pop_fn_state();
	next_ip();
}

static void inst_get_return_addr(ir_inst_t inst) { 
	word_t addr = get_return_addr();
	set(inst.arg1, addr);
	next_ip();
}

static void inst_load(ir_inst_t inst) { 
	word_t addr = get(inst.arg2);
	word_t res = load((char*) addr, inst.arg3);
	set(inst.arg1, res);
	next_ip();
}

static void inst_return(ir_inst_t inst) {
	if (current_state()->fn_id == g_main_fn) {
		g_is_running = 0;
	}
	prev_fn_state();
	next_ip();
}

static void inst_set_return_addr(ir_inst_t inst) {
	word_t addr = get(inst.arg1);
	set_return_addr((char*) addr);
	next_ip();
}

static void inst_store(ir_inst_t inst) {
	word_t addr = get(inst.arg1);
	word_t value = get(inst.arg2);
	store((char*) addr, value, inst.arg3);
	next_ip();
}

static void inst_sub(ir_inst_t inst) {
	word_t v1 = get(inst.arg2);
	word_t v2 = get(inst.arg3);
	word_t res = cast(v1 - v2, inst.arg4);
	set(inst.arg1, res);
	next_ip();
}

