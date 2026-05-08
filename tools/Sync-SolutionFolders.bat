@echo off
REM Wrapper script to run Sync-SolutionFolders.ps1 with proper execution policy
REM Usage: Sync-SolutionFolders.bat [-WhatIf]

setlocal

set SCRIPT_DIR=%~dp0
set PS_SCRIPT=%SCRIPT_DIR%Sync-SolutionFolders.ps1

REM Default to solution file in parent directory
set SOLUTION_PATH=%SCRIPT_DIR%..\KotorPatchManager.sln

REM Pass all arguments to PowerShell script
powershell.exe -ExecutionPolicy Bypass -File "%PS_SCRIPT%" -SolutionPath "%SOLUTION_PATH%" %*
