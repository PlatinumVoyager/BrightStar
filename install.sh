#!/usr/bin/bash

# if the above import does not work uncomment the 2nd line below this one |
#                                                                         v
##!/bin/bash

# octal \033 green for stdout information
OG="\033[1;32m"

# octal \033[0;m ansi escape sequence
OE="\033[0;m"

argv_0=$1

# create directories that 'git' doesnt
BUILD_DIRS="mkdir modules/obj modules/bin bin obj"
$BUILD_DIRS

MAKE_EXEC="chmod +x install_readline.sh"

echo -e "Making $OG \binstall_readline.sh$OE executable..."
$INSTALL_PARAMS

MAKE_EXEC_CMD=$($MAKE_EXEC)

# check return code
if [[ $? != 0 ]]; then
    echo -e "Failed to make install_readline.sh executable"
    exit 1

else
    echo -e "Done.\n"
fi

START_LIB="bash install_readline.sh"

# if you are reading this source file, call --skip-install=yes
# to skip past sudo 'enter password' prompt when building source files
if [ "$argv_0" == "--skip-install=yes" ]; then
    echo -e "Not installing, rebuilding source...\n"

else
    $START_LIB
fi

echo -e "Building from source...\n"

# build core module files first, then primary executable (BRIGHTSTAR)
BUILD_MODOBJ="make modobject"
$BUILD_MODOBJ

BUILD_MODBUILD="make modbuild"
$BUILD_MODBUILD

BUILD_MAIN="make"
$BUILD_MAIN

if [[ $? != 0 ]]; then
    echo "Failed to build source for BRIGHTSTAR!"
    exit 1

else 
    echo -e "\nDone.\n\nExecute $OG \b./bin/BRIGHTSTAR --help$OE to get started."
    exit 0
fi

# WireEye 2023 - BRIGHTSTAR SSDP analyzer/manipulation Framework
