# Mercenaries Recompiled - Cheat-Menu Build: installer
#
# Builds the cheat-menu fork of Mercenaries Recompiled on THIS PC from YOUR OWN
# USA retail Xbox copy of Mercenaries: Playground of Destruction. This package
# contains no game code and no game data: the game is translated and compiled
# here, from your disc, using the public source at
#   https://github.com/DuckyNerdball/Mercenaries-Recompiled (branch cheat-menu)
#
# What it does:
#   1. Asks for your Xbox ISO (default.xbe must match the supported retail build).
#   2. Installs what the build needs if missing: Python 3.12, Visual Studio 2022
#      Build Tools (MSVC 14.44.35207 + Windows SDK 10.0.26100). Several GB.
#   3. Downloads the source, runs the official Build-From-Iso.ps1 (20-40 min).
#   4. Installs the game into InstallDir\Game, enables the F9 cheat menu, and
#      generates two optional mods from your own game files:
#        All Supports - every support item in the PDA shop, $1 each
#        Clear Skies  - half the fog
#   5. Makes a desktop shortcut.
#
# Usage:  Install.cmd                       (asks for everything)
#         Install.cmd -IsoPath D:\Mercs.iso -InstallDir "D:\Games\Mercs Cheats"
#         Install.cmd -Update               (rebuild from the latest source, keep saves/settings)
#         Install.cmd -SkipBuild            (reinstall from an existing build in <InstallDir>\source)

[CmdletBinding()]
param(
    [string]$IsoPath,
    [string]$InstallDir = (Join-Path $env:USERPROFILE "Games\Mercenaries Cheat Build"),
    [string]$Repo = "DuckyNerdball/Mercenaries-Recompiled",
    [string]$Branch = "cheat-menu",
    [switch]$Update,
    [switch]$SkipPrereqs,
    [switch]$SkipBuild,
    [switch]$NoShortcut,
    [switch]$NoLaunch,
    # RecompLauncher's windowless setup step (recomplauncher.json "prepare"): no prompts, no
    # shortcut, no launch, "progress <0-1> <text>" lines on stdout. Build tools are only installed
    # when the player ticked their license options on the setup screen.
    [switch]$Launcher,
    [switch]$AcceptVsLicense,
    [switch]$AcceptPythonLicense
)

$ErrorActionPreference = 'Continue'   # Windows PowerShell 5.1 turns native stderr into errors under 'Stop'
$SupportedXbeSha = 'AA08EA21D952AC35F49C02C7E2ED08AA25AD7535FBBBCCC95636775F37BE99D7'
$Msvc = '14.44.35207'
$Sdk = '10.0.26100.0'

function Quote-Args([string[]]$a) {
    # Start-Process joins ArgumentList with spaces and does not quote; paths with spaces split.
    $a | ForEach-Object { if ($_ -match '\s' -and $_ -notmatch '^".*"$') { '"' + $_ + '"' } else { $_ } }
}

function Say([string]$m, [string]$c = 'Cyan') { Write-Host $m -ForegroundColor $c }
function Fail([string]$m, [int]$code = 1) {
    Write-Host "`nERROR: $m" -ForegroundColor Red
    Stop-Transcript | Out-Null
    if (-not $Launcher -and $Host.Name -eq 'ConsoleHost') { Read-Host 'Press Enter to close' | Out-Null }
    exit $code
}
function Progress([double]$f, [string]$t) {
    if (-not $Launcher) { return }
    [Console]::Out.WriteLine('progress ' + $f.ToString('0.00', [Globalization.CultureInfo]::InvariantCulture) + ' ' + $t)
    [Console]::Out.Flush()
}
if ($Launcher) { $NoShortcut = $true; $NoLaunch = $true }
function Refresh-Path { $env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User') }
function Has-Command([string]$n) { [bool](Get-Command $n -ErrorAction SilentlyContinue) }

$InstallDir = [IO.Path]::GetFullPath($InstallDir)
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
$log = Join-Path $InstallDir 'install.log'
Start-Transcript -Path $log -Append | Out-Null

Say "`n=== Mercenaries Recompiled - Cheat-Menu Build installer ===" 'Green'
Say "You need your own USA retail Xbox copy of Mercenaries (ISO). Nothing from the game is included here."
Say "Install folder: $InstallDir"

# ---------------------------------------------------------------- 1. ISO
$sourceDir = Join-Path $InstallDir 'source'
$gameDir = Join-Path $InstallDir 'Game'
$needIso = (-not $SkipBuild) -and ((-not $Update) -or -not (Test-Path (Join-Path $sourceDir 'game_files\mercenaries-retail\default.xbe')))
if ($needIso -and -not $IsoPath -and $Launcher) { Fail 'No disc image was passed (-IsoPath).' 2 }
if ($needIso -and -not $IsoPath) {
    Add-Type -AssemblyName System.Windows.Forms
    $dlg = New-Object System.Windows.Forms.OpenFileDialog
    $dlg.Title = 'Select your Mercenaries (USA, original Xbox) ISO'
    $dlg.Filter = 'Xbox disc image (*.iso)|*.iso|All files (*.*)|*.*'
    if ($dlg.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { Fail 'No ISO selected.' }
    $IsoPath = $dlg.FileName
}
if ($needIso) {
    if (-not (Test-Path -LiteralPath $IsoPath)) { Fail "ISO not found: $IsoPath" 2 }
    $IsoPath = (Resolve-Path -LiteralPath $IsoPath).Path
    Say "ISO: $IsoPath"
}

# ---------------------------------------------------------------- 2. Prerequisites
function Test-Python312 { if (-not (Has-Command 'py')) { return $false }; & py -3.12 -c "import sys" 2>$null; return ($LASTEXITCODE -eq 0) }
function Find-Toolset {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { return $null }
    foreach ($p in (& $vswhere -all -products * -version '[17.0,18.0)' -property installationPath)) {
        if (Test-Path (Join-Path $p "VC\Tools\MSVC\$Msvc")) { return $p }
    }
    return $null
}
function Test-Sdk { Test-Path (Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\Include\$Sdk\um\windows.h") }

if ($SkipBuild) { $SkipPrereqs = $true }
if (-not $SkipPrereqs) {
    Say "`n[1/5] Checking build tools..." 'Green'
    Progress 0.02 'Checking build tools'

    if (-not (Test-Python312)) {
        if ($Launcher -and -not $AcceptPythonLicense) { Fail 'Python 3.12 is needed to build the game. Tick the Python option on the setup screen.' 3 }
        Say 'Installing Python 3.12...'
        Progress 0.04 'Installing Python 3.12'
        if (Has-Command 'winget') {
            winget install --id Python.Python.3.12 -e --silent --accept-package-agreements --accept-source-agreements
        } else {
            $py = Join-Path $env:TEMP 'python-3.12.10-amd64.exe'
            Invoke-WebRequest 'https://www.python.org/ftp/python/3.12.10/python-3.12.10-amd64.exe' -OutFile $py -UseBasicParsing
            Start-Process $py -ArgumentList '/quiet', 'InstallAllUsers=0', 'PrependPath=1', 'Include_launcher=1' -Wait
        }
        Refresh-Path
        if (-not (Test-Python312)) { Fail 'Python 3.12 is still not available (py -3.12). Install it from python.org, then run this again.' }
    }
    Say '  Python 3.12: OK'

    if (-not (Find-Toolset) -or -not (Test-Sdk)) {
        if ($Launcher -and -not $AcceptVsLicense) { Fail 'Visual Studio 2022 Build Tools are needed to build the game. Tick the Build Tools option on the setup screen.' 3 }
        Progress 0.06 'Installing Visual Studio 2022 Build Tools (large download)'
        Say "Installing Visual Studio 2022 Build Tools (MSVC $Msvc + Windows SDK $Sdk). This is a large download..."
        $boot = Join-Path $env:TEMP 'vs_BuildTools.exe'
        Invoke-WebRequest 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile $boot -UseBasicParsing
        $components = @(
            '--add', 'Microsoft.VisualStudio.Workload.VCTools',
            '--add', 'Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64',
            '--add', 'Microsoft.VisualStudio.Component.Windows11SDK.26100',
            '--includeRecommended', '--passive', '--wait', '--norestart', '--nocache')
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        $existing = $null
        if (Test-Path $vswhere) { $existing = & $vswhere -products Microsoft.VisualStudio.Product.BuildTools -version '[17.0,18.0)' -property installationPath | Select-Object -First 1 }
        if ($existing) { $vsArgs = @("modify", "--installPath", $existing) + $components } else { $vsArgs = $components }
        $p = Start-Process $boot -ArgumentList (Quote-Args $vsArgs) -Wait -PassThru
        if ($p.ExitCode -notin 0, 3010) { Fail "Visual Studio Build Tools setup failed (exit $($p.ExitCode)). See %TEMP%\dd_*.log" }
        if (-not (Find-Toolset)) { Fail "MSVC $Msvc is still missing. Open Visual Studio Installer > Modify > Individual components and add 'MSVC v143 - VS 2022 C++ x64/x86 build tools (v14.44-17.14)'." }
        if (-not (Test-Sdk)) { Fail "Windows SDK $Sdk is still missing. Add 'Windows 11 SDK (10.0.26100.0)' in Visual Studio Installer." }
    }
    Say "  MSVC $Msvc + Windows SDK $Sdk : OK"
}

if (-not $SkipBuild) {
# ---------------------------------------------------------------- 3. Source
Say "`n[2/5] Getting the source ($Repo, $Branch)..." 'Green'
Progress 0.15 'Downloading the source'
if (Has-Command 'git') {
    if (Test-Path (Join-Path $sourceDir '.git')) {
        git -C $sourceDir fetch --depth 1 origin $Branch
        git -C $sourceDir reset --hard FETCH_HEAD
    } else {
        if (Test-Path $sourceDir) { Remove-Item $sourceDir -Recurse -Force }
        git clone --depth 1 -b $Branch "https://github.com/$Repo.git" $sourceDir
    }
    if ($LASTEXITCODE -ne 0) { Fail 'Downloading the source with git failed.' }
} else {
    # Without git installed the official build simply skips its git provenance step.
    $zip = Join-Path $env:TEMP 'merc-cheat-src.zip'
    Invoke-WebRequest "https://codeload.github.com/$Repo/zip/$Branch" -OutFile $zip -UseBasicParsing
    $tmp = Join-Path $env:TEMP 'merc-cheat-src'
    if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
    Expand-Archive $zip $tmp -Force
    $inner = Get-ChildItem $tmp -Directory | Select-Object -First 1
    New-Item -ItemType Directory -Force -Path $sourceDir | Out-Null
    robocopy $inner.FullName $sourceDir /E /NFL /NDL /NJH /NJS /XD game_files build .venv .tools | Out-Null
}
if (-not (Test-Path (Join-Path $sourceDir 'ports\mercenaries\scripts\Build-From-Iso.ps1'))) { Fail 'Source download is incomplete.' }

# ---------------------------------------------------------------- 4. Build
Say "`n[3/5] Building from your disc. This takes 20-40 minutes; leave the window open..." 'Green'
Progress 0.20 'Building the game from your disc (5-40 minutes)'
$build = Join-Path $sourceDir 'ports\mercenaries\scripts\Build-From-Iso.ps1'
$buildArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $build)
if ($needIso) { $buildArgs += @('-IsoPath', $IsoPath) }
else {
    # -Update with game files already extracted: regenerate and rebuild without re-reading the ISO.
    $buildArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command',
        "& '$sourceDir\ports\mercenaries\scripts\Setup-Tools.ps1'; & '$sourceDir\ports\mercenaries\scripts\Setup-Python.ps1'; & '$sourceDir\ports\mercenaries\scripts\Recompile.ps1' -GameDirectory '$sourceDir\game_files\mercenaries-retail'; & '$sourceDir\ports\mercenaries\scripts\Build.ps1'")
}
Push-Location $sourceDir
# Run the build directly. Start-Process -Wait also waits on every background process the
# build leaves behind (MSBuild nodes, vctip, mspdbsrv), so the installer never moved on.
$env:MSBUILDDISABLENODEREUSE = '1'
& powershell.exe @buildArgs
$buildExit = $LASTEXITCODE
Pop-Location
}
else { Say "`nSkipping the build; using the existing one in $sourceDir." 'Green'; $buildExit = 0 }
$bin = Join-Path $sourceDir 'build\mercenaries\bin\Release'
if ($buildExit -ne 0 -or -not (Test-Path (Join-Path $bin 'mercenaries_recomp.exe'))) { Fail "The build failed (exit $buildExit). The log is $log - open an issue on github.com/$Repo with it." 4 }
$xbe = Join-Path $sourceDir 'game_files\mercenaries-retail\default.xbe'
$xbeSha = (Get-FileHash $xbe -Algorithm SHA256).Hash
if ($xbeSha -ne $SupportedXbeSha) { Fail "Your disc's default.xbe ($xbeSha) is not the supported USA retail build." 5 }

# ---------------------------------------------------------------- 5. Install
Say "`n[4/5] Installing to $gameDir..." 'Green'
Progress 0.85 'Installing'
Get-Process 'mercenaries_recomp', 'Mercenaries Recompiled' -ErrorAction SilentlyContinue | Stop-Process -Force
New-Item -ItemType Directory -Force -Path $gameDir | Out-Null
# Never overwrite the player's saves, settings or mod choices.
robocopy $bin $gameDir /E /NFL /NDL /NJH /NJS /XD game_files mod-cache saves mod-saves mod-settings plugin-settings /XF mercenaries_recomp.ini mod-loader.ini mod-loader.lock | Out-Null

# Game files: reuse the copy the build extracted from your disc (same thing the launcher's first-run install does).
$retailDst = Join-Path $gameDir 'game_files\mercenaries-retail'
$marker = Join-Path $retailDst '.mercenaries-recompiled-install-v1'
if (-not (Test-Path $marker)) {
    robocopy (Join-Path $sourceDir 'game_files\mercenaries-retail') $retailDst /E /NFL /NDL /NJH /NJS | Out-Null
    [IO.File]::WriteAllText($marker, "Mercenaries Recompiled asset install v1`r`nretail_xbe_sha256=$xbeSha`r`n", [Text.Encoding]::ASCII)
}

# F9 cheat panel on
$dev = Join-Path $gameDir 'developer.ini'
if (Test-Path $dev) {
    $t = [IO.File]::ReadAllText($dev)
    $t = [Regex]::Replace($t, '(?m)^developer_menu=\d', 'developer_menu=1')
    [IO.File]::WriteAllText($dev, $t)
}

# 1080p borderless by default. Only fills in keys that are missing, so a resolution
# or display mode the player picked in Recomp Options is never overwritten.
# Modded launches read a per-profile copy under mod-settings, so fix those too.
function Set-DisplayDefaults([string]$path) {
    $text = ''
    if (Test-Path -LiteralPath $path) { $text = [IO.File]::ReadAllText($path) }
    $add = ''
    foreach ($kv in @(@('ResolutionScale', '3'), @('DisplayMode', '1'))) {
        if ($text -notmatch ('(?m)^\s*' + $kv[0] + '\s*=')) { $add += $kv[0] + '=' + $kv[1] + "`r`n" }
    }
    if (-not $add) { return }
    if ($text -match '(?m)^\[RecompOptions\][ \t]*\r?\n') {
        $text = [Regex]::Replace($text, '(?m)^\[RecompOptions\][ \t]*\r?\n', { param($m) $m.Value + $add }, 'None')
    } else {
        if ($text -and -not $text.EndsWith("`n")) { $text += "`r`n" }
        $text += "[RecompOptions]`r`n" + $add
    }
    if (Test-Path -LiteralPath $path) { (Get-Item -LiteralPath $path).IsReadOnly = $false }
    [IO.File]::WriteAllText($path, $text, [Text.Encoding]::ASCII)
}
Set-DisplayDefaults (Join-Path $gameDir 'mercenaries_recomp.ini')
$profiles = Join-Path $gameDir 'mod-settings'
if (Test-Path $profiles) {
    Get-ChildItem $profiles -Directory | Where-Object { $_.Name -notlike '.*' } | ForEach-Object {
        $p = Join-Path $_.FullName 'mercenaries_recomp.ini'
        if (Test-Path -LiteralPath $p) { Set-DisplayDefaults $p }
    }
}
Say "  Display: 1080p borderless unless already set in Recomp Options"

# ---------------------------------------------------------------- 6. Mods generated from YOUR game files
Say "`n[5/5] Generating mods from your own game files..." 'Green'
Progress 0.92 'Making the All Supports and Clear Skies mods'
$latin1 = [Text.Encoding]::GetEncoding(28591)
$data = Join-Path $retailDst 'DATAxbox'

# All Supports: every retail shop item at $1 (0 means "not for sale" to the shop), plus support items the shop never sells.
$extra = @'
template_support_airsuperiority|support.airstrikes.airsuperiority|shop.airstrikes.airsuperiority.short|shop.airstrikes.airsuperiority.long|Weapon|shop_airsuperiority
template_support_artillery|support.airstrikes.artillery|shop.airstrikes.p_artillery.short|shop.airstrikes.p_artillery.long|Weapon|shop_artillery
template_support_artillery2|support.airstrikes.artillery2|shop.airstrikes.artillery2.short|shop.airstrikes.artillery2.long|Weapon|shop_artillery2
template_support_artillery3|support.airstrikes.artillery3|shop.airstrikes.artillery3.short|shop.airstrikes.artillery3.long|Weapon|shop_artillery3
template_support_bunkerbuster|support.airstrikes.bunkerbuster|shop.airstrikes.bunkerbuster.short|shop.airstrikes.bunkerbuster.long|Weapon|shop_bunkerbuster
template_support_carpetbomb|support.airstrikes.carpetbomb|shop.airstrikes.carpetbomb.short|shop.airstrikes.carpetbomb.long|Weapon|shop_carpetbomb
template_support_clusterbomb|support.airstrikes.clusterbomb|shop.airstrikes.clusterbomb.short|shop.airstrikes.clusterbomb.long|Weapon|shop_clusterbomb
template_support_crate_allies|support.supplies.crate_allies|shop.supplies.crate_west.short|shop.supplies.crate_west.long|Supply|shop_crate_west
template_support_crate_alliesammoveh|support.supplies.crate_ChinaAmmoVeh|shop.supplies.ammoveh.short|shop.supplies.ammoveh.long|Supply|shop_crate_ammoveh
template_support_crate_alliesc4|support.supplies.crate_alliesc4|shop.supplies.crate_demolitions.short|shop.supplies.crate_demolitions.long|Supply|shop_crate_C4
template_support_crate_alliesheavy|support.supplies.crate_alliesheavy|shop.supplies.crate_heavy.short|shop.supplies.crate_heavy.long|Supply|shop_crate_heavy
template_support_crate_alliesstinger|support.supplies.crate_alliesstinger|shop.supplies.crate_stinger.short|shop.supplies.crate_stinger.long|Supply|shop_crate_stinger
template_support_crate_china|support.supplies.crate_china|shop.supplies.crate_east.short|shop.supplies.crate_east.long|Supply|shop_crate_east
template_support_crate_chinaammoveh|support.supplies.crate_chinaammoveh|shop.supplies.ammoveh.short|shop.supplies.ammoveh.long|Supply|shop_crate_ammoveh
template_support_crate_chinaarmor|support.supplies.crate_chinaarmor|shop.supplies.crate_armor.short|shop.supplies.crate_armor.long|Supply|shop_crate_armor
template_support_crate_chinaheavy|support.supplies.crate_chinaheavy|shop.supplies.crate_heavy.short|shop.supplies.crate_heavy.long|Supply|shop_crate_heavy
template_support_crate_chinastinger|support.supplies.crate_stinger|shop.supplies.crate_stinger.short|shop.supplies.crate_stinger.long|Supply|shop_crate_stinger
template_support_crate_chinavehicle|support.supplies.crate_chinavehicle|shop.supplies.crate_mafiaVehicle.short|shop.supplies.crate_mafiaVehicle.long|Supply|shop_crate_vehicle
template_support_crate_sk|support.supplies.crate_sk|shop.supplies.crate_west.short|shop.supplies.crate_west.long|Supply|shop_crate_west
template_support_crate_skc4|support.supplies.crate_skc4|shop.supplies.crate_demolitions.short|shop.supplies.crate_demolitions.long|Supply|shop_crate_C4
template_support_crate_skcovert|support.supplies.crate_skcovert|shop.supplies.crate_covert.short|shop.supplies.crate_covert.long|Supply|shop_crate_covert
template_support_crate_skheavy|support.supplies.crate_skheavy|shop.supplies.crate_heavy.short|shop.supplies.crate_heavy.long|Supply|shop_crate_heavy
template_support_crate_sksniper|support.supplies.crate_sksniper|shop.supplies.crate_sniper.short|shop.supplies.crate_sniper.long|Supply|shop_crate_sniper
template_support_crate_skstinger|support.supplies.crate_stinger|shop.supplies.crate_stinger.short|shop.supplies.crate_stinger.long|Supply|shop_crate_stinger
template_support_cruisemissile|support.airstrikes.cruisemissile|shop.airstrikes.cruisemissile.short|shop.airstrikes.cruisemissile.long|Weapon|shop_cruisemissile
template_support_deliverallieshumvee|support.vehicles.deliverallieshumvee.alt|shop.vehicles.deliverStolenAlliesHumvee.short|shop.vehicles.deliverStolenAlliesHumvee.long|Vehicle|shop_allieshumvee
template_support_deliveralliesstryker|support.vehicles.deliveralliesstryker|shop.vehicles.deliverStolenAlliesStryker.short|shop.vehicles.deliverStolenAlliesStryker.long|Vehicle|shop_alliesstryker
template_support_deliverchinajeep|support.vehicles.deliverchinajeep|shop.vehicles.deliverStolenChinaJeep.short|shop.vehicles.deliverStolenChinaJeep.long|Vehicle|shop_chinajeep
template_support_deliverskapc|support.vehicles.deliverStolenSKAPC|shop.vehicles.deliverStolenSKAPC.short|shop.vehicles.deliverStolenSKAPC.long|Vehicle|shop_skapc
template_support_deliverskhumveeat|support.vehicles.deliverskhumveeat|shop.vehicles.deliverStolenSKHumveeAT.short|shop.vehicles.deliverStolenSKHumveeAT.long|Vehicle|shop_skhumveeat
template_support_deliverskhumveemg|support.vehicles.deliverskhumveemg|shop.vehicles.deliverStolenSKHumveeMG.short|shop.vehicles.deliverStolenSKHumveeMG.long|Vehicle|shop_skhumveemg
template_support_deliverstolenalliesavenger1|support.vehicles.deliverstolenalliesavenger|shop.vehicles.deliverstolenalliesavenger.short|shop.vehicles.deliverstolenalliesavenger.long|Vehicle|shop_alliesavenger
template_support_deliverstolencivcargotruck|support.vehicles.deliverstolencivcargotruck|shop.vehicles.deliverStolenNKCargoTruck.short|shop.vehicles.deliverStolenNKCargoTruck.long|Vehicle|shop_nktransport
template_support_deliverstolennkcargotruck|support.vehicles.deliverstolennkcargotruck|shop.vehicles.deliverstolennkcargotruck.short|shop.vehicles.deliverstolennkcargotruck.long|Vehicle|shop_nktransport
template_support_deliverstolenpresstruck1|support.vehicles.deliverstolenpresstruck|shop.vehicles.deliverstolenpresstruck.short|shop.vehicles.deliverStolenPressCar.long|Vehicle|shop_presstruck
template_support_fuelairbomb|support.airstrikes.fuelairbomb|shop.airstrikes.fuelairbomb.short|shop.airstrikes.fuelairbomb.long|Weapon|shop_fuelairbomb
template_support_guidedmissile|support.airstrikes.guidedmissile|shop.airstrikes.cruisemissile.short|shop.airstrikes.cruisemissile.long|Weapon|shop_cruisemissile
template_support_gunship|support.airstrikes.gunship|shop.airstrikes.ac130.short|shop.airstrikes.ac130.long|Weapon|shop_gunship
template_support_gunship2|support.airstrikes.gunship2|shop.airstrikes.ac130.short|shop.airstrikes.ac130.long|Weapon|shop_gunship2
template_support_gunship3|support.airstrikes.gunship3|shop.airstrikes.ac130.short|shop.airstrikes.ac130.long|Weapon|shop_gunship3
template_support_hvysmartbomb|support.airstrikes.hvysmartbomb|shop.airstrikes.smartbomb.short|shop.airstrikes.smartbomb.long|Weapon|shop_smartbomb
template_support_smartbomb|support.airstrikes.smartbomb|shop.airstrikes.smartbomb.short|shop.airstrikes.smartbomb.long|Weapon|shop_smartbomb
template_support_surgicalstrike|support.airstrikes.surgicalstrike|shop.airstrikes.surgicalstrike.short|shop.airstrikes.surgicalstrike.long|Weapon|shop_surgicalstrike
template_support_tankbuster|support.airstrikes.tankbuster|shop.airstrikes.tankbuster.short|shop.airstrikes.tankbuster.long|Weapon|shop_tankbuster
'@
$shopSrc = Join-Path $data 'DATAPOD\itemshop.ini'
if (Test-Path $shopSrc) {
    $shop = [IO.File]::ReadAllText($shopSrc, $latin1)
    $shop = [Regex]::Replace($shop, '(?m)^(DefaultPrice[ \t]+)\d+', '${1}1')
    $sb = New-Object Text.StringBuilder
    [void]$sb.Append($shop.TrimEnd()).Append("`r`n`r`n; ---- Added by All Supports: support items the retail shop does not sell ----`r`n")
    foreach ($line in ($extra -split "`r?`n")) {
        $f = $line.Split('|'); if ($f.Count -ne 6) { continue }
        if ($shop -match ('(?mi)^Template_object_name\s+' + [Regex]::Escape($f[0]) + '\s*$')) { continue }
        [void]$sb.Append("`r`n[ShopItem]`r`nTemplate_object_name`t$($f[0])`r`nDisplayName`t`t$($f[1])`r`nDescription`t`t$($f[2])`r`nDetailedDescription`t$($f[3])`r`nCategory`t`t$($f[4])`r`nTexture`t`t`t$($f[5])`r`nDefaultPrice`t`t1`r`n")
    }
    $shopDst = Join-Path $gameDir 'mods\All Supports\game_files\DATAxbox\DATAPOD\itemshop.ini'
    New-Item -ItemType Directory -Force -Path (Split-Path $shopDst) | Out-Null
    [IO.File]::WriteAllText($shopDst, $sb.ToString(), $latin1)
    Say '  All Supports: done'
} else { Say '  All Supports: itemshop.ini not found, skipped' 'Yellow' }

# Clear Skies: halve fog alpha and fog density in the region configs.
$changed = 0
foreach ($region in 'SW', 'NW') {
    foreach ($ini in Get-ChildItem (Join-Path $data $region) -Filter *.ini -File -ErrorAction SilentlyContinue) {
        $t = [IO.File]::ReadAllText($ini.FullName, $latin1)
        $n = [Regex]::Replace($t, '(?m)^(FogRGBA[ \t]+\S+[ \t]+\S+[ \t]+\S+[ \t]+)(\d+)', { param($m) $m.Groups[1].Value + [string][Math]::Floor([int]$m.Groups[2].Value / 2) })
        $n = [Regex]::Replace($n, '(?mi)^(Fogdensity[ \t]+)([0-9]*\.?[0-9]+)', { param($m) $m.Groups[1].Value + ([double]::Parse($m.Groups[2].Value, [Globalization.CultureInfo]::InvariantCulture) / 2).ToString([Globalization.CultureInfo]::InvariantCulture) })
        if ($n -ne $t) {
            $dst = Join-Path $gameDir "mods\Clear Skies\game_files\DATAxbox\$region\$($ini.Name)"
            New-Item -ItemType Directory -Force -Path (Split-Path $dst) | Out-Null
            [IO.File]::WriteAllText($dst, $n, $latin1); $changed++
        }
    }
}
Say "  Clear Skies: $changed region files"

# Tick both mods on every install/update. Other mods and their order are kept; a new
# mod-loader.ini uses the normal save slots. Same format the mod selector writes.
$loader = Join-Path $gameDir 'mod-loader.ini'
$mods = New-Object System.Collections.ArrayList
$separate = '0'
if (Test-Path $loader) {
    $cur = $null; $folder = $null; $enabled = '0'
    foreach ($line in (Get-Content -LiteralPath $loader) + '[End]') {
        $l = $line.Trim([char]0xFEFF, ' ', "`t")
        if ($l -match '^\[(.+)\]$') {
            if ($cur -like 'Mod.*' -and $folder) { [void]$mods.Add([pscustomobject]@{ Folder = $folder; Enabled = $enabled }) }
            $cur = $Matches[1]; $folder = $null; $enabled = '0'
        } elseif ($l -match '^([^=]+)=(.*)$') {
            $k = $Matches[1].Trim(); $v = $Matches[2].Trim().Trim('"')
            if ($cur -eq 'Loader' -and $k -eq 'SeparateSaves') { $separate = $v }
            elseif ($cur -like 'Mod.*' -and $k -eq 'Folder') { $folder = $v }
            elseif ($cur -like 'Mod.*' -and $k -eq 'Enabled') { $enabled = $v }
        }
    }
}
foreach ($name in 'All Supports', 'Clear Skies') {
    $m = $mods | Where-Object { $_.Folder -eq $name } | Select-Object -First 1
    if ($m) { $m.Enabled = '1' } else { [void]$mods.Add([pscustomobject]@{ Folder = $name; Enabled = '1' }) }
}
$ini = [string][char]0xFEFF + "[Loader]`r`nCount=$($mods.Count)`r`nSeparateSaves=$separate`r`n"
for ($n = 0; $n -lt $mods.Count; $n++) { $ini += "`r`n[Mod.$n]`r`nFolder=`"$($mods[$n].Folder)`"`r`nEnabled=$($mods[$n].Enabled)`r`n" }
[IO.File]::WriteAllText($loader, $ini, (New-Object System.Text.UnicodeEncoding($false, $false)))
Say "  Mods ticked: All Supports, Clear Skies"

# Shortcut
$exe = Join-Path $gameDir 'Mercenaries Recompiled.exe'
if (-not $NoShortcut) {
    $ws = New-Object -ComObject WScript.Shell
    $lnk = $ws.CreateShortcut((Join-Path ([Environment]::GetFolderPath('Desktop')) 'Mercenaries (Cheat Build).lnk'))
    $lnk.TargetPath = $exe; $lnk.WorkingDirectory = $gameDir; $lnk.IconLocation = "$exe,0"; $lnk.Save()
}

Say "`nDone. Installed to $gameDir" 'Green'
Progress 1.00 'Done'
Say 'In game: F9 (or L3+R3 on a controller) opens the cheat panel. Hold LB + click R3 (or F7) calls an airstrike at your crosshair.'
Say 'In the mod selector, All Supports and Clear Skies are already ticked. Untick them to play without.'
Say "To update later: run Install.cmd -Update (keeps saves and settings)."
Stop-Transcript | Out-Null
if (-not $NoLaunch) { Start-Process $exe -WorkingDirectory $gameDir }
