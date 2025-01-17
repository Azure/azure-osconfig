#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# StartMCTest.sh
# Description: This script orchestrates tests on particular disk image for testing purposes.
#              It configures the VM with a specified image and places all required test artifacts in
#              addition to installing dependencies, running tests, and collecting logs/reports.
#              Returns an error code if any stage fails.
#              There is also both a generalize and a debug mode flag that can be used to generalize an
#              image and a debug mode that will leave the VM up for debugging.
#
# Usage: ./StartVMTest.sh [-i /path/to/image.img -p /path/to/policypackage.zip -c resource-count [-g]] [-m 512] [-r] [-d]
#        -i Image Path:            Path to the image qcow2 format
#        -p Policy Package:        Path to the policy package
#        -c Resource Count:        The number of resources to validate, tests will fail if this doesn't match (Default: 0)
#        -m VM Memory (Megabytes): Size of VMs RAM (Default: 512)
#        -r Remediation:           Perform remediation flag (Default: false)
#        -g Generalize Flag:       Generalize the current machine for tests. Performs the following:
#                                    - Remove logs and tmp directories
#                                    - Clean package management cache
#                                    - Clean cloud-init flags to reset cloud-init to initial-state
#        -d Debug Mode:            VM stays up for debugging (Default: false)
#
# Dependencies: 
#   - qemu-system-x86_64
#   - cloud-image-utils

# CLI variables
imagepath=""
vmmemory=512
debug=false
policypackage=""
remediation=false
resourcecount=0
generalize=false

# Private variables
tests_failed=false
use_sudo=false

usage() { 
    echo "Usage: $0 -i /path/to/image.img -p /path/to/policypackage.zip -c resource-count [-m 512] [-r] [-d]
    -i Image Path:            Path to the image qcow2 format
    -p Policy Package:        Path to the policy package
    -c Resource Count:        The number of resources to validate, tests will fail if this doesn't match
    -m VM Memory (Megabytes): Size of VMs RAM (Default: 512)
    -r Remediation:           Perform remediation flag (Default: false)
    -g Generalize Flag:       Generalize the current machine for tests. Performs the following:
                                - Remove logs and tmp directories
                                - Clean package management cache
                                - Clean cloud-init flags to reset cloud-init to initial-state
    -d Debug Mode Flag:       VM stays up for debugging (Default: false)" 1>&2; 
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
debug_mode() {
    if [ $debug = true ]; then
        echo "######################################################################"
        echo "Debug mode enabled. To connect via SSH:"
        echo "  ssh -i $(pwd)/$basepath/id_rsa user1@localhost -p $qemu_fwport"
        echo "When done with VM, terminate the process to free up VM resources."
        [ $use_sudo == "true" ] && echo "  sudo kill $pid_qemu" || echo "  kill $pid_qemu"
        echo "######################################################################"
    fi
}
cleanup() {
    if [ $debug = false ]; then
        do_sudo kill $pid_qemu
    fi
    exit $1
}
do_sudo() {
    if [ "$use_sudo" = true ]; then
        sudo "$@"
    else
        "$@"
    fi
}

trap cleanup 1 SIGINT

OPTSTRING=":i:p:c:m:rdg"

while getopts ${OPTSTRING} opt; do
    case ${opt} in
        i)
            imagepath=${OPTARG}
            echo "Image path: $imagepath."
            ;;
        p)
            policypackage=${OPTARG}
            echo "Policy package: $policypackage."
            ;;
        c)
            resourcecount=${OPTARG}
            echo "Resource count: $resourcecount."
            ;;
        m)
            vmmemory=${OPTARG}
            echo "VM memory: $vmmemory mb."
            ;;
        r)
            echo "Remediation Mode: enabled."
            remediation=true
            ;;
        d)
            echo "Debug Mode: enabled."
            debug=true
            ;;
        g)
            echo "Generalization Mode: enabled."
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

if [ -z "${imagepath}" ]; then
    usage
fi
if [ $generalize = false ] && [ -z "${policypackage}" ]; then
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

# Start QEMU
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
do_sudo bash -c "eval $qemu_command" || { echo "qemu failed! Check console for details." 1>&2; exit 1; }

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

# Dependencies Check
scp -P $qemu_fwport -i $basepath/id_rsa StartLocalTest.sh user1@localhost:~ || { echo "scp failed! Check console for details." 1>&2; cleanup 1; }
ssh $ssh_args "bash StartLocalTest.sh -s dependency_check"
check_if_error

if [ $generalize = true ]; then
    ssh $ssh_args "bash StartLocalTest.sh -g" || { echo "Generalization failed! Check console for details." 1>&2; cleanup 1; }
    if [ $debug = false ]; then
        echo "Shutting down VM..."
        ssh $ssh_args "sudo shutdown now"
    fi
    debug_mode
    echo "✅ Generalization complete!"
    # If image is to be generalized, exit here
    exit 0
fi

echo "Copying test artifacts to VM..."
policyPackageFileName=$(basename $policypackage)
scp -P $qemu_fwport -i $basepath/id_rsa $policypackage user1@localhost:~ || { echo "scp failed! Check console for details." 1>&2; cleanup 1; }
scp -P $qemu_fwport -i $basepath/id_rsa UniversalNRP.Tests.ps1 user1@localhost:~ || { echo "scp failed! Check console for details."1>&2; cleanup 1; }

# Run tests
ssh_command="bash StartLocalTest.sh -s run_tests -p $policyPackageFileName -c $resourcecount"
if [ $remediation = true ]; then
    ssh_command="$ssh_command -r"
fi
ssh $ssh_args "$ssh_command"
if [ $? -ne 0 ]; then
    echo "Tests failed! Check console for details." 1>&2;
    tests_failed=true
fi

# Log/Report collection
ssh $ssh_args "bash StartLocalTest.sh -s collect_logs"
check_if_error
temp_dir=$(mktemp -d)
scp -P $qemu_fwport -i $basepath/id_rsa user1@localhost:~/osconfig-logs.tar.gz $temp_dir/osconfig-logs.tar.gz || { echo "scp failed! Check console for details." 1>&2; cleanup 1; }
tar -xzf $temp_dir/osconfig-logs.tar.gz -C "$temp_dir"
rm $temp_dir/osconfig-logs.tar.gz
cp $log_file $temp_dir
tar -czf $basepath/osconfig-logs-$curtime.tar.gz -C "$temp_dir" . > /dev/null 2>&1
rm -rf "$temp_dir"
echo "Log archive created: $basepath/osconfig-logs-$curtime.tar.gz"

# Finished, optionally show debug banner, cleanup and exit with the Pester exit code
debug_mode
if [ $tests_failed = true ]; then
    echo "❌ Tests failed! Check console for details. Check logs or enabled debug [-d] to connect to VM with SSH" 1>&2;
    cleanup 1
else
    echo "✅ Tests passed!"
    cleanup 0
fi
