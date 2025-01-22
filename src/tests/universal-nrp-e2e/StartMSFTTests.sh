#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# StartMSFTTests.sh
# Description: This MSFT INTERNAL-ONLY script is used to download the latest Azure Policy packages from internal pipelines
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

if ! command -v jq &> /dev/null
then
    echo "jq could not be found, please install jq to proceed"
    exit 1
fi
if ! command -v az &> /dev/null
then
    echo "az-cli could not be found, please install az-cli to proceed"
    exit 1
fi

if command -v sudo &> /dev/null; then
    if [ "$(id -u)" -ne 0 ]; then
        sudo echo -n
    fi
fi

# echo "Downloading latest Azure Policy packages"
# mkdir -p $packageDir
# pipelineRunId=$(az pipelines runs list --pipeline-id $pipelineId --status completed --result succeeded --top 1 --query '[0].id' -o tsv)
# echo "Using latest succeeded run:$pipelineRunId"
# az pipelines runs artifact download --organization $azdevopsOrg --project $azdevopsProject --artifact-name $azdevopsArtifactName --path $packageDir --run-id $pipelineRunId

# $1 - imageFile
# $2 - policyPackage
# $3 - resourceCount
# $4 - VMMemory
# $5 - logDirectory
run_test() {
    local curtime=$(date +%Y%m%d_%H%M%S)
    local tempImage="${curtime}_${1}"
    cp $cacheDir/$1 $cacheDir/$tempImage
    ./StartVMTest.sh -i $cacheDir/$tempImage -p $packageDir/$2 -c $3 -m $4 -l $5 > /dev/null
    if [[ $? -eq 0 ]]; then
        echo "✅ Test on $1 completed successfully."
    else
        echo "❌ Test on $1 failed."
    fi
    rm $cacheDir/$tempImage
}

# $1 - imageFile
download_image() {
    if [ -z "$cacheDir/$1" ]; then
        echo "Downloading image: $1"
        az storage blob download --account-name $storageAccount --container-name $containerName --name $1 --file $cacheDir/$1 --auth-mode login --subscription $subscriptionId > /dev/null
    else
        local_file_date=$(stat -c %Y "$cacheDir/$1")
        blob_date=$(az storage blob show --account-name "$storageAccount" --container-name "$containerName" --name "$1" --auth-mode login --subscription $subscriptionId --query properties.lastModified --output tsv)
        blob_date_unix=$(date -d "$blob_date" +%s)
        if [[ $blob_date_unix -gt $local_file_date ]]; then
            echo "Newer $1 image found in blob storage, downloading image"
            az storage blob download --account-name $storageAccount --container-name $containerName --name $1 --file $cacheDir/$1 --auth-mode login --subscription $subscriptionId > /dev/null
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
    # TODO: Remove this logic in favor of specifying a directoryname
    # sleep 5
    # echo "find . -type d -name "*${imageFile%.*}" -printf '%p\n' | sort | tail -1"
    # latestDir=$(find . -type d -name "*${imageFile%.*}" -printf '%p\n' | sort | tail -1)
    # echo "find $latestDir -type f -name "*.log" -printf '%p\n'"
    # logFile=$(find "$logDir" -type f -name "*.log" -printf '%p\n')
    echo "  Log directory: $logDir"
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