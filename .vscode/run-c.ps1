param(
    [Parameter(Mandatory = $true)]
    [string]$SourceFile,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ProgramArgs
)

$sourcePath = Resolve-Path $SourceFile
$workspaceDir = Get-Location
$srcDir = Join-Path $workspaceDir "src"
$buildDir = Join-Path $workspaceDir "build"

$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $workspaceDir ".zig-cache-global"
New-Item -ItemType Directory -Force $env:ZIG_GLOBAL_CACHE_DIR | Out-Null
New-Item -ItemType Directory -Force $buildDir | Out-Null

$sdlRoot = Join-Path $workspaceDir "third_party\SDL3-3.2.24"
$sdlInclude = Join-Path $sdlRoot "include"
$sdlLib = Join-Path $sdlRoot "lib\x64"
$sdlDll = Join-Path $sdlLib "SDL3.dll"

$sourceFiles = @()
if (Test-Path $srcDir) {
    $sourceFiles = Get-ChildItem $srcDir -Filter *.c | Sort-Object Name | ForEach-Object { $_.FullName }
}

if ($sourceFiles.Count -eq 0) {
    $sourceFiles = @($sourcePath)
}

$outputExe = Join-Path $buildDir "project2.exe"

python -m ziglang cc -std=c11 -Wall -Wextra -pedantic @sourceFiles -I (Join-Path $workspaceDir "include") -I $sdlInclude -L $sdlLib -lSDL3 -o $outputExe
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$targetSdlDll = Join-Path $buildDir "SDL3.dll"
if (-not (Test-Path $targetSdlDll)) {
    Copy-Item -Path $sdlDll -Destination $targetSdlDll
}

& $outputExe @ProgramArgs
exit $LASTEXITCODE
