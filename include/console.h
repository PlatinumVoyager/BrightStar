#ifndef CONSOLE_H
#define CONSOLE_H

// function prototypes
void primary_cmd_handler(char *opts[], int token_count);
void primary_help_handler(void);

void launch_console_instance(void);
void invoke_cmd(void);
void clear_screen(void);
char *detain_newline(char *line);
char *return_banner_quote(char *str);
char *get_date_timestamp(char *ts);

void rl_console_memload_primary_array(void);

// tab completetion function prototypes
char **static_cmd_console_completion(const char *cmd, int start, int end);
char *static_cmd_console_generator(const char *cmd, int state);

#define BRIGHTSTAR_VERSION "1.0"
#define COMMAND_INVOKE "$(which clear)"

#define MAX_LINES 100
#define MAX_COMMAND_ARGS 8
#define MAX_CMD_BUFFER_LEN 100

#define READ_ERR_MSG fprintf(stderr, "%s BRIGHTSTAR::Error => fgets() read error\n", RED_ERR);

#define MAX_HIST_SIZE MAX_LINES

// this bit might just get a bit annoying now that I'm looking at it from stdout
// possibly remove in the future (public build)
#define PRINT_IDENTIFIER printf("%s Invoking primary identifier => \"%s\"\n", BLUE_OK, cmd);
#define PRINT_INFO_IDENTIFIER printf("%s BRIGHTSTAR::Info => supported command: \"%s\"\n", GREEN_OK, commands[i]);

#define GLOBAL_TAB_LOAD_STAT printf("Loading supported TAB strings into memory for: %s (ln:%d)\n", __FILE__, __LINE__);

#define PERFORM_SINGULAR_BOUND_CHECKING if (token_count > 1) \
                                        { \
                                            printf \
                                            ( \
                                                RED_ERR \
                                                " BRIGHTSTAR::Error => tried to pass single variable \"%s\" to singular argument \"%s\"\n", opts[1], cmd);\
                                            \
                                            return;\
                                        }\
                                        else \
                                        {\
                                            ;;\
                                        }\

#define GLOBAL_HANDLE_TAB_COMPLETION(tab_completion_cmd)\
                static int index, sz;\
                char *cmd_passed;\
                \
                if (!state)\
                {\
                    index = 0;\
                    sz = strlen(cmd);\
                }\
                \
                while ((cmd_passed = tab_completion_cmd[index++]))\
                {\
                    if (strncmp(cmd_passed, cmd, sz) == 0)\
                    {\
                        return strdup(cmd_passed);\
                    }\
                }\
                \
                return NULL;\

#endif