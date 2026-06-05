#include "vm.h"
#include "common.h"

// ========================================
// helper declaration
// ========================================

#define VM_STACK_CAPACITY (64 * 1024)

typedef struct vm_fn_state_t vm_fn_state_t;
struct vm_fn_state_t {
	int fn_id;
	int ip;

	word_t *temps;
	int temps_len;

	char *stack;
	int stack_size;

	char **param_addrs;
	int param_addrs_len;

	char *return_addr;
};

static vec_t g_list; // vector of ir_fn_t*
static int g_main_fn;
static int g_is_running = 1;
static vm_fn_state_t *g_fn_states;
static int g_fn_states_len;
static int g_current_fn_state;
static word_t g_last_value = 0; // for debug

static void init(vec_t list);
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
static void set_param_addr(int param_index, char *addr);
static word_t get_param_addr(int param_index);
static void run_inst();

static void inst_add(ir_inst_t inst);
static void inst_allocate(ir_inst_t inst);
static void inst_deallocate(ir_inst_t inst);
static void inst_begin_call(ir_inst_t inst);
static void inst_call(ir_inst_t inst);
static void inst_const(ir_inst_t inst);
static void inst_end_call(ir_inst_t inst);
static void inst_get_return_addr(ir_inst_t inst);
static void inst_get_param_addr(ir_inst_t inst);
static void inst_load(ir_inst_t inst);
static void inst_return(ir_inst_t inst);
static void inst_set_return_addr(ir_inst_t inst);
static void inst_set_param_addr(ir_inst_t inst);
static void inst_store(ir_inst_t inst);
static void inst_sub(ir_inst_t inst);
static void inst_mul(ir_inst_t inst);
static void inst_div(ir_inst_t inst);
static void inst_mod(ir_inst_t inst);

static void run_extern_fn(ir_fn_t *ir_fn);

// ========================================
// vm.h - definition
// ========================================

void run_vm(ir_t ir) {
	init(ir.fn_list);
	run();
}

// ========================================
// helper definition
// ========================================

static void init(vec_t list) {
	g_list = list;
	g_main_fn = -1;
	g_is_running = 1;
	g_fn_states = NULL;
	g_fn_states_len = 0;
	g_current_fn_state = -1;

	for (int i = 0; i < g_list.len; i++) {
		ir_fn_t *ir_fn = list.elems[i];
		if (strcmp(ir_fn->name, "main") == 0) {
			g_main_fn = ir_fn->id;
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
	printf("VALUE: %llu\n", g_last_value);
}

static ir_inst_t current_inst() {
	vm_fn_state_t *state = current_state();
	ir_fn_t *ir_fn = g_list.elems[state->fn_id];
	ir_inst_t *inst = ir_fn->insts.elems[state->ip];
	return *inst;
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
		.stack=calloc(sizeof(char), VM_STACK_CAPACITY),
		.stack_size=0,
		.return_addr=NULL,
	};
}

static void next_fn_state(int fn_id) {
	g_current_fn_state += 1;
	g_fn_states[g_current_fn_state].fn_id = fn_id;

	vm_fn_state_t *state = current_state();
	ir_fn_t *ir_fn = g_list.elems[state->fn_id];
	if (ir_fn->is_extern) {
		run_extern_fn(ir_fn);
	}
}

static void prev_fn_state() {
	g_current_fn_state -= 1;
}

static void pop_fn_state() {
	vm_fn_state_t *state = current_state() + 1;
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

	// DEBUGGING
	g_last_value = value;
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

	if (state->stack_size + size >= VM_STACK_CAPACITY) {
		fprintf(stderr, "Stack Overflow!\n");
		exit(1);
	}

	state->stack_size += size;
	return (word_t) &(state->stack[state->stack_size - size]);
}

static void deallocate(int size) {
	vm_fn_state_t *state = current_state();
	state->stack_size -= size;
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

static void set_param_addr(int param_index, char *addr) {
	vm_fn_state_t *state = current_state() + 1;

	if (param_index >= state->param_addrs_len) {
		state->param_addrs_len = param_index * 2 + 100;
		state->param_addrs = realloc(state->param_addrs, 
			state->param_addrs_len * sizeof(char*));
	}

	state->param_addrs[param_index] = addr;
}

static word_t get_param_addr(int param_index) {
	vm_fn_state_t *state = current_state();
	return (word_t) state->param_addrs[param_index];
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
	case IR_INST_GET_PARAM_ADDR: {
		inst_get_param_addr(inst);
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
	case IR_INST_SET_PARAM_ADDR: {
		inst_set_param_addr(inst);
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
	case IR_INST_MUL: {
		inst_mul(inst);
		break;
	}
	case IR_INST_DIV: {
		inst_div(inst);
		break;
	}
	case IR_INST_MOD: {
		inst_mod(inst);
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

static void inst_get_param_addr(ir_inst_t inst) {
	word_t addr = get_param_addr(inst.arg2);
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
		return;
	}
	prev_fn_state();
	next_ip();
}

static void inst_set_return_addr(ir_inst_t inst) {
	word_t addr = get(inst.arg1);
	set_return_addr((char*) addr);
	next_ip();
}

static void inst_set_param_addr(ir_inst_t inst) {
	word_t addr = get(inst.arg2);
	set_param_addr(inst.arg1, (char*) addr);
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
static void inst_mul(ir_inst_t inst) {
	word_t v1 = get(inst.arg2);
	word_t v2 = get(inst.arg3);
	word_t res = cast(v1 * v2, inst.arg4);
	set(inst.arg1, res);
	next_ip();
}

static void inst_div(ir_inst_t inst) {
	word_t v1 = get(inst.arg2);
	word_t v2 = get(inst.arg3);
	word_t res = cast(v1 / v2, inst.arg4);
	set(inst.arg1, res);
	next_ip();
}

static void inst_mod(ir_inst_t inst) {
	word_t v1 = get(inst.arg2);
	word_t v2 = get(inst.arg3);
	word_t res = cast(v1 % v2, inst.arg4);
	set(inst.arg1, res);
	next_ip();
}

static void run_extern_fn(ir_fn_t *ir_fn) {
	assert(strcmp(ir_fn->name, "printNum") == 0 && "Only printNum supported");

	// check if the type information is correct
	type_t *fn_type = ir_fn->type;
	assert(fn_type->kind == TYPE_FN);

	type_t *param0_type = fn_type->type.fn_type.param_types.elems[0];
	assert(param0_type->kind == TYPE_PRIMITIVE && param0_type->size == 4);

	type_t *return_type = fn_type->type.fn_type.return_type;
	assert(return_type->kind == TYPE_VOID);

	inst_get_param_addr((ir_inst_t) {.arg1=0, .arg2=0});
	inst_load((ir_inst_t) {.arg1=1, .arg2=0, .arg3=4});

	word_t v = get(1);
	printf("%llu\n", v);

	inst_return((ir_inst_t) {});
}

