#include <config_loader.h>

#include <parser_pattern.h>
#include <parser_stack.h>

#include <assert.h>
#include <stdio.h>
#include <yaml.h>

enum element_type {
    NONE,
    MAPSTART,
    MAPEND,
    SEQSTART,
    SEQEND,
    VALUE,
};

static void freeValues(int32_t type, void* value);
static int32_t stackMatcher(int32_t type, void* data, int32_t index, void* stackVoid);

int config_loader_get(const char* filename, struct mcast_config** configPtr)
{
    if (configPtr == NULL) {
        return 1;
    }

    struct mcast_config* config = calloc(1, sizeof(**configPtr));

    FILE* filePtr = fopen(filename, "r");
    if (filePtr == NULL) {
        return 2;
    }

    yaml_parser_t parser;
    if (yaml_parser_initialize(&parser) == 0) {
        fclose(filePtr);
        return 3;
    }

    struct parser_pattern* controlInterface = parser_pattern_create(5);
    {
        controlInterface = parser_pattern_append(controlInterface, MAPSTART, NULL);
        controlInterface = parser_pattern_append(controlInterface, VALUE, "control");
        controlInterface = parser_pattern_append(controlInterface, MAPSTART, NULL);
        controlInterface = parser_pattern_append(controlInterface, VALUE, "interface");
        controlInterface = parser_pattern_append(controlInterface, VALUE, NULL);
    }
    struct parser_pattern* controlPort = parser_pattern_create(5);
    {
        controlPort = parser_pattern_append(controlPort, MAPSTART, NULL);
        controlPort = parser_pattern_append(controlPort, VALUE, "control");
        controlPort = parser_pattern_append(controlPort, MAPSTART, NULL);
        controlPort = parser_pattern_append(controlPort, VALUE, "port");
        controlPort = parser_pattern_append(controlPort, VALUE, NULL);
    }

    struct parser_pattern* sessionInit = parser_pattern_create(4);
    {
        sessionInit = parser_pattern_append(sessionInit, MAPSTART, NULL);
        sessionInit = parser_pattern_append(sessionInit, VALUE, "streams");
        sessionInit = parser_pattern_append(sessionInit, SEQSTART, NULL);
        sessionInit = parser_pattern_append(sessionInit, MAPSTART, NULL);
    }
    struct parser_pattern* sessionSetVar = parser_pattern_create(6);
    {
        sessionSetVar = parser_pattern_append(sessionSetVar, MAPSTART, NULL);
        sessionSetVar = parser_pattern_append(sessionSetVar, VALUE, "streams");
        sessionSetVar = parser_pattern_append(sessionSetVar, SEQSTART, NULL);
        sessionSetVar = parser_pattern_append(sessionSetVar, MAPSTART, NULL);
        sessionSetVar = parser_pattern_append(sessionSetVar, VALUE, NULL);
        sessionSetVar = parser_pattern_append(sessionSetVar, VALUE, NULL);
    }

    // Stores the tree path
    struct parser_stack* stack = parser_stack_create(64);

    yaml_parser_set_input_file(&parser, filePtr);
    yaml_event_t event;

    do {
        if (!yaml_parser_parse(&parser, &event)) {
            yaml_parser_delete(&parser);
            fclose(filePtr);

            return 4;
        }

        // Update the stack
        switch (event.type) {
        case YAML_NO_EVENT:
            break;
        case YAML_STREAM_START_EVENT:
            break;
        case YAML_STREAM_END_EVENT:
            break;
        case YAML_DOCUMENT_START_EVENT:
            break;
        case YAML_DOCUMENT_END_EVENT:
            break;
        case YAML_SEQUENCE_START_EVENT:
            parser_stack_push(stack, SEQSTART, NULL);
            break;
        case YAML_SEQUENCE_END_EVENT:
            parser_stack_pop_until_type(stack, SEQSTART, freeValues);
            if (parser_stack_top(stack) > 0 && parser_stack_get_type(stack, 0) == VALUE) {
                // Pop one more if the sequence had a name associated with it
                char* val = parser_stack_get_value(stack, 0);
                free(val);
                parser_stack_pop(stack);
            }
            break;
        case YAML_MAPPING_START_EVENT:
            parser_stack_push(stack, MAPSTART, NULL);
            break;
        case YAML_MAPPING_END_EVENT:
            parser_stack_pop_until_type(stack, MAPSTART, freeValues);
            if (parser_stack_top(stack) > 0 && parser_stack_get_type(stack, 0) == VALUE) {
                // Pop one more if the mapping had a name associated with it
                char* val = parser_stack_get_value(stack, 0);
                free(val);
                parser_stack_pop(stack);
            }
            break;
        case YAML_ALIAS_EVENT:
            break;
        case YAML_SCALAR_EVENT: {
            char* value = strdup((char*)event.data.scalar.value);
            parser_stack_push(stack, VALUE, value);
        } break;
        }

        if (event.type != YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
        }

        // check stack for matches against patterns

        if (parser_pattern_iterate_reverse(controlInterface, stackMatcher, stack)) {
            config->controlInterface = parser_stack_get_value(stack, 0);
            parser_stack_pop(stack);

            free(parser_stack_get_value(stack, 0));
            parser_stack_pop(stack);
        } else if (parser_pattern_iterate_reverse(controlPort, stackMatcher, stack)) {
            char* portString = parser_stack_get_value(stack, 0);
            config->controlPort = atoi(portString);
            free(portString);
            parser_stack_pop(stack);

            free(parser_stack_get_value(stack, 0));
            parser_stack_pop(stack);
        } else if (parser_pattern_iterate_reverse(sessionSetVar, stackMatcher, stack)) {
            char* varValue = parser_stack_get_value(stack, 0);
            parser_stack_pop(stack);

            char* varName = parser_stack_get_value(stack, 0);
            parser_stack_pop(stack);

            int32_t freeVal = 0;

            if (strcmp(varName, "name") == 0) {
                config->sessionConfig->name = varValue;
            } else if (strcmp(varName, "source_group") == 0) {
                config->sessionConfig->sourceGroup = varValue;
            } else if (strcmp(varName, "dest_group") == 0) {
                config->sessionConfig->destinationGroup = varValue;
            } else if (strcmp(varName, "source_port") == 0) {
                config->sessionConfig->sourcePort = atoi(varValue);
                freeVal = 1;
            } else if (strcmp(varName, "dest_port") == 0) {
                config->sessionConfig->destinationPort = atoi(varValue);
                freeVal = 1;
            } else if (strcmp(varName, "source_interface") == 0) {
                config->sessionConfig->sourceInterface = varValue;
            } else if (strcmp(varName, "dest_interface") == 0) {
                config->sessionConfig->destinationInterface = varValue;
            } else if (strcmp(varName, "window_microsec") == 0) {
                config->sessionConfig->smoothingWindow = atoi(varValue);
                freeVal = 1;
            }

            if (freeVal) {
                free(varValue);
            }
            free(varName);
        } else if (parser_pattern_iterate_reverse(sessionInit, stackMatcher, stack)) {
            struct mcast_session_config* newSess = calloc(1, sizeof(struct mcast_session_config));
            newSess->next = config->sessionConfig;
            config->sessionConfig = newSess;
        }
    } while (event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete(&event);

    yaml_parser_delete(&parser);
    fclose(filePtr);

    while (parser_stack_top(stack)) {
        int32_t type = parser_stack_get_type(stack, 0);
        void* value = parser_stack_get_value(stack, 0);
        if (type == VALUE && value != NULL) {
            free(value);
        }
        parser_stack_pop(stack);
    }

    parser_stack_destroy(stack);

    parser_pattern_destroy(controlInterface);
    parser_pattern_destroy(controlPort);
    parser_pattern_destroy(sessionInit);
    parser_pattern_destroy(sessionSetVar);

    *configPtr = config;

    return 0;
}

static void freeValues(int32_t type, void* value)
{
    if (type == VALUE && value != NULL) {
        free(value);
    }
}

static int32_t stackMatcher(int32_t type, void* data, int32_t index, void* stackVoid)
{
    struct parser_stack* stack = stackVoid;

    if (parser_stack_top(stack) < index + 1) {
        return 0;
    }

    int32_t matches = parser_stack_get_type(stack, index) == type;

    switch (type) {
    case SEQSTART:
    case MAPSTART:
        break;
    case VALUE:
        if (data != NULL) {
            matches = matches && (strcmp((char*)data, parser_stack_get_value(stack, index)) == 0);
        }
        break;
    }

    return matches;
}
