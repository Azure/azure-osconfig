# Copyright (c) Microsoft Corporation. All rights reserved.
# UniversalNRP.Tests.ps1
param (
    [Parameter(Mandatory)]
    [string] $PolicyPackage
)

Describe 'Validate Universal NRP' {
    # Perform monitoring - Get
    Context "Get" {
        BeforeAll {
            $result = Get-GuestConfigurationPackageComplianceStatus -Path $PolicyPackage -Verbose
        }

        It 'Ensure the total resource instances count' {    
            $result.resources | Should -HaveCount 20
        }

        It 'Ensure resons are properly populated' {
            foreach ($instance in $result.resources) {
                if ($instance.properties.Reasons.Code -eq "PASS") {
                    $instance.complianceStatus | Should -BeTrue
                }
                else if ($instance.properties.Reasons.Code -eq "FAIL") {
                    $instance.complianceStatus | Should -BeFalse
                }
                else {
                    throw "Reasons are not properly populated"
                }
            }
        }

        It 'Ensure all resource instances have status' {    
            $result.resources.complianceStatus | Should -Not -BeNullOrEmpty
        }
    }

    # Perform Remediation - Set
    Context "Set" {
        BeforeAll {
            Start-GuestConfigurationPackageRemediation -Path $PolicyPackage -Verbose
            # Wait for remediation to complete
            Start-Sleep -Seconds 60
            $result = Get-GuestConfigurationPackageComplianceStatus -Path $PolicyPackage -Verbose
        }

        It 'Ensure the total resource instances count' {    
            $result.resources | Should -HaveCount 20
        }

        It 'Ensure resons are properly populated' {
            foreach ($instance in $result.resources) {
                if ($instance.properties.Reasons.Code -eq "PASS") {
                    $instance.complianceStatus | Should -BeTrue
                }
                else if ($instance.properties.Reasons.Code -eq "FAIL") {
                    throw "There should not be any failing resources after remediation"
                }
                else {
                    throw "Reasons are not properly populated"
                }
            }
        }

        It 'Ensure all resource instances pass audit' {    
            $result.resources.complianceStatus | Should -BeTrue
        }

        It 'Ensure overall audit pass' {
            $result.complianceStatus | Should -BeTrue
        }
    }
}
