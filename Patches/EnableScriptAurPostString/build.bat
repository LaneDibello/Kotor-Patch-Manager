@echo off
echo Building EnableScriptAurPostString Patch DLL...
echo.

REM Set up Visual Studio environment
REM Adjust this path if your VS installation is different
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"

REM Compile the patch DLL
cl /LD /O2 /MD /W3 aurpoststring_patch.cpp /link /OUT:windows_x86.dll

if %ERRORLEVEL% == 0 (
    echo.
    echo Build successful!
    echo Output: windows_x86.dll
    echo.
    echo Verifying exports...
    dumpbin /EXPORTS windows_x86.dll | findstr "EnableAurPostString_Hook"
) else (
    echo.
    echo Build failed with error code %ERRORLEVEL%
)

pause
