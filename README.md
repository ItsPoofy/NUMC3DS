# NuMC3DS

NuMC3DS is a loose Luma3DS mod for the USA release of Minecraft: New Nintendo 3DS Edition 1.9.19.

Target title: `00040000001B8700`  
Required update: `0004000E001B8700`, title version `v9408`

## Install

Copy the contents of `release/00040000001B8700` to:

```text
sd:/luma/titles/00040000001B8700/
```

Enable **Game patching** in the Luma3DS configuration menu. The release uses `code.ips`; it does not contain or install a `code.bin`.

For Azahar, copy that same directory to:

```text
%APPDATA%/Azahar/load/mods/00040000001B8700/
```

## Build

Install devkitARM and Python 3. Set `DEVKITARM` to the devkitARM root and optionally set `NUMC3DS_PYTHON` to a Python executable.

Provide your own decrypted v9408 update files at:

```text
inputs/mc3ds-usa-1.9.19/exefs/update/code.bin
inputs/mc3ds-usa-1.9.19/exefs/update/exheader.bin
```

Expected SHA-256 values:

```text
code.bin     7E58F876A531428B484A4A71BDCA364413CE86C6BB63250CA375605E41F260F0
exheader.bin 1D498C0F17D789641CE53FDF779E99F52A126B8C81731CB54693758FBF9482CE
```

Then run:

```powershell
.\Build-Regular.ps1 -SkipDeploy
```

The loose patch is written to `build/numc3ds`.

Private reverse-engineering databases, extracted game files, research notes, and scratch tooling are intentionally excluded from this repository.
