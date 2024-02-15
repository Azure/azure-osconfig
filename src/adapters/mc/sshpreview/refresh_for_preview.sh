#!/bin/bash

if [ -n "$(command -v yum)" ]; then
        echo "using yum"
        sudo yum remove openssh-server -y
        sudo rm /etc/ssh/sshd_config*
        sudo rm /etc/ssh/sshd_config.d/osconfig_*
        sudo yum install openssh-server -y

        if [[ $(yum list installed | grep -wi osconfig) ]]; then
                echo "OSConfig installed, updating"
                sudo yum update
                sudo yum install osconfig
        else
                echo "OSConfig not installed"
        fi
fi

if [ -n "$(command -v apt-get)" ]; then
        echo "using apt-get"
        sudo apt-get remove --purge openssh-server -y
        sudo rm /etc/ssh/sshd_config*
        sudo rm /etc/ssh/sshd_config.d/osconfig_*
        sudo apt-get install openssh-server -y

        if [[ $(apt list osconfig | grep -wi osconfig) ]]; then
                echo "OSConfig installed, updating"
                sudo apt-get update
                sudo apt-get install osconfig
        else
                echo "OSConfig not installed"
        fi
fi