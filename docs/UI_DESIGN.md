# KotOR Patch Manager - UI Design Document

## Overview

The KotOR Patch Manager uses a single-list design with checkboxes to manage patches. This document describes the current implementation and future enhancement plans.

## Current Design (MVP)

### Layout

```
┌─────────────────────────────────────────────────────────────┐
│ KotOR Patch Manager                                         │
├─────────────────────────────────────────────────────────────┤
│ Game:    [_______________________________] [Browse]         │
│ Patches: [_______________________________] [Browse][Refresh]│
├──────────────────────┬──────────────────────────────────────┤
│ Patches              │ Patch Details                        │
│ ┌──────────────────┐ │ ┌──────────────────────────────────┐ │
│ │☑ Patch 1     ↑ ↓│ │ │ Name: Selected Patch              │ │
│ │☑ Patch 2     ↑ ↓│ │ │ Version: 1.0.0                   │ │
│ │☐ Patch 3     ↑ ↓│ │ │ Author: Author Name              │ │
│ │☐ Patch 4     ↑ ↓│ │ │                                  │ │
│ │☐ Patch 5     ↑ ↓│ │ │ Description:                     │ │
│ └──────────────────┘ │ │ This patch fixes...              │ │
│                      │ │                                  │ │
│                      │ │                                  │ │
│                      │ └──────────────────────────────────┘ │
├──────────────────────┴──────────────────────────────────────┤
│ Status: 2 patches pending | Target: KOTOR 1 v1.03 (GOG)    │
├─────────────────────────────────────────────────────────────┤
│                    [Uninstall All] [Apply] [Launch]         │
└─────────────────────────────────────────────────────────────┘
```

### Color Scheme

- **Background**: `#000016` (0, 0, 22) - Dark navy blue
- **Foreground**: `#00AFFF` (0, 175, 255) - Cyan
- **Selected/Hover**: `#FFFF00` (255, 255, 0) - Yellow
- **Selection Background**: `#004488` - Blue highlight
- **Hover Background**: `#002244` - Darker blue

### Components

#### 1. Game Path Row
- **TextBox**: Displays path to game executable (swkotor.exe)
- **Browse Button**: Opens file picker for game executable
- **Behavior**: When game path is set, automatically detects KOTOR version and checks for installed patches

#### 2. Patches Path Row
- **TextBox**: Displays path to patches directory (containing .kpatch files)
- **Browse Button**: Opens folder picker for patches directory
- **Refresh Button**: Rescans patches directory and checks game installation status
- **Behavior**: When patches path is set, automatically scans for .kpatch files

#### 3. Patches List (Left Panel)
- **Format**: Single scrollable list with checkboxes
- **Each Row Contains**:
  - Checkbox (left)
  - Patch name and version (center)
  - Up/Down arrow buttons (right)

**Checkbox Behavior:**
- ☑ **Checked**: Patch will be installed when Apply is clicked
- ☐ **Unchecked**: Patch will not be installed when Apply is clicked
- **Checking a patch**: Moves it to the top of the list
- **Unchecking a patch**: Leaves it in current position

**Reordering:**
- **Up Arrow**: Moves patch up one position in list
- **Down Arrow**: Moves patch down one position in list
- **Order Significance**: Patches are applied in top-to-bottom order

**Selection:**
- Clicking a patch row (not checkbox) selects it
- Selected patch details appear in detail panel
- Selection has blue background highlight
- Hover has subtle darker blue background

#### 4. Patch Details Panel (Right Panel)
**MVP Fields (Current)**:
- **Name**: Full patch name
- **Version**: Semantic version (e.g., "1.2.0")
- **Author**: Patch creator
- **Description**: Multi-line description of what patch does

**Panel Behavior:**
- Shows "No patch selected" when nothing is selected
- Scrolls if description is long
- Updates immediately when selection changes

#### 5. Status Bar
**Left Side:**
- Shows pending changes count
- Examples:
  - "No pending changes" (checked patches match installed patches)
  - "2 patches pending" (differences exist)
  - "Ready" (initial state)

**Right Side:**
- Shows detected KOTOR version
- Format: "Target: KOTOR 1 v1.03 (GOG)"
- Shows "Unknown" if game not detected

#### 6. Action Buttons

**Uninstall All:**
- Unchecks all patches
- Triggers Apply automatically (which uninstalls everything)
- Disabled when no patches are currently installed
- Width: 120px, Height: 35px

**Apply:**
- Compares checked patches to installed patches
- If differences exist:
  1. Uninstalls all current patches
  2. Installs checked patches in list order
- If no checked patches: Uninstalls all (same as Uninstall All)
- Width: 120px, Height: 35px

**Launch:**
- Launches game with or without patches
- If patches installed: Uses DLL injection
- If no patches: Launches vanilla
- Width: 120px, Height: 35px

### Behavior Specifications

#### Startup Sequence
1. Load saved game path and patches path from settings
2. If patches path exists: Scan for .kpatch files
3. If game path exists: Detect KOTOR version
4. If both exist: Check installed patches and set checkboxes accordingly
5. Calculate pending changes

#### Checking for Installed Patches
When game path is set or Refresh is clicked:
1. Read `patch_config.toml` from game directory
2. Get list of installed patch IDs
3. Check boxes for patches that are installed
4. Uncheck boxes for patches that are not installed
5. Mark orphaned patches (installed but not in directory)

#### Orphaned Patches
**Definition**: Patches that are installed in the game but not found in the patches directory

**Handling**:
- Create placeholder `PatchItemViewModel` with:
  - `Id` from installed list
  - `Name` = "{Id} (not found)"
  - `IsOrphaned` = true
- Add to list with checkbox checked
- Show special info in detail panel
- When unchecked and Apply clicked: Removes from game, removes from list

#### Pending Changes Calculation
```
installedPatchIds = GetInstalledPatches()
checkedPatchIds = GetCheckedPatches()

if (installedPatchIds == checkedPatchIds && order matches):
    "No pending changes"
else:
    count = |installedPatchIds ∆ checkedPatchIds|  // symmetric difference
    "{count} patches pending"
```

#### Apply Operation Logic
```
checked = GetCheckedPatches()  // in list order

if (checked.isEmpty()):
    UninstallAllPatches()
else:
    UninstallAllPatches()
    foreach patch in checked:
        InstallPatch(patch)

RefreshInstalledStatus()
UpdatePendingChanges()
```

### State Management

#### Patch States
Patches no longer use visual color indicators. State information is only visible in the detail panel when selected.

**Possible States:**
- **Normal**: Patch exists in directory, not installed
- **Installed**: Patch exists in directory and is installed (checkbox checked at startup)
- **Orphaned**: Patch is installed but not found in directory
- **Incompatible**: Patch conflicts with checked patches (future enhancement)

**State Display:**
All state information shown in detail panel only (no colors, icons, or badges in list)

#### Persistence
**Settings File** (`settings.json`):
```json
{
  "GamePath": "C:\\path\\to\\swkotor.exe",
  "PatchesPath": "C:\\path\\to\\patches",
  "CheckedPatchIds": ["patch1", "patch2", "patch3"]
}
```

**Notes:**
- Checkbox states persist across application restarts
- When application starts, checkboxes are set from settings
- When installed patches are detected, they override saved checkbox state

---

## Future Enhancements

### Phase 2: Enhanced Detail Panel

**Additional Manifest Fields:**
- **Requires**: List of dependency patch IDs
  - Show as clickable links that select the dependency
  - Highlight missing dependencies
- **Conflicts**: List of conflicting patch IDs
  - Show which conflicts are currently checked
  - Warn before applying
- **Supported Versions**: Game versions this patch works with
  - Highlight if current game version is not supported
- **URL**: Link to patch homepage/documentation
  - Clickable link that opens in browser
- **License**: License information
  - Displayed as read-only text

**Hook Information** (from hooks.toml):
- List of functions hooked by this patch
- Hook types (DETOUR, SIMPLE)
- Memory addresses
- Expandable/collapsible section

**Example Layout:**
```
┌──────────────────────────────────────┐
│ Name: Widescreen Fix                │
│ Version: 2.1.0                       │
│ Author: John Doe                     │
│                                      │
│ Description:                         │
│ Enables widescreen resolutions...   │
│                                      │
│ ► Dependencies (click to expand)     │
│   ☑ base-patch v1.0                 │
│   ☐ graphics-lib v2.0 (MISSING)    │
│                                      │
│ ► Conflicts                          │
│   ⚠ legacy-widescreen                │
│                                      │
│ ► Technical Details                  │
│   Supported: KOTOR 1 v1.03 (GOG)    │
│   Hooks: 3 function(s)              │
│   License: MIT                      │
│   URL: [github.com/example]         │
└──────────────────────────────────────┘
```

### Phase 3: Drag-and-Drop Reordering

**Feature**: Alternative to Up/Down arrows for reordering patches

**Behavior:**
- Drag any patch by its row
- Drop between other patches to reorder
- Visual indicator shows drop position
- Works for both checked and unchecked patches

**Implementation Notes:**
- Use Avalonia's drag-and-drop APIs
- Maintain checkbox state during drag
- Update pending changes after drop

### Phase 4: Patch-Specific Options

**Feature**: Some patches may have configurable options

**TOML Extension:**
```toml
[[options]]
id = "resolution"
name = "Screen Resolution"
type = "choice"
choices = ["1920x1080", "2560x1440", "3840x2160"]
default = "1920x1080"

[[options]]
id = "enable_hud"
name = "Show HUD"
type = "boolean"
default = true
```

**UI Changes:**
- Add "Options" section to detail panel
- Show when selected patch has options defined
- Generate UI controls based on option type
- Save option values per-patch
- Pass options to patch DLL at runtime

### Phase 5: Search and Filtering

**Features:**
- Search box above patch list
- Filter patches by name, author, or description
- Show only checked patches
- Show only compatible patches
- Group by category (if added to manifest)

### Phase 6: Batch Operations

**Features:**
- "Check All" button
- "Uncheck All" button
- "Check Compatible" (checks patches compatible with current selection)
- "Export Patch List" (save checked patches to shareable file)
- "Import Patch List" (load checked patches from file)

### Phase 7: Visual Enhancements

**Optional State Indicators:**
- Small icons next to patch names
  - ⚠️ Orphaned
  - ⚙️ Has options configured
  - 🔗 Has dependencies
  - ❌ Has conflicts
- Tooltip on hover showing quick state summary
- Color coding as user-configurable option

### Phase 8: Patch Management

**Features:**
- Download patches from repository
- Update installed patches
- Patch changelog viewer
- Automatic conflict detection and resolution suggestions

---

## Technical Implementation Notes

### ViewModel Architecture

**MainViewModel:**
- `AllPatches: ObservableCollection<PatchItemViewModel>` - All patches in single list
- `SelectedPatch: PatchItemViewModel?` - Currently selected for detail panel
- `PendingChangesCount: int` - Number of differences between checked and installed
- `PendingChangesMessage: string` - Human-readable status message
- `HasInstalledPatches: bool` - Controls Uninstall All button state

**PatchItemViewModel:**
```csharp
public class PatchItemViewModel : ViewModelBase
{
    public string Id { get; set; }
    public string Name { get; set; }
    public string Version { get; set; }
    public string Author { get; set; }
    public string Description { get; set; }

    public bool IsChecked { get; set; }          // Checkbox state
    public bool IsOrphaned { get; set; }         // Not in directory but installed
    public int DisplayOrder { get; set; }        // Position in list

    // Future fields:
    public List<string> Requires { get; set; }
    public List<string> Conflicts { get; set; }
    public Dictionary<string, string> SupportedVersions { get; set; }
    public string? Url { get; set; }
    public string? License { get; set; }
}
```

### Key Methods

**CheckPatchStatusAsync():**
- Gets installed patches from game directory
- Sets checkboxes based on what's installed
- Creates orphaned patch placeholders
- Updates pending changes count

**OnPatchChecked():**
- If checking: Move patch to top of list
- If unchecking: Leave in current position
- Recalculate pending changes
- Save checkbox state to settings

**ApplyPatches():**
- Get list of checked patches in order
- Uninstall all currently installed patches
- Install checked patches in order
- Refresh installed status
- Clear pending changes

**RefreshPendingChanges():**
- Compare checked patches to installed patches
- Calculate count of differences
- Update status bar message
- Trigger UI updates

### File Structure

```
src/KPatchLauncher/
├── ViewModels/
│   ├── MainViewModel.cs           (refactored for single list)
│   ├── PatchItemViewModel.cs      (added IsChecked, removed colors)
│   ├── ViewModelBase.cs           (unchanged)
│   └── SimpleCommand.cs           (unchanged)
├── Views/
│   ├── MainWindow.axaml           (redesigned layout)
│   └── MainWindow.axaml.cs        (minimal code-behind)
├── Models/
│   └── AppSettings.cs             (updated to save checked patches)
└── Converters/
    └── (removed PatchStateColorConverter)
```

---

## Design Rationale

### Why Single List with Checkboxes?

**Advantages:**
1. **Simpler Mental Model**: Users understand checkboxes for "enabled/disabled"
2. **Single Source of Truth**: One list shows all patches
3. **Less Visual Clutter**: No need for transfer buttons or dual panels
4. **Better Scalability**: Detail panel can grow with more information
5. **Industry Standard**: Matches pattern used in mod managers, package managers, etc.

**Previous Issues with Dual List:**
- Confusing which list represents "installed" vs "available"
- Transfer buttons added complexity
- Color coding was subtle and not accessible
- Harder to see all patches at once

### Why Detail Panel?

**Advantages:**
1. **Expandable**: Can add more fields without cluttering list
2. **Contextual**: Shows info only when needed
3. **Accessible**: Can make font larger, scroll description
4. **Future-Proof**: Easy to add options, hooks info, etc.

### Why Status Bar for Pending Changes?

**Advantages:**
1. **Always Visible**: Users always know if changes are pending
2. **Non-Intrusive**: Doesn't block other UI
3. **Simple**: Clear message like "2 patches pending"
4. **Standard Pattern**: Common in file managers, IDEs

---

## Accessibility Considerations

### Current Implementation
- High contrast color scheme (cyan on dark blue)
- Clear selection/hover states with background changes
- Keyboard navigation support (arrow keys, tab, space for checkbox)
- Screen reader friendly (semantic controls)

### Future Enhancements
- User-configurable color themes
- Font size controls
- Optional visual state indicators (icons) for users who prefer them
- Tooltips on all interactive elements
- Keyboard shortcuts (Ctrl+A for check all, etc.)

---

## Testing Scenarios

### Basic Operations
- [ ] Check a patch → moves to top
- [ ] Uncheck a patch → stays in position
- [ ] Reorder with arrows → updates order
- [ ] Apply with checked patches → installs in order
- [ ] Apply with no checked patches → uninstalls all
- [ ] Uninstall All → unchecks all and applies

### Edge Cases
- [ ] Orphaned patches appear and can be removed
- [ ] Refreshing preserves checkbox state
- [ ] Pending changes accurate after each operation
- [ ] Uninstall All disabled when nothing installed
- [ ] Game version displays correctly
- [ ] Detail panel updates on selection change

### Persistence
- [ ] Checkbox state saved on exit
- [ ] Checkbox state restored on startup
- [ ] Installed patches override saved state
- [ ] Settings survive application crash

---

## Version History

- **v1.0** (Current): Single list with checkboxes, basic detail panel
- **v2.0** (Planned): Enhanced detail panel with dependencies, conflicts, hooks
- **v3.0** (Planned): Drag-and-drop, patch options, search/filter

---

## Related Documentation

- [IMPLEMENTATION_COMPLETE.md](../KotorPatcher/docs/IMPLEMENTATION_COMPLETE.md) - C++ patcher implementation
- [CLAUDE.md](../.claude/CLAUDE.md) - Project setup and build instructions
- Patch Manifest Specification (TODO)
- Hooks TOML Format (TODO)
