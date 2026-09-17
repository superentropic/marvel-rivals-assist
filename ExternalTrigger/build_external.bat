@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /std:c++20 /O2 /EHsc /W4 /DUNICODE /D_UNICODE external_trigger.cpp /Fe:MarvelExternalTrigger_v2.exe psapi.lib user32.lib
