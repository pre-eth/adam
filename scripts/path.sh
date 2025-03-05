#!/bin/bash

args=("$@")
printf "\n\033[1;36mAdd ADAM to your PATH? [y/n]\033[m" 
read ans
if [[ $ans -eq "Y" || $ans -eq "y" ]] ; then
    sudo cp ${args[0]} /usr/local/bin/${args[1]}
    printf "\033[1;32mADAM is now in your path. Run adam -h to get started!\033[m\n"
else
    printf "ADAM was not added to your path.\n"
fi