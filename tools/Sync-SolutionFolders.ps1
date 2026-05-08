<#
.SYNOPSIS
    Synchronizes Visual Studio solution folders with git-tracked files.

.DESCRIPTION
    This script scans the repository using 'git ls-files' and adds any tracked files
    that aren't already in the solution (either as part of a project or as SolutionItems).
    Files within project directories (.csproj/.vcxproj) are skipped since those projects
    manage their own files.

    NOTE: Files in .gitignore (like *.kpatch) won't be included. If a folder only contains
    gitignored files, it won't appear in the solution.

.PARAMETER SolutionPath
    Path to the .sln file to modify.

.PARAMETER WhatIf
    Preview changes without modifying the solution file.

.EXAMPLE
    .\Sync-SolutionFolders.ps1 -SolutionPath "KotorPatchManager.sln" -WhatIf

.EXAMPLE
    .\Sync-SolutionFolders.ps1 -SolutionPath "KotorPatchManager.sln"
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$SolutionPath,

    [switch]$WhatIf
)

# Solution folder type GUID (constant for all VS versions)
$SolutionFolderTypeGuid = "2150E333-8FDC-42A3-9474-1A3956D46DE8"

# Project type GUIDs we recognize
$CSharpProjectGuid = "FAE04EC0-301F-11D3-BF4B-00C04F79EFBC"
$CppProjectGuid = "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"

function Get-GitTrackedFiles {
    param([string]$RepoRoot)

    Push-Location $RepoRoot
    try {
        $files = git ls-files 2>$null
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to run 'git ls-files'. Is this a git repository?"
        }
        return $files
    } finally {
        Pop-Location
    }
}

function Get-ProjectDirectories {
    param([string]$Content)

    $projectDirs = @()

    # Match C# and C++ project entries
    $pattern = 'Project\("\{(?:' + $CSharpProjectGuid + '|' + $CppProjectGuid + ')\}"\)\s*=\s*"[^"]+",\s*"([^"]+)"'

    $matches = [regex]::Matches($Content, $pattern)
    foreach ($match in $matches) {
        $projectPath = $match.Groups[1].Value
        $projectDir = (Split-Path $projectPath -Parent) -replace '\\', '/'
        if ($projectDir) {
            $projectDirs += $projectDir.ToLower()
        }
    }

    return $projectDirs
}

function Get-ExistingSolutionFolders {
    param([string]$Content)

    $folders = @{}

    # Match solution folder project entries - capture name, display name, and GUID
    $pattern = 'Project\("\{' + $SolutionFolderTypeGuid + '\}"\)\s*=\s*"([^"]+)",\s*"([^"]+)",\s*"\{([^}]+)\}"'

    $matches = [regex]::Matches($Content, $pattern)
    foreach ($match in $matches) {
        $folderName = $match.Groups[1].Value
        $folderGuid = $match.Groups[3].Value
        $folders[$folderName] = $folderGuid
    }

    return $folders
}

function Get-ExistingSolutionItems {
    param([string]$Content)

    $items = @{}

    # Match all SolutionItems entries: path\to\file = path\to\file
    $pattern = '(?m)^\s*([^\s=]+)\s*=\s*\1\s*$'

    $matches = [regex]::Matches($Content, $pattern)
    foreach ($match in $matches) {
        $itemPath = ($match.Groups[1].Value) -replace '\\', '/'
        $items[$itemPath.ToLower()] = $true
    }

    return $items
}

function Get-NestedProjects {
    param([string]$Content)

    $nested = @{}

    # Match NestedProjects entries: {CHILD} = {PARENT}
    $pattern = '(?m)^\s*\{([^}]+)\}\s*=\s*\{([^}]+)\}'

    $matches = [regex]::Matches($Content, $pattern)
    foreach ($match in $matches) {
        $childGuid = $match.Groups[1].Value
        $parentGuid = $match.Groups[2].Value
        $nested[$childGuid] = $parentGuid
    }

    return $nested
}

function Get-FolderGuidByName {
    param(
        [string]$Name,
        [hashtable]$ExistingFolders
    )

    if ($ExistingFolders.ContainsKey($Name)) {
        return $ExistingFolders[$Name]
    }
    return $null
}

function Build-FolderHierarchy {
    param(
        [string[]]$FilePaths,
        [hashtable]$ExistingFolders,
        [hashtable]$ExistingNested
    )

    $folderTree = @{}

    # Build reverse map: GUID -> Name for existing folders
    $guidToName = @{}
    foreach ($name in $ExistingFolders.Keys) {
        $guidToName[$ExistingFolders[$name]] = $name
    }

    foreach ($filePath in $FilePaths) {
        $parts = $filePath -split '/'
        if ($parts.Count -gt 1) {
            $currentPath = ""
            for ($i = 0; $i -lt $parts.Count - 1; $i++) {
                $folderName = $parts[$i]
                $parentPath = $currentPath
                $currentPath = if ($currentPath) { "$currentPath/$folderName" } else { $folderName }

                if (-not $folderTree.ContainsKey($currentPath)) {
                    # Check if a folder with this name exists
                    $existingGuid = Get-FolderGuidByName -Name $folderName -ExistingFolders $ExistingFolders

                    $folderTree[$currentPath] = @{
                        Name = $folderName
                        Path = $currentPath
                        Parent = $parentPath
                        Files = @()
                        Guid = $existingGuid
                        IsExisting = ($null -ne $existingGuid)
                    }
                }
            }

            # Add file to its immediate parent folder
            $parentFolder = ($parts[0..($parts.Count - 2)]) -join '/'
            $folderTree[$parentFolder].Files += $filePath
        } else {
            # File is in root - add to a special "Solution Items" folder
            if (-not $folderTree.ContainsKey("")) {
                $existingGuid = Get-FolderGuidByName -Name "Solution Items" -ExistingFolders $ExistingFolders
                $folderTree[""] = @{
                    Name = "Solution Items"
                    Path = ""
                    Parent = $null
                    Files = @()
                    Guid = $existingGuid
                    IsExisting = ($null -ne $existingGuid)
                }
            }
            $folderTree[""].Files += $filePath
        }
    }

    return $folderTree
}

function Build-ProjectBlock {
    param(
        [string]$FolderName,
        [string]$FolderGuid,
        [string[]]$Files
    )

    $lines = @()
    $lines += "Project(`"{$SolutionFolderTypeGuid}`") = `"$FolderName`", `"$FolderName`", `"{$FolderGuid}`""

    if ($Files.Count -gt 0) {
        $lines += "`tProjectSection(SolutionItems) = preProject"
        foreach ($file in ($Files | Sort-Object)) {
            $winPath = $file -replace '/', '\'
            $lines += "`t`t$winPath = $winPath"
        }
        $lines += "`tEndProjectSection"
    }

    $lines += "EndProject"

    return $lines -join "`r`n"
}

function Add-FilesToExistingFolder {
    param(
        [string]$Content,
        [string]$FolderGuid,
        [string[]]$Files
    )

    # Find the Project block with this GUID and add files to its ProjectSection
    # Pattern: Project("{GUID}") = "Name", "Name", "{$FolderGuid}"
    #          [ProjectSection...]
    #          EndProject

    $escapedGuid = [regex]::Escape($FolderGuid)

    # Check if ProjectSection exists
    $projectPattern = "(?ms)(Project\(`"\{$SolutionFolderTypeGuid\}`"\)\s*=\s*`"[^`"]+`",\s*`"[^`"]+`",\s*`"\{$escapedGuid\}`")\s*(ProjectSection\(SolutionItems\)\s*=\s*preProject)(.*?)(EndProjectSection)\s*(EndProject)"

    if ($Content -match $projectPattern) {
        # ProjectSection exists - add files before EndProjectSection
        $newItems = ""
        foreach ($file in ($Files | Sort-Object)) {
            $winPath = $file -replace '/', '\'
            $newItems += "`r`n`t`t$winPath = $winPath"
        }

        $Content = [regex]::Replace($Content, $projectPattern, "`$1`r`n`t`$2`$3$newItems`r`n`t`$4`r`n`$5")
    } else {
        # No ProjectSection - need to add one
        $noSectionPattern = "(?ms)(Project\(`"\{$SolutionFolderTypeGuid\}`"\)\s*=\s*`"[^`"]+`",\s*`"[^`"]+`",\s*`"\{$escapedGuid\}`")\s*(EndProject)"

        if ($Content -match $noSectionPattern) {
            $newSection = "`r`n`tProjectSection(SolutionItems) = preProject"
            foreach ($file in ($Files | Sort-Object)) {
                $winPath = $file -replace '/', '\'
                $newSection += "`r`n`t`t$winPath = $winPath"
            }
            $newSection += "`r`n`tEndProjectSection`r`n"

            $Content = [regex]::Replace($Content, $noSectionPattern, "`$1$newSection`$2")
        }
    }

    return $Content
}

# Main script logic
try {
    $solutionFullPath = Resolve-Path $SolutionPath -ErrorAction Stop
    $solutionDir = Split-Path $solutionFullPath -Parent

    Write-Host "Solution: $solutionFullPath"
    Write-Host "Repository: $solutionDir"
    Write-Host ""

    $content = Get-Content $solutionFullPath -Raw -Encoding UTF8

    # Get project directories
    $projectDirs = Get-ProjectDirectories -Content $content
    Write-Host "Project directories (files managed by projects):"
    foreach ($dir in $projectDirs) {
        Write-Host "  - $dir"
    }
    Write-Host ""

    # Get existing solution state
    $existingFolders = Get-ExistingSolutionFolders -Content $content
    $existingSolutionItems = Get-ExistingSolutionItems -Content $content
    $existingNested = Get-NestedProjects -Content $content

    Write-Host "Existing solution folders: $($existingFolders.Keys -join ', ')"
    Write-Host "Existing solution items: $($existingSolutionItems.Count)"
    Write-Host ""

    # Get all git-tracked files
    $gitFiles = Get-GitTrackedFiles -RepoRoot $solutionDir
    Write-Host "Git tracked files: $($gitFiles.Count)"

    # Filter files
    $filesToAdd = @()
    foreach ($file in $gitFiles) {
        $fileLower = $file.ToLower()

        # Skip solution and project files
        if ($fileLower -match '\.(sln|csproj|vcxproj|vcxproj\.filters)$') { continue }

        # Skip files in project directories
        $inProjectDir = $false
        foreach ($projDir in $projectDirs) {
            if ($fileLower.StartsWith("$projDir/")) {
                $inProjectDir = $true
                break
            }
        }
        if ($inProjectDir) { continue }

        # Skip if already a solution item
        if ($existingSolutionItems.ContainsKey($fileLower)) { continue }

        $filesToAdd += $file
    }

    Write-Host "Files to add: $($filesToAdd.Count)"
    Write-Host ""

    if ($filesToAdd.Count -eq 0) {
        Write-Host "All git-tracked files are already in the solution. Nothing to do."
        exit 0
    }

    # Build folder hierarchy
    $folderTree = Build-FolderHierarchy -FilePaths $filesToAdd -ExistingFolders $existingFolders -ExistingNested $existingNested

    # Assign GUIDs to new folders and determine what needs creating vs updating
    $foldersToCreate = @()
    $foldersToUpdate = @()
    $nestedToAdd = @()

    # Process folders in sorted order (ensures parents before children)
    foreach ($path in ($folderTree.Keys | Sort-Object)) {
        $folder = $folderTree[$path]

        if (-not $folder.Guid) {
            $folder.Guid = [guid]::NewGuid().ToString().ToUpper()
        }

        if ($folder.IsExisting) {
            if ($folder.Files.Count -gt 0) {
                $foldersToUpdate += $folder
            }
        } else {
            $foldersToCreate += $folder

            # Determine parent GUID for nesting
            if ($folder.Parent) {
                $parentFolder = $folderTree[$folder.Parent]
                if ($parentFolder) {
                    $nestedToAdd += @{
                        Child = $folder.Guid
                        Parent = $parentFolder.Guid
                    }
                }
            }
        }
    }

    # Display what will happen
    if ($foldersToCreate.Count -gt 0) {
        Write-Host "New folders to create: $($foldersToCreate.Count)" -ForegroundColor Green
        foreach ($folder in $foldersToCreate) {
            Write-Host "  + $($folder.Name) ($($folder.Path))" -ForegroundColor Green
            Write-Host "    GUID: {$($folder.Guid)}"
            Write-Host "    Files: $($folder.Files.Count)"
        }
        Write-Host ""
    }

    if ($foldersToUpdate.Count -gt 0) {
        Write-Host "Existing folders to update: $($foldersToUpdate.Count)" -ForegroundColor Cyan
        foreach ($folder in $foldersToUpdate) {
            Write-Host "  ~ $($folder.Name) (adding $($folder.Files.Count) files)" -ForegroundColor Cyan
            foreach ($f in $folder.Files) {
                Write-Host "      $f"
            }
        }
        Write-Host ""
    }

    if ($WhatIf) {
        Write-Host "=== WhatIf Mode - No changes will be made ===" -ForegroundColor Yellow
        exit 0
    }

    if ($foldersToCreate.Count -eq 0 -and $foldersToUpdate.Count -eq 0) {
        Write-Host "No changes needed."
        exit 0
    }

    # Create backup
    $backupPath = "$solutionFullPath.$(Get-Date -Format 'yyyyMMdd_HHmmss').bak"
    Copy-Item $solutionFullPath $backupPath
    Write-Host "Backup created: $backupPath"
    Write-Host ""

    # Build and insert new project blocks
    if ($foldersToCreate.Count -gt 0) {
        $newProjectBlocks = @()
        foreach ($folder in $foldersToCreate) {
            $block = Build-ProjectBlock -FolderName $folder.Name -FolderGuid $folder.Guid -Files $folder.Files
            $newProjectBlocks += $block
        }

        # Insert before "Global" line
        $globalLinePattern = "(?m)^Global\r?$"
        $projectBlocksText = ($newProjectBlocks -join "`r`n") + "`r`n"
        $content = [regex]::Replace($content, $globalLinePattern, "$projectBlocksText`Global")
    }

    # Add files to existing folders
    foreach ($folder in $foldersToUpdate) {
        $content = Add-FilesToExistingFolder -Content $content -FolderGuid $folder.Guid -Files $folder.Files
    }

    # Insert nested project entries
    if ($nestedToAdd.Count -gt 0) {
        $nestedEntries = @()
        foreach ($entry in $nestedToAdd) {
            $nestedEntries += "`t`t{$($entry.Child)} = {$($entry.Parent)}"
        }
        $nestedEntriesText = "`r`n" + ($nestedEntries -join "`r`n")

        $nestedSectionPattern = "(?ms)(GlobalSection\(NestedProjects\)\s*=\s*preSolution.*?)((\r?\n)\s*EndGlobalSection)"
        $content = [regex]::Replace($content, $nestedSectionPattern, "`$1$nestedEntriesText`$2")
    }

    # Write modified content
    Set-Content $solutionFullPath $content -NoNewline -Encoding UTF8

    Write-Host "Solution file updated successfully!" -ForegroundColor Green
    Write-Host "  Created: $($foldersToCreate.Count) folders"
    Write-Host "  Updated: $($foldersToUpdate.Count) folders"

} catch {
    Write-Host "Error: $_" -ForegroundColor Red

    if ($backupPath -and (Test-Path $backupPath)) {
        Write-Host "Restoring backup..."
        Copy-Item $backupPath $solutionFullPath -Force
        Write-Host "Backup restored."
    }

    exit 1
}
