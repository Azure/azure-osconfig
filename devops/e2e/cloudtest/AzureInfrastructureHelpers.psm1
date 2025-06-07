# Copyright (c) Microsoft Corporation. All rights reserved.

#Requires -Modules Az, Hyper-V
<#
.SYNOPSIS
Creates an Azure managed disk from a local VHD file and adds it to an Azure Compute Gallery.

.DESCRIPTION
This function uploads a local VHD file to Azure as a managed disk, assigns the necessary role to the current user, and adds the disk to an Azure Compute Gallery. It also creates an image definition and an image version in the gallery.

.PARAMETER DiskPath
The local path to the VHD file.

.PARAMETER SubscriptionName
The name of the Azure subscription.

.PARAMETER ResourceGroupName
The name of the resource group.

.PARAMETER DiskName
The name of the managed disk to be created.

.PARAMETER GalleryName
The name of the Azure Compute Gallery.

.PARAMETER OSType
The operating system type of the disk (e.g., Windows or Linux).

.PARAMETER Publisher
The publisher of the image.

.PARAMETER Offer
The offer of the image.

.PARAMETER Sku
The SKU of the image.

.PARAMETER Location
The Azure region where the resources will be created.

.PARAMETER ImageVersionName
The name of the image version to be created.

.PARAMETER OSState
The state of the operating system (e.g., Generalized or Specialized).

.EXAMPLE
Create-AzureManagedDisk -DiskPath "C:\VHDs\mydisk.vhd" -SubscriptionName "MySubscription" -ResourceGroupName "MyResourceGroup" -DiskName "MyDisk" -GalleryName "MyGallery" -OSType "Windows" -Publisher "MyPublisher" -Offer "MyOffer" -Sku "MySku" -Location "EastUS" -ImageVersionName "1.0.0" -OSState "Generalized"

.INPUTS
None. The function does not accept pipeline input.

.OUTPUTS
PSGalleryImageVersion
#>
function Create-AzureManagedDisk {
    param (
        [Parameter(Mandatory=$true)]
        [string] $DiskPath,

        [Parameter(Mandatory=$true)]
        [string] $SubscriptionName,

        [Parameter(Mandatory=$true)]
        [string] $ResourceGroupName,

        [Parameter(Mandatory=$true)]
        [string] $DiskName,

        [Parameter(Mandatory=$true)]
        [string] $GalleryName,

        [Parameter(Mandatory=$true)]
        [string] $OSType,

        [Parameter(Mandatory=$true)]
        [string] $Publisher,

        [Parameter(Mandatory=$true)]
        [string] $Offer,

        [Parameter(Mandatory=$true)]
        [string] $Sku,

        [Parameter(Mandatory=$true)]
        [string] $Location,

        [Parameter(Mandatory=$true)]
        [string] $ImageVersionName,

        [Parameter(Mandatory=$true)]
        [string] $OSState
    )

    $ErrorActionPreference = "Stop"

    if (-not (Get-AzContext)) {
        Write-Host "Signing into Azure..."
        Connect-AzAccount
    }
    $subscription = Get-AzSubscription -SubscriptionName $SubscriptionName
    if ($null -eq $subscription) {
        throw "Subscription with name '$SubscriptionName' not found."
    }

    # Create managed disk for image gallery
    Set-AzContext -SubscriptionId $subscription.Id
    $currentUser = Get-AzADUser -UserPrincipalName (Get-AzContext).Account.Id
    $roleAssignment = Get-AzRoleAssignment -SignInName $currentUser.userPrincipalName -RoleDefinitionName "Data Operator for Managed Disks" -Scope "/subscriptions/$subscription"
    if ($null -eq $roleAssignment) {
        Write-Host "Assigning role 'Data Operator for Managed Disks' to user '$($currentUser.userPrincipalName)' in subscription '$($subscription.Name)'"
        New-AzRoleAssignment -SignInName $currentUser.userPrincipalName -RoleDefinitionName "Data Operator for Managed Disks" -Scope "/subscriptions/$subscription"
    }
    $disk = Add-AzVhd -LocalFilePath $DiskPath -ResourceGroupName $ResourceGroupName -Location $Location -DiskName $DiskName -OsType $OSType

    # Add to Compute Gallery
    $existingImage = Get-AzGalleryImageDefinition -GalleryName $GalleryName -ResourceGroupName $ResourceGroupName -Name $DiskName -ErrorAction SilentlyContinue
    if ($null -eq $existingImage) {
        Write-Host "Creating image definition '$DiskName' in gallery '$GalleryName' ..."
        New-AzGalleryImageDefinition -GalleryName $GalleryName -ResourceGroupName $ResourceGroupName -Location $Location -Name $DiskName -OsType $OSType -Publisher $Publisher -Offer $Offer -Sku $Sku -OsState $OSState
    } else {
        Write-Host "Image definition '$DiskName' already exists in gallery '$GalleryName'."
    }

    # Create image version
    $managedDisk = Get-AzDisk -ResourceGroupName $ResourceGroupName -DiskName $DiskName
    $osDisk = @{Source = @{Id = $managedDisk.Id}}
    $existingImageVersion = Get-AzGalleryImageVersion -GalleryName $GalleryName -ResourceGroupName $ResourceGroupName -GalleryImageDefinitionName $DiskName -Name $ImageVersionName -ErrorAction SilentlyContinue
    if ($null -eq $existingImageVersion) {
        Write-Host "Creating image version '$ImageVersionName' for image definition '$DiskName' in gallery '$GalleryName'. Replicating, may take a few minutes..."
        $imageVersion = New-AzGalleryImageVersion -GalleryName $GalleryName -ResourceGroupName $ResourceGroupName -GalleryImageDefinitionName $DiskName -Name $ImageVersionName -Location $Location -OSDiskImage $osDisk
    } else {
        throw "Image version '$ImageVersionName' already exists for image definition '$DiskName' in gallery '$GalleryName'."
    }

    # Remove uneeded managed disk
    Remove-AzDisk -ResourceGroupName $ResourceGroupName -DiskName $DiskName -Force

    $imageVersion
}
Export-ModuleMember -Function Create-AzureManagedDisk
