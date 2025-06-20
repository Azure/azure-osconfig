#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# StartTests.sh
# Description: This Microsoft internal only script is used to download the latest Azure Policy packages from internal pipelines
#              and run the tests on the specified test data provided.
# Usage: ./StartTests.sh [-r run-id] [-m vm-memory-mb] [-j max-concurrent-jobs] [-d policy-package-directory] [-n no-gui]
# Options:
#        -r run-id: Specify the run-id of the pipeline run to download the packages from (Default: latest-succeeded-run-id)
#        -m vm-memory-mb: Specify the memory in MB to be used for the VMs (Default: 512)
#        -j max-concurrent-jobs: Specify the maximum number of concurrent jobs to run the tests (Default: 5)
#        -d policy-package-directory: Specify the directory containing the policy packages to use (Default: download from Azure DevOps pipeline)
#        -n no-gui: Disable GUI for the VMs (Default: false)
#        -p no-parallel-downloads: Disable downloading images in paralell (Default: false)
# Dependencies: cloud-localds, qemu-system-x86, jq, az, bsdmainutils

test_data='[
    {"distroName": "amazonlinux-2", "imageFile": "amazonlinux-2.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "amazonlinux-2", "imageFile": "amazonlinux-2.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"},
    {"distroName": "centos-7", "imageFile": "centos-7.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "centos-7", "imageFile": "centos-7.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"},
    {"distroName": "centos-8", "imageFile": "centos-8.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "centos-8", "imageFile": "centos-8.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"},
    {"distroName": "debian-10", "imageFile": "debian-10.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "debian-10", "imageFile": "debian-10.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"},
    {"distroName": "oraclelinux-7", "imageFile": "oraclelinux-7.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "oraclelinux-7", "imageFile": "oraclelinux-7.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"},
    {"distroName": "rhel-7", "imageFile": "rhel-7.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "rhel-7", "imageFile": "rhel-7.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"},
    {"distroName": "rockylinux-9", "imageFile": "rockylinux-9.qcow2", "policyPackage": "AzureLinuxBaseline.zip", "qemuArgs": "-cpu host"},
    {"distroName": "rockylinux-9", "imageFile": "rockylinux-9.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip", "qemuArgs": "-cpu host"},
    {"distroName": "sles-12", "imageFile": "sles-12.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "sles-12", "imageFile": "sles-12.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"},
    {"distroName": "ubuntu-16.04", "imageFile": "ubuntu-16.04.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "ubuntu-16.04", "imageFile": "ubuntu-16.04.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"},
    {"distroName": "ubuntu-18.04", "imageFile": "ubuntu-18.04.qcow2", "policyPackage": "AzureLinuxBaseline.zip"},
    {"distroName": "ubuntu-18.04", "imageFile": "ubuntu-18.04.qcow2", "policyPackage": "LinuxSshServerSecurityBaseline.zip"}
]'

subscriptionId="ce58a062-8d06-4da9-a85b-28939beb0119"
storageAccount="osconfigstorage"
containerName="diskimages"
pipelineId=394569
pipelineRunId=""
policyDirectory=""
azdevopsOrg="https://dev.azure.com/msazure/"
azdevopsProject="One"
azdevopsArtifactName="drop_package_mc_packages"
vmmemory=512
maxConcurrentJobs=5
nogui=false
noParallelDownload=false

cacheDir=".cache"
packageDir="$cacheDir/packages"
countCompletedTests=0
countInProgressTests=0
countPendingTests=0
countTotalTests=0
waitInterval=2

usage() {
    echo "Usage: $0 [[-r run-id] | [-d policy-package-directory]] [-m vm-memory-mb] [-j max-concurrent-jobs] [-n no-gui] [-p no-parallel-downloads]" >&2
    exit 1
}

OPTSTRING=":j:m:r:d:np"

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
        r)
            pipelineRunId=${OPTARG}
            echo "Using pipeline run id: $pipelineRunId"
            ;;
        d)
            policyDirectory=${OPTARG}
            echo "Using policy directory: $policyDirectory"
            ;;
        n)
            nogui=true
            echo "No GUI mode enabled."
            ;;
        p)
            noParallelDownload=true
            echo "No parallel downloads enabled."
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

failedTests=false
declare -a test_summary=()
print_test_summary_table() {
    echo -e "\n"
    for i in "${!test_summary[@]}"; do
        echo -e "${test_summary[$i]}"
    done | column -s $'|' -t
}
test_summary+=("Result|Distro Name|Policy Package|Total|Errors|Failures|Skipped|Log Directory")
test_summary+=("------|-----------|--------------|-----|------|--------|-------|-------------")

print_test_summary() {
    sumTests=0; sumErrors=0; sumFailures=0; sumSkipped=0
    for test in "${!testToLogDirMapping[@]}"; do
        logDir=${testToLogDirMapping[$test]}
        pid=${testToPidMapping[$test]}
        exitCode=${pidToExitCodeMapping[$pid]}
        # Extract distro name and policy package from test key (distroName--policyPackage)
        distroName=$(echo $test | awk -F'--' '{print $1}')
        policyPackage=$(echo $test | awk -F'--' '{print $2}')
        result="Pass"
        logArchive="$(find $logDir -name '*.tar.gz')"
        tempDir=$(mktemp -d)
        tar -xzf $logArchive -C $tempDir &> /dev/null
        testReport="$(find $tempDir -name '*.xml')"
        # If there is no test report, consider the test as failed
        if [ ! -f "$testReport" ]; then
            result="Fail"
            failedTests=true
            test_summary+=("$result|$distroName|$policyPackage|0|0|0|0|$logDir")
            rm -rf $tempDir
            continue
        fi
        totalTests=$(grep 'testsuite.*UniversalNRP.Tests.ps1.*' $testReport | awk -F'tests="' '{print $2}' | awk -F'"' '{print $1}')
        totalErrors=$(grep 'testsuite.*UniversalNRP.Tests.ps1.*' $testReport | awk -F'errors="' '{print $2}' | awk -F'"' '{print $1}')
        totalFailures=$(grep 'testsuite.*UniversalNRP.Tests.ps1.*' $testReport | awk -F'failures="' '{print $2}' | awk -F'"' '{print $1}')
        totalSkipped=$(grep 'testsuite.*UniversalNRP.Tests.ps1.*' $testReport | awk -F'skipped="' '{print $2}' | awk -F'"' '{print $1}')
        sumTests=$((sumTests + totalTests))
        sumErrors=$((sumErrors + totalErrors))
        sumFailures=$((sumFailures + totalFailures))
        sumSkipped=$((sumSkipped + totalSkipped))
        rm -rf $tempDir

        if [ "$exitCode" -gt 0 ] || [ "$totalErrors" -gt 0 ] || [ "$totalFailures" -gt 0 ]; then
            failedTests=true
            result="Fail"
        fi

        test_summary+=("$result|$distroName|$policyPackage|$totalTests|$totalErrors|$totalFailures|$totalSkipped|$logDir")
    done
    test_summary+=(" | |------|-----|------|--------|-------| ")
    test_summary+=(" | |TOTALS|$sumTests|$sumErrors|$sumFailures|$sumSkipped| ")
    print_test_summary_table

    if [ "$failedTests" = true ]; then
        echo "❌ Tests failed!" >&2
        exit 1
    else
        echo "✅ Tests successfull!"
        exit 0
    fi
}

trap print_test_summary SIGINT

# Ensure local dependencies are installed [cloud-localds, qemu-system-x86_64, jq, az, column]
dependencies=(cloud-localds qemu-system-x86_64 jq az column)
for dep in "${dependencies[@]}"; do
    if ! command -v $dep &> /dev/null; then
        echo "$dep not found. Please install it and try again." >&2
        exit 1
    fi
done

if command -v sudo &> /dev/null; then
    if [ "$(id -u)" -ne 0 ]; then
        sudo echo -n
    fi
fi

# Check if the Azure DevOps extension is already installed
if ! az extension list --output table | grep -q azure-devops; then
    echo "Azure DevOps extension not found. Installing..."
    az extension add --name azure-devops
fi

azure_login() {
    if ! az account show --subscription "$subscriptionId" &> /dev/null; then
        echo "ERROR: Subscription '$subscriptionId' is not available in this Azure context." >&2
        az account list --output table
        exit 1
    fi
    az account set --subscription $subscriptionId
}

get_pipeline_run_id() {
    local runId=""
    runId=$(az pipelines runs list --organization $azdevopsOrg --project $azdevopsProject --pipeline-id $pipelineId --status completed --result succeeded --top 1 --query '[0].id' -o tsv)
    if [[ -z "$runId" ]]; then
        failedLogin=true
        echo "Unable to retreive pipeline run id, running \"az login\"" >&2
        if ! az login; then
            echo "Failed to login to Azure. Please check your credentials and try again." >&2
        else
            azure_login
            failedLogin=false
            runId=$(az pipelines runs list --organization $azdevopsOrg --project $azdevopsProject --pipeline-id $pipelineId --status completed --result succeeded --top 1 --query '[0].id' -o tsv)
        fi
        [[ "$failedLogin" = true ]] && { echo "Unable to retrieve pipeline run id after login attempt, please check credentials and try again." >&2; exit 1; }
    fi
    echo $runId
}

download_policy_packages() {
    echo "Downloading latest Azure Policy packages from OneBranch pipeline..."
    pipelineRunId=$(get_pipeline_run_id)
    echo "Using latest succeeded run:$pipelineRunId"
    az pipelines runs artifact download --organization $azdevopsOrg --project $azdevopsProject --artifact-name $azdevopsArtifactName --path $packageDir --run-id $pipelineRunId
}

mkdir -p $packageDir
if [[ -z "$policyDirectory" ]]; then
    # If no policy directory provided, download the latest policy packages from Azure DevOps pipeline
    download_policy_packages
else
    # If policy directory is provided, copy the packages from the specified directory
    cp -r "$policyDirectory"/* $packageDir
    azure_login
fi

run_test() {
    local imageFile=$1
    local policyPackage=$2
    local vmMemory=$3
    local logDirectory=$4
    local distroName=$5
    local qemuArgs=$6

    local curtime=$(date +%Y%m%d_%H%M%S)
    local tempImage="${curtime}_${imageFile}"
    cp $cacheDir/$imageFile $cacheDir/$tempImage
    # Start tests with given policy package and remediation enabled
    ./StartVMTest.sh -i $cacheDir/$tempImage -p $packageDir/$policyPackage -m $vmMemory -r -l $logDirectory -h $distroName-osconfige2etest ${qemuArgs:+-a "$qemuArgs"} > /dev/null
    rm $cacheDir/$tempImage
}

download_image() {
    local imageFile=$1
    local_file_md5=$(md5sum "$cacheDir/$imageFile" 2>/dev/null | awk '{ print $1 }')
    blob_md5=$(az storage blob show --account-name "$storageAccount" --container-name "$containerName" --name "$imageFile" --auth-mode login --subscription $subscriptionId --query properties.contentSettings.contentMd5 --output tsv)
    if [[ "$blob_md5" != "$local_file_md5" ]]; then
        echo "MD5 hash mismatch for $imageFile, downloading image"
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
        if [ "$noParallelDownload" = true ]; then
            download_image $imageFile
        else
            download_image $imageFile &
            pids+=($!)
        fi
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
countPendingTests=$countTotalTests
declare -A testToLogDirMapping
declare -A testToPidMapping
declare -A pidToExitCodeMapping
echo "Starting tests [Total: $countTotalTests]"
for row in $(echo "${test_data}" | jq -r '.[] | @base64'); do
    _jq() {
        echo ${row} | base64 --decode | jq -r ${1}
    }

    distroName=$(_jq '.distroName')
    imageFile=$(_jq '.imageFile')
    policyPackage=$(_jq '.policyPackage')
    qemuArgs=$(_jq '.qemuArgs')
    if [ "$qemuArgs" = "null" ]; then
        qemuArgs=""
    fi
    # if nogui is true, append -display none to qemuArgs
    if [ "$nogui" = true ]; then
        qemuArgs="${qemuArgs} -display none"
    fi

    # Wait for a slot if the number of concurrent jobs reaches the limit
    showWaitingMessage=true
    while [ ${#pids[@]} -ge $maxConcurrentJobs ]; do
        if [ "$showWaitingMessage" = true ]; then
            echo -ne "\r\033[K[$((100 * $countCompletedTests / $countTotalTests))% Complete] Waiting for a slot to run the next test (Completed: $countCompletedTests, In Progress: $countInProgressTests, Queued: $countPendingTests) ..."
            showWaitingMessage=false
        fi
        for i in "${!pids[@]}"; do
            if wait_with_timeout ${pids[$i]} $waitInterval; then
                # Test completed
                wait ${pids[$i]}
                exit_code=$?
                pidToExitCodeMapping[${pids[$i]}]=$exit_code
                unset pids[$i]
                countCompletedTests=$((countCompletedTests + 1))
                countInProgressTests=$((countInProgressTests - 1))
                showWaitingMessage=true
                break
            else
                # Test still running
                echo -n "."
            fi
        done
    done

    curtime=$(date +%Y%m%d_%H%M%S)
    logDir="$cacheDir/${curtime}_${imageFile%.*}"
    run_test $imageFile $policyPackage $vmmemory $logDir $distroName "$qemuArgs" &
    testPid=$!
    pids+=($testPid)

    testToPidMapping["$distroName--$policyPackage"]=$testPid
    testToLogDirMapping["$distroName--$policyPackage"]=$logDir

    countInProgressTests=$((countInProgressTests + 1))
    countPendingTests=$((countPendingTests - 1))
    sleep 1
done
# Queue is empty as this point, wait for all tests to complete
showWaitingMessage=true
while [ ${#pids[@]} -gt 0 ]; do
    for i in "${!pids[@]}"; do
        if [ "$showWaitingMessage" = true ]; then
            echo -ne "\r\033[K[$((100 * $countCompletedTests / $countTotalTests))% Complete] (Completed: $countCompletedTests, In Progress: $countInProgressTests, Queued: $countPendingTests) ..."
            showWaitingMessage=false
        fi
        if wait_with_timeout ${pids[$i]} $waitInterval; then
            wait ${pids[$i]}
            exit_code=$?
            pidToExitCodeMapping[${pids[$i]}]=$exit_code
            unset pids[$i]
            countCompletedTests=$((countCompletedTests + 1))
            countInProgressTests=$((countInProgressTests - 1))
            showWaitingMessage=true
        else
            echo -n "."
        fi
    done
done

# Collect and print test summary
echo -e "\nAll tests are complete."
print_test_summary
