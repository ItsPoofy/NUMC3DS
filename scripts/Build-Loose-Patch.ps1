[CmdletBinding()]
param(
    [ValidateSet('module', 'bootstrap', 'layout')]
    [string]$DiagnosticStage = 'module',
    [ValidateSet('modded', 'stock')]
    [string]$DiagnosticResources = 'modded',
    [string[]]$AdditionalNativeSources = @(),
    [string[]]$AdditionalIncludePaths = @(),
    [string[]]$AdditionalDefines = @(),
    [ValidateSet(0, 1)]
    [int]$Optimizations = 1,
    [int]$NativeSize = 0x000C0000,
    [string]$BuildDirectory = 'build\numc3ds',
    [switch]$SkipDeploy
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$InputCode = Join-Path $ProjectRoot 'inputs\mc3ds-usa-1.9.19\exefs\update\code.bin'
$InputExHeader = Join-Path $ProjectRoot 'inputs\mc3ds-usa-1.9.19\exefs\update\exheader.bin'

function Resolve-BuildExecutable([string]$Description, [string[]]$Candidates) {
    foreach ($Candidate in $Candidates) {
        if ([string]::IsNullOrWhiteSpace($Candidate)) { continue }
        if (Test-Path -LiteralPath $Candidate -PathType Leaf) {
            return [IO.Path]::GetFullPath($Candidate)
        }
        $Command = Get-Command $Candidate -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($null -ne $Command) { return $Command.Source }
    }
    throw "Missing $Description. Install devkitARM/Python or set DEVKITARM/NUMC3DS_PYTHON."
}

$LocalToolchain = Join-Path $ProjectRoot 'tools\msys64\opt\devkitpro\devkitARM\bin'
$DevkitBin = if ($env:DEVKITARM) { Join-Path $env:DEVKITARM 'bin' } else { Join-Path $ProjectRoot '.missing-devkitarm' }
$Compiler = Resolve-BuildExecutable 'arm-none-eabi-gcc' @((Join-Path $LocalToolchain 'arm-none-eabi-gcc.exe'), (Join-Path $DevkitBin 'arm-none-eabi-gcc.exe'), 'arm-none-eabi-gcc.exe')
$Objcopy = Resolve-BuildExecutable 'arm-none-eabi-objcopy' @((Join-Path $LocalToolchain 'arm-none-eabi-objcopy.exe'), (Join-Path $DevkitBin 'arm-none-eabi-objcopy.exe'), 'arm-none-eabi-objcopy.exe')
$Readelf = Resolve-BuildExecutable 'arm-none-eabi-readelf' @((Join-Path $LocalToolchain 'arm-none-eabi-readelf.exe'), (Join-Path $DevkitBin 'arm-none-eabi-readelf.exe'), 'arm-none-eabi-readelf.exe')
$Nm = Resolve-BuildExecutable 'arm-none-eabi-nm' @((Join-Path $LocalToolchain 'arm-none-eabi-nm.exe'), (Join-Path $DevkitBin 'arm-none-eabi-nm.exe'), 'arm-none-eabi-nm.exe')
$Python = Resolve-BuildExecutable 'Python 3' @($env:NUMC3DS_PYTHON, (Join-Path $ProjectRoot 'tools\msys64\mingw64\bin\python.exe'), 'python.exe')
$ResourceValidator = Join-Path $ProjectRoot 'scripts\Test-LoosePatchResources.py'
$ManifestWriter = Join-Path $ProjectRoot 'scripts\Write-LoosePatchManifest.ps1'
$ChangelogSource = Join-Path $ProjectRoot 'NUMC3DS-CHANGELOG-SOURCE.txt'
$IpsWriter = Join-Path $ProjectRoot 'scripts\Write-CodeIps.ps1'
$BuildRoot = [IO.Path]::GetFullPath((Join-Path $ProjectRoot $BuildDirectory))
$ObjectRoot = Join-Path $BuildRoot 'objects'
$RomfsRoot = Join-Path $BuildRoot 'romfs\numc3ds'
$NativeRomfs = Join-Path $ProjectRoot 'native\romfs'

$ExpectedCodeHash = '7E58F876A531428B484A4A71BDCA364413CE86C6BB63250CA375605E41F260F0'
$ExpectedExHeaderHash = '1D498C0F17D789641CE53FDF779E99F52A126B8C81731CB54693758FBF9482CE'
$ExHeaderFsAccessOffset = 0x248
$ExHeaderFsAccessDirectSdmc = [uint32]0x00000080
$ExHeaderIoAccessOffset = 0x3F0
$ExHeaderIoAccessDirectSdmc = [uint16]0x0200
$ExHeaderInfoSize = 0x400
$ApplicationTitleId = [uint64]0x00040000001B8700
$UpdateTitleId = [uint64]0x0004000E001B8700
$HookOffset = 0x001307A4
$LayeredFsPayloadOffset = 0x00818824
$LayeredFsPayloadSize = 0x0000011C
$CaveOffset = $LayeredFsPayloadOffset + $LayeredFsPayloadSize
$CaveSize = 0x00819000 - $CaveOffset
$HookAddress = 0x002307A4
$CaveAddress = 0x00100000 + $CaveOffset
$OriginalTextSize = 0x00818824
$RoundedTextSize = 0x00819000
$DeclaredTextSize = $OriginalTextSize
$RoAddress = 0x00919000
$DataAddress = 0x00A29000
$OriginalDataSize = 0x00010B70
$OriginalBssSize = 0x001060C0
$OriginalCodeSize = 0x0093A000
$AchievementRegisterCallOffset = 0x0032C76C
$AchievementRegisterCallAddress = 0x0042C76C
$AchievementReserveCountOffset = 0x0032C560
$AchievementCountAssertOffset = 0x0032D688
$WolfOnTameOffset = 0x00483F14
$WolfOnTameAddress = 0x00583F14
$MaxWorldGuardOffset = 0x001D3E58
$InvalidChunkPanicOffset = 0x000F53E0
$LevelChunkMetadataPatchOffset = 0x00057D3C
$LevelChunkMetadataPatchAddress = 0x00157D3C
$MobAnimationScaleOffset = 0x003E61D0
$MapUploaderNullCheckOffset = 0x005775B8
$MapUploaderBranchOffset = 0x005775BC
$MapUploaderAddUuidOffset = 0x005775C0
$MapUploaderSetDstOffset = 0x005775C8
$MapItemSkipIsValidOffset = 0x0062C724
$MapItemSafeStepOffset = 0x0062C73C
$MapItemFullRowOffset = 0x0062C74C
$MapItemUpdateElevOffset = 0x0062D130
$MapItemLoadYOffset = 0x0062D140
$MapItemLoadRowOffset = 0x0062D144
$MapItemCmpRowOffset = 0x0062D148
$MapItemLoopBleOffset = 0x0062D3FC
$MapItemSkipUnpopulatedChunkOffset = 0x0062CB70
$MapDecorationPlayerTypeOffset = 0x00226344
$MinimapSkipCenterCursorOffset = 0x005764E0
$OptionItemDefaultOffsetCodeOffset = 0x005A8058
$OptionsRowWrapperOffsetCodeOffset = 0x0034E5C0
$EditWorldPlayButtonShadowOffset = 0x001C74A8
$EditWorldDeleteButtonShadowOffset = 0x001C765C


foreach ($Required in @($InputCode, $InputExHeader, $Compiler, $Objcopy, $Readelf, $Nm, $Python, $ResourceValidator, $ManifestWriter, $ChangelogSource, $IpsWriter)) {
    if (-not (Test-Path -LiteralPath $Required -PathType Leaf)) {
        throw "Missing required file: $Required"
    }
}
if (-not (Test-Path -LiteralPath $NativeRomfs -PathType Container)) {
    throw "Missing mod assets: $NativeRomfs"
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $InputCode).Hash -ne $ExpectedCodeHash) {
    throw 'The USA update code.bin does not match the mapped v9408 executable.'
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $InputExHeader).Hash -ne $ExpectedExHeaderHash) {
    throw 'The USA update ExHeader does not match the mapped v9408 executable.'
}
$ExpectedBuildRoot = [IO.Path]::GetFullPath((Join-Path $ProjectRoot 'build'))
$ResolvedBuildRoot = [IO.Path]::GetFullPath($BuildRoot)
if (-not $ResolvedBuildRoot.StartsWith($ExpectedBuildRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to reset an unexpected build root: $ResolvedBuildRoot"
}
if (Test-Path -LiteralPath $BuildRoot -PathType Container) {
    Remove-Item -LiteralPath $BuildRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $ObjectRoot, $RomfsRoot | Out-Null
Copy-Item -Path (Join-Path $NativeRomfs '*') -Destination (Join-Path $BuildRoot 'romfs') -Recurse -Force
$CommonFlags = @(
    '-Os', '-mcpu=mpcore', '-marm', '-mfpu=vfpv2', '-mfloat-abi=hard', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-fno-unwind-tables', '-fno-asynchronous-unwind-tables', '-fno-stack-protector',
    '-nostdlib', '-Wl,--build-id=none', '-Wl,--gc-sections'
)

$BootstrapElf = Join-Path $ObjectRoot 'bootstrap.elf'
$BootstrapBin = Join-Path $ObjectRoot 'bootstrap.bin'
$BootstrapStageDefine = if ($DiagnosticStage -in @('bootstrap', 'layout')) { '-DNUMC3DS_BOOTSTRAP_ONLY=1' } else { '-DNUMC3DS_BOOTSTRAP_ONLY=0' }
& $Compiler @CommonFlags ('-DNUMC3DS_NATIVE_FILE_SIZE=0x{0:X}' -f $NativeSize) $BootstrapStageDefine '-Wl,-T,native/bootstrap/bootstrap.ld' ("-Wl,-Map,$ObjectRoot/bootstrap.map") '-o' $BootstrapElf 'native/bootstrap/bootstrap.c' 'native/bootstrap/achievement_restore.c'
if ($LASTEXITCODE -ne 0) { throw 'Bootstrap compilation failed.' }
& $Objcopy '-O' 'binary' $BootstrapElf $BootstrapBin
if ($LASTEXITCODE -ne 0) { throw 'Bootstrap extraction failed.' }

$SplashElf = Join-Path $ObjectRoot 'splash_loader.elf'
$SplashBin = Join-Path $ObjectRoot 'splash_loader.bin'
& $Compiler @CommonFlags '-Wl,-T,native/core/ui/splash_loader.ld' ("-Wl,-Map,$ObjectRoot/splash_loader.map") '-o' $SplashElf 'native/core/ui/splash_loader.c'
if ($LASTEXITCODE -ne 0) { throw 'Splash loader compilation failed.' }
& $Objcopy '-O' 'binary' $SplashElf $SplashBin
if ($LASTEXITCODE -ne 0) { throw 'Splash loader extraction failed.' }

function Get-BootstrapSymbolAddress([string]$Name) {
    $Match = & $Nm '-n' $BootstrapElf | Select-String -Pattern ('^([0-9a-fA-F]+)\s+[A-Za-z]\s+' + [Regex]::Escape($Name) + '$')
    if ($LASTEXITCODE -ne 0 -or $null -eq $Match -or $Match.Count -ne 1) {
        throw "Bootstrap symbol is missing or ambiguous: $Name"
    }
    return [Convert]::ToUInt32($Match.Matches[0].Groups[1].Value, 16)
}

$AchievementRegisterTarget = Get-BootstrapSymbolAddress 'numc3ds_register_iron_golem_and_leader'
$WolfOnTameTarget = Get-BootstrapSymbolAddress 'numc3ds_wolf_on_tame'
$LevelChunkMetadataPatchTarget = Get-BootstrapSymbolAddress 'numc3ds_levelchunk_metadata_write_patch'


$CoreElf = Join-Path $ObjectRoot 'native.elf'
$CoreBin = Join-Path $ObjectRoot 'native.unpadded.bin'
$NativeBin = Join-Path $ObjectRoot 'native.bin'
$CoreSources = @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'native\core') -Recurse -Filter '*.c' |
    Where-Object { $_.Name -ne 'splash_loader.c' } |
    ForEach-Object { $_.FullName.Substring($ProjectRoot.Length + 1).Replace('\','/') } | Sort-Object)
$CoreSources += 'native/diagnostics/network_debug.c'
$CoreSources += $AdditionalNativeSources
if ($CoreSources.Count -eq 0) { throw 'No core source files found.' }
Write-Host ('Compiling {0} core translation units:' -f $CoreSources.Count)
foreach ($Src in $CoreSources) { Write-Host ("  " + $Src) }
$LinkArgs = [Collections.Generic.List[string]]::new()
foreach ($Flag in $CommonFlags) { $LinkArgs.Add($Flag) }
$LinkArgs.Add('-DNUMC3DS_MCPE_TARGET=1')
$LinkArgs.Add("-DNUMC3DS_OPTIMIZATIONS=$Optimizations")
foreach ($Define in $AdditionalDefines) { $LinkArgs.Add($Define) }
$LinkArgs.Add(('-DNUMC3DS_NATIVE_FILE_SIZE=0x{0:X}' -f $NativeSize))
$LinkArgs.Add('-Wl,--emit-relocs')
$LinkArgs.Add('-Wl,-T,native/core/core.ld')
$LinkArgs.Add("-Wl,-Map,$ObjectRoot/native.map")
$LinkArgs.Add("-I$ProjectRoot/native/include")
$LinkArgs.Add("-I$ProjectRoot/native/core")
$LinkArgs.Add("-I$ProjectRoot/native/core/commands")
$LinkArgs.Add("-I$ProjectRoot/native/core/commands/infrastructure")
$LinkArgs.Add("-I$ProjectRoot/native/diagnostics")
$LinkArgs.Add("-I$ProjectRoot/native/core/network/auth/bearssl/inc")
$LinkArgs.Add("-I$ProjectRoot/native/core/network/auth/bearssl/src")
foreach ($IncludePath in $AdditionalIncludePaths) { $LinkArgs.Add("-I$IncludePath") }
$LinkArgs.Add('-o')
$LinkArgs.Add($CoreElf)
foreach ($Src in $CoreSources) { $LinkArgs.Add($Src) }
$LinkArgs.Add('-lm')
& $Compiler @LinkArgs
if ($LASTEXITCODE -ne 0) { throw 'Native core compilation failed.' }
& $Objcopy '-O' 'binary' $CoreElf $CoreBin
if ($LASTEXITCODE -ne 0) { throw 'Native core extraction failed.' }

$CoreBytes = [IO.File]::ReadAllBytes($CoreBin)
$Relocations = [Collections.Generic.List[uint32]]::new()
$RelocationText = (& $Readelf '-rW' $CoreElf | Out-String)
if ($LASTEXITCODE -ne 0) { throw 'Native relocation inspection failed.' }
foreach ($Line in ($RelocationText -split "`r?`n")) {
    if ($Line -match '^\s*([0-9a-fA-F]{8})\s+[0-9a-fA-F]{8}\s+(R_ARM_[A-Z0-9_]+)') {
        $Offset = [Convert]::ToUInt32($Matches[1], 16)
        $Type = $Matches[2]
        if ($Type -eq 'R_ARM_ABS32') {
            if ($Offset -ge $CoreBytes.Length) { throw "Relocation offset is outside the native image: $Line" }
            $Relocations.Add($Offset)
        } elseif ($Type -notin @('R_ARM_CALL', 'R_ARM_JUMP24', 'R_ARM_REL32', 'R_ARM_PREL31')) {
            throw "Unsupported native relocation: $Line"
        }
    }
}

$HeaderSize = 40
$ImageOffset = $HeaderSize
$RelocationOffset = ($ImageOffset + $CoreBytes.Length + 3) -band -4
$UsedSize = $RelocationOffset + ($Relocations.Count * 4)
if ($UsedSize -gt $NativeSize) {
    throw ('Native module is {0} bytes; the fixed resource is {1} bytes.' -f $UsedSize, $NativeSize)
}
$PaddedCore = [byte[]]::new($NativeSize)
$Header = [uint32[]]@(
    0x434D754E, 1, $HeaderSize, 1, $ImageOffset, $CoreBytes.Length, 0,
    $RelocationOffset, $Relocations.Count, 0
)
for ($Index = 0; $Index -lt $Header.Count; $Index++) {
    [Array]::Copy([BitConverter]::GetBytes($Header[$Index]), 0, $PaddedCore, $Index * 4, 4)
}
[Array]::Copy($CoreBytes, 0, $PaddedCore, $ImageOffset, $CoreBytes.Length)
for ($Index = 0; $Index -lt $Relocations.Count; $Index++) {
    [Array]::Copy([BitConverter]::GetBytes($Relocations[$Index]), 0, $PaddedCore, $RelocationOffset + ($Index * 4), 4)
}
[IO.File]::WriteAllBytes($NativeBin, $PaddedCore)
if ($DiagnosticStage -eq 'module') {
    [IO.File]::WriteAllBytes((Join-Path $RomfsRoot 'native.bin'), $PaddedCore)
}

function Get-JoaatHash([string]$Value) {
    [uint64]$Hash = 0
    foreach ($Byte in [Text.Encoding]::UTF8.GetBytes($Value.ToLowerInvariant())) {
        $Hash = ($Hash + $Byte) -band 0xFFFFFFFFL
        $Hash = ($Hash + (($Hash -shl 10) -band 0xFFFFFFFFL)) -band 0xFFFFFFFFL
        $Hash = ($Hash -bxor ($Hash -shr 6)) -band 0xFFFFFFFFL
    }
    $Hash = ($Hash + (($Hash -shl 3) -band 0xFFFFFFFFL)) -band 0xFFFFFFFFL
    $Hash = ($Hash -bxor ($Hash -shr 11)) -band 0xFFFFFFFFL
    $Hash = ($Hash + (($Hash -shl 15) -band 0xFFFFFFFFL)) -band 0xFFFFFFFFL
    return [uint32]$Hash
}

function Build-PatchedLanguage(
    [string]$Locale,
    [string]$InputPath,
    [string]$TipsPath,
    [string]$QuickCommandsPath,
    [string]$AchievementMessagesPath,
    [string]$OutputPath
) {
    $LanguageBytes = [IO.File]::ReadAllBytes($InputPath)
    $LanguageCount = [BitConverter]::ToUInt32($LanguageBytes, 0)
    $LanguageLengthOffset = 4 + ($LanguageCount * 8)
    $LanguageTextLength = [BitConverter]::ToUInt32($LanguageBytes, $LanguageLengthOffset)
    $LanguageTextOffset = $LanguageLengthOffset + 4
    if ($LanguageTextOffset + $LanguageTextLength -ne $LanguageBytes.Length) {
        throw "$Locale language table has an unexpected layout."
    }

    $LanguageEntries = [Collections.Generic.List[object]]::new()
    $ExistingLanguageHashes = @{}
    for ($Index = 0; $Index -lt $LanguageCount; $Index++) {
        $EntryHash = [BitConverter]::ToUInt32($LanguageBytes, 4 + ($Index * 8))
        $EntryOffset = [BitConverter]::ToUInt32($LanguageBytes, 8 + ($Index * 8))
        $LanguageEntries.Add([pscustomobject]@{
            Hash = $EntryHash
            Offset = $EntryOffset
        })
        $ExistingLanguageHashes[$EntryHash] = $true
    }

    $RequestedLocalization = [Collections.Generic.List[object]]::new()
    foreach ($MessagePath in @($TipsPath, $QuickCommandsPath, $AchievementMessagesPath)) {
        foreach ($Line in Get-Content -LiteralPath $MessagePath -Encoding UTF8) {
            if ([string]::IsNullOrWhiteSpace($Line)) { continue }
            $Separator = $Line.IndexOf("`t")
            if ($Separator -le 0) { throw "Malformed localization row in $Locale`: $Line" }
            $Key = $Line.Substring(0, $Separator)
            $Text = $Line.Substring($Separator + 1)
            $RequestedLocalization.Add([pscustomobject]@{
                Key = $Key
                Hash = Get-JoaatHash $Key
                Bytes = [Text.Encoding]::UTF8.GetBytes($Text)
            })
        }
    }
    $McpeLanguagePath = Join-Path $McpeTextRoot ($Locale + '.lang')
    if (-not (Test-Path -LiteralPath $McpeLanguagePath -PathType Leaf)) {
        throw "MCPE command descriptions are missing for $Locale."
    }
    $CommandBlockKeys = @(Get-Content -LiteralPath (Join-Path $ProjectRoot 'native/core/ui/command_block_localization.txt') | Where-Object { $_ })
    $DescriptionTextByKey = @{}
    $AllMissingCommandKeys = [Collections.Generic.List[string]]::new()
    foreach ($Line in Get-Content -LiteralPath $McpeLanguagePath -Encoding UTF8) {
        $Separator = $Line.IndexOf('=')
        if ($Separator -le 0) { continue }
        $Key = $Line.Substring(0, $Separator)
        $Text = $Line.Substring($Separator + 1)
        $Comment = $Text.IndexOf("`t")
        if ($Comment -ge 0) { $Text = $Text.Substring(0, $Comment) }
        $DescriptionTextByKey[$Key] = $Text
        if ($Key.StartsWith('commands.') -and -not $ExistingLanguageHashes.ContainsKey((Get-JoaatHash $Key))) {
            $AllMissingCommandKeys.Add($Key)
        }
    }
    if (-not $DescriptionTextByKey.ContainsKey('entity.command_block_minecart.name') -and $DescriptionTextByKey.ContainsKey('item.command_block_minecart.name')) {
        $DescriptionTextByKey['entity.command_block_minecart.name'] = $DescriptionTextByKey['item.command_block_minecart.name']
    }
    $RequestedMcpeKeys = [Collections.Generic.List[string]]::new()
    foreach ($Key in $AllMissingCommandKeys) {
        if (-not $RequestedMcpeKeys.Contains($Key)) {
            $RequestedMcpeKeys.Add($Key)
        }
    }
    foreach ($Key in $CommandBlockKeys) {
        if (-not $RequestedMcpeKeys.Contains($Key) -and -not $ExistingLanguageHashes.ContainsKey((Get-JoaatHash $Key))) {
            $RequestedMcpeKeys.Add($Key)
        }
    }
    foreach ($Key in $RequestedMcpeKeys) {
        $Description = if ($DescriptionTextByKey.ContainsKey($Key)) { [string]$DescriptionTextByKey[$Key] } else { '' }
        $RequestedLocalization.Add([pscustomobject]@{
            Key = $Key
            Hash = Get-JoaatHash $Key
            Bytes = [Text.Encoding]::UTF8.GetBytes($Description)
        })
    }
    $BaseLocalizationCount = 99
    $ExpectedLocalizationCount = $BaseLocalizationCount + $RequestedMcpeKeys.Count
    if ($RequestedLocalization.Count -ne $ExpectedLocalizationCount) {
        throw ('Expected 69 loading tips, 15 quick-command labels, 13 world-transfer labels, 2 achievement messages, and {1} command/map descriptions for {0}; found {2} total entries.' -f $Locale, $RequestedMcpeKeys.Count, $RequestedLocalization.Count)
    }

    $AppendedLanguageText = [Collections.Generic.List[byte]]::new()
    $RequestedHashes = @{}
    foreach ($Entry in $RequestedLocalization) {
        if ($ExistingLanguageHashes.ContainsKey([uint32]$Entry.Hash)) {
            throw ('Localization hash 0x{0:X8} for {1} is already present in {2}.' -f $Entry.Hash, $Entry.Key, $Locale)
        }
        if ($RequestedHashes.ContainsKey([uint32]$Entry.Hash)) {
            throw ('Localization hash collision at 0x{0:X8} for {1} in {2}.' -f $Entry.Hash, $Entry.Key, $Locale)
        }
        $RequestedHashes[[uint32]$Entry.Hash] = $true
        $LanguageEntries.Add([pscustomobject]@{
            Hash = [uint32]$Entry.Hash
            Offset = [uint32]($LanguageTextLength + $AppendedLanguageText.Count)
        })
        $AppendedLanguageText.AddRange([byte[]]$Entry.Bytes)
        $AppendedLanguageText.Add(0)
    }

    $LanguageEntries = @($LanguageEntries | Sort-Object Hash)
    $PatchedLanguage = [byte[]]::new($LanguageBytes.Length + ($RequestedLocalization.Count * 8) + $AppendedLanguageText.Count)
    [Array]::Copy([BitConverter]::GetBytes([uint32]($LanguageCount + $RequestedLocalization.Count)), 0, $PatchedLanguage, 0, 4)
    for ($Index = 0; $Index -lt $LanguageEntries.Count; $Index++) {
        [Array]::Copy([BitConverter]::GetBytes([uint32]$LanguageEntries[$Index].Hash), 0, $PatchedLanguage, 4 + ($Index * 8), 4)
        [Array]::Copy([BitConverter]::GetBytes([uint32]$LanguageEntries[$Index].Offset), 0, $PatchedLanguage, 8 + ($Index * 8), 4)
    }
    $PatchedTextLengthOffset = 4 + ($LanguageEntries.Count * 8)
    [Array]::Copy([BitConverter]::GetBytes([uint32]($LanguageTextLength + $AppendedLanguageText.Count)), 0, $PatchedLanguage, $PatchedTextLengthOffset, 4)
    $PatchedTextOffset = $PatchedTextLengthOffset + 4
    [Array]::Copy($LanguageBytes, $LanguageTextOffset, $PatchedLanguage, $PatchedTextOffset, $LanguageTextLength)
    [Array]::Copy($AppendedLanguageText.ToArray(), 0, $PatchedLanguage, $PatchedTextOffset + $LanguageTextLength, $AppendedLanguageText.Count)
    [IO.File]::WriteAllBytes($OutputPath, $PatchedLanguage)
}

$BootstrapBytes = [IO.File]::ReadAllBytes($BootstrapBin)
if ($BootstrapBytes.Length -gt $CaveSize) {
    throw ('Bootstrap is {0} bytes; the executable cave is {1} bytes.' -f $BootstrapBytes.Length, $CaveSize)
}
$InputCodeBytes = [IO.File]::ReadAllBytes($InputCode)
$PatchedCode = [byte[]]::new($OriginalCodeSize)
[Array]::Copy($InputCodeBytes, 0, $PatchedCode, 0, $InputCodeBytes.Length)

function Get-CodeWord([int]$Offset) {
    return [BitConverter]::ToUInt32($PatchedCode, $Offset)
}

function Set-CodeWord([int]$Offset, [int64]$Expected, [int64]$Replacement, [string]$Description) {
    $ExpectedWord = [uint32]($Expected -band 0xFFFFFFFFL)
    $ReplacementWord = [uint32]($Replacement -band 0xFFFFFFFFL)
    $Actual = Get-CodeWord $Offset
    if ($Actual -ne $ExpectedWord) {
        throw ('{0} does not match the mapped instruction at code.bin +0x{1:X}: expected 0x{2:X8}, found 0x{3:X8}.' -f $Description, $Offset, $ExpectedWord, $Actual)
    }
    [Array]::Copy([BitConverter]::GetBytes($ReplacementWord), 0, $PatchedCode, $Offset, 4)
}

function Get-ArmBranch([uint32]$Source, [uint32]$Target, [bool]$Link) {
    $Displacement = [int64]$Target - ([int64]$Source + 8)
    if (($Displacement -band 3) -ne 0 -or $Displacement -lt -33554432 -or $Displacement -gt 33554428) {
        throw ('ARM branch from 0x{0:X8} to 0x{1:X8} is out of range or unaligned.' -f $Source, $Target)
    }
    $Opcode = if ($Link) { 0xEB000000L } else { 0xEA000000L }
    return [uint32]($Opcode -bor (($Displacement / 4) -band 0x00FFFFFFL))
}

if ($DiagnosticStage -eq 'module') {
    Set-CodeWord $AchievementRegisterCallOffset 0xEB131026 (Get-ArmBranch $AchievementRegisterCallAddress $AchievementRegisterTarget $true) 'Leader of the Pack registration call'
    Set-CodeWord $AchievementReserveCountOffset 0xE3A0003B 0xE3A0003C 'Achievement registry reserve count'
    Set-CodeWord $AchievementCountAssertOffset 0xE350003B 0xE350003C 'Achievement registry count assertion'
    Set-CodeWord $WolfOnTameOffset 0xE12FFF1E (Get-ArmBranch $WolfOnTameAddress $WolfOnTameTarget $false) 'Wolf on-tame lifecycle'
    Set-CodeWord $MaxWorldGuardOffset 0x3A000012 0xEA000012 'New-or-load eight-world rejection guard'
    Set-CodeWord $InvalidChunkPanicOffset 0x05887000 0xE1A00000 'Invalid chunk-record panic write'
    Set-CodeWord $LevelChunkMetadataPatchOffset 0xE59D000C (Get-ArmBranch $LevelChunkMetadataPatchAddress $LevelChunkMetadataPatchTarget $false) 'Level-chunk metadata write patch'
    Set-CodeWord $MobAnimationScaleOffset 0x40800000 0x40200000 'Mob animation scale'
    # InGamePlayScreen map texture uploader NULL-safety guard
    Set-CodeWord $MapUploaderNullCheckOffset 0xE1A06000 0xE1B06000 'InGamePlayScreen uploadMapTexture movs r6, r0'
    Set-CodeWord $MapUploaderBranchOffset 0xE28D2024 0x0A00002E 'InGamePlayScreen uploadMapTexture beq epilogue if null'
    Set-CodeWord $MapUploaderAddUuidOffset 0xE2801008 0xE2861008 'InGamePlayScreen uploadMapTexture add r1, r6, #8'
    Set-CodeWord $MapUploaderSetDstOffset 0xE1A00002 0xE28D0024 'InGamePlayScreen uploadMapTexture add r0, sp, #0x24'
    Set-CodeWord 0x0062C498 0xE3A00050 0xE3A00030 'MapItem 3DS sampling radius 48 blocks with MCPE circular dithering'
    Set-CodeWord 0x0062C8A4 0xE12FFF32 0xEBE947D6 'MapItem sample only available chunks through native ChunkSource service'
    Set-CodeWord $MapItemUpdateElevOffset 0xEE100A10 0xEE100A10 'MapItem preserve water depth shading'
    Set-CodeWord $MapItemLoadYOffset 0xEEF0AA4A 0xEEF0AA4A 'MapItem preserve previous elevation on all terrain paths'
    Set-CodeWord $MapItemLoadRowOffset 0xE59D001C 0xE59D001C 'MapItem preserve map row load'
    Set-CodeWord $MapItemCmpRowOffset 0xE3500000 0xE3500000 'MapItem preserve map row bounds check'
    Set-CodeWord $MapDecorationPlayerTypeOffset 0xE3A00002 0xE3A00000 'MapItemSavedData set player locator decoration type 0 (white player cursor)'
    Set-CodeWord $MinimapSkipCenterCursorOffset 0x0A000053 0xEA000053 'InGamePlayScreen renderBottomScreen skip redundant center cursor'
    # Unified 20px vertical offset for Settings toggles, sliders, and option items
    Set-CodeWord $OptionItemDefaultOffsetCodeOffset 0x03A0001A 0x03A00014 'OptionItem default title-to-control vertical offset 20px'
    Set-CodeWord $OptionsRowWrapperOffsetCodeOffset 0xE3E00005 0xE3A00000 'OptionsRow wrapper offset 0px'
    # EditWorldScreen Play and Delete button drop shadow removal
    Set-CodeWord $EditWorldPlayButtonShadowOffset 0xE3A01002 0xE3A01000 'EditWorldScreen Play button drop shadow disabled'
    Set-CodeWord $EditWorldDeleteButtonShadowOffset 0xE3A01002 0xE3A01000 'EditWorldScreen Delete button drop shadow disabled'
    # Offhand Equipment Slot: Touch UI horizontal centering (X: 94 -> 77) & 5-slot expansion (0..4)
    Set-CodeWord 0x0032E988 0xE3A0105E 0xE3A0104D 'ContainerInventoryScreen init button start X 77px'
    Set-CodeWord 0x0032EDBC 0xE3580004 0xE3580005 'ContainerInventoryScreen init button count 5'
    Set-CodeWord 0x0032EE60 0xE3A0105D 0xE3A0104C 'ContainerInventoryScreen render slot start X 76px'
    Set-CodeWord 0x0032EFEC 0xE3550004 0xE3550005 'ContainerInventoryScreen render slot count 5'
    Set-CodeWord 0x0032DCF0 0xE3A0105D 0xE3A0104C 'ContainerInventoryScreen renderHover slot start X 76px'
    Set-CodeWord 0x0032DEDC 0xE3580004 0xE3580005 'ContainerInventoryScreen renderHover slot count 5'
    Set-CodeWord 0x0032DF14 0xE3A0605B 0xE3A0604B 'ContainerInventoryScreen renderCursor start X 75px'
    Set-CodeWord 0x0032DF28 0xE3500003 0xE3500004 'ContainerInventoryScreen renderCursor cmp slot 4'
    Set-CodeWord 0x0032E528 0xE3590004 0xE3590005 'ContainerInventoryScreen touchClick equipment slot check'
    Set-CodeWord 0x0032E5FC 0xE3590003 0xE3590004 'ContainerInventoryScreen touchClick equip navigation (1)'
    Set-CodeWord 0x0032E6B0 0xE3590003 0xE3590004 'ContainerInventoryScreen touchClick equip navigation (2)'
    Set-CodeWord 0x00606ABC 0xE3500003 0xE3500004 'ContainerInventoryScreen isValidSlot cmp slot 4'
    Set-CodeWord 0x00606954 0xE3500003 0xE3500004 'ContainerInventoryScreen getItem cmp slot 4'
    Set-CodeWord 0x00606A20 0xE3500003 0xE3500004 'ContainerInventoryScreen canSwapOrPlace cmp slot 4'
    Set-CodeWord 0x005FBC30 0xE3500003 0xE3500004 'BaseInventoryScreen isSlotEmpty cmp slot 4'
    # Offhand Equipment Slot: Visual background boxes & silhouettes shifted to 76.0f (5 boxes)
    Set-CodeWord 0x0032EE08 0x42BA0000 0x42980000 'ContainerInventoryScreen render slot background start X 76.0f'
    Set-CodeWord 0x0032E810 0xE3A01004 0xE3A01005 'ContainerInventoryScreen render slot background 5 boxes'
    # Dual Map In-Hand: allow offhand item to render even when main hand holds filled map or empty map
    Set-CodeWord 0x00299F78 0x0A000080 0xE1A00000 'ItemInHandRenderer renderOffhandItem allow filled map in main hand'
    Set-CodeWord 0x00299FAC 0xCA000073 0xE1A00000 'ItemInHandRenderer renderOffhandItem allow empty map in main hand'
    # Totem parity: disable 3DS inventory slots 9..17 fallback (totems only work in offhand or main hand)
    Set-CodeWord 0x005015E0 0xE5946000 0xEA000010 'Player_getEquippedTotem disable inventory search'
    Set-CodeWord 0x004FF8F8 0x1A000024 0xEA000024 'Player_consumeTotem disable inventory search'
    # Signs: 4 lines parity (restore line 4 downward navigation in TextEditScreen)
    Set-CodeWord 0x001B7694 0xE3500003 0xE3500004 'TextEditScreen button.menu_down cmp line 4'
    # Fix teleport flinging: Entity::teleportTo pass entity motion vector (+484) to Entity::move instead of rotation (+496)
    Set-CodeWord 0x004E9244 0xE2841E1F 0xE2841F79 'Entity_teleportTo pass motion vector to move instead of rotation'
    # Fix AllocUsedBlockFromFreeBlock invalid pointer crash (Crash 9): guard prev/next pointer writes against negative/invalid pointers
    Set-CodeWord 0x0001BD0C 0x1583100C 0xC583100C 'AllocUsedBlockFromFreeBlock strgt r1, [r3, #12]'
    Set-CodeWord 0x0001BD10 0x05801000 0xD5801000 'AllocUsedBlockFromFreeBlock strle r1, [r0, #0]'
    Set-CodeWord 0x0001BD18 0x15813008 0xC5813008 'AllocUsedBlockFromFreeBlock strgt r3, [r1, #8]'
    Set-CodeWord 0x0001BD20 0x05803004 0xD5803004 'AllocUsedBlockFromFreeBlock strle r3, [r0, #4]'
    # Splash text dynamic loader & code cave:
    # 0x0045DAF0 is replaced with our compact splashes.json loader (~1.3 KB)
    # The remainder of the stock 19.5 KB routine up to 0x0046274C (~18.2 KB) is cleared to zero as a free code cave.
    $SplashBytes = [IO.File]::ReadAllBytes($SplashBin)
    $SplashCaveOffset = 0x0035DAF0
    $SplashCaveEnd = 0x0036274C
    if ($SplashBytes.Length -gt ($SplashCaveEnd - $SplashCaveOffset)) {
        throw ('Splash loader is {0} bytes; available cave is {1} bytes.' -f $SplashBytes.Length, ($SplashCaveEnd - $SplashCaveOffset))
    }
    [Array]::Copy($SplashBytes, 0, $PatchedCode, $SplashCaveOffset, $SplashBytes.Length)
    [Array]::Clear($PatchedCode, $SplashCaveOffset + $SplashBytes.Length, $SplashCaveEnd - ($SplashCaveOffset + $SplashBytes.Length))
}

if ($DiagnosticStage -ne 'layout') {
    $ExpectedHook = [byte[]](0xE8, 0xDF, 0xFF, 0xEB)
    for ($Index = 0; $Index -lt 4; $Index++) {
        if ($PatchedCode[$HookOffset + $Index] -ne $ExpectedHook[$Index]) {
            throw 'The resource-loader hook instruction is not the mapped original BL.'
        }
    }
    for ($Index = 0; $Index -lt $BootstrapBytes.Length; $Index++) {
        if ($PatchedCode[$CaveOffset + $Index] -ne 0) {
            throw ('The intended executable cave is occupied at +0x{0:X}.' -f $Index)
        }
    }
    for ($Index = 0; $Index -lt $LayeredFsPayloadSize; $Index++) {
        if ($PatchedCode[$LayeredFsPayloadOffset + $Index] -ne 0) {
            throw ('The LayeredFS payload reservation is occupied at +0x{0:X}.' -f $Index)
        }
    }
    [Array]::Copy($BootstrapBytes, 0, $PatchedCode, $CaveOffset, $BootstrapBytes.Length)
    $BranchWords = [int](($CaveAddress - ($HookAddress + 8)) / 4)
    $BranchInstruction = [uint32](0xEB000000L + ([int64]$BranchWords -band 0x00FFFFFFL))
    $BranchBytes = [BitConverter]::GetBytes($BranchInstruction)
    [Array]::Copy($BranchBytes, 0, $PatchedCode, $HookOffset, 4)
}

$OutputExHeader = Join-Path $BuildRoot 'exheader.bin'
$StockExHeader = [IO.File]::ReadAllBytes($InputExHeader)
$PatchedExHeader = [byte[]]::new($ExHeaderInfoSize)
[Array]::Copy($StockExHeader, 0, $PatchedExHeader, 0, $PatchedExHeader.Length)
$FsAccess = [BitConverter]::ToUInt32($PatchedExHeader, $ExHeaderFsAccessOffset) -bor $ExHeaderFsAccessDirectSdmc
$FsAccessBytes = [BitConverter]::GetBytes([uint32]$FsAccess)
[Array]::Copy($FsAccessBytes, 0, $PatchedExHeader, $ExHeaderFsAccessOffset, $FsAccessBytes.Length)
$IoAccess = [BitConverter]::ToUInt16($PatchedExHeader, $ExHeaderIoAccessOffset) -bor $ExHeaderIoAccessDirectSdmc
$IoAccessBytes = [BitConverter]::GetBytes([uint16]$IoAccess)
[Array]::Copy($IoAccessBytes, 0, $PatchedExHeader, $ExHeaderIoAccessOffset, $IoAccessBytes.Length)
[IO.File]::WriteAllBytes($OutputExHeader, $PatchedExHeader)
$OutputIps = Join-Path $BuildRoot 'code.ips'
& $IpsWriter -InputCode $InputCode -PatchedCodeBytes $PatchedCode -OutputIps $OutputIps
if ($LASTEXITCODE -ne 0) { throw 'IPS patch generation failed.' }

Write-Host ('Built IPS-only loose code patch diagnostic stage {0} with DirectSdmc enabled ({1} byte bootstrap)' -f $DiagnosticStage, $BootstrapBytes.Length)
Write-Host ('Preserved stock text size 0x{0:X8}; reserved 0x{1:X} bytes for Luma LayeredFS before bootstrap 0x{2:X8}.' -f $DeclaredTextSize, $LayeredFsPayloadSize, $CaveAddress)
Write-Host ('Built runtime native module ({0} bytes reserved, {1} bytes used, {2} relocations).' -f $NativeSize, $UsedSize, $Relocations.Count)
Write-Host 'Copied the checked-in release assets.'
& $Python $ResourceValidator $BuildRoot '--stock-code' $InputCode '--stock-exheader' $InputExHeader '--diagnostic-stage' $DiagnosticStage '--expected-native-size' ('0x{0:X}' -f $NativeSize)
if ($LASTEXITCODE -ne 0) { throw 'Release resource validation failed.' }
if ($DiagnosticResources -eq 'stock') {
    Remove-Item -LiteralPath (Join-Path $BuildRoot 'romfs') -Recurse -Force
}
Copy-Item -LiteralPath $ChangelogSource -Destination (Join-Path $BuildRoot 'NUMC3DS-CHANGELOG.txt') -Force
& $ManifestWriter -BuildRoot $BuildRoot -InputCode $InputCode
if ($LASTEXITCODE -ne 0) { throw 'Release manifest generation failed.' }
if (-not $SkipDeploy) {
    $AzaharMods = Join-Path $env:APPDATA 'Azahar\load\mods\00040000001B8700'
    $DeployScript = Join-Path $PSScriptRoot 'Deploy-LoosePatch.ps1'
    if (Test-Path -LiteralPath $DeployScript -PathType Leaf) {
        & $DeployScript -BuildRoot $BuildRoot -TargetRoot $AzaharMods
    }
}
