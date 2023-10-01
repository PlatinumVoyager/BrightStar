#ifndef IMPORTS_H
#define IMPORTS_H

/*
    This file serves as a dynamically allocated header file (include guard)
    that will be utilized to update system wide imports used to give
    BRIGHTSTAR its core/main functionality
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <locale.h>
#include <errno.h>
#include <wchar.h>

#include <readline/readline.h>
#include <readline/history.h>

#define BLUE_OK "[\033[94;1m*\033[0;m]"
#define GREEN_OK "[\033[92;1m+\033[0;m]"
#define RED_ERR "\033[0;31m[-]\033[0;m"
#define ESCAPE "\033[0;m"

#endif