#define _DEFAULT_SOURCE

#include "../include/imports.h"
#include "../include/console.h"
#include "../include/invoke.h"

char *commands[] = 
{   
    "help",
    "?",
    "invoke",
    "clear",
    "quit", 
    "q",
    "exit"
};

int INVOKE_COUNT = 0;
int RL_CONSOLE_MEMLOADED_STAT = 0;
extern int BACK_CALLED_FROM_BACKEND;

char *console_tab_complete_cmds[(sizeof(commands) / sizeof(commands[0])) + 1];

// replace each newline character with a 
// NULL terminator
char *detain_newline(char *line)
{
    size_t str_len = strlen(line);

    if (str_len > 0 && line[str_len - 1] == '\n')
    {
        line[str_len - 1] = '\0';
    }

    return line;
}


char *return_banner_quote(char *str)
{
    FILE *fp;

    // 100 maximum quotes, 90 characters in length max each
    char lines[MAX_LINES][90];

    int line_count = 0;
    char line[MAX_LINES];

    srand(time(NULL)); // seed the number generator

    fp = fopen("src/quote/quote_strings.txt", "r");

    if (fp == NULL)
    {
        fprintf(stderr, "%s BRIGHTSTAR::Error => failed to open quote string file. Check location!\n", RED_ERR);

        exit(0);
    }

    while (fgets(line, sizeof(line), fp) != NULL && line_count < MAX_LINES)
    {
        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n')
        {
            // replace the newline with a NULL terminator, stating that we wish to store
            // the affected string within a locally defined character array
            line[len - 1] = '\0';
        }

        strcpy(lines[line_count], line); // copy the line into the new character array
        line_count++;
    }

    fclose(fp);

    if (line_count > 0)
    {
        // random index = line picked
        int index = rand() % line_count;
        str = lines[index];

        return str;
    }

    char *empty = "EMPTY LINE";

    return empty;
}


void clear_screen(void)
{
    size_t ctrl_l = strlen(COMMAND_INVOKE);

    char *cmd_buffer;
    cmd_buffer = (char *) malloc(ctrl_l * sizeof(char));
    
    if (cmd_buffer == NULL)
    {
        exit(0); // failed to allocate enough memory
    }
    
    strncpy(cmd_buffer, COMMAND_INVOKE, ctrl_l);
    system((const char *) cmd_buffer);

    return;
}


void primary_cmd_handler(char *opts[], int token_count)
{
    if (token_count < 1)
    {
        invoke_cmd();
    }

    char *cmd = opts[0];

    size_t cmd_err = 0;
    size_t cmd_len = sizeof(commands) / sizeof(commands[0]);

    // check to see if command is found in the list of supported commands
    for (size_t i = 0; i < cmd_len; i++)
    {
        if (strcmp(cmd, commands[i]) == 0)
        {
            INVOKE_COUNT++; // increment number ONLY by supported commands

            // look for supported targets
            if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                primary_help_handler();
            }

            else if (strcmp(cmd, "invoke") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                BACK_CALLED_FROM_BACKEND = 0;

                PRINT_IDENTIFIER
                invoke_handler();
            }

            else if (strcmp(cmd, "clear") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                // clear console output
                clear_screen();

                return;
            }

            else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0 || strcmp(cmd, "exit") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                global_quit();
            }
        }

        else
        {
            cmd_err++;

            if (strlen(cmd) == 0)
            {
                break;
            }

            else if (cmd_err == cmd_len)
            {
                printf("%s BRIGHTSTAR::Error => no command found under name: \"%s\"\n", RED_ERR, cmd);

                return;
            }

            continue;
        }
    }

    return;
}


void invoke_cmd(void)
{
    int tokCount = 0;
    int hist_size = 0;

    char *cmd_buffer;
    char *commands[8];

    char *prompt_str = "[BRIGHTSTAR:\033[90;1m\033[0;m]> ";
    size_t prompt_sz = strlen(prompt_str) + 2;

    char prompt[prompt_sz];
    snprintf(prompt, prompt_sz, "[BRIGHTSTAR:\033[90;1m%d\033[0;m]> ", INVOKE_COUNT);

    rl_console_memload_primary_array();

    rl_attempted_completion_function = static_cmd_console_completion;

    while (1)
    {
        cmd_buffer = readline(prompt);

        if (cmd_buffer != NULL)
        {
            if (cmd_buffer[0] != '\0')
            {
                add_history(cmd_buffer);
                hist_size++;
            }

            char *args = strtok(cmd_buffer, " \n");

            while (args != NULL && tokCount < 8)
            {
                commands[tokCount] = strdup(args);
                args = strtok(NULL, " \n"); // get the next tokenized input

                tokCount++;
            }

            if (hist_size > MAX_HIST_SIZE)
            {
                remove_history(where_history());
                hist_size--;
            }

            free(cmd_buffer);

            primary_cmd_handler(commands, tokCount);
        }

        else 
        {
            READ_ERR_MSG
            global_quit();
        }

        for (int i = 0; i < tokCount; i++)
        {
            free(commands[i]);
        }

        tokCount = 0;
    }

    return;
}


char **static_cmd_console_completion(const char *cmd, int start, int end)
{
    rl_attempted_completion_over = 1;

    return rl_completion_matches(cmd, static_cmd_console_generator);
}


char *static_cmd_console_generator(const char *cmd, int state)
{
    GLOBAL_HANDLE_TAB_COMPLETION(console_tab_complete_cmds)
}


void rl_console_memload_primary_array(void)
{
    switch (RL_CONSOLE_MEMLOADED_STAT)
    {
        case 0:
        {
            goto RL_INIT_MEMLOAD_CONSOLE;
        }

        case 1:
        {
            goto RL_NO_MEMLOAD_CONSOLE;
        }
    }

    RL_INIT_MEMLOAD_CONSOLE:
    {
        int main_check_index = 0;

        // obtain sizes
        size_t console_opts_sz = sizeof(commands) / sizeof(commands[0]); // 7
        size_t cmd_sz_final = console_opts_sz + 1; // 14

        for (size_t i = 0; i < cmd_sz_final - 1; i++)
        {
            // set values accordingly
            // get the size of the strings first
            // start with toml_discovery_names
            console_tab_complete_cmds[main_check_index] = strdup(commands[i]);
            main_check_index++;
        }

        // NULL terminate string for usage with 'rl_attempted_completion_function'
        console_tab_complete_cmds[cmd_sz_final] = '\0';

        RL_CONSOLE_MEMLOADED_STAT = 1;
    }

    RL_NO_MEMLOAD_CONSOLE:
    {
        ;;
    }

    return;
}


char *get_date_timestamp(char *ts)
{
    char timestamp[15];
    time_t datetime = time(NULL);

    if (datetime == -1)
    {
        fprintf(stderr, "%s Failed to obtain current date from <time.h> time_t struct!\n", RED_ERR);

        return "0";
    }

    struct tm *timenow = localtime(&datetime);

    if (timenow == NULL)
    {
        fprintf(stderr, "%s Failed setting up tm timenow struct for current datetime string format positioning!\n", RED_ERR);

        return "0";
    }

    snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d %s", timenow->tm_hour, timenow->tm_min, timenow->tm_sec, timenow->tm_hour < 12 ? "AM" : "PM");
    ts = timestamp;

    return ts;
}


void launch_console_instance(void)
{
    printf("%s BRIGHTSTAR::Info => Console initialized\n", GREEN_OK);

    time_t datetime = time(NULL);

    if (datetime == -1)
    {
        fprintf(stderr, "%s Failed to obtain current date from <time.h> time_t struct!\n", RED_ERR);

        return;
    }

    struct tm *timenow = localtime(&datetime);

    if (timenow == NULL)
    {
        fprintf(stderr, "%s Failed setting up tm timenow struct for current datetime string format positioning!\n", RED_ERR);

        return;
    }

    char *timestamp = get_date_timestamp(0);
    char *banner_str_quote = return_banner_quote(0);

    char *banner[] = {
            "BRIGHTSTAR v", BRIGHTSTAR_VERSION,
            " (WireEye 2023)", "\n",
            "Console Access Time: \033[92;1m", timestamp, "\033[0;m\n\n",
            "\033[90;1m\"", banner_str_quote, "\"\033[0;m\n\n",
            "HELP/? to display options"
    };

    size_t banner_size = sizeof(banner) /  sizeof(banner[0]);
    size_t banner_len = 0;

    for (size_t i = 0; i < banner_size; i++)
    {
        banner_len += strlen(banner[i]);
    }

    char banner_buffer[banner_len + 1];
    banner_buffer[0] = '\0';

    for (size_t i = 0; i < banner_size; i++)
    {
        strcat(banner_buffer, banner[i]);
    }

    printf("\n%s\n\n", banner_buffer);
    invoke_cmd();

    return;
}

void primary_help_handler(void)
{
    char *help = "\nCOMMANDS\n";
    size_t help_len = strlen(help);

    printf("%s", help);

    for (int i = 0; i < help_len; i++)
    {
        printf("=");

        if (i == help_len - 3)
        {
            break;
        }
    }

    printf(
        "\n\n"
        "\033[90;1mhelp/?\033[0;m\t\tdisplay this message indicator for usage information\n"
        "\033[90;1minvoke\033[0;m\t\tinvoke an actionable primary system subshell\n"
        "\033[90;1mclear\033[0;m\t\tclear the current console buffer (stdout)\n"
        "\033[90;1mquit/q/exit\033[0;m\texit the console application and close stdin\n\n"
    );

    return;
}