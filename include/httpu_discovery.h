#ifndef HTTPU_DISCOVERY_H
#define HTTPU_DISCOVERY_H

#include "../include/toml.h"

void httpu_cmd_handler(char *opts[], int token_count);
void httpu_discovery_handler(void);
int httpu_set_cmd_handler(char *option, char *value, char *module_options[], int count);
int set_search_target_opt(char *st); // is warranted

// check if all values are valid, and set before calling the main method
int module_pre_method_check(void);

void display_preliminary_options(void);
void populate_preliminary_options(toml_table_t *config);

void confer_variable_values(char *opt, size_t index);
void httpu_discovery_return_help(void);

void rl_httpu_memload_primary_array(void);
void register_main_module_method();

#define PRIME_MODULE_LOCATION "modules/bin/httpu-discovery.mod"

#define HTTPU_SETCMD_MAX 3
#define SOCKT_BOUNDARY_MAX 999
#define SSDP_MX_MAX_VALUE 300

// the total number of elements within the toml_discovery_names and httpu_scope_options character arrays
// which contain the string values of each supported argument (option) that can be passed to "set <OPTION> <VALUE>"
// they match up with the values stored in the toml configuration file for the supported modules, because
// obtaining the description as well as the name is more straightforward.
#define SCOPE_DISCOVERY_SIZE 8

#define SEARCH_TARGET_FIRST 3450
#define SEARCH_TARGET_LAST (atoi(st) - 1)

#define STATIC_SZ cmd_sz_final + 1

#define GENERIC_RETURN_SUCCESS 0
#define GENERIC_ERROR_RETURN_NONE -1
#define ILLEGAL_SET_OPTIONS_PASSED -2

// --------
#define SET_0ALL_TRUE 1
#define SET_0ALL_FALSE 0

// tab completetion function prototypes
char **static_cmd_httpu_completion(const char *cmd, int start, int end);
char *static_cmd_httpu_generator(const char *cmd, int state);

#define DEBUG_PRINT_ARG_STRLEN printf("GOT %s (%ld)\n", cmd, sizeof(httpu_module_options[i]));
#define SET_STATIC_OPTION_BOUNDRY toml_discovery_values[count] = '\0'; toml_discovery_values[count] = strdup(value); \
                                  ZERO_ALL_CALLED = 0; SET_0ALL_FLAG = SET_0ALL_TRUE; \
                                  printf("%s => \"%s\"\n", option, value); \
                                  return 0; \

#define PRE_CONDITIONAL_VALUE_CHECK if (strcmp(value, toml_discovery_values[count]) == 0)\
                                {\
                                    printf("%s BRIGHTSTAR::Error => target \"%s\" already set to value \"%s\"\n", RED_ERR, option, toml_discovery_values[count]);\
                                    \
                                    return -1;\
                                }\

#define FINISH_TOML_DISCOVERY_OPTS \
                ZERO_ALL_CALLED = SET_0ALL_FALSE;\
                SET_0ALL_FLAG = SET_0ALL_TRUE;\
                \
                printf("%s => \"%s\"\n", option, toml_discovery_values[count]);\
                \
                return GENERIC_RETURN_SUCCESS;\



#endif

// WireEye 2023 - BRIGHTSTAR SSDP analyzer/manipulation Framework