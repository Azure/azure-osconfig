#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# StartTests.sh
# Description: This Microsoft internal only script is used to download the latest Azure Policy packages from internal pipelines
#              and run the tests on the specified test data provided.
# Usage: ./StartTests.sh [-r run-id] [-m vm-memory-mb] [-j max-concurrent-jobs]
#        -r run-id: Specify the run-id of the pipeline run to download the packages from (Default: latest-succeeded-run-id)
#        -m vm-memory-mb: Specify the memory in MB to be used for the VMs (Default: 512)
#        -j max-concurrent-jobs: Specify the maximum number of concurrent jobs to run the tests (Default: 5)
# Dependencies: cloud-localds, qemu-system-x86, jq, az

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
maxConcurrentJobs=5

cacheDir=".cache"
packageDir="$cacheDir/packages"
countCompletedTests=0
countInProgressTests=0
countPendingTests=0
countTotalTests=0
waitInterval=2

usage() { 
    echo "Usage: $0 [-r run-id] [-m vm-memory-mb] [-j max-concurrent-jobs]" 1>&2;
    exit 1; 
}

OPTSTRING=":j:m:p:"

while getopts ${OPTSTRING} opt; do
    case ${opt} in
        j)
            maxConcurrentJobs=${OPTARG}
            echo "Max concurrent jobs set to $maxConcurrentJobs"
            ;;
        m)
            vmmemory=${OPTARG}
            echo "VM memory set to $vmmemory MB"
            ;;
        p)
            pipelineRunId=${OPTARG}
            echo "Using pipeline run id: $pipelineRunId"
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

# Check if the Azure DevOps extension is already installed
if ! az extension list --output table | grep -q azure-devops; then
    echo "Azure DevOps extension not found. Installing..."
    az extension add --name azure-devops
fi

if command -v sudo &> /dev/null; then
    if [ "$(id -u)" -ne 0 ]; then
        sudo echo -n
    fi
fi

get_pipeline_run_id() {
    local runId=""
    runId=$(az pipelines runs list --organization $azdevopsOrg --project $azdevopsProject --pipeline-id $pipelineId --status completed --result succeeded --top 1 --query '[0].id' -o tsv)
    if [[ -z "$runId" ]]; then
        failedLogin=true
        echo "Unable to retreive pipeline run id, running \"az login\"" 1>&2
        if ! az login; then
            echo "Failed to login to Azure. Please check your credentials and try again." 1>&2
        else
            az account set --subscription $subscriptionId
            failedLogin=false
            runId=$(az pipelines runs list --organization $azdevopsOrg --project $azdevopsProject --pipeline-id $pipelineId --status completed --result succeeded --top 1 --query '[0].id' -o tsv)
        fi
        [[ "$failedLogin" = true ]] && { echo "Unable to retrieve pipeline run id after login attempt, please check credentials and try again." 1>&2; exit 1; }
    fi
    echo $runId
}

echo "Downloading latest Azure Policy packages from OneBranch pipeline..."
mkdir -p $packageDir
pipelineRunId=$(get_pipeline_run_id)
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
    # Start tests with given policy package and remediation enabled
    ./StartVMTest.sh -i $cacheDir/$tempImage -p $packageDir/$policyPackage -c $resourceCount -m $vmMemory -r -l $logDirectory > /dev/null
    if [[ $? -eq 0 ]]; then
        echo "✅ Test on $imageFile with $policyPackage completed successfully."
    else
        echo "❌ Test on $imageFile with $policyPackage failed."
    fi
    rm $cacheDir/$tempImage
}

download_image() {
    local imageFile=$1
    local_file_date=$(stat -c %Y "$cacheDir/$imageFile" 2>/dev/null)
    blob_date=$(az storage blob show --account-name "$storageAccount" --container-name "$containerName" --name "$imageFile" --auth-mode login --subscription $subscriptionId --query properties.lastModified --output tsv)
    blob_date_unix=$(date -d "$blob_date" +%s)
    if [[ $blob_date_unix -gt $local_file_date ]]; then
        echo "Newer $imageFile image found in blob storage, downloading image"
        az storage blob download --account-name $storageAccount --container-name $containerName --name $imageFile --file $cacheDir/$imageFile --auth-mode login --subscription $subscriptionId > /dev/null
    fi
}

# Download Distro Images
pids=()
declare -A downloaded_images
for row in $(echo "${test_data}" | jq -r '.[] | @base64'); do
    _jq() {
        echo ${row} | base64 --decode | jq -r ${1}
    }
    imageFile=$(_jq '.imageFile')
    countTotalTests=$((countTotalTests + 1))
    if [[ -z "${downloaded_images[$imageFile]}" ]]; then
        download_image $imageFile &
        pids+=($!)
        downloaded_images[$imageFile]=1
    fi
done
for pid in "${pids[@]}"; do
    wait $pid
done

# Start Tests using Distro Image
wait_with_timeout() {
    local pid=$1
    local timeout=$2
    
    local interval=1
    local elapsed=0
    
    while kill -0 $pid 2>/dev/null; do
        if [ $elapsed -ge $timeout ]; then
            return 1
        fi
        sleep $interval
        elapsed=$((elapsed + interval))
    done
    return 0
}
pids=()
exit_codes=()
countPendingTests=$countTotalTests
echo "Starting tests (Total: $countTotalTests)..."
for row in $(echo "${test_data}" | jq -r '.[] | @base64'); do
    _jq() {
        echo ${row} | base64 --decode | jq -r ${1}
    }

    imageFile=$(_jq '.imageFile')
    policyPackage=$(_jq '.policyPackage')
    resourceCount=$(_jq '.resourceCount')

    # Wait for a slot if the number of concurrent jobs reaches the limit
    showWaitingMessage=true
    while [ ${#pids[@]} -ge $maxConcurrentJobs ]; do
        if [ "$showWaitingMessage" = true ]; then
            echo "[$((100 * $countCompletedTests / $countTotalTests))% Complete] Waiting for a slot to run the next test (Completed: $countCompletedTests, In Progress: $countInProgressTests, Queued: $countPendingTests) ..."
            showWaitingMessage=false
        fi
        for i in "${!pids[@]}"; do
            if wait_with_timeout ${pids[$i]} $waitInterval; then
                wait ${pids[$i]}
                exit_codes+=($?)
                unset pids[$i]
                countCompletedTests=$((countCompletedTests + 1))
                countInProgressTests=$((countInProgressTests - 1))
                showWaitingMessage=true
                break
            fi
        done
    done

    echo "Running tests with image: $imageFile, on policy package: $policyPackage with resource count: $resourceCount..."
    curtime=$(date +%Y%m%d_%H%M%S)
    logDir="$cacheDir/${curtime}_${imageFile%.*}"
    run_test $imageFile $policyPackage $resourceCount $vmmemory $logDir &
    pids+=($!)
    countInProgressTests=$((countInProgressTests + 1))
    countPendingTests=$((countPendingTests - 1))
    echo "  Log directory: $logDir"
    sleep 1
done
for pid in "${pids[@]}"; do
    wait $pid
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
