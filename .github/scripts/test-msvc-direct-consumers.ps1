param(
    [Parameter(Mandatory = $true)]
    [string]$Prefix,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeLibrary
)

$ErrorActionPreference = 'Stop'

$pcdir = Join-Path $Prefix 'lib\pkgconfig'
@(
    'xpdev.pc',
    'xpdev-static.pc',
    'xpdev-comio.pc',
    'xpdev-comio-static.pc',
    'xpdev-encode.pc',
    'xpdev-encode-static.pc',
    'xpdev-hash.pc',
    'xpdev-hash-static.pc'
) | ForEach-Object {
    if (-not (Test-Path (Join-Path $pcdir $_))) {
        throw "pkg-config module was not installed: $_"
    }
}

$dynamicPc = Get-Content -Raw (Join-Path $pcdir 'xpdev.pc')
if ($dynamicPc -notmatch '-lxpdev-1' -or
    $dynamicPc -notmatch '-DWRAPPER_IMPORTS') {
    throw 'xpdev.pc does not describe the DLL import interface'
}
$staticPc = Get-Content -Raw (Join-Path $pcdir 'xpdev-static.pc')
if ($staticPc -notmatch '-lxpdev_static' -or
    $staticPc -match 'WRAPPER_IMPORTS' -or
    $staticPc -notmatch '-liphlpapi') {
    throw 'xpdev-static.pc does not describe the static interface'
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$install = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
$vcvars = Join-Path $install 'VC\Auxiliary\Build\vcvars64.bat'
& cmd.exe /d /s /c "`"$vcvars`" >nul && set" |
    ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            Set-Item -Force "env:$($Matches[1])" $Matches[2]
        }
    }

$include = Join-Path $Prefix 'include'
$lib = Join-Path $Prefix 'lib'
$runtimeFlag = if ($RuntimeLibrary -match 'MultiThreadedDLL') {
    '/MD'
} else {
    '/MT'
}

function Build-DirectConsumer($name, $language, $linkage) {
    $compilerArgs = @(
        '/nologo',
        $runtimeFlag,
        "/I$include",
        '/DXPDEV_USE_CONFIG_H',
        '/D_WIN32',
        '/D_WIN32_WINNT=0x0501',
        '/DWINVER=0x0501',
        '/DMSVCRT_VERSION=0x0501',
        '/D_WIN32_IE=0x0500'
    )
    if ($language -eq 'C') {
        $source = (Resolve-Path 'tests/pkg-config-consumer.c').Path
        $compilerArgs += @('/TC', '/std:c11', '/experimental:c11atomics')
    } else {
        $source = (Resolve-Path 'tests/pkg-config-consumer.cpp').Path
    }

    if ($linkage -eq 'shared') {
        $compilerArgs += @(
            '/DWRAPPER_IMPORTS',
            '/DCOMIO_IMPORTS',
            '/DB64_IMPORTS',
            '/DXPDEV_ENCODE_IMPORTS',
            '/DMD5_IMPORTS',
            '/DXPDEV_HASH_IMPORTS'
        )
        $libraries = @(
            'xpdev-comio-1.lib',
            'xpdev-encode-1.lib',
            'xpdev-hash-1.lib',
            'xpdev-1.lib'
        )
    } else {
        $compilerArgs += @('/DXPDEV_ENCODE_STATIC', '/DXPDEV_HASH_STATIC')
        $libraries = @(
            'xpdev-comio_static.lib',
            'xpdev-encode_static.lib',
            'xpdev-hash_static.lib',
            'xpdev_static.lib',
            'iphlpapi.lib',
            'ws2_32.lib',
            'winmm.lib',
            'netapi32.lib',
            'ole32.lib',
            'uuid.lib'
        )
    }

    $executable = Join-Path (Get-Location) "$name.exe"
    $compilerArgs += @($source, "/Fe:$executable", '/link', "/libpath:$lib")
    $compilerArgs += $libraries
    & cl.exe @compilerArgs
    if ($LASTEXITCODE -ne 0) {
        throw "cl.exe failed for $name"
    }
    & $executable
    if ($LASTEXITCODE -ne 0) {
        throw "$name failed"
    }
}

$env:PATH = "$(Join-Path $Prefix 'bin');$env:PATH"
Build-DirectConsumer direct-shared-c C shared
Build-DirectConsumer direct-shared-cxx CXX shared
Build-DirectConsumer direct-static-c C static
Build-DirectConsumer direct-static-cxx CXX static
