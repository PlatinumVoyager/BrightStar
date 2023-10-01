#ifndef SETSUB_H
#define SETSUB_H

#include "../include/toml.h"

void is_toml_loaded(void);
void list_supported_modules(void);
void setmod_cmd_handler(char *opt);
toml_table_t *parse_target_toml(char *file);
void iterate_modules_array(toml_table_t *config);

// no need to declare max supported descriptions, as max modules will max at desc
#define MAX_SUPPORTED_MODULES 5

#define TOML_CONFIG_FILE "src/config/defaultcfg.toml"
#define TOML_MODULE_DEFINITIONS "src/modules/mods.toml"

#define CURRENT_ROW i + 1

#define MAX_SUPPORTED_TARGETS 21

toml_table_t *base_configuration;
toml_table_t *module_information;

#define CONFIG_MSG "\033[100;97m[CONFIG]\033[0;m"

#define DISPLAY_MODULE_INFO printf("\tName: \033[92;1m%s\033[0;m\n", modules[i]); \
                    printf("\tInfo: %s\n\n", module_descriptions[i]); \

#define DISPLAY_STATIC_MSG printf("%s BRIGHTSTAR::Info => Module found. Loading statically stored engagement information:\n\n", GREEN_OK);

#define DISPLAY_MAIN_ALL DISPLAY_STATIC_MSG \
    DISPLAY_MODULE_INFO \

#endif
