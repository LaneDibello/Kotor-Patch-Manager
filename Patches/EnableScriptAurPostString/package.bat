@echo off
echo Packaging EnableScriptAurPostString.kpatch...
echo.

REM Check if DLL exists
if not exist "windows_x86.dll" (
    echo ERROR: windows_x86.dll not found!
    echo Please build the DLL first by running build.bat
    pause
    exit /b 1
)

REM Create temporary directory structure
if exist "temp_package" rmdir /s /q "temp_package"
mkdir "temp_package"
mkdir "temp_package\binaries"

REM Copy files to temp directory
echo Copying files...
copy "manifest.toml" "temp_package\" >nul
copy "hooks.toml" "temp_package\" >nul
copy "windows_x86.dll" "temp_package\binaries\" >nul

REM Create the .kpatch file (which is just a ZIP file)
echo Creating .kpatch archive...
cd temp_package

REM Use PowerShell to create ZIP, then rename to .kpatch
powershell -command "Compress-Archive -Path * -DestinationPath '..\EnableScriptAurPostString.zip' -Force"

cd ..

REM Rename .zip to .kpatch
if exist "EnableScriptAurPostString.zip" (
    if exist "EnableScriptAurPostString.kpatch" del "EnableScriptAurPostString.kpatch"
    ren "EnableScriptAurPostString.zip" "EnableScriptAurPostString.kpatch"
)

REM Clean up temp directory
rmdir /s /q "temp_package"

if exist "EnableScriptAurPostString.kpatch" (
    echo.
    echo SUCCESS! Created EnableScriptAurPostString.kpatch
    echo.
    echo File contents:
    powershell -command "Add-Type -Assembly System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::OpenRead('EnableScriptAurPostString.kpatch').Entries | ForEach-Object { Write-Host '  ' $_.FullName }"
) else (
    echo.
    echo ERROR: Failed to create .kpatch file
)

echo.
pause
