#include <config_loader.h>

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

struct stack_element {
    enum element_type type;

    char* value;
};

// MAPPINGS, VALUE('control'), MAPPINGS, VALUE('interface'), VALUE('*') -> set interface
static const struct stack_element controlSetInterfacePattern[] = {
    { MAPSTART, NULL },
    { VALUE, "control" },
    { MAPSTART, NULL },
    { VALUE, "interface" },
    { VALUE, NULL },
    { NONE, NULL }
};

// MAPPINGS, VALUE('control'), MAPPINGS, VALUE('port'), VALUE('*') -> set port
static const struct stack_element controlSetPortPattern[] = {
    { MAPSTART, NULL },
    { VALUE, "control" },
    { MAPSTART, NULL },
    { VALUE, "port" },
    { VALUE, NULL },
    { NONE, NULL }
};

// MAPPINGS, VALUE('streams'), SEQS, MAPPINGS -> create new session and make it the current writer
static const struct stack_element sessionInitializePattern[] = {
    { MAPSTART, NULL },
    { VALUE, "streams" },
    { SEQSTART, NULL },
    { MAPSTART, NULL },
    { NONE, NULL }
};

// MAPPINGS, VALUE('streams'), SEQS, MAPPINGS, VALUE('*'), VALUE('*') -> set variable in current writer
static const struct stack_element sessionSetVarPattern[] = {
    { MAPSTART, NULL },
    { VALUE, "streams" },
    { SEQSTART, NULL },
    { MAPSTART, NULL },
    { VALUE, NULL },
    { VALUE, NULL },
    { NONE, NULL }
};

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

    // Stores the tree path
    struct stack_element stack[16];
    memset(stack, 0, 16 * sizeof(stack[0]));

    // Holds the index of the next stack insertion
    int32_t stackTop = 0;

    yaml_parser_set_input_file(&parser, filePtr);
    yaml_event_t event;

    // State:
    //   Stack of parse values
    //     Maintain structure of the parse tree by pairing start & end nodes' removal
    //   Control data
    //   Stream config linked list

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
            stack[stackTop].value = NULL;
            stack[stackTop++].type = SEQSTART;
            break;
        case YAML_SEQUENCE_END_EVENT:
            while (stackTop > 0 && stack[stackTop - 1].type != SEQSTART) {
                --stackTop;
                if (stack[stackTop].type == VALUE && stack[stackTop].value != NULL) {
                    free(stack[stackTop].value);
                    stack[stackTop].value = NULL;
                }
            }
            break;
        case YAML_MAPPING_START_EVENT:
            stack[stackTop].value = NULL;
            stack[stackTop++].type = MAPSTART;
            break;
        case YAML_MAPPING_END_EVENT:
            while (stackTop > 0 && stack[stackTop - 1].type != MAPSTART) {
                --stackTop;
                if (stack[stackTop].type == VALUE && stack[stackTop].value != NULL) {
                    free(stack[stackTop].value);
                    stack[stackTop].value = NULL;
                }
            }
            break;
        case YAML_ALIAS_EVENT:
            break;
        case YAML_SCALAR_EVENT:
            stack[stackTop].type = VALUE;
            stack[stackTop++].value = strdup((char*)event.data.scalar.value);
            break;
        }
        if (event.type != YAML_STREAM_END_EVENT)
            yaml_event_delete(&event);

        // check stack for matches against patterns
        int32_t foundMatch = 0;

        if (!foundMatch) {
            foundMatch = 1;
            // If any of these cases are not met, terminate:
            //   Match failed in a previous iteration
            //   At the end of the stack
            //   At the end of the pattern
            for (int32_t i = 0; foundMatch && i < stackTop && controlSetInterfacePattern[i].type != NONE; ++i) {
                foundMatch = foundMatch && stack[i].type == controlSetInterfacePattern[i].type;

                int32_t valueCheck = controlSetInterfacePattern[i].value == NULL || strcmp(stack[i].value, controlSetInterfacePattern[i].value) == 0;

                foundMatch = foundMatch && valueCheck;
            }

            if (foundMatch) {
                // Set the interface for the control system
                // Take ownership of the interface string
                config->controlInterface = stack[--stackTop].value;
                stack[stackTop].value = NULL;
                stack[stackTop].type = NONE;
                // Free the label string
                free(stack[--stackTop].value);
                stack[stackTop].value = NULL;
                stack[stackTop].type = NONE;
            }
        }

        if (!foundMatch) {
            foundMatch = 1;
            for (int32_t i = 0; foundMatch && i < stackTop; ++i) {
                foundMatch = foundMatch && stack[i].type == controlSetPortPattern[i].type;

                int32_t valueCheck = controlSetPortPattern[i].value == NULL || strcmp(stack[i].value, controlSetPortPattern[i].value) == 0;

                foundMatch = foundMatch && valueCheck;
                foundMatch = foundMatch && controlSetPortPattern[i].type != NONE;
            }

            if (foundMatch) {
                // Set the port for the control system
                // Free the string
                config->controlPort = atoi(stack[--stackTop].value);
                free(stack[stackTop].value);
                stack[stackTop].value = NULL;
                stack[stackTop].type = NONE;
                // Free the label string
                free(stack[--stackTop].value);
                stack[stackTop].value = NULL;
                stack[stackTop].type = NONE;
            }
        }

        if (!foundMatch) {
            foundMatch = 1;
            for (int32_t i = 0; foundMatch && i < stackTop; ++i) {
                foundMatch = foundMatch && stack[i].type == sessionSetVarPattern[i].type;

                int32_t valueCheck = sessionSetVarPattern[i].value == NULL || strcmp(stack[i].value, sessionSetVarPattern[i].value) == 0;

                foundMatch = foundMatch && valueCheck;
                foundMatch = foundMatch && sessionSetVarPattern[i].type != NONE;
            }

            if (foundMatch) {
                // Set the corresponding variable in the active session
                const char* val = stack[stackTop - 2].value;

                if (strcmp(val, "name") == 0) {
                    config->sessionConfig->name = stack[--stackTop].value;
                    stack[stackTop].value = NULL;
                    stack[stackTop].type = NONE;
                    free(stack[--stackTop].value);
                    stack[stackTop].value = NULL;
                    stack[stackTop].type = NONE;
                } else if (strcmp(val, "source_group") == 0) {

                } else if (strcmp(val, "dest_group") == 0) {

                } else if (strcmp(val, "source_port") == 0) {

                } else if (strcmp(val, "dest_port") == 0) {

                } else if (strcmp(val, "source_interface") == 0) {

                } else if (strcmp(val, "dest_interface") == 0) {

                } else if (strcmp(val, "window_microsec") == 0) {
                }
            }
        }

        // sessionInitializePattern
        if (!foundMatch) {
            foundMatch = 1;
            for (int32_t i = 0; foundMatch && i < stackTop; ++i) {
                foundMatch = foundMatch && stack[i].type == sessionInitializePattern[i].type;

                int32_t valueCheck = sessionInitializePattern[i].value == NULL || strcmp(stack[i].value, sessionInitializePattern[i].value) == 0;

                foundMatch = foundMatch && valueCheck;
                foundMatch = foundMatch && sessionInitializePattern[i].type != NONE;
            }

            if (foundMatch) {
                struct mcast_session_config* newConfig = calloc(1, sizeof(*newConfig));
                newConfig->next = config->sessionConfig;
                config->sessionConfig = newConfig;
            }
        }

    } while (event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete(&event);

    yaml_parser_delete(&parser);
    fclose(filePtr);

    *configPtr = config;

    return 0;
}
