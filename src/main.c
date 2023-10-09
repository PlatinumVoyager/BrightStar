#include "../include/imports.h"
#include "../include/console.h"


// optional define directives
// display help upon './bin/BRIGHTSTAR --help'
void display_help(void);


int main(int argc, char *argv[])
{
    srand(time(NULL));
    setlocale(LC_ALL, "");

    if (argc < 2)
    {
        goto DROP_CONSOLE;
    }

    char *cmd = argv[1];
    char *supported_targets[] = {"--help", "-h", "--console"};

    size_t st_len = sizeof(supported_targets) / sizeof(supported_targets[0]);
    size_t scount = st_len - st_len; // 0

    for (size_t x = 0; x < st_len; x++)
    {
        if (strcmp(cmd, supported_targets[x]) == 0)
        {
            if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0)
            {
                // display operational help support
                display_help();
            }

            else if (strcmp(cmd, "--console") == 0)
            {
                PRINT_IDENTIFIER

                DROP_CONSOLE:

                // drop into sub operational command/control console
                launch_console_instance();
            }

            else 
            {
                return -1;
            }
        }
        else 
        {
            scount++;

            if (scount == st_len)
            {
                printf("%s BRIGHTSTAR::Error => no targets found for: \"%s\"\n", RED_ERR, cmd);

                return -1;
            }

            continue;
        }
        
        // ..
    }


    return 0;
}


/*
    Display functional help utilities

    @param void - no arguments passed
*/
void display_help(void)
{
    size_t version_len = strlen("BRIGHTSTAR v") + strlen(BRIGHTSTAR_VERSION) + strlen(" (WireEye 2023)\n");

    char version[version_len + 1]; // + 1 = NULL terminator '\0'
    snprintf(version, sizeof(version), "BRIGHTSTAR v%s (WireEye 2023)\n", BRIGHTSTAR_VERSION);

    printf("%s\n", version);

    char *display_str = "OPERATIONAL HELP";
    display_str = display_str + '\0';
    
    size_t display_len = strlen(display_str);

    printf("%s\n", display_str);

    for (int x = 0; x < display_len; x++)
    {
        printf("=");

        if (x == display_len - 1)
        {
            break;
        }
    }

    printf("\n\n");

    char *msg_entity = "-h/--help\t\tshow operational support for BRIGHTSTAR\n" \
                       "--console\t\tinteract with operator console through current process\n";

    printf("%s\n", msg_entity);

    return;
}

// WireEye 2023 - BRIGHTSTAR SSDP analyzer/manipulation Framework