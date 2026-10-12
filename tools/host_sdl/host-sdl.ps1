<#
.SYNOPSIS
  Build, run and inspect the virtual host-sdl ESPHome device from Windows via WSL2.

.DESCRIPTION
  The ESPHome host platform only builds on Linux, so this wrapper runs
  tools/host_sdl/run.sh inside WSL. When the repo lives on a mapped network
  drive (e.g. X: -> \\server\config) the share is mounted into WSL first.
  The SDL window opens on the Windows desktop through WSLg.

.EXAMPLE
  pwsh tools/host_sdl/host-sdl.ps1 run              # build + run default layout
  pwsh tools/host_sdl/host-sdl.ps1 run 480x320      # build + run gui/480x320.yaml
  pwsh tools/host_sdl/host-sdl.ps1 config 128x128   # validate only
  pwsh tools/host_sdl/host-sdl.ps1 screenshot       # save PNG of the running UI
  pwsh tools/host_sdl/host-sdl.ps1 page next        # navigate (next|prev|<index>)
  pwsh tools/host_sdl/host-sdl.ps1 setup            # one-time WSL toolchain install
  pwsh tools/host_sdl/host-sdl.ps1 stop             # kill a running instance
#>
param(
  [Parameter(Position = 0)]
  [ValidateSet('run', 'compile', 'config', 'clean', 'screenshot', 'page', 'setup', 'stop', 'shell')]
  [string]$Command = 'run',

  # gui layout for run/compile/config/clean, or page target for 'page'
  [Parameter(Position = 1)]
  [string]$Arg = '',

  [string]$Distro = 'Ubuntu',
  [int]$ScreenshotPort = 8080,
  [string]$Out = ''
)

$ErrorActionPreference = 'Stop'
$repoWin = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$mountRoot = '/mnt/ha-config'

function Get-WslRepoPath {
  $qualifier = Split-Path $repoWin -Qualifier            # e.g. X:
  $relative = $repoWin.Substring($qualifier.Length).TrimStart('\') -replace '\\', '/'
  $drive = Get-PSDrive -Name $qualifier.TrimEnd(':') -ErrorAction SilentlyContinue
  if ($drive -and $drive.DisplayRoot -like '\\*') {
    # Mapped network drive: mount the UNC share into WSL (drvfs, uses Windows credentials)
    $unc = $drive.DisplayRoot
    $uid = (wsl.exe -d $Distro --cd ~ -- id -u).Trim()
    wsl.exe -d $Distro -u root --cd ~ -- mkdir -p $mountRoot
    wsl.exe -d $Distro -u root --cd ~ -- mountpoint -q $mountRoot
    if ($LASTEXITCODE -ne 0) {
      Write-Host "Mounting $unc at $mountRoot in WSL ($Distro)"
      $uncBash = $unc.Replace('\', '\\')
      wsl.exe -d $Distro -u root --cd ~ -- bash -c "mount -t drvfs '$uncBash' '$mountRoot' -o 'metadata,uid=$uid,gid=$uid' 2>/dev/null || mount '$mountRoot'"
      if ($LASTEXITCODE -ne 0) { throw "Failed to mount $unc in WSL" }
    }
    return "$mountRoot/$relative".TrimEnd('/')
  }
  # Local drive: WSL automounts it under /mnt/<letter>
  return "/mnt/$($qualifier.TrimEnd(':').ToLower())/$relative".TrimEnd('/')
}

function Invoke-Wsl([string]$script) {
  # Login shell so WSLg's DISPLAY / WAYLAND_DISPLAY are set
  wsl.exe -d $Distro --cd ~ -- bash -lc $script
  exit $LASTEXITCODE
}

switch ($Command) {
  'screenshot' {
    if (-not $Out) {
      $dir = Join-Path $PSScriptRoot 'screenshots'
      New-Item -ItemType Directory -Force $dir | Out-Null
      $Out = Join-Path $dir ("host-sdl-{0:yyyyMMdd-HHmmss}.png" -f (Get-Date))
    }
    Invoke-WebRequest "http://127.0.0.1:$ScreenshotPort/screenshot" -OutFile $Out -TimeoutSec 10 -UseBasicParsing
    Write-Host $Out
    exit 0
  }
  'page' {
    if (-not $Arg) { $Arg = 'next' }
    $resp = Invoke-WebRequest "http://127.0.0.1:$ScreenshotPort/page/$Arg" -TimeoutSec 10 -UseBasicParsing
    Write-Host $resp.Content.Trim()
    exit 0
  }
  'stop' {
    Invoke-Wsl "pkill -f '.esphome-host/build/.*/program' && echo stopped || echo 'not running'"
  }
  'shell' {
    $repo = Get-WslRepoPath
    wsl.exe -d $Distro --cd ~ -- bash -lc "cd '$repo' && exec bash -l"
    exit $LASTEXITCODE
  }
  'setup' {
    $repo = Get-WslRepoPath
    Invoke-Wsl "bash '$repo/tools/host_sdl/setup_wsl.sh'"
  }
  default {
    $repo = Get-WslRepoPath
    Invoke-Wsl "bash '$repo/tools/host_sdl/run.sh' $Command '$Arg'"
  }
}

