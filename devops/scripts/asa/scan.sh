#!/bin/bash

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set -e

ASA_FAIL_ON="error"
OUTPUT_DIR="report"
AFTER_AUDIT_REPORT_DIR=${OUTPUT_DIR}/after_audit
AFTER_REMEDIATION_REPORT_DIR=${OUTPUT_DIR}/after_remediation
OSCONFIG_LOG_FILE="/var/log/osconfig_nrp.log"
ASA_LOG_FILENAME="asa.log"

function clear_osconfig_logs {
  echo "" > ${OSCONFIG_LOG_FILE}
}

function log {
    local msg=$1
    local level=$2

    echo "[$(date +%H:%M:%S) ${level}] ${msg}"
}

function print_usage {
    echo -e ""
    echo -e "Usage: $(basename ${0}) [OPTIONS]"
    echo -e ""
    echo -e "\t-p, --package\t\tURL to policy package ZIP file"
    echo -e "\t-c, --checksum\t\tPolicy package file SHA256 check sum to verify before executing anything"
    echo -e "\t-fo, --failon\t\tFail if there were findings in ASA scan with given severity level or above: error, warning, note"
    echo -e ""
    echo -e "Example:"
    echo -e "\t$(basename ${0}) -p https://host/AzureLinuxBaseline.zip -fo error"
    echo -e ""
}

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -p|--package) POLICY_PACKAGE_FILE="$2"; shift ;;
        -c|--checksum) POLICY_PACKAGE_SHA256="$2"; shift ;;
        -fo|--failon) ASA_FAIL_ON="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; print_usage; exit 1 ;;
    esac
    shift
done

POLICY_NAME=$(basename "$POLICY_PACKAGE_FILE")

if [ -n "${POLICY_PACKAGE_FILE}" ]; then
  log "Downloading package policy file..." "INFO"
  wget "${POLICY_PACKAGE_FILE}" -O "${POLICY_NAME}"
  log "Package policy file downloaded." "INFO"
else
  log "Policy package file URL not provided." "ERROR"
  print_usage
  exit 1
fi

if [ -n "${POLICY_PACKAGE_SHA256}" ]; then
  log "Checksum provided, checking policy package SHA256..." "INFO"
  if echo "${POLICY_PACKAGE_SHA256} ./${POLICY_NAME}" | sha256sum --check --status; then
    log "Wrong policy package checksum." "ERROR"
    exit 1
  fi
else
  log "Policy package checksum wasn't provided. The check was omitted." "WARNING"
fi

if [ ! -s "./${POLICY_NAME}" ]; then
  log "Policy file (./${POLICY_NAME}) does not exists or empty." "ERROR"
  exit 2
fi

log "Creating report directories..." "INFO"
mkdir -p ${AFTER_AUDIT_REPORT_DIR}
mkdir -p ${AFTER_REMEDIATION_REPORT_DIR}

log "Running ASA collection before GC audit..." "INFO"
echo "ASA COLLECT BEFORE GC AUDIT" > ${OUTPUT_DIR}/${ASA_LOG_FILENAME}
asa collect -a >> ${OUTPUT_DIR}/${ASA_LOG_FILENAME}

log "Running Get-GuestConfigurationPackageComplianceStatus..." "INFO"
clear_osconfig_logs
pwsh --command "Get-GuestConfigurationPackageComplianceStatus -Path ./${POLICY_NAME}"
cp ${OSCONFIG_LOG_FILE} ${AFTER_AUDIT_REPORT_DIR}

log "Running ASA collection after GC audit..." "INFO"
echo ""; echo "ASA COLLECT AFTER GC AUDIT" >> ${OUTPUT_DIR}/${ASA_LOG_FILENAME}
asa collect -a >> ${OUTPUT_DIR}/${ASA_LOG_FILENAME}
asa export-collect --outputsarif --outputpath ${AFTER_AUDIT_REPORT_DIR} >> ${OUTPUT_DIR}/${ASA_LOG_FILENAME}

log "Running Start-GuestConfigurationPackageRemediation..." "INFO"
clear_osconfig_logs
pwsh --command "Start-GuestConfigurationPackageRemediation -Path ./${POLICY_NAME}"
cp ${OSCONFIG_LOG_FILE} ${AFTER_REMEDIATION_REPORT_DIR}

log "Running ASA collection after GC remediation..." "INFO"
echo ""; echo "ASA COLLECT AFTER GC REMEDIATION" >> ${OUTPUT_DIR}/${ASA_LOG_FILENAME}
asa collect -a >> ${OUTPUT_DIR}/${ASA_LOG_FILENAME}
asa export-collect --outputsarif --outputpath ${AFTER_REMEDIATION_REPORT_DIR} >> ${OUTPUT_DIR}/${ASA_LOG_FILENAME}

# Another run just to check compliance status after remediation
log "Running Get-GuestConfigurationPackageComplianceStatus..." "INFO"
pwsh --command "Get-GuestConfigurationPackageComplianceStatus -Path ./${POLICY_NAME}"

AFTER_AUDIT_SARIF=$(ls -Art ${AFTER_AUDIT_REPORT_DIR}/*.Sarif | tail -n 1)
AFTER_REMEDIATION_SARIF=$(ls -Art ${AFTER_REMEDIATION_REPORT_DIR}/*.Sarif | tail -n 1)

log "Checking after audit ASA report..." "INFO"
sarif --check ${ASA_FAIL_ON} summary ${AFTER_AUDIT_SARIF}
log "Checking after remediation ASA report..." "INFO"
sarif --check ${ASA_FAIL_ON} summary ${AFTER_REMEDIATION_SARIF}
log "All good! Finishing..." "INFO"
