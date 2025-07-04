#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# StartLocalTest.sh
# Description: This script orchestrates tests on a local machine. Installs dependencies,
#              runs tests, and collects logs/reports. Returns an error code if any stage fails.
#
# Usage: ./StartLocalTest.sh [-s stage-name] [-p policy-package.zip [-r] [-g]] [-g]
#        -s stage-name:         Specify the stage name. Valid options are: dependency_check, run_tests, collect_logs.
#                               If no stage is specified, all stages will be executed in this order:
#                                   dependency_check, run_tests, collect_logs
#        -p policy-package.zip: The Azure Policy Package to test
#        -r remediate-flag:     When the flag is enabled, performs remediation on the Policy Package (Default: No remediation performed)
#        -g generalize-flag:    Generalize the current machine for tests. Performs the following:
#                                   - Remove logs and tmp directories
#                                   - Clean package management cache
#                                   - Clean cloud-init flags to reset cloud-init to initial-state
# Dependencies: curl, wget, unzip

# Powershell and OMI are also required for the tests but they have different installation steps and do not use the distros package manager.
dependencies=(curl wget unzip)
dependencies_ps="@('GuestConfiguration', 'Pester')"
powershell_version="7.5.0"
powershell_uri="https://github.com/PowerShell/PowerShell/releases/download/v$powershell_version/powershell-$powershell_version-linux-x64.tar.gz"
omi_base_uri="https://github.com/microsoft/omi/releases/download/v1.9.1-0/omi-1.9.1-0"

stageName=""
policypackage=""
remediation=false
resourcecount=0
generalize=false

# Private variables
use_sudo=false

usage() {
    echo "Usage: $0 [-s stage-name] [-p policy-package.zip [-r]] [-g]
        -s stage-name:         Specify the stage name. Valid options are: dependency_check, run_tests, collect_logs.
                               If no stage is specified, all stages will be executed in this order:
                                    dependency_check, run_tests, collect_logs

            - dependency_check: Checks to ensure dependencies are satisfied.
                                Checks for and installs them if not present:
                                    - Powershell +modules: MachineConfiguration, Pester
                                    - OMI
                                    - unzip, curl, wget

            - run_tests:        Runs the tests (Powershell Pester Tests).

            - collect_logs:     Creates a tar.gz archive with the osconfig logs and JUnit Test Report

        -p policy-package.zip:  The Azure Policy Package to test

        -r remediate-flag:      When the flag is enabled, performs remediation on the Policy Package (Default: No remediation performed)

        -g generalize-flag:     Generalize the current machine for tests. Performs the following:
                                    - Remove logs and tmp directories
                                    - Clean package management cache
                                    - Clean cloud-init flags to reset cloud-init to initial-state" 1>&2;

    exit 1;
}

install_package() {
    if command -v apt &> /dev/null; then
        sudo apt update -y
        sudo apt install -y "$@"
    elif command -v yum &> /dev/null; then
        sudo yum update -y
        sudo yum install -y "$@"
    elif command -v dnf &> /dev/null; then
        sudo dnf update -y
        sudo dnf install -y "$@"
    elif command -v zypper &> /dev/null; then
        sudo zypper refresh
        sudo zypper install -y "$@"
    else
        echo "Unsupported Linux distribution." >&2
        exit 1
    fi
}
_unzip() {
    if ! command -v unzip &> /dev/null; then
        echo "unzip not found. Installing unzip..." >&2
        install_package unzip > /dev/null 2>&1
    fi
    unzip "$@"
}
dependency_check() {
    echo "Checking dependencies..."
    for dep in "${dependencies[@]}"; do
        if ! command -v $dep &> /dev/null; then
            echo -e "\n$dep not found. Installing $dep..."
            install_package $dep > /dev/null 2>&1
        fi
    done
    if ! pwsh --version > /dev/null 2>&1; then
        echo -e "\nPowershell not found. Installing Powershell..."
        # Download the powershell '.tar.gz' archive
        curl -L -o /tmp/powershell.tar.gz $powershell_uri
        # Create the target folder where powershell will be placed - first part of version
        do_sudo mkdir -p /opt/microsoft/powershell/${powershell_version%%.*}
        # Expand powershell to the target folder
        do_sudo tar zxf /tmp/powershell.tar.gz -C /opt/microsoft/powershell/${powershell_version%%.*}
        # Set execute permissions
        do_sudo chmod +x /opt/microsoft/powershell/${powershell_version%%.*}/pwsh
        # Create the symbolic link that points to pwsh
        do_sudo ln -s /opt/microsoft/powershell/${powershell_version%%.*}/pwsh /usr/bin/pwsh
    fi
    if pwsh --version; then
        echo "Powershell found. Checking for required modules..."
        do_sudo pwsh -Command "
            \$modules = $dependencies_ps
            foreach (\$module in \$modules) {
                if (-not (Get-Module -ListAvailable -Name \$module)) {
                    Write-Host "Installing module: \$module"
                    Install-Module -Name \$module -Force -SkipPublisherCheck
                }
            }"
    else
        err=true
    fi
    if [ ! -d "/opt/omi/lib" ]; then
        echo -e "\nOMI not found. Installing OMI..."
        # OMI package is based on OpenSSL Versions: 1.0, 1.1, 3.x
        openssl_version=$(openssl version | awk '{print $2}' | cut -d'.' -f1,2)
        omi_url=""
        if [ "$openssl_version" = "1.0" ]; then
            omi_url="$omi_base_uri.ssl_100.ulinux.s.x64"
        elif [ "$openssl_version" = "1.1" ]; then
            omi_url="$omi_base_uri.ssl_110.ulinux.s.x64"
        elif [[ "$openssl_version" =~ ^3\.[0-9]+$ ]]; then
            omi_url="$omi_base_uri.ssl_300.ulinux.s.x64"
        else
            echo "Unknown OpenSSL version ($openssl_version). This system may not be compatible with OMI."
            err=true
        fi
        if command -v dpkg &> /dev/null; then
            omi_url="$omi_url.deb"

        elif command -v rpm &> /dev/null; then
            omi_url="$omi_url.rpm"
        else
            echo "Unknown package manager. This system may not be Debian or RPM-based."
            err=true
        fi
        if [ "$err" != true ]; then
            echo "Downloading OMI package at $omi_url"
            wget $omi_url -O /tmp/omi.${omi_url##*.}
            if command -v dpkg &> /dev/null; then
                do_sudo dpkg -i /tmp/omi.${omi_url##*.}
            elif command -v rpm &> /dev/null; then
                do_sudo rpm -Uvh /tmp/omi.${omi_url##*.}
            fi
        fi
    fi
    if [ "$err" = true ]; then
        return 1
    fi
    echo "done!"
    return 0
}
get_instance_count() {
    local package=$1
    local instanceCount=0
    tempDir=$(mktemp -d)
    _unzip -q $package -d $tempDir > /dev/null 2>&1
    instanceCount=$(find $tempDir -name "${package%.*}.mof" -exec grep -c "instance of OsConfigResource as \$OsConfigResource" {} \;)
    rm -rf $tempDir
    echo $instanceCount
}
run_tests() {
    echo "Running tests..."
    echo "Policy Package: $policypackage"
    echo "Resource Count: $resourcecount"
    echo "Remediation   : $remediation"
    skipremediation=$([ "$remediation" = "false" ] && echo '1')
    cat << EOF > bootstrap.ps1
param (
    [string] \$PolicyPackage,
    [switch] \$SkipRemediation,
    [int] \$ResourceCount,
    [string] \$PesterTests
)
\$params = @{
    PolicyPackage = \$PolicyPackage
    SkipRemediation = \$SkipRemediation
    ResourceCount = \$ResourceCount
}
\$container = New-PesterContainer -Path \$PesterTests -Data \$params
\$pesterConfig = [PesterConfiguration]@{
    Run = @{
        Exit = \$true
        Container = \$container
    }
    Output = @{
        Verbosity = 'Detailed'
    }
    TestResult = @{
        Enabled      = \$true
        OutputFormat = 'JUnitXml'
        OutputPath   = "\$(hostname)-testResults.xml"
    }
    Should = @{
        ErrorAction = 'Continue'
    }
};
Invoke-Pester -Configuration \$pesterConfig
EOF
    do_sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/omi/lib/ pwsh -File ./bootstrap.ps1 -PolicyPackage "$policypackage" ${skipremediation:+-SkipRemediation} -ResourceCount $resourcecount -PesterTests "$HOME/UniversalNRP.Tests.ps1"
    return $?
}
generalize() {
    echo "Generalizing machine..."
    # Clear bash history both for current user and root
    rm -f ~/.bash_history
    do_sudo su -c "
        rm -f ~/.bash_history
        # Clear system logs
        rm -rf /var/log/*
        # Clear authentication logs
        rm -rf /var/log/auth.log /var/log/secure /var/log/wtmp /var/log/btmp
        # Remove temporary files and directories
        rm -rf /tmp/* /var/tmp/*
        # Clean up apt cache (if using Debian-based distro)
        if command -v apt-get &> /dev/null; then
            apt-get clean
        fi
        # Clean up yum cache (if using RPM-based distro)
        if command -v yum &> /dev/null; then
            yum clean all
        fi
        # Clean up dnf cache (if using RPM-based distro with dnf)
        if command -v dnf &> /dev/null; then
            dnf clean all
        fi
        # Clean cloud-init allowing for new cloud-config on next boot
        cloud-init clean
    "
    echo "done!"
    return 0
}
collect_logs() {
    echo "Collecting logs/reports..."
    temp_dir=$(mktemp -d)
    mkdir -p $temp_dir/osconfig-logs
    do_sudo cp -r /var/log/osconfig* $temp_dir/osconfig-logs/
    do_sudo chmod 644 $temp_dir/osconfig-logs/*
    cp *testResults.xml $temp_dir/osconfig-logs
    tar -czvf osconfig-logs.tar.gz -C $temp_dir osconfig-logs
    rm -rf $temp_dir/osconfig-logs
    return 0
}
do_sudo() {
    if [ "$use_sudo" = true ]; then
        sudo "$@"
    else
        "$@"
    fi
}

OPTSTRING=":s:p:rg"

while getopts ${OPTSTRING} opt; do
    case ${opt} in
        s)
            stageName=${OPTARG}
            # Check if stageName is valid
            if [ "$stageName" != "dependency_check" ] && [ "$stageName" != "run_tests" ] && [ "$stageName" != "collect_logs" ]; then
                echo "Invalid stage name: $stageName"
                usage
            fi
            ;;
        p)
            policypackage=${OPTARG}
            ;;
        r)
            remediation=true
            ;;
        g)
            generalize=true
            ;;
        :)
            echo "Option -${OPTARG} requires an argument."
            usage
            ;;
        ?)
            echo "Invalid option: -${OPTARG}."
            usage
            ;;
    esac
done

if command -v sudo &> /dev/null; then
    if [ "$(id -u)" -ne 0 ]; then
        echo "sudo is available. Attempting to sudo..."
        sudo echo "done!"
        use_sudo=true
    fi
fi

if [ "$stageName" = "dependency_check" ]; then
    dependency_check
    exit $?
fi

if [ "$stageName" = "collect_logs" ]; then
    collect_logs
    exit $?
fi

if [ $generalize = true ]; then
    dependency_check && generalize
    exit $?
fi

if [ -z "$policypackage" ]; then
    echo "Policy package not provided." >&2
    usage
fi

resourcecount=$(get_instance_count $policypackage)
if [ -z "$resourcecount" ] || [ "$resourcecount" -eq 0 ]; then
    echo "Resource count invalid: $resourcecount" >&2
    exit 1
fi
if [ -z "$HOME/UniversalNRP.Tests.ps1" ]; then
    echo "UniversalNRP.Tests.ps1 not found. Copy Powershell script into $HOME directory" >&2
fi

if [ "$stageName" = "run_tests" ]; then
    run_tests
    exit $?
fi

# Default stage - Run all stages
if [ "$stageName" = "" ]; then
    dependency_check && run_tests
    collect_logs
fi
