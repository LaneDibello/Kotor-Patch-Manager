@echo off
setlocal enabledelayedexpansion

REM =============================================================================
REM publish-minimal.bat - Quick Release Publisher (Launcher + Tools Only)
REM =============================================================================

echo.
echo ===================================================
echo   KotOR Patch Manager - Minimal Release
echo ===================================================
echo.

set /p VERSION="Enter version (#.#.# format): "
if "%VERSION%"=="" set VERSION=test-build

set RELEASE_NAME=KotorPatchManager-v%VERSION%
set RELEASE_DIR=releases\%RELEASE_NAME%

REM Clean
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%\bin" >nul 2>&1
mkdir "%RELEASE_DIR%\tools" >nul 2>&1

REM Build KotorPatcher.dll
echo [1/5] Building KotorPatcher...
cd src\KotorPatcher
msbuild KotorPatcher.vcxproj /p:Configuration=Release /p:Platform=Win32 /p:OutDir="..\..\%RELEASE_DIR%\bin" /m >nul 2>&1
cd ..\..

REM Build launcher
echo [2/5] Building KPatchLauncher...
cd src\KPatchLauncher
dotnet publish -c Release -r win-x86 --self-contained -p:PublishSingleFile=true -o "..\..\%RELEASE_DIR%\bin"
if exist "..\..\%RELEASE_DIR%\bin\*.pdb" del "..\..\%RELEASE_DIR%\bin\*.pdb" >nul 2>&1
cd ..\..

REM Build Patches
echo [3/5] Building patches...
set PATCHES_DIR=Patches
set RELEASE_SUBDIR=patches
set /p INCLUDE_PATCHES="Include patches? (y/n): "
if /i "%INCLUDE_PATCHES%"=="y" (
    mkdir "%RELEASE_DIR%\%RELEASE_SUBDIR%" >nul 2>&1

    echo   Scanning "%PATCHES_DIR%" for patches with manifest.toml...

    for /D %%D in ("%PATCHES_DIR%\*") do (
        if exist "%%~fD\manifest.toml" (
            echo   Building %%~nxD...
            pushd "%%~fD"

            set SKIP_PAUSE=1
            call "..\create-patch.bat" >nul 2>&1
            set SKIP_PAUSE=

            set "COPIED_ANY="
            for %%K in ("*.kpatch") do (
                if exist "%%~fK" (
                    copy /Y "%%~fK" "%~dp0%RELEASE_DIR%\%RELEASE_SUBDIR%\" >nul
                    echo     [OK] %%~nxK
                    set "COPIED_ANY=1"
                )
            )

            if not defined COPIED_ANY (
                echo     [WARN] No .kpatch produced for %%~nxD
            )

            if exist ".\additional\*" (
                rem Destination: ...\patches\<PatchName> additional files\
                set "ADDL_DEST=%~dp0%RELEASE_DIR%\%RELEASE_SUBDIR%\%%~nxD additional files"
                if not exist "!ADDL_DEST!" mkdir "!ADDL_DEST!" >nul 2>&1

                robocopy ".\additional" "!ADDL_DEST!" /E >nul 2>&1
                if errorlevel 8 (
                    echo     [ERR] Failed to copy additional files for %%~nxD
                ) else (
                    echo     [OK] Copied additional files for %%~nxD
                )
            ) 

            popd
        )
    )

) else (
    echo   Skipping patches
)


REM Copy create-patch.bat
echo [4/5] Copying tools...
copy "Patches\create-patch.bat" "%RELEASE_DIR%\tools\" >nul

REM Create README
echo [5/5] Creating README...

set "README_FILE=%RELEASE_DIR%\README.txt"

> "%README_FILE%" (
  echo KotOR Patch Manager v%VERSION%
  echo.
  echo Contents:
  echo   bin/KPatchLauncher.exe - Main application
  echo   tools/create-patch.bat - Patch creation tool
  echo   patches/ - pre-built patches I've been developing with this project
  echo.
  echo Quick Start:
  echo   1. Run bin/KPatchLauncher.exe
  echo   2. Point to your KOTOR installation
  echo   3. Point to your patch directory of choice
  echo   4. Apply and enjoy!
  echo.
  echo Created by Lane (Discord: @lane_d)
)

REM Zip it
cd releases
powershell -command "Compress-Archive -Path '%RELEASE_NAME%' -DestinationPath '%RELEASE_NAME%.zip' -Force"
cd ..

for %%I in ("releases\%RELEASE_NAME%.zip") do set SIZE=%%~zI
set /a SIZE_MB=!SIZE! / 1048576

echo.
echo SUCCESS! Created releases\%RELEASE_NAME%.zip (!SIZE_MB! MB)
echo.
pause
