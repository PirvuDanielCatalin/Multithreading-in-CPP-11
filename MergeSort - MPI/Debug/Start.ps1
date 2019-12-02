$Global:Words_Count = 0

function Get-WordsCount() {
    param (
        [Parameter(Mandatory = $true)] $File_Name
    )
        
    $Content = Get-Content "$PSScriptRoot\$File_Name"
    $Global:Words_Count = ($Content | Measure-Object -Word).Words
}

Get-WordsCount

$Global:Max_Process_Rank = 0
$Global:Leaves = @()
$Global:Dormant = @()

function Get-ProcessesInfo {
    param (
        [Parameter(Mandatory = $true)] $Words_Count,
        [Parameter(Mandatory = $true)] $Rank
    )

    if ($Words_Count -eq 1 -or $Words_Count -eq 2) {
        if ($Rank -gt $Global:Max_Process_Rank) {
            $Global:Max_Process_Rank = $Rank
        }
        $Global:Leaves += ($Rank)
    }
    else {
        [int] $Half1 = [System.Math]::Floor($Words_Count / 2)
        [int] $Half2 = $Words_Count - $Half1

        [int] $LeftChildRank = 2 * $Rank + 1
        [int] $RightChildRank = 2 * $Rank + 2

        Get-ProcessesInfo -Words_Count $Half1 -Rank $LeftChildRank
        Get-ProcessesInfo -Words_Count $Half2 -Rank $RightChildRank
    }
}

if ($Global:Words_Count -ne 0) {
    Get-ProcessesInfo -Words_Count $Global:Words_Count -Rank 0

    $Global:Tree_Height = [int]([Math]::Log($Global:Max_Process_Rank) / [Math]::Log(2))
    
    $Start = [int]([Math]::Pow(2, $Global:Tree_Height) - 1)
    $Final = 2 * $Start

    for ($Rank = $Start; $Rank -le $Final; $Rank++) {
        if (!$Global:Leaves.Contains($Rank)) {
            $Global:Dormant += ($Rank)
        }
    }

    Write-Host "-------------------------------------------------"
    Write-Host "Max rank is"$Global:Max_Process_Rank
    Write-Host "Tree height is"$Global:Tree_Height
    Write-Host "Array of leaves is ["($Global:Leaves -Join ", ")"]"
    Write-Host "Arry of dormant processes is ["($Global:Dormant -Join ", ")"] "
    Write-Host "-------------------------------------------------"

    # Run MPI C++ Program
    mpiexec -l -n ($Global:Max_Process_Rank + 1) $PSScriptRoot\MergeSort-SendReceive.exe $Global:Max_Process_Rank $Global:Words_Count $($Global:Dormant -Join ", ")
    # Path of mpiexec.exe = "C:\Program Files\Microsoft MPI\Bin\mpiexec.exe"
}
