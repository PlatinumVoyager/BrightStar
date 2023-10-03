#define _DEFAULT_SOURCE

#include "../include/imports.h"
#include "../include/invoke.h"
#include "../include/console.h"

// commands (path trees)
#include "../include/setmod.h"
#include "../include/fort.h"

char *invoke_command_buffer[] = 
{   
    "help", 
    "?",
    "targets",
    "setmod",
    "back",
    "clear",
    "quit", 
    "q",
    "exit"
};

char *invoke_tab_complete_cmds[sizeof(invoke_command_buffer) / sizeof(invoke_command_buffer[0]) + 1];

int RL_INVOKE_MEMLOADED_STAT = 0;
int LIST_IS_WARRANTED = 1;
int LOADED_BEFORE_MOD = 0;
int EXT_SSDP_ST_LOADED = 0;

extern int BACK_CALLED_FROM_BACKEND;

char *ssdp_targets[MAX_SUPPORTED_TARGETS];


char **static_cmd_invoke_completion(const char *cmd, int start, int end)
{
    rl_attempted_completion_over = 1;

    return rl_completion_matches(cmd, static_cmd_invoke_generator);
}


char *static_cmd_invoke_generator(const char *cmd, int state)
{
    GLOBAL_HANDLE_TAB_COMPLETION(invoke_tab_complete_cmds)
}


void rl_invoke_memload_primary_array(void)
{
    switch (RL_INVOKE_MEMLOADED_STAT)
    {
        case 0:
        {
            goto RL_INIT_MEMLOAD_INVOKE;
        }

        case 1:
        {
            goto RL_NO_MEMLOAD_INVOKE;
        }
    }

    RL_INIT_MEMLOAD_INVOKE:
    {
        int main_check_index = 0;

        // obtain sizes
        size_t console_opts_sz = sizeof(invoke_command_buffer) / sizeof(invoke_command_buffer[0]); // 7
        size_t cmd_sz_final = console_opts_sz + 1; // 14

        for (size_t i = 0; i < cmd_sz_final - 1; i++)
        {
            // set values accordingly
            // get the size of the strings first
            // start with toml_discovery_names
            invoke_tab_complete_cmds[main_check_index] = strdup(invoke_command_buffer[i]);
            main_check_index++;
        }

        // NULL terminate string for usage with 'rl_attempted_completion_function'
        invoke_tab_complete_cmds[cmd_sz_final] = '\0';

        RL_INVOKE_MEMLOADED_STAT = 1;
    }

    RL_NO_MEMLOAD_INVOKE:
    {
        ;;
    }

    return;
}


void invoke_handler(void)
{
    int tokCount = 0; // total token count strtok()
    int hist_size = 0;

    char *command_buffer; // COMMANDLEN len + tokens
    char *commands[MAX_TOKENS]; // support at MOST 8 tokens

    char *prompt_str = "[SUBSHELL:\033[90;1m#SSDP\033[0;m]> ";
    size_t prompt_len = strlen(prompt_str);

    char prompt[prompt_len];
    prompt[0] = '\n';

    switch (BACK_CALLED_FROM_BACKEND)
    {
        case 0:
        {
            // this check makes sure this message below is displayed once the back argument is set
            printf
            (
                "\nINVOKE System Subshell version \033[0;32m%s\033[0;m\n\n",
                SYSTEM_SUBSHELL_VERSION
            );

            printf("HELP/? to display options\n\n");

            EXT_SSDP_ST_LOADED = 0;
        }

        case 1:
        {
            break;
        }

        default:
        {
            break;
        }
    }

    // +3 for decimal size (rare to have over 100+ invoke_command_buffer entered into a single session) + NULL terminator '\0'
    snprintf(prompt, sizeof(prompt) + 3, "%s", prompt_str);

    rl_invoke_memload_primary_array();

    rl_attempted_completion_function = static_cmd_invoke_completion;

    while (1)
    {
        command_buffer = readline(prompt);

        if (command_buffer != NULL)
        {
            if (command_buffer[0] != '\0')
            {
                add_history(command_buffer);
                hist_size++;
            }

            // separate by " " and "\n"
            // divides console input into tokens
            char *args = strtok(command_buffer, " \n");

            while (args != NULL && tokCount < MAX_TOKENS)
            {
                commands[tokCount] = strdup(args);
                args = strtok(NULL, " \n"); // proceed to the next token

                tokCount++;
            }

            if (hist_size > MAX_HIST_SIZE)
            {
                remove_history(where_history());
                hist_size--;
            }

            // free(command_buffer);

            invoke_cmd_handler(commands, tokCount);
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


void invoke_cmd_handler(char *opts[], int token_count)
{
    if (token_count < 1)
    {
        BACK_CALLED_FROM_BACKEND++;
        invoke_handler();
    }

    char *cmd;

    cmd = opts[0];

    size_t cmd_count = 0;
    size_t cmd_size = sizeof(invoke_command_buffer) / sizeof(invoke_command_buffer[0]);

    // check if the search target strings were loaded into memory pre module shell
    if (EXT_SSDP_ST_LOADED > 0)
    {
        // this character array of supported search targets was loaded into memory before actually calling "setmod <module>/<id>" via "targets"
        // or "show targets"
        // the sole purpose is for pretty text formatting :)
        LOADED_BEFORE_MOD = 1;
    }

    // go through all of the possible tokens
    for (size_t i = 0; i < cmd_size; i++)
    {
        if (strcmp(opts[0], invoke_command_buffer[i]) == 0)
        {
            // look for supported targets
            if (strcmp(opts[0], "help") == 0 || strcmp(opts[0], "?") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                invoke_help_handler();
            }

            else if (strcmp(opts[0], "targets") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                LIST_IS_WARRANTED = 1;
                
                list_ssdp_operations();
            }

            else if (strcmp(opts[0], "setmod") == 0)
            {        
                // check for length of arguments passed to setsub
                if (token_count < MINIMUM_OPTS)
                {
                    // dull 0 out (zero out)
                    printf("%s Usage: setmod <module>/list/?\n", BLUE_OK);

                    return;
                }

                else if (token_count > MINIMUM_OPTS)
                {
                    printf("%s BRIGHTSTAR::Error => illegal number of arguments passed to \"%s\"\n", RED_ERR, opts[0]);

                    return;
                }

                else
                {
                    // lazy way to set the primary shell module
                    setmod_cmd_handler(opts[1]);
                }
            }

            else if (strcmp(opts[0], "back") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                BACK_CALLED_FROM_BACKEND = 0;

                invoke_cmd();
            }

            else if (strcmp(opts[0], "clear") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                clear_screen();

                return;
            }

            else if (strcmp(opts[0], "quit") == 0 || strcmp(opts[0], "q") == 0 || strcmp(opts[0], "exit") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING
                
                global_quit();
            }
        }

        else
        {
            cmd_count++;

            if (token_count == 0)
            {
                break;
            }

            if (cmd_count == cmd_size)
            {
                printf("%s BRIGHTSTAR::Error => no targets found for: \"%s\"\n", RED_ERR, opts[0]);

                return;
            }

            continue;
        }
    }

    return;
}


void list_ssdp_operations(void)
{
    // define targets
    char *dummy_searcht[] =
    {
        // common
        "ssdp:all",
        "upnp:rootdevice",

        // DTs (device types)
        // internet gateway device
        "urn:schemas-upnp-org:device:WANConnectionDevice:1",
        "urn:schemas-upnp-org:device:InternetGatewayDevice:1",
        "urn:schemas-upnp-org:device:InternetGatewayDevice:2",
        
        // WAN common interface config
        "urn:schemas-upnp-org:device:WANCommonInterfaceConfig:1",

        // media server
        "urn:schemas-upnp-org:device:MediaServer:1",
        "urn:schemas-upnp-org:device:MediaServer:2",
        "urn:schemas-upnp-org:device:MediaServer:3",
        "urn:schemas-upnp-org:device:MediaServer:4",
        
        // media rendering
        "urn:schemas-upnp-org:device:MediaRenderer:1",
        "urn:schemas-upnp-org:device:MediaRenderer:2",
        "urn:schemas-upnp-org:device:MediaRenderer:3",

        // binary lights
        "urn:schemas-upnp-org:device:BinaryLight:1",

        // dimmable lights
        "urn:schemas-upnp-org:device:DimmableLight:1",

        // temperature sensor
        "urn:schemas-upnp-org:device:TemperatureSensor:1",

        // alarm clocks
        "urn:schemas-upnp-org:device:AlarmClock:1",
        
        // content directory
        "urn:schemas-upnp-org:device:ContentDirectory:1",
        "urn:schemas-upnp-org:device:ContentDirectory:2",
        "urn:schemas-upnp-org:device:ContentDirectory:3",
        "urn:schemas-upnp-org:device:ContentDirectory:4",

        // print queue
        "urn:schemas-upnp-org:device:PrintQueue:1"
    };

    size_t opt_len = sizeof(dummy_searcht) / sizeof(dummy_searcht[0]);

    // will check if loaded into memory, if not loaded either after calling
    // "targets" it will be loaded after calling "set ST <VALUE>"
    if (EXT_SSDP_ST_LOADED == 0)
    {
        // set -> EXT_SSDP_ST_LOADED = 1; after execution
        memload_ssdp_targets(dummy_searcht, opt_len);
    }

    // if it is already loaded into memory
    // and "show targets" or calling "targets" in the INVOKE system subshell
    // is called, display 
    if (LIST_IS_WARRANTED == 1)
    {
        ;;
    }

    // if not, die.
    else 
    {
        return;
    }

    ft_table_t *table = ft_create_table();

    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

    printf("\n");

    ft_write_ln(table, "ID", "SEARCH TARGET");

    for (int i = 0; i < opt_len; i++)
    {
        char ID[3];
        char TARGET[60];

        snprintf(ID, sizeof(ID), "%d", i + 1);
        snprintf(TARGET, sizeof(TARGET), "%s", ssdp_targets[i]);

        ft_set_cell_prop(table, CURRENT_ROW, 1, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_ITALIC);
    
        ft_write_ln(table, ID, TARGET);
    }

    ft_set_border_style(table, FT_SOLID_STYLE);
    printf("%s\n", ft_to_string(table));

    ft_destroy_table(table);

    return;
}


void memload_ssdp_targets(char *mem_target[], size_t len)
{
    printf("%s Loading supported search target strings into memory\n", BLUE_OK);

    for (size_t i = 0; i < len; i++)
    {
        ssdp_targets[i] = strdup(mem_target[i]);

        if (strlen(ssdp_targets[i]) < 1)
        {
            printf("%s BRIGHTSTAR::Error => failed to load \"%s\" into GLOBAL TARGET LISTING!\n", RED_ERR, mem_target[i]);

            exit(0);
        }

        continue;
    }

    // ALL external SSDP supported search targets have been loaded into memory
    EXT_SSDP_ST_LOADED = 1; // set to 1; ++ = should only be executed once  

    printf("%s Target array positioned in memory at %p\n", GREEN_OK, &ssdp_targets);

    return;
}


void global_quit(void)
{
    int i;

    printf("%s Exiting current process", BLUE_OK);
    fflush(stdout);

    for (i = 0; i < 3; i++)
    {
        printf(".");
        fflush(stdout);
    
        sleep(1);
    }

    sleep(0.2);

    printf("\033[0;32mDONE\033[0;m\n");

    exit(0);
}


void invoke_help_handler(void)
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
        "\033[90;1mtargets\033[0;m\t\tdisplay maintained information about operational search targets\n"
        "\n\033[90;1msetmod\033[0;m\n\tset auxiliary module to load for primary operation\n\n"
        "\t\033[90;1mload\033[0;m\t\tload primary configuration information (REQUIRED)\n"
        "\t\033[90;1mlist/?\033[0;m\t\tlist supported importable engagement modules\n"
        "\t\033[90;1m<name>/<id>\033[0;m\tset the primary engagement module/id to use\n"
        "\n\033[90;1mback\033[0;m\t\treturn to the primary operator console (BRIGHTSTAR)\n"
        "\033[90;1mclear\033[0;m\t\tclear the current console buffer (stdout)\n"
        "\033[90;1mquit/q/exit\033[0;m\texit the console application and close stdin\n\n"
    );

    return;
}
