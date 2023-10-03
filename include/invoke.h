#ifndef INVOKE_H
#define INVOKE_H

void invoke_handler(void);
void invoke_cmd_handler(char *opts[], int token_count);
void invoke_help_handler(void);
void list_ssdp_operations(void);
void memload_ssdp_targets(char *mem_target[], size_t len);
void global_quit(void);

void rl_invoke_memload_primary_array(void);

// tab completetion function prototypes
char **static_cmd_invoke_completion(const char *cmd, int start, int end);
char *static_cmd_invoke_generator(const char *cmd, int state);

#define MAX_TOKENS 8
#define MAX_INPUT_LEN 50
#define SYSTEM_SUBSHELL_VERSION "1.2"

#define MINIMUM_OPTS 2

#define INVOKE_PRIMARY_IDENTIFIER printf("%s Invoking primary identifier => \"%s\"\n", BLUE_OK, opts[0]);
#define INVOKE_INFO_IDENTIFIER printf("%s BRIGHTSTAR::Info => supported command: \"%s\"\n", GREEN_OK, opts[0]);

#endif