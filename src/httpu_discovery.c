#define _DEFAULT_SOURCE

#include "../include/imports.h"
#include "../include/invoke.h"
#include "../include/console.h"
#include "../include/setmod.h"
#include "../include/httpu_discovery.h"

#include "../include/fort.h"


char *httpu_module_options[] = {
    "help", "?",
    "set",
    "show", "ls", "dir",
    "init", "start", "run",
    "clear",
    "back"
    // not adding "exit", lazy way to force usage of global_quit() <- not defined here
};

// PORT
// SCOPE_ADDRESS
// TIMEOUT
// SSDP MX
// PACKET MAX
// SEARCH TARGET
// MANDATORY EXTENSION

// used to align with description values
// when iterating over the array
char *toml_discovery_names[] = {
    "PORT",
    "MSA",
    "SOCKT",
    "SSDPMX",
    "PMX",
    "ST",
    "MANEXT",
    "RECVMAX"
};

// used for actual data passed to the cli when setting custom OPTIONS
char *httpu_scope_options[] = {
    "PORT",
    "MSA",
    "SOCKT",
    "SSDPMX",
    "PMX",
    "ST",
    "MANEXT", // can't be changed, thus removed as an option, yet left in so "show MANEXT" can be executed
              // why? the character array 'toml_discovery_names' holds 7 adjacent positions within 
              // the virtually allocated process memory, if "MAXEXT" is removed from
              // 'httpu_scope_options' then 'httpu_scope_options' (6) < 'toml_discovery_names' (7)
    "RECVMAX"
};


// store TAB supported commands, if this buffer is not large enough
// produces a segmentation fault for "out of bounds" memory access violations
char *primary_tab_complete_cmds[(SCOPE_DISCOVERY_SIZE * 2) + SCOPE_DISCOVERY_SIZE];

// values obtained from the 'defaultcfg.toml' configuration file
char *toml_discovery_values[8];

// the string value of the TOML configuration options
char *toml_name_description[8] = {
    "primary port of target multicast domain",
    "local administrative multicast scope address for SSDP (HTTP/UDP)",
    "maximum SO_RCVTIMEO (SOL_SOCKET) timeout duration in seconds",
    "maximum interval for server \"wait\" response duration in seconds",
    "the maximum amount of packets to send into the target multicast domain",
    "the engagement string for the root control point via HTTP M-SEARCH header",
    "mandatory extension string for the root control point (must be \"ssdp:discover\")",
    "the amount of unicast SSDP response messages to print to stdout before exiting"
};

char port_f[5]; 
char socktimeout_f[5];
char httpumx_f[5];
char pmx_f[5];
char st_f[60];

// prevents "0 (unlimited)" from appearing next to SSDPM <VALUE> when calling show
int SET_RECVMAX_BOUNDS = 0;

// max of 1000 supported
char recvmx_f[RECEVIED_MAX_BOUNDARY];

// show (unlimited) next to 0 for RECVMAX
int GOT_RECVMAX_0 = 0;

// was "show targets" called? this is used for pretty formatting to stdout
int SHOW_TARGETS_CALLED = 0;

// determines whether or not the supported strings elements
// for TAB completion were loaded into memory or not
int RL_HTTPU_MEMLOADED_STAT = 0;

// determines if a digit is passed as a potentially supported search target string
int SET_DIGIT_MAGIC = 0;

// determines whether or not the ST ID is the first element of the character array if (atoi(st) == 1)
int FIRST_INDEX_SELECTED = 0;

// determines whether or not the operator has modified any values before calling "set 0_ALL"
int USER_NOT_DEFINED = 0;

// to determine whether or not the user is in the subsytem shell or module shell
int BACK_CALLED_FROM_BACKEND = 0;

// memory for the above character arrays
// to determine if subsequent memory was already allocated and filled
int POPULATE_MEM_ALLOCATED = 0;

// used to pretty print when calling "set ST <name>/<id>"
int CALLED_EXT_NOT_LOADED = 0;

// were the search target strings loaded before calling "setmod load"?
// enables proper text stdout (standard output) alignment via '\n' when loading/setting external supported SSDP search targets
extern int LOADED_BEFORE_MOD;

// determines if the ST strings were loading previously through 'ssdp_list_operations()'
extern int EXT_SSDP_ST_LOADED;

// determines whether or not user is setting or listing search targets
extern int LIST_IS_WARRANTED;

// used to determine if a factory reset of all default options was just successful, to block from calling it again
int ZERO_ALL_CALLED = SET_0ALL_FALSE;

// used to determine if the set 0_ALL argument is called before any variables have been set
int SET_0ALL_FLAG = SET_0ALL_FALSE;

// used when loading search target strings into a secondary character array
extern char *ssdp_targets[MAX_SUPPORTED_TARGETS];


/*
    Logic for dealing with 'opts'

    @param opts - characeter array of supported commands (user input)
    @param token_count - the amount of characters within the array
*/
void httpu_cmd_handler(char *opts[], int token_count)
{
    // if <ENTER> was commited to the prompt, return prompt
    if (token_count < 1)
    {
        httpu_discovery_handler();
    }

    if (POPULATE_MEM_ALLOCATED == 0)
    {
        populate_preliminary_options(base_configuration);
    }

    char *cmd, *option, *value;
    cmd = opts[0];

    // re-allocating memory everytime a command is entered, needs to be changed
    // globally across the project as 'httpu_discovery.c' is not the only source
    // file that uses this method
    size_t opt_count, show_count;
    opt_count = show_count = 0;

    size_t opt_size = sizeof(httpu_module_options) / sizeof(httpu_module_options[0]);

    size_t httpu_scope_count = 0; // used to check against incorrect values passed to "set"
    size_t httpu_scope_opt_size = sizeof(httpu_scope_options) / sizeof(httpu_module_options[0]);

    for (size_t i = 0; i < opt_size; i++)
    {
        if (strcmp(cmd, httpu_module_options[i]) == 0)
        {
            if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0)
            {
                // used to tell if the argument has a parameter passed to it with a max count of 1
                PERFORM_SINGULAR_BOUND_CHECKING

                httpu_discovery_return_help();
            }

            else if (strncmp(cmd, "set", strlen(cmd)) == 0)
            {
                if (token_count == HTTPU_SETCMD_MAX - 1 && strcmp(opts[1], "0_ALL") == 0)
                {
                    switch (ZERO_ALL_CALLED)
                    {
                        case SET_0ALL_FALSE:
                        {
                            // assume no values have been modified by default
                            // this will by default throw an error when "set 0_ALL" is called
                            // in order to preserve dynamic memory allocation to the affected
                            // character arrays if the user has not modified a value
                            // basically: why reload ALL values to their default counterparts within the .TOML
                            // configuration file if you'll be setting those same exact values to what was
                            // already loaded and stored in memory at that position in the first place
                            // Ex: if x = x; why call 'x = x' and set its value to x again? (x2)
                            if (SET_0ALL_FLAG == SET_0ALL_FALSE)
                            {
                                // called before any values have been changed from their default counterparts
                                USER_NOT_DEFINED++;

                                goto USER_NDEF_ERR;
                            }

                            // if SET_0ALL_FLAG = 1 (SET_0ALL_TRUE) this means that the operator has modified a value in place
                            // thus allowing them to change all values back to their default counterparts
                            // this is not an efficient solution/technique to adhere to since if the operator changes n x 1 value (n = # of options changed)
                            // and calls "set 0_ALL" EVERY value contained within the defautl .TOML configuration file will be reloaded into their subsequent
                            // character arrays in which virtual memory has already been mapped to physical memory and allocated to the calling process (executable)
                            else if (SET_0ALL_FLAG == SET_0ALL_TRUE)
                            {
                                printf("%s BRIGHTSTAR::Info => set GLOBAL target => \"0_ALL\"\n", BLUE_OK);
                                populate_preliminary_options(base_configuration);

                                ZERO_ALL_CALLED++;

                                break;
                            }

                            // ..
                        }

                        case SET_0ALL_TRUE:

                        USER_NDEF_ERR:
                        {
                            // no values have been changed and "set 0_ALL" was called
                            if (USER_NOT_DEFINED == 1)
                            {
                                printf("%s BRIGHTSTAR::Error => no changes to default configuration values detected\n", RED_ERR);
                                
                                // states that a check will be performed whenever "set 0_ALL" is called
                                // this will get the state of the program and verify if any changes were made
                                USER_NOT_DEFINED--;
                            }

                            // the operator has already called "set 0_ALL"
                            else
                            {
                                printf("%s BRIGHTSTAR::Error => GLOBAL target \"0_ALL\" already set\n", RED_ERR);
                            }

                            printf("\t** Avoiding redundant memory allocation routine\n\n");

                            break;
                        }
                    }

                    return;
                }

                else if (token_count == HTTPU_SETCMD_MAX)
                {
                    option = opts[1];   
                    value = opts[2];

                    // check if OPTION is contained within the 'httpu_scope_options' array
                    for (size_t i = 0; i < httpu_scope_opt_size; i++)
                    {
                        /*
                            "PORT" = SSDP PORT
                            "MSA" = TARGET
                            "SOCKT" = SOCKET TIMEOUT
                            "SSDPMX" = MX
                            "PMX" = PACKET COUNT
                            "ST" = SEARCH TARGET
                        */

                        // check if option is a valid option
                        int SET_CMD_RETURN = httpu_set_cmd_handler(option, value, i);

                        switch (SET_CMD_RETURN)
                        {
                            case GENERIC_RETURN_SUCCESS:
                            {
                                break;
                            }

                            case GENERIC_ERROR_RETURN_NONE:
                            {
                                return;
                            }

                            case ILLEGAL_SET_OPTIONS_PASSED:
                            {
                                httpu_scope_count++;

                                if (httpu_scope_count == httpu_scope_opt_size)
                                {
                                    printf("%s BRIGHTSTAR::Error => illegal usage of command arguments to \"set\"\n", RED_ERR);
                                }

                                continue;
                            }
                        }

                        // end switch
                    }

                    continue;
                }

                else if (token_count == HTTPU_SETCMD_MAX - 1)
                {
                    option = opts[1];

                    for (size_t x = 0; x < httpu_scope_opt_size; x++)
                    {
                        if (strcmp(option, httpu_scope_options[x]) == 0)
                        {
                            if (strcmp(option, "MANEXT") == 0)
                            {
                                printf("%s BRIGHTSTAR::Error => invalid option \"%s\" to \"set\": cannot modify value\n", RED_ERR, option);
                            } 

                            else 
                            {
                                printf("%s BRIGHTSTAR::Error => you must specify a <VALUE> to pass when calling \"%s %s\"\n", RED_ERR, cmd, option);
                            }

                            return;
                        }

                        else
                        {
                            show_count++;

                            if (show_count == httpu_scope_opt_size)
                            {
                                printf("%s BRIGHTSTAR::Error => unknown option passed as argument: \"%s\"\n", RED_ERR, opts[1]);
                            }
                        }

                        continue;
                    }
                }
                else
                {
                    // print command usage
                    printf("%s Usage: set <OPTION> <VALUE>\n", BLUE_OK);

                    return;
                }
            }

            else if (strcmp(cmd, "show") == 0 || strcmp(cmd, "ls") == 0 || strcmp(cmd, "dir") == 0)
            {
                // checks for the 2nd option to "show"
                if (token_count == (HTTPU_SETCMD_MAX - 1))
                {
                    option = opts[1];   
                    value = opts[2];

                    char *option = opts[1];

                    if (strcmp(option, "targets") == 0)
                    {
                        LIST_IS_WARRANTED = 1;
                        list_ssdp_operations();

                        SHOW_TARGETS_CALLED = 1;

                        return;
                    }

                    for (size_t x = 0; x < httpu_scope_opt_size; x++)
                    {
                        if (strcmp(option, httpu_scope_options[x]) == 0)
                        {
                            confer_variable_values(option, x);
                        }

                        else
                        {
                            show_count++;

                            if (show_count == httpu_scope_opt_size)
                            {
                                printf("%s BRIGHTSTAR::Error => unknown option passed as argument: \"%s\"\n", RED_ERR, opts[1]);
                            }
                        }

                        continue;
                    }

                    return;
                }

                // no arguments passed is equivalent to show ALL
                else if (token_count == 1)
                {
                    // detect if this function has already been called once to deter redundant memory allocation
                    switch (POPULATE_MEM_ALLOCATED)
                    {
                        case 0: // memory was not allocated, character arrays are not populated
                        {
                            populate_preliminary_options(base_configuration);
                            display_preliminary_options();

                            POPULATE_MEM_ALLOCATED++;

                            return;
                        }

                        case 1: // character arrays are populated with module information/variables
                        {
                            display_preliminary_options();

                            POPULATE_MEM_ALLOCATED = 1; // cheap way to fix, "show" not showing data since

                            return;
                        }
                    }
                }

                else 
                {
                    printf("%s BRIGHTSTAR::Error => illegal usage of command arguments to \"show\"\n", RED_ERR);
                }

                // ..

                return;
            }

            else if (strcmp(cmd, "init") == 0 || strcmp(cmd, "start") == 0 || strcmp(cmd, "run") == 0)
            {
                // define a generic return message to use
                int ret_msg = module_pre_method_check();

                // verify that all values have been set accordingly and execute the main method of this module (httpu-discovery)
                switch (ret_msg)
                {
                    case GENERIC_RETURN_SUCCESS:
                    {
                        printf("%s BRIGHTSTAR::Info => static iteration over optional values completed\n\n", GREEN_OK);

                        // success, exit and continue
                        break;
                    }

                    case GENERIC_ERROR_RETURN_NONE:
                    {
                        // failed, die.
                        return;
                    }
                }
                
                // call main function handler and supply values needed to operate successfully when called by the operator directly
                // via "init", "start" or "run" respectively
                register_main_module_method();

                return;
            }

            else if (strcmp(cmd, "clear") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                clear_screen();

                return;
            }

            else if (strcmp(cmd, "back") == 0)
            {
                PERFORM_SINGULAR_BOUND_CHECKING

                // if BACK_CALLED_FROM_BACKEND is 0, show banner, else prompt.
                BACK_CALLED_FROM_BACKEND++;
                invoke_handler();
            }
        }

        else 
        {
            switch (token_count)
            {
                case 0:
                    break;

                default:
                {
                    opt_count++;

                    if (opt_count == opt_size)
                    {
                        printf("%s BRIGHTSTAR::Error => no command found under name: \"%s\"\n", RED_ERR, cmd);
                    }

                    continue;
                }
            }
        }

        // .. end for
        return;
    }

    return;
}


/*
    Used by libreadline

    @param cmd - command (literal command)
    @param start - start of ?
    @param end - end of ?
*/
char **static_cmd_httpu_completion(const char *cmd, int start, int end)
{
    rl_attempted_completion_over = 1;

    return rl_completion_matches(cmd, static_cmd_httpu_generator);
}


/*
    @param cmd - commands to store
    @param state - state of readline 'current word'
*/
char *static_cmd_httpu_generator(const char *cmd, int state)
{
    GLOBAL_HANDLE_TAB_COMPLETION(primary_tab_complete_cmds)
}


/*
    Make sure the available TAB completion characters are loaded properly so they can be used
    these include module commands, etc.

    @param void - no arguments passed
*/
void rl_httpu_memload_primary_array(void)
{
    switch (RL_HTTPU_MEMLOADED_STAT)
    {
        case 0:
        {
            goto RL_INIT_MEMLOAD_HTTPU;
        }

        case 1:
        {
            goto RL_NO_MEMLOAD_HTTPU;
        }
    }

    RL_INIT_MEMLOAD_HTTPU:
    {
        int main_check_index = 0;

        // obtain sizes
        size_t toml_names_sz = sizeof(toml_discovery_names) / sizeof(toml_discovery_names[0]); // 7
        size_t httpu_module_opt_sz = sizeof(httpu_module_options) / sizeof(httpu_module_options[0]); // 9

        size_t cmd_sz_final = toml_names_sz + httpu_module_opt_sz; // 14

        for (size_t i = 0; i < toml_names_sz; i++)
        {
            // set values accordingly
            // get the size of the strings first
            // start with toml_discovery_names
            primary_tab_complete_cmds[main_check_index] = strdup(toml_discovery_names[i]);
            main_check_index++;
        }

        // parse and allocate module options to source array (main)
        for (size_t i = 0; i < httpu_module_opt_sz; i++)
        {
            primary_tab_complete_cmds[main_check_index] = strdup(httpu_module_options[i]);
            main_check_index++;
        }

        // NULL terminate string for usage with 'rl_attempted_completion_function'
        primary_tab_complete_cmds[cmd_sz_final + 1] = '\0';

        RL_HTTPU_MEMLOADED_STAT = 1;
    }

    RL_NO_MEMLOAD_HTTPU:
    {
        ;;
    }

    return;
}


/*
    Main command handler

    @param void - no arguments passed
*/
void httpu_discovery_handler(void)
{
    int tokCount = 0;
    int hist_size = 0;

    char *cmd_buffer;
    char *commands[MAX_COMMAND_ARGS];

    // load the targetd character array into memory
    // this character array will contain ONLY valid string members that can
    // be utilized with the TAB completion feature provided by libreadline
    // check if the TAB array is loaded before each call to 'httpu_discovery_handler' (prompt)
    // in order to ensure they can be utilized at runtime
    rl_httpu_memload_primary_array();

    rl_attempted_completion_function = static_cmd_httpu_completion;

    char *prompt = "[MODULE:\033[90;1m#SSDP\033[0;m\033[91;1m(httpu-discovery)\033[0;m]$ ";

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

            while (args != NULL && tokCount < MAX_COMMAND_ARGS)
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

            httpu_cmd_handler(commands, tokCount);
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


/*
    List all current options with their currently set values when calling 'show'

    @param void - no arguments passed
*/
void display_preliminary_options(void)
{
    ft_table_t *table = ft_create_table();
    size_t dstrlen = sizeof(toml_discovery_names) / sizeof(toml_discovery_names[0]);

    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

    ft_write_ln(table, "Option", "Description", "Value");

    // Name, value
    for (size_t i = 0; i < dstrlen; i++)
    {
        size_t name_len = strlen(toml_discovery_names[i]) * 2;
        size_t desc_len = strlen(toml_name_description[i]) * 2;
        size_t value_len = strlen(toml_discovery_values[i]) * 2;

        char name[name_len];
        char desc[desc_len];

        // change signature to support (unlimited) when RECVMAX = 0;
        char *value = NULL;

        snprintf(name, name_len, "%s", toml_discovery_names[i]);
        snprintf(desc, desc_len, "%s", toml_name_description[i]);

        // setup (unlimited)
        if (strcmp(toml_discovery_values[i], "0") == 0 && strcmp(name, "RECVMAX") == 0)
        {
            size_t sz = snprintf(NULL, 0, "%s (unlimited)", toml_discovery_values[i]);

            value = (char *) malloc(sz * sizeof(value));

            if (value == NULL)
            {
                printf("%s BRIGHTSTAR::Error => failed to allocate memory for RECVMAX formatting!\n", RED_ERR);

                return;
            }

            sprintf(value, "%s (unlimited)", toml_discovery_values[i]);
        }

        else 
        {
            value = (char *) malloc (value_len * sizeof(value));

            if (value == NULL)
            {
                printf("%s BRIGHTSTAR::Error => failed to allocate memory for RECVMAX formatting!\n", RED_ERR);

                return;
            }

            snprintf(value, value_len, "%s", toml_discovery_values[i]);
        }

        ft_write_ln(table, name, desc, value);
    }

    ft_set_border_style(table, FT_SIMPLE_STYLE);
    printf("\n%s\n", ft_to_string(table));

    ft_destroy_table(table);

    return;
}


/*
    Load generic (default) options for the httpu-discovery module

    @param config - the TOML configuration file that will load the default/generic values
*/
void populate_preliminary_options(toml_table_t *config)
{
    int pos = 0;

    toml_table_t *default_config = toml_table_in(config, "ssdp_options");

    if (!default_config)
    {
        printf("%s BRIGHTSTAR::Critical => TABLE \"ssdp_options\" WAS NOT FOUND!\n", RED_ERR);

        exit(GENERIC_ERROR_RETURN_NONE);
    }

    toml_datum_t port = toml_int_in(default_config, "port");
    toml_datum_t target = toml_string_in(default_config, "multicast_scope_address");

    toml_datum_t socktimeout = toml_int_in(default_config, "sock_timeout");
    toml_datum_t httpumx = toml_int_in(default_config, "httpumx");
    
    toml_datum_t pmx = toml_int_in(default_config, "packetmx");
    toml_datum_t st = toml_string_in(default_config, "st"); // search target
    toml_datum_t man_ext = toml_string_in(default_config, "manext");

    toml_datum_t recv_max = toml_int_in(default_config, "receivedmax");

    if (!port.ok) { printf("%s BRIGHTSTAR::Error => could not find field \"ssdp_options.port\"\n", RED_ERR); exit(-1); }
    else if (!target.ok) { printf("%s BRIGHTSTAR::Error => could not find field \"ssdp_options.multicast_scope_address\"\n", RED_ERR); exit(-1); }
    else if (!socktimeout.ok){ printf("%s BRIGHTSTAR::Error => could not find field \"ssdp_options.sock_timeout\"\n", RED_ERR); exit(-1); }
    else if (!httpumx.ok) { printf("%s BRIGHTSTAR::Error => could not find field \"ssdp_options.httpumx\"\n", RED_ERR); exit(-1); }
    else if (!pmx.ok) { printf("%s BRIGHTSTAR::Error => could not find field \"ssdp_options.packetmx\"\n", RED_ERR); exit(-1); }
    else if (!st.ok) { printf("%s BRIGHTSTAR::Error => could not find field \"ssdp_options.st\"\n", RED_ERR); exit(-1); }
    else if (!man_ext.ok) { printf("%s BRIGHTSTAR::Error => could not find field \"ssdp_options.manext\"\n", RED_ERR); exit(-1); }
    else if (!recv_max.ok) { printf("%s BRIGHTSTAR::Error => could not find field \"ssdp_options.receivedmax\"\n", RED_ERR); exit(-1); }

    // port integer value
    snprintf(port_f, sizeof(port.u.i), "%ld", port.u.i);
    toml_discovery_values[pos] = port_f; pos++;

    // target string value
    toml_discovery_values[pos] = target.u.s; pos++;

    // socket timeout option
    snprintf(socktimeout_f, sizeof(socktimeout_f), "%ld", socktimeout.u.i);
    toml_discovery_values[pos] = socktimeout_f; pos++;

    // mx ssdp controller wait interval
    snprintf(httpumx_f, sizeof(httpumx), "%ld", httpumx.u.i);
    toml_discovery_values[pos] = httpumx_f; pos++;
    
    // maxmimum packet count 
    snprintf(pmx_f, sizeof(pmx_f), "%ld", pmx.u.i);
    toml_discovery_values[pos] = pmx_f; pos++;
    
    // search target
    snprintf(st_f, sizeof(st_f), "%s", st.u.s);
    toml_discovery_values[pos] = st_f; pos++;

    // mandatory extension (cannot be changed)
    toml_discovery_values[pos] = man_ext.u.s; pos++;

    // received maximum amount of responses
    snprintf(recvmx_f, sizeof(recvmx_f), "%ld", recv_max.u.i);
    toml_discovery_values[pos] = recvmx_f; pos++;

    // proof of memory allocation success, values have been stored successfully
    POPULATE_MEM_ALLOCATED = 1;

    return;
}


// this function is responsible for actually
// invoking the module (binary) that was built
// by the operator before calling ./bin/BRIGHTSTAR

/*
    @param void - no arguments passed
*/
void register_main_module_method(void)
{
    // Mnemonic             Index (toml_discovery_values)
    // ========             =============================
    // PORT                 0
    // SCOPE_ADDRESS        1
    // TIMEOUT              2
    // SSDP MX              3
    // PACKET MAX           4
    // SEARCH TARGET        5
    // MANDATORY EXTENSION  6   (SKIP)
    // RECVMAX              7

    // allocate space for sys command
    char *msa = toml_discovery_values[TOML_DISCOVERY_SCOPE_ADDR]; char *port = toml_discovery_values[TOML_DISCOVERY_PORT]; 
    char *st = toml_discovery_values[TOML_DISCOVERY_ST]; char *mx = toml_discovery_values[TOML_DISCOVERY_SSDPMX];

    char *packet_mx, *sockt_opt, *recv_max;
    
    packet_mx = toml_discovery_values[TOML_DISCOVERY_PMX]; // packet maximum to send
    sockt_opt = toml_discovery_values[TOML_DISCOVERY_TIMEOUT]; // socket timeout option
    recv_max = toml_discovery_values[TOML_DISCOVERY_RECVMAX]; // maximum responses option

    char *target = PRIME_MODULE_LOCATION; 
    char *opts_msg = "CURRENT OPTIONS";

    size_t opts_sz = strlen(opts_msg); // '\0'

    printf("%s\n", opts_msg);

    for (int i = 0; i < opts_sz; i++)
    {
        printf("=");
    }

    printf("\n\n");

    // generic table buffer, not important enough to generate full tables with libfort
    printf("\tName\t\tValue\n");
    printf("\t----\t\t-----\n");
    printf("\tMSA\t\t%s\n\tPORT\t\t%s\n\tST\t\t%s\n\tSSDPMX\t\t%s\n\tPMX\t\t%s\n\tTIMEOUT\t\t%s\n\tRECVMAX\t\t%s\n\n", msa, port, st, mx, packet_mx, sockt_opt, recv_max);

    size_t final_sz = strlen(target) + strlen(msa) + strlen(port) + strlen(st) + strlen(mx) + strlen(packet_mx) + strlen(sockt_opt) + strlen(recv_max) + 1;

    char *cmd = (char *) malloc(final_sz * sizeof(cmd));

    if (cmd == NULL)
    {
        printf("%s BRIGHTSTAR::Error => failed to execute module!\n", RED_ERR);

        return;
    }

    snprintf(cmd, final_sz * 2, "%s %s %s %s %s %s %s %s", target, msa, port, st, mx, packet_mx, sockt_opt, recv_max);    
    printf("%s Executing => \"./%s\"\n", GREEN_OK, cmd);

    system((const char *) cmd);
    free(cmd);

    return;
}


/*
    @param option - the target option to switch values from
    @param value - the value passed after option
    @param count - the current generic index used when parsing character arrays
*/
int httpu_set_cmd_handler(char *option, char *value, int count)
{
    if (strcmp(option, httpu_scope_options[count]) == 0)
    {
        if (strcmp(option, "PORT") == 0)
        {
            PRE_CONDITIONAL_VALUE_CHECK

            if (atoi(value) < 1024 || atoi(value) > 65535)
            {
                printf("%s BRIGHTSTAR::Error => target PORT \"%s\" not within range of 1024 - 65535!\n", RED_ERR, value);

                return GENERIC_ERROR_RETURN_NONE;
            }

            SET_STATIC_OPTION_BOUNDRY
        }

        else if (strcmp(option, "MSA") == 0)
        {
            PRE_CONDITIONAL_VALUE_CHECK

            // need to make a bounds checking function for the multicast scope address value
            // instead of setting it directly like below
            if (strlen(value) > 0)
            {
                SET_STATIC_OPTION_BOUNDRY
            }

            else 
            {
                printf("%s BRIGHTSTAR::Error => can not set value to len of 0!\n", RED_ERR);

                return GENERIC_ERROR_RETURN_NONE;
            }
        }

        else if (strcmp(option, "SOCKT") == 0)
        {
            PRE_CONDITIONAL_VALUE_CHECK

            if (atoi(value) > SOCKT_BOUNDARY_MAX)
            {
                printf("%s BRIGHTSTAR::Error => setsockopt value is over boundary: %s > 999 (SOCKT_BOUNDARY_MAX)\n", RED_ERR, value);

                return GENERIC_ERROR_RETURN_NONE;
            }

            SET_STATIC_OPTION_BOUNDRY
        }

        else if (strcmp(option, "SSDPMX") == 0)
        {
            PRE_CONDITIONAL_VALUE_CHECK

            if (atoi(value) > SSDP_MX_MAX_VALUE)
            {
                printf("%s BRIGHTSTAR::Error => target SSDPMX value \"%s\" > %d!\n", RED_ERR, value, SSDP_MX_MAX_VALUE);

                return GENERIC_ERROR_RETURN_NONE;
            }

            SET_STATIC_OPTION_BOUNDRY
        }

        else if (strcmp(option, "PMX") == 0)
        {
            PRE_CONDITIONAL_VALUE_CHECK

            if (atoi(value) <= 0)
            {
                printf("%s BRIGHTSTAR::Error => foreign value of \"%s\" passed as argument. Not allowed\n", RED_ERR, value);

                return GENERIC_ERROR_RETURN_NONE;
            }

            // no packet limit
            SET_STATIC_OPTION_BOUNDRY
        }

        else if (strcmp(option, "ST") == 0)
        {
            PRE_CONDITIONAL_VALUE_CHECK

            // check if search target is within bounds of supported ssdp_targets
            // in order to do this we need to make sure that the supported search targets are loaded
            // into the calling processes virtual memory, everytime 'list_ssdp_operations()' is called
            // it will ALWAYS check to verify/see that such character arrays are loaded
            LIST_IS_WARRANTED = 0;

            // what if the user calls "set ST <value>" before calling "show"
            // this is handled within 'invoke.c' via a check to EXT_SSDP_ST_LOADED
            // loads SSDP supported search target strings into memory if not already loaded before
            // by calling "list_ssdp_operations"
            int determine_st_path = set_search_target_opt(value);

            switch (LOADED_BEFORE_MOD)
            {
                // it was not previously loaded before entering the current shell
                case 0:
                {
                    // when calling 'set_search_target_opt' -> 'ssdp_list_operations' will load search target
                    // strings into memory and set EXT_SSDP_ST_LOADED at the end of the function on success
                    if (EXT_SSDP_ST_LOADED == 1)
                    {
                        // this is pretty much just a generic intercept of stdout to 
                        // obtain a formatted (visually appealing) state

                        // if it is already loaded do not print a newline
                        // just exit
                        if (CALLED_EXT_NOT_LOADED > 0)
                        {
                            break;
                        }

                        if (SHOW_TARGETS_CALLED)
                        {
                            break;
                        }

                        // only shown if the ssdp supported search target strings have not been loaded before calling "set ST <VALUE>"
                        printf("\n");

                        CALLED_EXT_NOT_LOADED = 1;

                        break;
                    }
                }

                // if the module was loaded before, dont print \n, just break
                default:
                {
                    // if already loaded break, no need to add a redundant newline to stdout
                    break;
                }
            }

            // only on string values
            if (determine_st_path == GENERIC_RETURN_SUCCESS)
            {
                // a valid search target exists within the argument passed
                // search targets were not loaded prior to calling "set ST <VALUE>"
                SET_STATIC_OPTION_BOUNDRY
            }

            // operator passed itegral type instead of string 'word', value is of n'th index of 'ssdp_targets'
            else if (determine_st_path > FIRST_INDEX_SELECTED && determine_st_path <= (sizeof(ssdp_targets) / sizeof(ssdp_targets[0])))
            {
                toml_discovery_values[count] = '\0'; 
                toml_discovery_values[count] = strdup(ssdp_targets[determine_st_path]);
                
                FINISH_TOML_DISCOVERY_OPTS
            }

            // user set ST ID to 1
            else if (determine_st_path == SEARCH_TARGET_FIRST)
            {
                toml_discovery_values[count] = '\0'; 
                toml_discovery_values[count] = strdup(ssdp_targets[FIRST_INDEX_SELECTED]);
                
                FINISH_TOML_DISCOVERY_OPTS
            }

            // error
            else if (determine_st_path < 0)
            {
                // search target does not exist
                return GENERIC_ERROR_RETURN_NONE;
            }
        }

        else if (strcmp(option, "MANEXT") == 0)
        {
            printf("%s BRIGHTSTAR::Error => passing value \"%s\" to non-modifiable option \"MANEXT\"\n", RED_ERR, value);

            return GENERIC_ERROR_RETURN_NONE;
        }

        // RECEIVED MAX
        else if (strcmp(option, "RECVMAX") == 0)
        {
            SET_RECVMAX_BOUNDS = 1;

            PRE_CONDITIONAL_VALUE_CHECK

            if (atoi(value) < 0 || atoi(value) > RECEVIED_MAX_BOUNDARY || !isdigit(value[0]))
            {
                printf("%s BRIGHTSTAR::Error => invalid argument of \"%s\" passed to \"RECVMAX\" was detected. Value is non 0 or above\n", RED_ERR, value);

                return GENERIC_ERROR_RETURN_NONE;
            }

            SET_STATIC_OPTION_BOUNDRY
        }
    }

    else 
    {
        // operator supplied unsupported argument to "set"
        return ILLEGAL_SET_OPTIONS_PASSED;
    }

    return GENERIC_RETURN_SUCCESS;
}


/*
    Logic for showing specific options with their values set/shown

    @param opt - the option specified when calling 'show'
    @param index - the current index of toml_discovery_values
*/
void confer_variable_values(char *opt, size_t index)
{
    // "PORT" = SSDP PORT
    // "MSA" = TARGET
    // "SOCKT" = SOCKET TIMEOUT
    // "SSDPMX" = MX
    // "PMX" = PACKET COUNT
    // "ST" = SEARCH TARGET

    if (strcmp(opt, "PORT") == 0)
    {
        opt = "SSDP PORT";
    }

    else if (strcmp(opt, "MSA") == 0)
    {
        opt = "MULTICAST SCOPE ADDRESS";
    }

    else if (strcmp(opt, "SOCKT") == 0)
    {
        opt = "SOCKET TIMEOUT";
    }

    else if (strcmp(opt, "SSDPMX") == 0)
    {
        opt = "SSDP MAX INTERVAL";
    }

    else if (strcmp(opt, "PMX") == 0)
    {
        opt = "PACKET COUNT";
    }

    else if (strcmp(opt, "ST") == 0)
    {
        opt = "SEARCH TARGET";
    }

    else if (strcmp(opt, "RECVMAX") == 0)
    {
        opt = "RECEIVED MAX";

        // got option, set value for display (unlimited)
        if (strcmp(toml_discovery_values[TOML_DISCOVERY_RECVMAX], "0") == 0)
        {
            GOT_RECVMAX_0 = 1;
        }
    }

    size_t opt_size = strlen(opt);

    printf("%s\n", opt);

    for (size_t i = 0; i < opt_size; i++)
    {
        printf("=");

        if (i == opt_size)
        {
            break;
        }
    }

    printf("\n");

    ft_table_t *table = ft_create_table();
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);

    ft_write_ln(table, "Description", "Value");

    // Description
    ft_set_cell_prop(table, 0, 0, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_UNDERLINED);
    ft_set_cell_prop(table, 0, 0, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

    // Value
    ft_set_cell_prop(table, 0, 1, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_UNDERLINED);
    ft_set_cell_prop(table, 0, 1, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

    char *desc = toml_name_description[index];
    char *value = NULL;

    // for formatting RECVMAX (unlimited) when value = 0
    switch (GOT_RECVMAX_0)
    {
        case 0:
        {
            value = toml_discovery_values[index];

            break;
        }

        case 1:
        {
            char *current_value = toml_discovery_values[TOML_DISCOVERY_RECVMAX]; // TOML_DISCOVERY_RECVMAX = 7

            size_t sz = snprintf(NULL, 0, "%s (unlimited)", current_value);

            value = (char *) malloc(sz * sizeof(value));

            if (value == NULL)
            {
                printf("%s BRIGHTSTAR::Error => failed to allocate memory for RECVMAX formatting!\n", RED_ERR);

                return;
            }

            sprintf(value, "%s (unlimited)", current_value);

            GOT_RECVMAX_0--;

            break;
        }
    }

    ft_write_ln(table, desc, value);

    ft_set_border_style(table, FT_EMPTY2_STYLE);
    printf("%s", ft_to_string(table));

    ft_destroy_table(table);

    return;
}


/*
    Logic for handling integral values or string based values for the search target

    @param st - search target string or ID passed to 'set ST <VALUE>'
*/
int set_search_target_opt(char *st)
{
    // load all supported search targets into memory if not already loaded
    // this SHOULD be run first
    list_ssdp_operations();

    int table_count = 0;
    size_t table_len = sizeof(ssdp_targets) / sizeof(ssdp_targets[0]) + 1;

    // st_match must MATCH the size of 'st_sz'
    size_t st_match = 0;

    // prevent "set ST 3<characters here>" from registering the nth (3) index for 'ssdp_targets'
    size_t st_sz = strlen(st);

    if (isdigit(st[0]))
    {
        // got ID (potentially), received integer from string
        // potentially because the first index of st is an integer but the 2nd index
        // of st may be a char type
        SET_DIGIT_MAGIC = 1;
    }

    for (size_t i = 0; i < table_len; i++)
    {
        switch (SET_DIGIT_MAGIC)
        {
            // no ID was passed, assume regular string
            case 0:
            {
                if (strcmp(st, ssdp_targets[i]) == 0)
                {
                    // operator defined search target was found in supported search target character array
                    return GENERIC_RETURN_SUCCESS;
                }

                else 
                {
                    // search target did not match the i'th index of 'ssdp_targets'
                    table_count++;

                    if (table_count == table_len)
                    {
                        printf("%s BRIGHTSTAR::Error => failed to find supported search target: \"%s\" within bounds\n", RED_ERR, st);

                        return GENERIC_ERROR_RETURN_NONE;
                    }

                    continue;
                }
            }

            // ID was passed/detected and picked up by internal logic below
            case 1:
            {
                SET_DIGIT_MAGIC--;

                // check if every character passed as the 'st' is of an integral data type
                for (size_t i = 0; i < st_sz; )
                {
                    if (isdigit(st[i]))
                    {
                        st_match++;
                    }
                    else 
                    {
                        printf("%s BRIGHTSTAR::Error => the argument \"%s\" passed to \"set\" contains invalid characters\n", RED_ERR, st);

                        return GENERIC_ERROR_RETURN_NONE;
                    }

                    if (st_match == st_sz)
                    {
                        // all numeric is a pass
                        // ST ID is greater than the amount of supported search targets
                        if (atoi(st) > table_len || atoi(st) == 0)
                        {
                            printf("%s BRIGHTSTAR::Error => integral search target ID \"%d\" is out of bounds\n", RED_ERR, atoi(st));

                            return GENERIC_ERROR_RETURN_NONE;
                        }

                        else 
                        {
                            break;
                        }
                    }

                    i++;
                }

                // st - 1 since character array index actually starts at 0, n - 1 = actual value
                if (strcmp(ssdp_targets[atoi(st - 1)], ssdp_targets[i]) == 0)
                {
                    // requested last member of ssdp_targets                    
                    if (atoi(st) == table_len)
                    {
                        return SEARCH_TARGET_LAST;
                    }

                    // first index of array selected
                    else if (atoi(st) == 1)
                    {
                        return SEARCH_TARGET_FIRST;
                    }

                    // if the ST ID is not 1, or last index n
                    // so if its in between the first and last index
                    else if (atoi(st) > 1 && atoi(st) <= table_len)
                    {
                        return (atoi(st) - 1);
                    }
                }

                else 
                {
                    // search target did not match the i'th index of 'ssdp_targets'
                    table_count++;

                    if (table_count >= table_len)
                    {
                        printf("%s BRIGHTSTAR::Error => failed to find supported search target: \"%s\" within bounds\n", RED_ERR, st);

                        return GENERIC_ERROR_RETURN_NONE;
                    }
                }
            }
        }

        continue;
    }

    return 0;
}


/*
    Checks to see if all values within 'toml_discovery_values' are ! NULL

    @param void - no arguments passed
*/
int module_pre_method_check(void)
{
    // total amount of options that need to be checked
    size_t discovery_sz = sizeof(toml_discovery_values) / sizeof(toml_discovery_values[0]);

    for (size_t i = 0; i < discovery_sz; i++)
    {
        if (strlen(toml_discovery_values[i]) > 0)
        {
            // a value exists, do nothing
            ;;
        }

        else 
        {
            printf("%s BRIGHTSTAR::Error => could not find value for \"%s\"\n", RED_ERR, toml_discovery_names[i]);

            return GENERIC_ERROR_RETURN_NONE;
        }
    }

    return GENERIC_RETURN_SUCCESS;
}


/*
    Show operational help utilities

    @param void - no arguments passed
*/
void httpu_discovery_return_help(void)
{
    char *str = "COMMANDS";
    size_t str_len = strlen(str);

    printf("\n%s\n", str);

    for (size_t i = 0; i < str_len; i++)
    {
        printf("=");

        if (i == str_len)
        {
            break;
        }
    }

    printf("\n\n");

    printf(
        "\033[90;1mhelp/?\033[0;m\tdisplay this inline module information buffer to stdout\n"
        "\033[90;1mset\033[0;m\n\tset \033[0;3mOPTION\033[0;m to operator supplied \033[0;3mVALUE\033[0;m (i.e, \"set <OPTION> <VALUE>\")\n\n"
        "\t\033[90;1m0_ALL\033[0;m\treturn all fields to their default values\n\n"
        "\033[90;1mshow/ls/dir\033[0;m\n\t\tshow available default current module values or \"show <OPTION>\"\n\n"
        "\t\033[90;1mtargets\033[0;m\t\tshow supported SSDP header search targets\n"
        "\t\033[90;1m<OPTION>\033[0;m\tdisplay information about the targeted modules \033[0;3mOPTION\033[0;m\n\n"
        "\033[90;1mclear\033[0;m\t\tclear the current console buffer (stdout)\n"
        "\033[90;1mback\033[0;m\t\treturn to the invoke SUBSHELL console (BRIGHTSTAR:#SSDP)\n"
        "\033[90;1minit/start/run\033[0;m\tstage the primary execution method (main module) to an \033[0;3mACTIVE\033[0;m state\n\n"
    );

    return;
}

// WireEye 2023 - BRIGHTSTAR SSDP analyzer/manipulation Framework