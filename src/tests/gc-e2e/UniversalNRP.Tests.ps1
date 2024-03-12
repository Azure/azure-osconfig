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
            $result | Should -Not -BeNullOrEmpty
            $result.resources | Should -HaveCount 20
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
            $result | Should -Not -BeNullOrEmpty
            $result.resources | Should -HaveCount 20
        }

        It 'Ensure resons are populated' {
            $result | Should -Not -BeNullOrEmpty
            foreach ($instance in $result.resources) {
                $instance.properties.Reasons | Should -Not -BeNullOrEmpty
            }
        }

        It 'Ensure all resource instances pass audit' {    
            $result | Should -Not -BeNullOrEmpty
            $result.resources.complianceStatus | Should -BeTrue
        }

        It 'Ensure overall audit pass' {
            $result | Should -Not -BeNullOrEmpty
            $result.complianceStatus | Should -BeTrue
        }
    }
}
