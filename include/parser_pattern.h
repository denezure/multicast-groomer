#pragma once

#include <stdint.h>

struct parser_pattern;
struct parser_stack;

struct parser_pattern* parser_pattern_create(int32_t lenEst);
void parser_pattern_destroy(struct parser_pattern* pattern);

struct parser_pattern* parser_pattern_append(struct parser_pattern* pattern,
    int32_t type, void* value);

// Callback is called with:
//   Type,
//   ValuePtr,
//   Index,
//   UserData
// for each element in the pattern
// Will continue iterating until the pattern ends or a zero is returned from callback
int32_t parser_pattern_iterate(struct parser_pattern* pattern,
    int32_t (*callback)(int32_t, void*, int32_t, void*),
    void* data);

// Index still goes from 0 -> N-1 here
int32_t parser_pattern_iterate_reverse(struct parser_pattern* pattern,
    int32_t (*callback)(int32_t, void*, int32_t, void*),
    void* data);
