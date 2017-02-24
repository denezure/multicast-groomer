#include <parser_pattern.h>

#include <parser_stack.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct parser_pattern {
    int32_t length;
    int32_t capacity;
    struct stack_element pattern[];
};

struct parser_pattern* parser_pattern_create(int32_t lenEst)
{
    int32_t patternSize = sizeof(struct parser_pattern) + lenEst * sizeof(struct stack_element);
    struct parser_pattern* pp = calloc(1, patternSize);

    if (pp) {
        pp->capacity = lenEst;
    }

    return pp;
}

void parser_pattern_destroy(struct parser_pattern* pattern)
{
    assert(pattern);
    free(pattern);
}

struct parser_pattern* parser_pattern_append(struct parser_pattern* pattern,
    int32_t type, void* value)
{
    assert(pattern);

    int32_t canAppend = 1;

    if (pattern->length >= pattern->capacity) {
        int32_t newSize = sizeof(struct parser_pattern) + 2 * pattern->capacity * sizeof(struct stack_element);

        struct parser_pattern* newPattern = realloc(pattern, newSize);
        if (newPattern == NULL) {
            printf("Realloc() failed!\n");
            canAppend = 0;
        } else {
            newPattern->capacity *= 2;
            pattern = newPattern;
        }
    }

    if (canAppend) {
        pattern->pattern[pattern->length++] = (struct stack_element){ value, type };
    }

    return pattern;
}

int32_t parser_pattern_iterate(struct parser_pattern* pattern,
    int32_t (*callback)(int32_t, void*, int32_t, void*),
    void* data)
{
    int32_t result = 1;
    for (int32_t i = 0;
         i < pattern->length && (result = callback(pattern->pattern[i].type, pattern->pattern[i].value, i, data));
         ++i) {
    }

    return result;
}

int32_t parser_pattern_iterate_reverse(struct parser_pattern* pattern,
    int32_t (*callback)(int32_t, void*, int32_t, void*),
    void* data)
{
    int32_t result = 1;
    int32_t index = 0;
    for (int32_t i = pattern->length - 1;
         i >= 0 && (result = callback(pattern->pattern[i].type, pattern->pattern[i].value, index++, data));
         --i) {
    }

    return result;
}

