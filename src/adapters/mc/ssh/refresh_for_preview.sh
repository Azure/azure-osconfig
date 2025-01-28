#!/bin/bash

if [ -n "$(command -v yum)" ]; then
        echo "using yum"
        yum remove openssh-server -y
        rm /etc/ssh/sshd_config*
        rm /etc/ssh/sshd_config.d/osconfig_*
        yum install openssh-server -y

        if [[ $(yum list installed | grep -wi osconfig) ]]; then
                echo "OSConfig installed, updating"
                yum update
                yum install osconfig
        else
                echo "OSConfig not installed"
        fi
fi

if [ -n "$(command -v apt-get)" ]; then
        echo "using apt-get"
        apt-get remove --purge openssh-server -y
        rm /etc/ssh/sshd_config*
        rm /etc/ssh/sshd_config.d/osconfig_*
        apt-get install openssh-server -y

        if [[ $(apt list osconfig | grep -wi osconfig) ]]; then
                echo "OSConfig installed, updating"
                apt-get update
                apt-get install osconfig
        else
                echo "OSConfig not installed"
        fi
fi
