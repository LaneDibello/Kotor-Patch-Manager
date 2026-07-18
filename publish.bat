@echo off
setlocal enabledelayedexpansion

REM =============================================================================
REM publish.bat - Release Publisher
REM =============================================================================

echo.
echo ===================================================
echo   KotOR Patch Manager 
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
msbuild KotorPatchManager.sln /p:Configuration=Release /p:Platform=x86 /t:KotorPatcher /m >nul 2>&1
if exist "bin\Release\KotorPatcher.dll" (
    copy /Y "bin\Release\KotorPatcher.dll" "%RELEASE_DIR%\bin\" >nul
    echo   [OK] KotorPatcher.dll built successfully
) else (
    echo   [ERROR] KotorPatcher.dll not found in bin\Release\
)

REM Build launcher
echo [2/5] Building KPatchLauncher...
cd src\KPatchLauncher
dotnet publish -c Release -r win-x86 --self-contained -p:PublishSingleFile=true -o "..\..\%RELEASE_DIR%\bin"
if exist "..\..\%RELEASE_DIR%\bin\*.pdb" del "..\..\%RELEASE_DIR%\bin\*.pdb" >nul 2>&1
cd ..\..

REM Copy AddressDatabases
echo   Copying AddressDatabases...
mkdir "%RELEASE_DIR%\bin\AddressDatabases" >nul 2>&1
copy /Y "AddressDatabases\*.db" "%RELEASE_DIR%\bin\AddressDatabases\" >nul

REM Build Patches
echo [3/5] Building patches...
set PATCHES_DIR=Patches
set RELEASE_SUBDIR=patches
set /p INCLUDE_PATCHES="Include patches? (y/n): "
if /i "%INCLUDE_PATCHES%"=="y" (
    mkdir "%RELEASE_DIR%\%RELEASE_SUBDIR%" >nul 2>&1

    rem Patches build independently (isolated dirs, no shared PDB), so fan them
    rem out across parallel jobs. publish-patches.ps1 builds, collects the
    rem .kpatch files and "additional files" folders, and prints a summary.
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0publish-patches.ps1" -PatchesDir "%~dp0%PATCHES_DIR%" -OutDir "%~dp0%RELEASE_DIR%\%RELEASE_SUBDIR%"

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

echo.
echo SUCCESS! Created releases\%RELEASE_NAME%.zip
echo.
pause
