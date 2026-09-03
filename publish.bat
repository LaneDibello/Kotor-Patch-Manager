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

REM Stage sqlite3.dll beside KotorPatcher.dll. GameAPI-based patch DLLs import
REM sqlite3.dll to read addresses.db at runtime; the applicator copies it from
REM here into the game directory at install time. Without it those patches fail
REM to load in the game (Windows ships winsqlite3.dll, not sqlite3.dll).
if exist "lib\sqlite3.dll" (
    copy /Y "lib\sqlite3.dll" "%RELEASE_DIR%\bin\" >nul
    echo   [OK] sqlite3.dll staged
) else (
    echo   [ERROR] lib\sqlite3.dll not found - GameAPI patches will fail at runtime
)

REM Stage the KProxy. It takes the place of the game's binkw32.dll when the user
REM picks the library proxy under Options, and the manager copies it from here into
REM the game directory at install time. There is no MSVC project for it, only
REM src\KProxy\build-mingw.sh, so it is staged when a build has produced it and
REM the release goes out without the proxy option working when it has not.
if exist "bin\Release\binkw32.dll" (
    copy /Y "bin\Release\binkw32.dll" "%RELEASE_DIR%\bin\" >nul
    echo   [OK] binkw32.dll staged ^(KProxy^)
) else (
    echo   [WARN] binkw32.dll not found in bin\Release\
    echo          Options ^> "Use library proxy" will have nothing to stage.
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

REM Copy LICENSE
echo   Copying LICENSE...
if exist "LICENSE" (
    copy /Y "LICENSE" "%RELEASE_DIR%\LICENSE.txt" >nul
    echo   [OK] LICENSE.txt staged
) else (
    echo   [ERROR] LICENSE not found in repository root
)

REM Create README
echo [5/5] Creating README...

set "README_FILE=%RELEASE_DIR%\README.txt"

> "%README_FILE%" (
  echo KotOR Patch Manager v%VERSION%
  echo.
  echo Contents:
  echo   bin/KPatchLauncher.exe - Main application
  echo   bin/KotorPatcher.dll   - Runtime patcher loaded into the game
  echo   bin/binkw32.dll        - KProxy: loads the patcher when the game starts,
  echo                            used when Options ^> "Use library proxy" is on
  echo   bin/sqlite3.dll        - Address database access for GameAPI patch DLLs
  echo   tools/create-patch.bat - Patch creation tool
  echo   patches/ - pre-built patches I've been developing with this project
  echo   LICENSE.txt - MIT License
  echo.
  echo Quick Start:
  echo   1. Run bin/KPatchLauncher.exe
  echo   2. Point to your KOTOR installation
  echo   3. Point to your patch directory of choice
  echo   4. Apply and enjoy!
  echo.
  echo Deployment ^(Options menu^):
  echo   unchecked - the manager starts the game and injects the patcher
  echo   checked   - KProxy replaces the game's binkw32.dll and loads the patcher
  echo               itself; the original is kept as binkw32Hooked.dll
  echo   The choice is locked while patches are installed.
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
