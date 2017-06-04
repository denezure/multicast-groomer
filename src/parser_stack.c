#include <parser_stack.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

struct parser_stack {
    int32_t length;
    int32_t top;
    struct stack_element stack[];
};

struct parser_stack* parser_stack_create(int32_t size)
{
    struct parser_stack* stack = calloc(1, sizeof(*stack) + size * sizeof(stack->stack[0]));
    if (stack) {
        stack->length = size;
    }

    return stack;
}

void parser_stack_destroy(struct parser_stack* stack)
{
    assert(stack != NULL);

    free(stack);
}

int32_t parser_stack_get_type(struct parser_stack* stack, int32_t depth)
{
    assert(stack != NULL);
    assert(depth < stack->top);

    return stack->stack[stack->top - 1 - depth].type;
}

void* parser_stack_get_value(struct parser_stack* stack, int32_t depth)
{
    assert(stack != NULL);
    assert(depth < stack->top);

    return stack->stack[stack->top - 1 - depth].value;
}

void parser_stack_push(struct parser_stack* stack, int32_t type, void* data)
{
    assert(stack != NULL);
    assert(stack->top < stack->length);

    int32_t index = stack->top;
    stack->stack[index].type = type;
    stack->stack[index].value = data;
    ++stack->top;
}

void parser_stack_push_n(struct parser_stack* stack, const int32_t* types, void* const* data, int32_t len)
{
    assert(stack != NULL);
    assert(stack->top + len <= stack->length);

    for (int32_t i = 0; i < len; ++i) {
        int32_t index = stack->top;

        stack->stack[index].type = types[i];
        stack->stack[index].value = data[i];

        ++stack->top;
    }
}

void parser_stack_pop(struct parser_stack* stack)
{
    assert(stack != NULL);
    assert(stack->top > 0);

    int32_t index = stack->top - 1;
    stack->stack[index].type = 0;
    stack->stack[index].value = NULL;
    stack->top = index;
}

void parser_stack_pop_n(struct parser_stack* stack, int32_t n)
{
    assert(stack != NULL);
    assert(stack->top >= n);

    int32_t index = stack->top - 1;

    for (int32_t i = 0; i < n; ++i) {
        stack->stack[index - i].type = 0;
        stack->stack[index - i].value = NULL;
    }

    stack->top -= n;
}

int32_t parser_stack_length(struct parser_stack* stack)
{
    assert(stack != NULL);
    return stack->length;
}

int32_t parser_stack_top(struct parser_stack* stack)
{
    assert(stack != NULL);
    return stack->top;
}

void parser_stack_pop_until_type(struct parser_stack* stack, int32_t type, void (*valueHandler)(int32_t, void *))
{
    while (parser_stack_top(stack) > 0 && parser_stack_get_type(stack, 0) != type) {
        valueHandler(parser_stack_get_type(stack, 0), parser_stack_get_value(stack, 0));
        parser_stack_pop(stack);
    }

    if (parser_stack_top(stack) > 0 && parser_stack_get_type(stack, 0) == type)
    {
        valueHandler(parser_stack_get_type(stack, 0), parser_stack_get_value(stack, 0));
        parser_stack_pop(stack);
    }
}

enum element_type {
    NONE,
    MAPSTART,
    MAPEND,
    SEQSTART,
    SEQEND,
    VALUE,
};

void parser_stack_print(struct parser_stack *stack)
{
    printf("Stack: ");
    for (int32_t i = stack->top - 1; i >= 0; --i)
    {
        int32_t type = parser_stack_get_type(stack, i);
        void *val = parser_stack_get_value(stack, i);
        switch (type)
        {
            case MAPSTART:
                printf("MAPSTART | ");
                break;
            case MAPEND:
                printf("MAPEND | ");
                break;
            case SEQSTART:
                printf("SEQSTART | ");
                break;
            case SEQEND:
                printf("SEQEND | ");
                break;
            case VALUE:
                printf("V('%s') | ", (char*)val);
                break;
            default:
                printf("??? | ");
                break;
        }
    }
    printf("\n");
}
