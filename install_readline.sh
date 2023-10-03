#!/usr/bin/bash

# if the above import does not work uncomment the 2nd line below this one |
#                                                                         v
##!/bin/bash

argv_0=$1

# octal \033 green for stdout information
OG="\033[1;32m"

# octal red for error
OR="\033[1;31m"

# octal \033[0;m ansi escape sequence
OE="\033[0;m"

# custom sequence strings
GSS="[$OG+$OE]" # success sequence string
RSS="$OR[-]$OE" # error sequence string

# the targeted system (source) files needed
TARGETS="libreadline8 readline-common libreadline-dev"

# the targeted libreadline version
LIB_VERSION="8.1.2-1"

# verify that it was indeed installed
GREP_TARGET="grep -i $LIB_VERSION"
DPKG_LIST="dpkg -l"

# the primary command for a proper execution of BRIGHTSTAR
CMD="sudo apt install $TARGETS -y"

# generic dummy strings to print to stdout for 
INIT_MSG="\t** Installing required files for procedure to return success (no exit) on return"
BRIGHTSTAR="$GSS BRIGHTSTAR::Info => Installing Libreadline (>= 8.1.2-1) via apt..."

# install libreadline8 && libreadline-dev
echo -e "$BRIGHTSTAR\n$INIT_MSG\n\nThe following command:\n\t$OG$CMD$OE\n\nwill be executed...\n"
sleep 3

$CMD

# processor name, platform name
STAT_PROC_PNPN="uname -io"
BUILD_STAT=$($STAT_PROC_PNPN)

if [[ $($DPKG_LIST | $GREP_TARGET) ]]; then
    echo -e "\n$GSS Target files needed for BRIGHTSTAR installation complete."
    echo -e "\tIf running \"install_readline.sh\" directly:\n\t\tExecute $OG \bmake modobject && make modbuild && make$OE to build source into binary format ($BUILD_STAT)\n\n\t\tExecute $OG \bmake clean && make modobject && make modbuild && make$OE if you decide to modify the source.\n"
    
    exit 0

else
    # this does NOT mean that libreadline is not potentially installed to a directory on your local system. It just states the exact version
    # used in development of BRIGHTSTAR was not found as installed within you local system package listing
    echo -e "\n$RSS BRIGHTSTAR::Error => could not verify the targeted version of \"libreadline $LIB_VERSION\" at the time of installation."
    exit 1
fi

# WireEye 2023 - BRIGHTSTAR SSDP analyzer/manipulation Framework