#pragma once

#include <stdint.h>

struct parser_stack;

struct parser_stack* parser_stack_create(int32_t size);
void parser_stack_destroy(struct parser_stack *stack);

// depth = 0 => return the most recently pushed value
// depth = 1 => return the 2nd most recently pushed value
// etc.
int32_t parser_stack_get_type(struct parser_stack* stack, int32_t depth);
void* parser_stack_get_value(struct parser_stack* stack, int32_t depth);

void parser_stack_push(struct parser_stack *stack, int32_t type, void* data);
// data is a pointer to an array of the data element pointers
void parser_stack_push_n(struct parser_stack *stack, const int32_t *types, void* const* data, int32_t len);

// Returns the number of elements popped
void parser_stack_pop(struct parser_stack *stack);
void parser_stack_pop_n(struct parser_stack *stack, int32_t n);

void parser_stack_pop_until_type(struct parser_stack* stack, int32_t type, void (*valueHandler)(int32_t, void *));

int32_t parser_stack_length(struct parser_stack *stack);
int32_t parser_stack_top(struct parser_stack *stack);

void parser_stack_print(struct parser_stack *stack);

struct stack_element {
    void* value;
    int32_t type;
};
