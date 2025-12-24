@echo off
setlocal enabledelayedexpansion

REM =============================================================================
REM create-patch.bat - Unified KotOR Patch Builder and Packager
REM =============================================================================
REM This script automatically:
REM 1. Detects patch type (SIMPLE or DETOUR based on .cpp files)
REM 2. Builds DLLs if needed (auto-detects Visual Studio)
REM 3. Packages everything into a .kpatch file
REM
REM Usage: create-patch.bat [PatchName]
REM   If PatchName is not provided, uses current directory name
REM =============================================================================

echo.
echo ===================================================
echo   KotOR Patch Manager - Patch Creation Tool
echo ===================================================
echo.

REM Get patch name from argument or current directory
if "%~1"=="" (
    for %%I in (.) do set PATCH_NAME=%%~nxI
    echo Using current directory name: !PATCH_NAME!
) else (
    set PATCH_NAME=%~1
    echo Using provided patch name: !PATCH_NAME!
)

echo.

REM =============================================================================
REM Step 1: Validate required files
REM =============================================================================
echo [1/5] Validating patch files...

if not exist "manifest.toml" (
    echo ERROR: manifest.toml not found!
    echo Please create a manifest.toml file in this directory.
    echo See templates/manifest.template.toml for an example.
    pause
    exit /b 1
)
echo   [OK] manifest.toml found

REM Check for any *hooks.toml files
set HOOKS_FOUND=0
for %%F in (*hooks.toml) do set HOOKS_FOUND=1
if !HOOKS_FOUND! EQU 0 (
    echo ERROR: No hooks files found ^(*hooks.toml^)!
    echo Please create at least one hooks file in this directory.
    pause
    exit /b 1
)
echo   [OK] hooks file^(s^) found

REM =============================================================================
REM Step 2: Detect patch type and build DLLs if needed
REM =============================================================================
echo.
echo [2/5] Detecting patch type...

REM Count .cpp files
set CPP_COUNT=0
for %%F in (*.cpp) do set /a CPP_COUNT+=1

if !CPP_COUNT! EQU 0 (
    echo   Patch type: SIMPLE ^(no C++ files detected^)
    echo   Skipping DLL compilation
    set BUILD_DLL=0
) else (
    echo   Patch type: DETOUR ^(!CPP_COUNT! C++ file^(s^) detected^)
    echo   DLL compilation required
    set BUILD_DLL=1
)

REM Build DLL if needed
if !BUILD_DLL! EQU 1 (
    echo.
    echo [3/5] Compiling patch DLL...

    REM Try to find Visual Studio using vswhere
    set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    set VCVARSALL=

    if exist !VSWHERE! (
        echo   Searching for Visual Studio installation...
        for /f "usebackq tokens=*" %%i in (`!VSWHERE! -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            set VS_PATH=%%i
        )

        if defined VS_PATH (
            set VCVARSALL=!VS_PATH!\VC\Auxiliary\Build\vcvars32.bat
            echo   Found Visual Studio at: !VS_PATH!
        )
    )

    REM Fallback to manual path if vswhere failed
    if not defined VCVARSALL (
        echo   vswhere.exe not found, trying default VS 2022 path...
        set VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat
    )

    REM Check if vcvarsall exists
    if not exist "!VCVARSALL!" (
        echo.
        echo ERROR: Visual Studio not found!
        echo.
        echo Please install Visual Studio 2022/2019/2017 with C++ build tools.
        echo Or set VCVARSALL environment variable to point to vcvars32.bat
        echo.
        echo Expected path: !VCVARSALL!
        pause
        exit /b 1
    )

    REM Set up Visual Studio environment
    echo   Setting up Visual Studio environment...
    call "!VCVARSALL!" >nul 2>&1

    REM Generate exports.def if it doesn't exist
    if not exist "exports.def" (
        echo   Generating exports.def...
        echo LIBRARY !PATCH_NAME! > exports.def
        echo EXPORTS >> exports.def

        REM Extract function names from .cpp files
        for %%F in (*.cpp) do (
            findstr /R /C:"extern.*__cdecl" "%%F" > temp_exports.txt
            for /f "tokens=5 delims=( " %%G in (temp_exports.txt) do (
                echo     %%G >> exports.def
                echo   Added export: %%G
            )
            del temp_exports.txt >nul 2>&1
        )
    )

    REM Compile all .cpp files into one DLL
    echo   Compiling DLL from !CPP_COUNT! source file^(s^)...
    set CPP_FILES=
    for %%F in (*.cpp) do set CPP_FILES=!CPP_FILES! %%F

    REM Add Common directory files if they exist (including subdirectories)
    set COMMON_FILES=
    if exist "..\Common\*.cpp" (
        for %%F in (..\Common\*.cpp) do set COMMON_FILES=!COMMON_FILES! "%%F"
        echo   Including Common library files...
    )
    if exist "..\Common\GameAPI\*.cpp" (
        for %%F in (..\Common\GameAPI\*.cpp) do set COMMON_FILES=!COMMON_FILES! "%%F"
        echo   Including GameAPI library files...
    )

    cl /LD /O2 /MD /W3 /std:c++17 /I"..\Common" /I"..\..\lib" !CPP_FILES! !COMMON_FILES! /link /DEF:exports.def /LIBPATH:"..\..\lib" sqlite3.lib /OUT:windows_x86.dll >build.log 2>&1

    if !ERRORLEVEL! NEQ 0 (
        echo.
        echo ERROR: DLL compilation failed!
        echo Check build.log for details.
        type build.log
        pause
        exit /b 1
    )

    echo   [OK] DLL compiled successfully: windows_x86.dll

    REM Verify exports
    echo   Verifying DLL exports...
    dumpbin /EXPORTS windows_x86.dll | findstr /V /C:"ordinal hint" | findstr /V /C:"Summary" | findstr /V /C:"Exports" | findstr /V /C:"dumpbin" | findstr /V /C:"Microsoft" | findstr /V /C:"Copyright" >nul
    if !ERRORLEVEL! EQU 0 (
        echo   [OK] DLL exports verified
    )
) else (
    echo.
    echo [3/5] Skipping DLL compilation ^(SIMPLE patch^)
)

REM =============================================================================
REM Step 4: Package .kpatch file
REM =============================================================================
echo.
echo [4/5] Creating .kpatch package...

REM Clean up any existing package
if exist "temp_package" rmdir /s /q "temp_package" >nul 2>&1
if exist "!PATCH_NAME!.kpatch" del "!PATCH_NAME!.kpatch" >nul 2>&1
if exist "!PATCH_NAME!.zip" del "!PATCH_NAME!.zip" >nul 2>&1

REM Create temporary directory structure
mkdir "temp_package" >nul 2>&1

REM Copy required files
echo   Copying files...
copy "manifest.toml" "temp_package\" >nul
echo   [OK] manifest.toml

REM Copy ALL hooks files (*hooks.toml pattern)
set HOOKS_COUNT=0
for %%F in (*hooks.toml) do (
    copy "%%F" "temp_package\" >nul
    echo   [OK] %%F
    set /a HOOKS_COUNT+=1
)

if !HOOKS_COUNT! EQU 0 (
    echo   WARNING: No hooks files found ^(*hooks.toml^)
)

REM Copy DLL if it exists
if !BUILD_DLL! EQU 1 (
    if exist "windows_x86.dll" (
        mkdir "temp_package\binaries" >nul 2>&1
        copy "windows_x86.dll" "temp_package\binaries\" >nul
        echo   [OK] binaries/windows_x86.dll
    ) else (
        echo   ERROR: windows_x86.dll not found after build!
        rmdir /s /q "temp_package"
        pause
        exit /b 1
    )
)

REM Create ZIP archive
echo   Creating archive...
cd temp_package
powershell -command "Compress-Archive -Path * -DestinationPath '..\!PATCH_NAME!.zip' -Force" >nul 2>&1
cd ..

REM Rename to .kpatch
if exist "!PATCH_NAME!.zip" (
    ren "!PATCH_NAME!.zip" "!PATCH_NAME!.kpatch"
) else (
    echo   ERROR: Failed to create ZIP archive
    rmdir /s /q "temp_package"
    pause
    exit /b 1
)

REM Clean up temp directory
rmdir /s /q "temp_package"

REM =============================================================================
REM Step 5: Verify package
REM =============================================================================
echo.
echo [5/5] Verifying package...

if exist "!PATCH_NAME!.kpatch" (
    echo   [OK] Package created: !PATCH_NAME!.kpatch
    echo.
    echo   Package contents:
    powershell -command "Add-Type -Assembly System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::OpenRead('!PATCH_NAME!.kpatch').Entries | ForEach-Object { Write-Host '     ' $_.FullName }"
    echo.
    echo ===================================================
    echo   SUCCESS! Patch created successfully.
    echo ===================================================
    echo.
) else (
    echo   ERROR: Package verification failed
    pause
    exit /b 1
)

REM Clean up build artifacts
echo Cleaning up build artifacts...
del *.obj *.lib *.exp build.log >nul 2>&1

if not defined SKIP_PAUSE pause
