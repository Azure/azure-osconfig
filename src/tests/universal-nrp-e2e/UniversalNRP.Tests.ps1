# Copyright (c) Microsoft Corporation. All rights reserved.
# UniversalNRP.Tests.ps1
param (
    [Parameter(Mandatory)]
    [string] $PolicyPackage,

    [Parameter(Mandatory)]
    [int] $ResourceCount,

    [bool] $SkipRemediation = $false
)

Describe 'Validate Universal NRP' {
    # Perform Remediation - Set
    Context "Set" -Skip:$SkipRemediation {
        It 'Perform remediation' {
            Start-GuestConfigurationPackageRemediation -Path $PolicyPackage
            # Wait for remediation to complete
            Start-Sleep -Seconds 30
        }
    }

    # Perform monitoring - Get
    Context "Get" {
        BeforeAll {
            $result = Get-GuestConfigurationPackageComplianceStatus -Path $PolicyPackage
        }

        It 'Ensure the total resource instances count' {
            $result.resources | Should -HaveCount $ResourceCount
        }

        It 'Ensure reasons are properly populated' {
            foreach ($instance in $result.resources) {
                $resourceName = $instance.properties.DesiredObjectName
                if ($instance.properties.Reasons.Code.StartsWith("BaselineSettingCompliant:{")) {
                    $instance.complianceStatus | Should -BeTrue -Because "Policy '$resourceName' has compliant reason code but wrong compliance status"
                } elseif ($instance.properties.Reasons.Code.StartsWith("BaselineSettingNotCompliant:{")) {
                    $instance.complianceStatus | Should -BeFalse -Because "Policy '$resourceName' has non-compliant reason code but wrong compliance status"
                } else {
                    throw "Reasons are not properly populated for policy '$resourceName'. Reason Code: $($instance.properties.Reasons.Code)"
                }
            }
        }

        It 'Ensure all resource instances have status' {
            $result.resources.complianceStatus | Should -Not -BeNullOrEmpty
        }
    }
}
