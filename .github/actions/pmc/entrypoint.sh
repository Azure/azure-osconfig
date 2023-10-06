#!/bin/sh -l

output=$(pmc \
    --base-url $PMC_CLI_BASE_URL \
    --msal-client-id $PMC_CLI_MSAL_CLIENT_ID \
    --msal-scope $PMC_CLI_MSAL_SCOPE \
    --msal-authority $PMC_CLI_MSAL_AUTHORITY \
    --msal-cert-path $PMC_CLI_MSAL_CERT_PATH \
    --msal-sniauth \
    $1)

if [ $? -ne 0 ]; then
  exit 1
fi

echo result=$output >> $GITHUB_OUTPUT
