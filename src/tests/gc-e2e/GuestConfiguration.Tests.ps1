# Copyright (c) Microsoft Corporation. All rights reserved.
# GuestConfiguration.Tests.ps1
param (
    [Parameter(Mandatory)]
    [string] $PolicyPackage
)

Describe 'Validate GuestConfiguration NRP' {
    # Perform monitoring - Get
    Context "Get" {
        BeforeAll {
            $result = Get-GuestConfigurationPackageComplianceStatus -Path $PolicyPackage -Verbose
        }

        It 'Correct resource count - 20' {    
            $result | Should -Not -BeNullOrEmpty
            $result.resources | Should -HaveCount 20
        }
    }

    # Perform Remediation - Set
    Context "Set" {
        BeforeAll {
            $result = Start-GuestConfigurationPackageRemediation -Path $PolicyPackage -Verbose
        }

        It 'Ensure resource count = 20' {    
            $result | Should -Not -BeNullOrEmpty
            $result.resources | Should -HaveCount 20
        }

        It 'Ensure resources are all compliant' {    
            $result | Should -Not -BeNullOrEmpty
            $result.resources.complianceStatus | Should -BeTrue
        }

        It 'Ensure whole policy is compliant' {
            $result | Should -Not -BeNullOrEmpty
            $result.complianceStatus | Should -BeTrue
        }
    }
}
