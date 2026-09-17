@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /std:c++20 /O2 /EHsc /W4 /DUNICODE /D_UNICODE trigger_controller.cpp /Fe:MarvelTriggerController.exe user32.lib winmm.lib
