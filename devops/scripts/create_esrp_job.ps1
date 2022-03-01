<#
  .SYNOPSIS
  MSFT-Internal ESRP Signing Tool

  .DESCRIPTION
  Creates an ESRP job manifest for signing production binaries (MSFT-Internal)

  .PARAMETER FilesToSign
  Files to submit for signing

  .PARAMETER SourceRoot
  The source path to use for the signing root

  .PARAMETER TargetRoot
  The target path to use for placing the signed binaries

  .PARAMETER JobFile
  The name for the job JSON file to create (Default: Job.json)

  .INPUTS
  None

  .OUTPUTS
  Job.json

  .EXAMPLE
  PS> .\create_esrp_job.ps1 osconfig_w.x.y.z_alpha.deb C:\Input C:\Output

  .EXAMPLE
  PS> .\create_esrp_job.ps1 @("osconfig_w.x.y.z_alpha.deb","osconfig_w.x.y.z_beta.deb") C:\Input C:\Output

  .EXAMPLE
  PS> .\create_esrp_job.ps1 (ls .\Input\ | select -expand Name) ($pwd.Path + '\Input') ($pwd.Path + '\Output')
#>

param(
    [Parameter(Mandatory=$true)]
    [String[]] $FilesToSign,
    [Parameter(Mandatory=$true)]
    [String] $SourceRoot,
    [Parameter(Mandatory=$true)]
    [String] $TargetRoot,
    [String] $JobFile = "Job.json"
)

$TO_SIGN = @()
foreach ($file in $FilesToSign)
{
  $TO_SIGN += @"
    {
        "SourceLocation": "$file"
    },
"@
    Write-Host Adding "$file" to signing list
}

$json = @"
{
  "Version": "1.0.0",
  "SignBatches": [
    {
      "SourceLocationType": "UNC",
      "SourceRootDirectory": "$SourceRoot",
      "DestinationLocationType": "UNC",
      "DestinationRootDirectory": "$TargetRoot",
      "SignRequestFiles": [
        $TO_SIGN
      ],
      "SigningInfo": {
        "Operations": [
          {
            "KeyCode": "CP-450779-Pgp",
            "OperationCode": "LinuxSign",
            "Parameters": {},
            "ToolName": "sign",
            "ToolVersion": "1.0"
          }
        ]
      }
    }
  ]
}
"@
$json | ConvertTo-Json | % { [System.Text.RegularExpressions.Regex]::Unescape($_).Replace("\","\\").Trim('"') } | Set-Content $JobFile