#!/bin/bash
# This script requires Azure CLI version 2.25.0 or later. Check version with `az --version`.

# SERVICE_PRINCIPAL_NAME: Must be unique within your AD tenant
SERVICE_PRINCIPAL_NAME=<service_principal_name>
# Replace with valid Azure Subscription ID
SUBSCRIPTION_ID=<subscription_id>

# Create the service principal with rights scoped to the subscription.
SP_PASSWD=$(az ad sp create-for-rbac --name $SERVICE_PRINCIPAL_NAME --scopes /subscriptions/$SUBSCRIPTION_ID --role Contributor --query password --output tsv)
SP_APP_ID=$(az ad sp list --display-name $SERVICE_PRINCIPAL_NAME --query [].appId --output tsv)

# Output the service principal's credentials; use these in your services and applications to authenticate.
echo "Service principal ID: $SP_APP_ID"
echo "Service principal password: $SP_PASSWD"