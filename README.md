# NuMC3DS

NuMC3DS is a title update continuation for the USA release of Minecraft: New Nintendo 3DS Edition 1.9.19.

## Build

Install devkitARM and Python 3. Set `DEVKITARM` to the devkitARM root and optionally set `NUMC3DS_PYTHON` to a Python executable.

Provide your own decrypted update files at:

```text
inputs/mc3ds-usa-1.9.19/exefs/update/code.bin
inputs/mc3ds-usa-1.9.19/exefs/update/exheader.bin
```

Then run:

```powershell
.\Build-Regular.ps1 -SkipDeploy
```

The loose patch is written to `build/numc3ds`.
