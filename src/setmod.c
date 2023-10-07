#include "../include/imports.h"
#include "../include/setmod.h"
#include "../include/httpu_discovery.h"

#include "../include/fort.h"
#include <stdio.h>


char *setmod_options[] = {
    "list", "?", "load"
};

char modules[MAX_SUPPORTED_MODULES][20];
char module_descriptions[MAX_SUPPORTED_MODULES][100];

size_t module_size = 0;

int IS_SETMOD_LOAD = 0; // this determines whether or not "setmod load" was called before "setmod list" 
int IS_TOML_LOADED = 0; // defines whether or not the .toml files were loaded
                        // only set to '1' if BOTH files have been loaded into memory (post parsed)

char *mod_print = "SUPPORTED MODULES";
char *mod_info = "MODULE INFORMATION";

toml_table_t *base_configuration;
toml_table_t *module_information;


/*
    Handles singular arguments passed to setmod respectively

    @param opt - the option passed to 'setmod'
*/
void setmod_cmd_handler(char *opt)
{   
    size_t sub_count = 0, mod_count = 0;
    size_t options_size = sizeof(setmod_options) / sizeof(setmod_options[0]);

    for (size_t i = 0; i < options_size; i++)
    {
        if (strcmp(opt, setmod_options[i]) == 0)
        {
            if (strcmp(opt, "list") == 0 || strcmp(opt, "?") == 0)
            {   
                switch (IS_SETMOD_LOAD)
                {
                    case 0:
                    {
                        // configuration files were not loaded, user exec: "setmod list" without calling "setmod load"
                        printf
                        (
                            RED_ERR 
                            " BRIGHTSTAR::Error => failed to find valid TOML configuration data in memory\n"
                            "\tTry executing => \"setmod load\" first\n"    
                        );

                        return;
                    }

                    case 1:
                    {
                        // file was loaded "setmod load" called before "setmod list"
                        list_supported_modules();

                        return;
                    }
                }
            }

            else if (strcmp(opt, "load") == 0)
            {
                // call function once, to check if the default configuration files are loaded
                // need to only load on a VALID option passed or at startup
                is_toml_loaded();             
                IS_SETMOD_LOAD = 1;

                return;
            }
        }

        // if opt was not a supported command, assume module invocation
        else
        {
            // not a command, must be a module
            // possibly just make a function and dump the code following this into it
            // this does not scale well with multiple modules in future support/release builds
            if (strcmp(modules[atoi(opt) - 1], "httpu-discovery") == 0)
            {
                DISPLAY_MAIN_ALL

                httpu_discovery_handler();
            }

            for (size_t i = 0; i < module_size; i++)
            {
                if (strcmp(opt, modules[i]) == 0)
                {
                    DISPLAY_MAIN_ALL

                    // call function to actually start operating on the invoked module
                    if (strcmp(opt, "httpu-discovery") == 0)
                    {
                        httpu_discovery_handler();
                    }

                    return;
                }

                else
                {
                    mod_count++; // module not found

                    if (mod_count == module_size)
                    {
                        break;
                    }
                }

                continue;
            }

            sub_count++; // first cmd[0] was not found within supported cli option bounds

            if (sub_count == options_size)
            {
                printf("%s BRIGHTSTAR::Error => no targets found for: \"%s\"\n", RED_ERR, opt);

                return;
            }

            continue;
        }
    }

    return;
}


/*
    Checks if the TOML configuration files have been loaded into memory or not before calling 'setmod list'

    @param void - no arguments passed
*/
void is_toml_loaded(void)
{
    // PURPOSE: to determine if BOTH configuration files have been loaded successfully or not
    switch (IS_TOML_LOADED)
    {
        case 0:
        {
            printf("%s BRIGHTSTAR::Info => Loading configuration files from primary source location on FS (file system)\n", BLUE_OK);

            // attempt to load both files into memory
            base_configuration = parse_target_toml(TOML_CONFIG_FILE);

            if (base_configuration == NULL)
            {
                // could not load/parse primary configuration file (abort)
                printf("%s BRIGHTSTAR::Error => could not load primary configuration file! Check/install \'src/config/defaultcfg.toml\'", RED_ERR);

                exit(-1);
            }

            else 
            {
                // load module support file
                module_information = parse_target_toml(TOML_MODULE_DEFINITIONS);

                if (module_information == NULL)
                {
                    // could not load/parse primary configuration file (abort)
                    printf("%s BRIGHTSTAR::Error => could not load module configuration file! Check/install \'src/modules/mod.toml\'", RED_ERR);

                    exit(-1);
                }

                // load modules along with primary names/definitions to populate character arrays
                iterate_modules_array(module_information);
                printf("\n");

                // both files have been loaded successfully
                IS_TOML_LOADED++;

                break;
            }
        }

        case 1:
        {
            // both files have been successfully loaded into memory for usage
            printf("%s BRIGHTSTAR::Info => \"setmod load\" no configuration files to load\n", GREEN_OK);

            break;
        }
    }

    return;
}


/*
    Lists the available modules when TOML configuration files are loaded into memory

    @param void - no arguments passed
*/
void list_supported_modules(void)
{
    ft_table_t *table = ft_create_table();

    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

    ft_write_ln(table, "ID", "Name", "Description");

    for (size_t i = 0; i < module_size; i++)
    {
        char ID[2];
        char module[20];
        char desc[100];

        // ID buffer, mod_name, mod_desc
        snprintf(ID, sizeof(ID), "%ld", i + 1);
        snprintf(module, sizeof(module), "%s", modules[i]);
        snprintf(desc, sizeof(desc), "%s", module_descriptions[i]);

        if (strlen(modules[i]) < 1)
        {
            ft_set_border_style(table, FT_SIMPLE_STYLE);
            printf("\n%s\n", ft_to_string(table)); 

            return;
        }

        else 
        {
            ft_set_cell_prop(table, CURRENT_ROW, 0, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);
            ft_write_ln(table, ID, module, desc);
        }
    }

    ft_destroy_table(table);

    return;
}


// load static configuration files =>
//           1. '../src/config/defaultcfg.toml'
//           2. '../src/modules/mod.toml'

/*
    Parses the current passed argument for 'file'

    @param file - target TOML configuration file
*/
toml_table_t *parse_target_toml(char *file)
{
    FILE *target;
    char errbuff[200];

    if ((target = fopen(file, "r")) == NULL)
    {
        printf("%s BRIGHTSTAR::Error => could not obtain a proper handle to the file \'%s\'\n", RED_ERR, file);

        return NULL;
    }

    printf("\t%s Loaded TOML data stream from file \"%s\" successfully\n", CONFIG_MSG, file);

    toml_table_t *config = toml_parse_file(target, errbuff, sizeof(errbuff));

    fclose(target);

    if (!config)
    {
        // failed to open configuration file (.toml)
        printf("%s BRIGHTSTAR::Error => %s\n", RED_ERR, errbuff);

        return NULL;
    }

    return config;
}


// must have a preloaded toml configuration file 'sitting' in memory in order to 
// call this function
// designed specifically for iterating through the supported modules offered by BRIGHTSTAR

/*
    Load TOML module configuration into memory

    @param config - no arguments passed
*/
void iterate_modules_array(toml_table_t *config)
{
    toml_table_t *modules_array = toml_table_in(config, "modules_supported");

    if (!modules_array)
    {
        printf("%s BRIGHTSTAR::Error => configuration file exists but could not parse \"modules.supported\"! (success != 0)\n", RED_ERR);

        exit(-1);
    }

    toml_array_t *supported_modules = toml_array_in(modules_array, "current");

    if (!supported_modules)
    {
        printf("%s BRIGHTSTAR::Error => could not find/parse \"current\" array field under \"modules.supported\" table!\n", RED_ERR);

        exit(-1);
    }

    for (int i = 0; ; i++)
    {
        toml_datum_t module = toml_string_at(supported_modules, i);

        if (!module.ok)
        {
            break;
        }

        // use strtok to tokenize values based on a targets separator ":"
        // thus saving both module name and descriptions into separate character arrays

        // set to module name
        char *index = strtok(module.u.s, ":");

        if (index != NULL && i < MAX_SUPPORTED_MODULES)
        {   
            // copy module name into array
            strncpy(modules[i], index, sizeof(modules[i]) - 1);
            modules[i][sizeof(modules[i]) - 1] = '\0';

            index = strtok(NULL, ":"); // skip to the next token

            // if a description exists within the TOML file
            if (index != NULL)
            {
                // copy module descriptiton into array
                strncpy(module_descriptions[i], index, sizeof(module_descriptions[i]) - 1);
                module_descriptions[i][sizeof(module_descriptions[i]) - 1] = '\0';
            }
        }
    }

    module_size = sizeof(modules) / sizeof(modules[0]);

    return;
}

// WireEye 2023 - BRIGHTSTAR SSDP analyzer/manipulation Framework