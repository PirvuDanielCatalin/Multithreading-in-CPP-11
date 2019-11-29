$Global:Words_Count = (Get-Content "$PSScriptRoot\Test Fisier.txt" | Measure-Object -Word).Words
$Global:Processes_Count = 0
$Global:Max_Process_Rank = 0
$Global:Leaves = @()

function Get-ProcessesInfo {
    param (
        [Parameter(Mandatory = $true)] $Words_Count,
        [Parameter(Mandatory = $true)] $Processes_Count,
        [Parameter(Mandatory = $true)] $Rank
    )

    if ($Words_Count -eq 1 -or $Words_Count -eq 2) {
        $Global:Processes_Count += 1
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

        Get-ProcessesInfo -Words_Count $Half1 -Processes_Count $Processes_Count -Rank $LeftChildRank
        Get-ProcessesInfo -Words_Count $Half2 -Processes_Count $Processes_Count -Rank $RightChildRank
        $Global:Processes_Count += 1
    }
}

Get-ProcessesInfo -Words_Count $Global:Words_Count -Processes_Count $Global:Processes_Count -Rank 0
# Write-Host $Global:Processes_Count
# Write-Host $Global:Max_Process_Rank
# Write-Host ($Global:Leaves -Join ", ")
mpiexec -n $Global:Max_Process_Rank $PSScriptRoot\MergeSort-SendReceive.exe $Global:Max_Process_Rank $Global:Words_Count $($Global:Leaves -Join ", ")
# Path of mpiexec.exe = "C:\Program Files\Microsoft MPI\Bin\mpiexec.exe"