# Marvel Rivals Assist

This is a Windows C++ project with an AutoHotkey v2 control script.

The project has two separate parts:

- trigger control
- aim control

The aim target uses visibility, distance, and a target position. The AHK script sends smooth relative mouse movement.

## Build

Open a Developer PowerShell for Visual Studio 2022 and run:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  ".\rivals.vcxproj" /m `
  /p:Configuration=Release /p:Platform=x64 `
  /p:TargetName=msaud_drv_external_ahk `
  /p:OutDir=".\Output\Binaries\Rebuilt\" /v:q
```

The AHK script is `testbloody.ahk`.

- `Insert` turns the assist on or off.
- `0` closes the script.
- `7` and `8` change smoothing.
- `F7` and `F8` change the aim height.
- `F9` and `F10` change the FOV.

The script starts disabled.

## Notes

The shared state is 48 bytes. The current game offsets are in `SDK/SDK/Basic.hpp`.

This repository is private.
