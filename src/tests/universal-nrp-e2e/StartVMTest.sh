#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# StartMCTest.sh
# Description: This script orchestrates tests on particular disk image for testing purposes.
#              It configures the VM with a specified image and places all required test artifacts in
#              addition to installing dependencies, running tests, and collecting logs/reports.
#
# Usage: ./StartMCTest.sh -i /path/to/image.img -p /path/to/policypackage.zip -c 42 [-s] [-m 512] [-d]
#        -i Image Path: Path to the image in qcow2 format
#        -p Policy Package: Path to the policy package
#        -c Resource Count: The number of resources to validate
#        -s Skip Remediation: Skip remediation flag (Default: false)
#        -d Debug Mode: VM stays up for debugging (Default: false)
#        -m VM Memory Size (Megabytes): Size of VM's RAM (Default: 512)
#
# Dependencies: 
#   - qemu-system-x86_64
#   - cloud-image-utils

# CLI variables
imagepath=""
policypackage=""
skipremediation=false
resourcecount=0
vmmemory=512
key=""
debug=false

# Private variables
tests_failed=false
use_sudo=false

usage() { 
    echo "Usage: $0 -i /path/to/image.img -p /path/to/policypackage.zip -c 42 [-s] [-m 512] [-d]
    -i Image Path: Path to the image qcow2 format
    -p Policy Package: Path to the policy package
    -c Resource Count: The number of resources to validate
    -s Skip Remediation: Skip remediation flag (Default: false)
    -d Debug Mode: VM stays up for debugging (Default: false)
    -m VM Memory Size (Megabytes): Size of VMs RAM (Default: 512)" 1>&2; 
    exit 1; 
}
check_if_error() {
    if [ $? -ne 0 ]; then
        echo "Remote commands failed! Check console for details."
    fi
}
find_free_port() {
    while true; do
        port=$(shuf -i 1024-65535 -n 1)
        nc -z localhost $port &> /dev/null
        if [ $? -ne 0 ]; then
            echo "$port"
            return 0
        fi
    done
}
cleanup() {
    if [ $debug = false ]; then
        [ $use_sudo == "true" ] && sudo kill $pid_qemu || kill $pid_qemu
    fi
    exit $1
}
trap cleanup 1 SIGINT

OPTSTRING=":i:p:c:m:sd"

while getopts ${OPTSTRING} opt; do
    case ${opt} in
        i)
            imagepath=${OPTARG}
            ;;
        p)
            policypackage=${OPTARG}
            ;;
        c)
            resourcecount=${OPTARG}
            ;;
        s)
            skipremediation=true
            ;;
        d)
            debug=true
            ;;
        m)
            vmmemory=${OPTARG}
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

if [ -z "${imagepath}" ] || [ -z "${policypackage}" ]; then
    usage
fi

# Ensure local dependencies are installed [cloud-localds, qemu-system-x86_64]
if ! command -v cloud-localds &> /dev/null; then
    echo "cloud-localds not found. Please install cloud-image-utils package and try again." 1>&2
    exit 1
fi
if ! command -v qemu-system-x86_64 &> /dev/null; then
    echo "qemu-system-x86_64 not found. Please install qemu-system-x86_64 and try again." 1>&2
    exit 1
fi

if command -v sudo &> /dev/null; then
    if [ "$(id -u)" -ne 0 ]; then
        echo "sudo is available. Attempting to sudo for better VM performance..."
        sudo echo "done!"
        use_sudo=true
    fi
fi

basepath="_${imagepath%.*}"
curtime=$(date +%Y%m%d_%H%M%S)
mkdir -p $basepath/metadata
log_file="$basepath/script_$curtime.log"
# Redirect stdout and stderr to the log file
exec > >(tee -a "$log_file") 2>&1

# Create SSH keys for cloud-config
if [ ! -f "$basepath/id_rsa" ]; then
    echo "Generating SSH keys..."
    ssh-keygen -t rsa -b 4096 -f $basepath/id_rsa -N ""
    chmod 600 $basepath/id_rsa
fi

# Create metadata and user-data files for cloud-init
cat << EOF > $basepath/metadata/meta-data
instance-id: someid/somehostname
EOF
cat << EOF > $basepath/metadata/user-data
#cloud-config
ssh_pwauth: false
hostname: osconfige2etest
fqdn: osconfige2etest.local
users:
  - name: user1
    sudo: ALL=(ALL) NOPASSWD:ALL
    groups: sudo
    shell: /bin/bash
    plain_text_passwd: password
    lock_passwd: false
    ssh-authorized-keys:
      - $(cat $basepath/id_rsa.pub)
runcmd:
  - echo "127.0.0.1 $(hostname) $(hostname -f)" >> /etc/hosts

EOF
touch $basepath/metadata/vendor-data

# Create seed.img for cloud-init
cloud-localds $basepath/seed.img $basepath/metadata/user-data $basepath/metadata/meta-data

# Start QEMU, point to local IMDS server for cloud-init to use instance
qemu_fwport=$(find_free_port)
qemu_imgformat=$( [ "${imagepath##*.}" = "raw" ] && echo "raw" || echo "qcow2" )
echo "Starting QEMU using $qemu_imgformat format SSH port (22) forwarded to $qemu_fwport with $vmmemory mb RAM ..." 

qemu_command=$(cat <<EOF
qemu-system-x86_64                                                    \
    -net nic                                                          \
    -net user,hostfwd=tcp::$qemu_fwport-:22                           \
    -machine accel=kvm                                                \
    -m $vmmemory                                                      \
    -daemonize                                                        \
    -drive file=$imagepath,format=$qemu_imgformat                     \
    -drive if=virtio,format=raw,file=$basepath/seed.img
EOF
)
[ $use_sudo == "true" ] && sudo bash -c "eval $qemu_command" || eval $qemu_command

pid_qemu=$(ps aux | grep -m 1 "qemu.*hostfwd=tcp::$qemu_fwport-:22" | awk '{print $2}')
echo "QEMU process started with PID: $pid_qemu"

echo -n "Waiting for VM provisioning..."
ssh_args="-i $basepath/id_rsa -o ConnectTimeout=5 -o StrictHostKeyChecking=no user1@localhost -p $qemu_fwport"
while true; do
    ssh $ssh_args 'exit' > /dev/null 2>&1 && break
    echo -n "."
    sleep 5
done
echo "done!"

# Remote Dependencies Check
ssh $ssh_args << 'EOF'
    echo -n "Checking dependencies..."
    if ! pwsh --version > /dev/null 2>&1; then
        # echo "[ERROR] Missing Powershell. Please install it and try again." >&2
        # err=true
        echo "Powershell not found. Installing Powershell..."
        # Download the powershell '.tar.gz' archive
        curl -L -o /tmp/powershell.tar.gz https://github.com/PowerShell/PowerShell/releases/download/v7.4.6/powershell-7.4.6-linux-x64.tar.gz

        # Create the target folder where powershell will be placed
        sudo mkdir -p /opt/microsoft/powershell/7

        # Expand powershell to the target folder
        sudo tar zxf /tmp/powershell.tar.gz -C /opt/microsoft/powershell/7

        # Set execute permissions
        sudo chmod +x /opt/microsoft/powershell/7/pwsh

        # Create the symbolic link that points to pwsh
        sudo ln -s /opt/microsoft/powershell/7/pwsh /usr/bin/pwsh
    fi
    if [ ! -d "/opt/omi/lib" ]; then
        echo "OMI not found. Installing OMI..."
        # OMI package is based on OpenSSL Versions: 1.0, 1.1, 3.0
        openssl_version=$(openssl version | awk '{print $2}' | cut -d'.' -f1,2)
        omi_url=""
        if [ "$openssl_version" = "1.0" ]; then
            omi_url="https://github.com/microsoft/omi/releases/download/v1.9.1-0/omi-1.9.1-0.ssl_100.ulinux.s.x64"
        elif [ "$openssl_version" = "1.1" ]; then
            omi_url="https://github.com/microsoft/omi/releases/download/v1.9.1-0/omi-1.9.1-0.ssl_110.ulinux.s.x64"
        elif [ "$openssl_version" = "3.0" ]; then
            omi_url="https://github.com/microsoft/omi/releases/download/v1.9.1-0/omi-1.9.1-0.ssl_300.ulinux.s.x64"
        else
            echo "Unknown OpenSSL version. This system may not be compatible with OMI."
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
                sudo dpkg -i /tmp/omi.${omi_url##*.}
            elif command -v rpm &> /dev/null; then
                sudo rpm -Uvh /tmp/omi.${omi_url##*.}
            fi
        fi
    fi
    if [ "$err" = true ]; then
        exit 1
    fi
    echo "done!"
EOF
check_if_error

echo "Copying test artifacts to VM..."
policyPackageFileName=$(basename $policypackage)
scp -P $qemu_fwport -i $basepath/id_rsa $policypackage user1@localhost:~ || { echo "scp failed! Check console for details." 1>&2; cleanup 1; }

remote_home_dir=$(ssh $ssh_args 'echo $HOME')
cat << EOF > $basepath/bootstrap.ps1
Install-Module -Name GuestConfiguration -Force
Install-Module Pester -Force -SkipPublisherCheck
Import-Module Pester -Passthru

\$params = @{
    PolicyPackage = "$remote_home_dir/$policyPackageFileName"
    SkipRemediation = \$$skipremediation
    ResourceCount = $resourcecount
}
\$container = New-PesterContainer -Path $remote_home_dir/UniversalNRP.Tests.ps1 -Data \$params
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
        OutputPath   = '$imagepath.testResults.xml'
    }
    Should = @{
        ErrorAction = 'Continue'
    }
};
Invoke-Pester -Configuration \$pesterConfig
EOF
check_if_error

scp -P $qemu_fwport -i $basepath/id_rsa $basepath/bootstrap.ps1 user1@localhost:~ || { echo "scp failed! Check console for details." 1>&2; cleanup 1; }
scp -P $qemu_fwport -i $basepath/id_rsa UniversalNRP.Tests.ps1 user1@localhost:~ || { echo "scp failed! Check console for details."1>&2; cleanup 1; }

# Run tests
ssh $ssh_args << 'EOF'
    echo "Running tests..."
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/omi/lib/ pwsh -Command ./bootstrap.ps1
EOF
if [ $? -ne 0 ]; then
    echo "Tests failed! Check console for details." 1>&2;
    tests_failed=true
fi

# Log/Report collection
ssh $ssh_args << 'EOF'
    echo "Collecting logs/reports..."
    mkdir -p ~/osconfig-logs
    sudo cp -r /var/log/osconfig* ~/osconfig-logs/
    cp *testResults.xml ~/osconfig-logs
    tar -czvf osconfig-logs.tar.gz osconfig-logs
EOF
check_if_error
temp_dir=$(mktemp -d)
scp -P $qemu_fwport -i $basepath/id_rsa user1@localhost:~/osconfig-logs.tar.gz $temp_dir/osconfig-logs.tar.gz || { echo "scp failed! Check console for details." 1>&2; cleanup 1; }
tar -xzf $temp_dir/osconfig-logs.tar.gz -C "$temp_dir"
rm $temp_dir/osconfig-logs.tar.gz
cp $log_file $temp_dir
tar -czf $basepath/osconfig-logs-$curtime.tar.gz -C "$temp_dir" .
rm -rf "$temp_dir"
echo "Log archive created: $basepath/osconfig-logs-$curtime.tar.gz"

# Finished, optionally show debug banner, cleanup and exit with the Pester exit code
if [ $debug = true ]; then
    echo "######################################################################"
    echo "Debug mode enabled. To connect via SSH:"
    echo "  ssh -i $(pwd)/$basepath/id_rsa user1@localhost -p $qemu_fwport"
    echo "When done with VM, terminate the process to free up VM resources."
    [ $use_sudo == "true" ] && echo "  sudo kill $pid_qemu" || echo "  kill $pid_qemu"
    echo "######################################################################"
fi

if [ $tests_failed = true ]; then
    echo "❌ Tests failed! Check console for details. Check logs or enabled debug [-d] to connect to VM with SSH" 1>&2;
    cleanup 1
else
    echo "✅ Tests passed!"
    cleanup 0
fi
