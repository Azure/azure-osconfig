#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# StartTests.sh
# Description: This Microsoft internal only script is used to download the latest Azure Policy packages from internal pipelines
#              and run the tests on the specified test data provided.
# Usage: ./StartMSFTTests.sh [-r run-id] [-m vm-memory-mb]
#        -r run-id: Specify the run-id of the pipeline run to download the packages from
#        -m vm-memory-mb: Specify the memory in MB to be used for the VMs

test_data='[
    {"imageFile": "centos-7.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "resourceCount": 168},
    {"imageFile": "centos-7.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "resourceCount": 20},
    {"imageFile": "centos-8.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "resourceCount": 168},
    {"imageFile": "centos-8.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "resourceCount": 20},
    {"imageFile": "debian-10.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "resourceCount": 168},
    {"imageFile": "debian-10.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "resourceCount": 20},
    {"imageFile": "oraclelinux-7.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "resourceCount": 168},
    {"imageFile": "oraclelinux-7.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "resourceCount": 20},
    {"imageFile": "rhel-7.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "resourceCount": 168},
    {"imageFile": "rhel-7.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "resourceCount": 20},
    {"imageFile": "sles-12.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "resourceCount": 168},
    {"imageFile": "sles-12.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "resourceCount": 20},
    {"imageFile": "ubuntu-16.04.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "resourceCount": 168},
    {"imageFile": "ubuntu-16.04.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "resourceCount": 20},
    {"imageFile": "ubuntu-18.04.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "resourceCount": 168},
    {"imageFile": "ubuntu-18.04.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "resourceCount": 20}
]'

subscriptionId="ce58a062-8d06-4da9-a85b-28939beb0119"
storageAccount="osconfigstorage"
containerName="diskimages"
pipelineId=394569
pipelineRunId=""
azdevopsOrg="https://dev.azure.com/msazure/"
azdevopsProject="One"
azdevopsArtifactName="drop_package_mc_packages"
vmmemory=512

cacheDir=".cache"
packageDir="$cacheDir/packages"

usage() { 
    echo "Usage: $0 [-r run-id] [-m vm-memory-mb]" 1>&2;
    exit 1; 
}

OPTSTRING=":m:p:"

while getopts ${OPTSTRING} opt; do
    case ${opt} in
        m)
            vmmemory=${OPTARG}
            ;;
        p)
            pipelineRunId=${OPTARG}
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

# Ensure local dependencies are installed [cloud-localds, qemu-system-x86_64, jq, az]
dependencies=(cloud-localds qemu-system-x86_64 jq az)
for dep in "${dependencies[@]}"; do
    if ! command -v $dep &> /dev/null; then
        echo "$dep not found. Please install it and try again." 1>&2
        exit 1
    fi
done

if command -v sudo &> /dev/null; then
    if [ "$(id -u)" -ne 0 ]; then
        sudo echo -n
    fi
fi

echo "Downloading latest Azure Policy packages"
mkdir -p $packageDir
pipelineRunId=$(az pipelines runs list --organization $azdevopsOrg --project $azdevopsProject --pipeline-id $pipelineId --status completed --result succeeded --top 1 --query '[0].id' -o tsv)
echo "Using latest succeeded run:$pipelineRunId"
az pipelines runs artifact download --organization $azdevopsOrg --project $azdevopsProject --artifact-name $azdevopsArtifactName --path $packageDir --run-id $pipelineRunId

run_test() {
    local imageFile=$1
    local policyPackage=$2
    local resourceCount=$3
    local vmMemory=$4
    local logDirectory=$5

    local curtime=$(date +%Y%m%d_%H%M%S)
    local tempImage="${curtime}_${imageFile}"
    cp $cacheDir/$imageFile $cacheDir/$tempImage
    ./StartVMTest.sh -i $cacheDir/$tempImage -p $packageDir/$policyPackage -c $resourceCount -m $vmMemory -l $logDirectory > /dev/null
    if [[ $? -eq 0 ]]; then
        echo "✅ Test on $imageFile completed successfully."
    else
        echo "❌ Test on $imageFile failed."
    fi
    rm $cacheDir/$tempImage
}

download_image() {
    local imageFile=$1
    if [ -z "$cacheDir/$imageFile" ]; then
        echo "Downloading image: $imageFile"
        az storage blob download --account-name $storageAccount --container-name $containerName --name $imageFile --file $cacheDir/$imageFile --auth-mode login --subscription $subscriptionId > /dev/null
    else
        local_file_date=$(stat -c %Y "$cacheDir/$imageFile")
        blob_date=$(az storage blob show --account-name "$storageAccount" --container-name "$containerName" --name "$imageFile" --auth-mode login --subscription $subscriptionId --query properties.lastModified --output tsv)
        blob_date_unix=$(date -d "$blob_date" +%s)
        if [[ $blob_date_unix -gt $local_file_date ]]; then
            echo "Newer $imageFile image found in blob storage, downloading image"
            az storage blob download --account-name $storageAccount --container-name $containerName --name $imageFile --file $cacheDir/$imageFile --auth-mode login --subscription $subscriptionId > /dev/null
        fi
    fi
}

# Download Distro Images
pids=()
for row in $(echo "${test_data}" | jq -r '.[] | @base64'); do
    _jq() {
        echo ${row} | base64 --decode | jq -r ${1}
    }
    download_image $(_jq '.imageFile') &
    pids+=($!)
done
for pid in "${pids[@]}"; do
    wait $pid
done

# Start Tests using Distro Image
pids=()
for row in $(echo "${test_data}" | jq -r '.[] | @base64'); do
    _jq() {
        echo ${row} | base64 --decode | jq -r ${1}
    }

    imageFile=$(_jq '.imageFile')
    policyPackage=$(_jq '.policyPackage')
    resourceCount=$(_jq '.resourceCount')

    echo "Running tests with image: $imageFile, on policy package: $policyPackage with resource count: $resourceCount ..."
    curtime=$(date +%Y%m%d_%H%M%S)
    logDir="$cacheDir/${curtime}_${imageFile%.*}"
    run_test $imageFile $policyPackage $resourceCount $vmmemory $logDir &
    pids+=($!)
    echo "  Log directory: $logDir"
    sleep 1
done

# Wait for tests to complete and capture exit codes
exit_codes=()
for pid in "${pids[@]}"; do
    wait $pid
    exit_codes+=($?)
done

# Report success/failure and exit with appropriate exit code
echo "All tests are complete."
failedTests=false
for i in "${!exit_codes[@]}"; do
    if [ "${exit_codes[$i]}" -gt 0 ]; then
        failedTests=true
        break
    fi
done
if [ "$failedTests" = true ]; then
    echo "❌ Tests failed!" 1>&2;
    exit 1
else
    echo "✅ Tests successfull!"
    exit 0
fi
