param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskBuild = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $taskBuild | Out-Null
$taskApp = Join-Path $taskRoot 'USER\APP'
$taskExe = Join-Path $taskBuild 'test_core.exe'
& $Compiler -std=c99 -Wall -Wextra -Werror -pedantic -I $taskApp `
    (Join-Path $PSScriptRoot 'test_core.c') (Join-Path $taskApp 'app_core.c') `
    (Join-Path $taskApp 'control_math.c') (Join-Path $taskApp 'vision_protocol.c') -lm -o $taskExe
if ($LASTEXITCODE -ne 0) { throw 'Core test compilation failed' }
& $taskExe
if ($LASTEXITCODE -ne 0) { throw 'Core tests failed' }
$taskKeys = Join-Path $taskRoot 'HAREWER\KEY'
$taskKeyExe = Join-Path $taskBuild 'test_keys.exe'
& $Compiler -std=c99 -Wall -Wextra -Werror -pedantic `
    -I (Join-Path $PSScriptRoot 'stubs') -I $taskKeys `
    (Join-Path $PSScriptRoot 'test_keys.c') (Join-Path $taskKeys 'key.c') -o $taskKeyExe
if ($LASTEXITCODE -ne 0) { throw 'Key test compilation failed' }
& $taskKeyExe
if ($LASTEXITCODE -ne 0) { throw 'Key tests failed' }
