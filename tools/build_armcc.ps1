param([string]$ArmCompilerDirectory = $env:ARMCC_BIN)
$ErrorActionPreference = 'Stop'
if (-not $ArmCompilerDirectory) {
    $taskKeil = Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\Keil\Products\MDK' -ErrorAction SilentlyContinue
    if ($taskKeil) { $ArmCompilerDirectory = Join-Path $taskKeil.Path 'ARMCC\bin' }
}
if (-not $ArmCompilerDirectory) { throw 'Pass -ArmCompilerDirectory or set ARMCC_BIN.' }
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskUser = Join-Path $taskRoot 'USER'
$taskBuild = Join-Path $taskRoot 'OBJ\FreeRTOSCLI'
New-Item -ItemType Directory -Force -Path $taskBuild | Out-Null
[xml]$taskProject = Get-Content -Raw -LiteralPath (Join-Path $taskUser 'AT8236.uvprojx')
$taskTarget = $taskProject.Project.Targets.Target
$taskIncludes = $taskTarget.TargetOption.TargetArmAds.Cads.VariousControls.IncludePath -split ';'
$taskIncludeArgs = @()
foreach ($taskInclude in $taskIncludes) {
    $taskIncludeArgs += '-I' + [System.IO.Path]::GetFullPath((Join-Path $taskUser $taskInclude))
}
$taskObjects = @()
foreach ($taskFile in $taskTarget.Groups.Group.Files.File) {
    $taskSource = [System.IO.Path]::GetFullPath((Join-Path $taskUser $taskFile.FilePath))
    $taskObject = Join-Path $taskBuild ([System.IO.Path]::GetFileNameWithoutExtension($taskSource) + '.o')
    Write-Output ('Compiling ' + $taskFile.FileName)
    if ($taskFile.FileType -eq '2') {
        & (Join-Path $ArmCompilerDirectory 'armasm.exe') --cpu Cortex-M3 --pd '__MICROLIB SETA 1' -o $taskObject $taskSource
    } else {
        & (Join-Path $ArmCompilerDirectory 'armcc.exe') --cpu Cortex-M3 --c99 --apcs=interwork `
            --library_type=microlib --split_sections -O1 -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER `
            @taskIncludeArgs -c $taskSource -o $taskObject
    }
    if ($LASTEXITCODE -ne 0) { throw ('Compilation failed: ' + $taskFile.FileName) }
    $taskObjects += $taskObject
}
$taskAxf = Join-Path $taskBuild 'BallCar_FreeRTOS.axf'
& (Join-Path $ArmCompilerDirectory 'armlink.exe') --cpu Cortex-M3 --library_type=microlib `
    --scatter (Join-Path $taskUser 'ball_car.sct') --entry Reset_Handler --map --info sizes `
    --list (Join-Path $taskBuild 'BallCar_FreeRTOS.map') @taskObjects -o $taskAxf
if ($LASTEXITCODE -ne 0) { throw 'Linking failed' }
& (Join-Path $ArmCompilerDirectory 'fromelf.exe') --i32combined `
    --output (Join-Path $taskBuild 'BallCar_FreeRTOS.hex') $taskAxf
if ($LASTEXITCODE -ne 0) { throw 'HEX generation failed' }
Write-Output ('ARM build passed: ' + $taskAxf)
