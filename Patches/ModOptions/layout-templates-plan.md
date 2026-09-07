# Replace the LB_OPTIONS proto item with per-type layout templates

## Context

`populateOptionsListBox` has gone messy for a structural reason: a list box has exactly
**one** `PROTOITEM`, and `LB_OPTIONS`'s is a `ButtonToggle` (`CONTROLTYPE=7`, fill
`i_checkbox01`). Only toggles genuinely conform to it. Everything else borrows the
toggle's params block and mutates it:

- `BorrowedBorderImages` swaps corner/edge/fill on a **shared** params block and restores
  them, because `CSWGuiBorder::Initialize` copies whatever it is handed. The comment at
  `OptionsMenu.h:341-346` records a real bug this already caused — a highlight border
  drawing the checkbox circle.
- Art resrefs (`lbl_optslider`, `blueborder`, `blackfill`, `lbl_optslidera`) are hardcoded
  in C++, so a resolution or reskin mod cannot touch them.
- `selectedParams` / `hilightSelectedParams` are fetched for every row but used only by
  Toggle — and never deleted, so four wrappers leak per repopulate.
- Row height derives from the toggle's `43`, which every other type then overrides anyway.

Outcome: each option type loads from **its own template control in the .gui**, so styling
and geometry live in a file modders can edit, and the borrow/restore dance disappears.

The proto item pattern is kept for `LB_DESC`, where it is correct — those rows really are
homogeneous labels.

---

## What the reverse engineering established

All confirmed against `swkotor.exe` 1.03 (hash matches `kotor1_gog_103`):

| Fact | Evidence |
|---|---|
| `TGuiPanel.CONTROLS` is a **List** of structs `[1,5,17,23,29]`, each with a `TAG` | GFF dump of `modoptionmenu.gui` |
| `InitControl` → `CSWGuiControl::Load_2 @ 0x418840`, which walks that list by tag and calls the type's virtual `Load` | `0x40B948` calls `0x418840`; `Load_2` calls `GetListCount` then `GetListElement` in a loop |
| `Load_2` also sets `this->gui_object` (offset `0x34`) to the owning panel | `41886e: mov [ecx+0x34], eax` |
| Vtable slot **18** is `Load(CResGFF*, CResStruct*)`, uniform for every control type | `0x418838: call [eax+0x48]`; DB psz-8 `Load` on all types |
| `StopLoadFromLayout` **destroys the GFF and nulls `panel->gff`** | `0x40B914: mov [esi+0x2c], 0` |
| A layout GFF is just `CResGFF(GUI, "GUI ", resref)`, then `CRes::Demand` | `StartLoadFromLayout` decomp |
| `control_res_list` **is** the CONTROLS list: `GetList(gff, &list, &root, "CONTROLS")` | `StartLoadFromLayout` decomp, last line before `bit_flags |= 2` |
| `StopLoadFromLayout` also **rewrites every registered control's MOVETO indices into pointers** | `StopLoadFromLayout` decomp, the loop after `gff = NULL` |

That last pair is what makes this workable: rather than restructuring the menu so rows are
built inside the `StartLoadFromLayout` window, **we open our own `CResGFF` and keep it**.
Row building then works at any time, including from `onDefault`.

`ResourceType::GUI = 2047` already exists (`Patches/Common/Common.h:266`).

---

## Step 0 — the `.gui` change (yours, in K-GFF)

Add four controls to `TGuiPanel`'s `CONTROLS` list. They are **never added to a list box**
and never drawn — they exist only to be loaded from.

| Tag | CONTROLTYPE | Carries |
|---|---|---|
| `OPT_TOGGLE` | 7 (ButtonToggle) | `BORDER`, `HILIGHT`, `SELECTED`, `HILIGHTSELECTED`, `TEXT`, `EXTENT` |
| `OPT_SLIDER` | 8 (Slider) | `BORDER`, `HILIGHT`, `THUMB`, `MAXVALUE`, plus a `TEXT` block for the name line |
| `OPT_TEXT` | 12 (EditBox) | `TEXT`, `BORDER`, plus a `HILIGHT` block for the focus frame |

Note: the original plan included some suggestions for the list type. I'm putting that off until a future PR. We're just focused on toggle, slider, and text right now. 

**Do not give the templates `MOVETO` fields** — see Risks.

The simplest way to author these is to copy the existing `PROTOITEM` struct for `OPT_TOGGLE`,
and copy `LB_OPTIONS`'s `SCROLLBAR` (type 9, which already has `BORDER`/`THUMB`) as the
starting point for `OPT_SLIDER`.

`EXTENT.HEIGHT` on each template becomes that type's row height, replacing
`TEXT_ROW_HEIGHT_PERCENT` and `OptionsSlider::RowHeight`. `WIDTH` stays ignored — rows are
stretched to the viewport.

`HILIGHT` on `OPT_TEXT` and `TEXT` on `OPT_SLIDER` are fields the stock `Load` does **not**
read; Step 4 adds that.

---

## Step 1 — GameAPI additions

`Patches/Common/GameAPI/`

**`CSWGuiControl::LoadFromLayout(CSWGuiObject* owner, CResGFF* gff, CResList* controls, CExoString* tag)`**
— wraps `Load_2` (DB key `CSWGuiControl` / `Load_2`, `0x418840`). This is the whole
mechanism: it finds the tagged struct and dispatches the type's own `Load`.

**`CSWGuiControl::LoadFromGff(CResGFF*, CResStruct*)`** — calls
`originalVirtual(ControlVTableSlot::Load)`. One implementation covers every type, so no
per-class `Load` address is needed.

**`CSWGuiControl::OverrideLoad(void* handler)`** + `LoadThunk` — patches slot 18, following
`OverrideDraw` / `DrawThunk` at `CSWGuiControl.cpp:282-297` exactly.

**`CSWGuiBorder::Load`, `CSWGuiText::Load`** — both `(this, CResGFF*, CResStruct*, CExoString* label)`,
DB `0x4153E0` and `0x416050`. Needed by Step 4 to pull named sub-structs.

Nothing else is required: `CResGFF` is already wrapped in full (every `ReadField*`,
`GetTopLevelStruct`, `GetList`, `GetListElement`), and `CResStruct` / `CResList` are PODs in
`CResGFF.h:61-68`. No panel `gff`/`control_res_list` accessor is needed, because we open our
own GFF.

---

## Step 2 — a layout handle in ModOptions

New small type (`Patches/ModOptions/OptionsLayout.h`), owned by `OptionsMenu`:

```cpp
// The panel's own GFF dies with StopLoadFromLayout, so hold a second one. Open() and
// the destructor mirror what StartLoadFromLayout / StopLoadFromLayout do to theirs.
class OptionsLayout {
public:
    bool Open(const char* resref);   // see sequence below
    bool Load(CSWGuiControl* control, CSWGuiObject* owner, const char* tag);
    bool IsOpen() const { return gff != nullptr; }
    ~OptionsLayout();                // Release(), then delete
private:
    CResGFF* gff = nullptr;
    CResStruct root{};
    CResList controls{};
};
```

`Open` follows the game's own order exactly — **`Demand` is not optional**, it is what
actually loads the resource:

```cpp
gff = new CResGFF(GUI, "GUI ", &CResRef(resref));   // ResourceType::GUI, Common.h
gff->Demand();                                       // CRes::Demand, CRes.h:57
gff->GetTopLevelStruct(&root);
gff->GetList(&controls, &root, "CONTROLS");          // the same list the panel keeps
```

Teardown is `gff->Release()` then `delete gff`, matching `StopLoadFromLayout`. Both
`Demand` and `Release` are already wrapped on `CRes`, which `CResGFF` derives from.

`Load` is `control->LoadFromLayout(owner, gff, &controls, &CExoString(tag))`, plus a
`debugLog` when the tag is absent — see Risks.

---

## Step 3 — rewrite `populateOptionsListBox`

`Patches/ModOptions/OptionsMenu.h`

Deleted outright: `GetProtoItem` and all five param wrappers, `BorrowedBorderImages`
(`:347-377`), `initializeEditBox`, `initializeSlider`, every hardcoded art resref, and
`TEXT_ROW_HEIGHT_PERCENT`.

Each row becomes:

```cpp
case ModOptionType::Slider: {
    OptionsSlider* slider = new OptionsSlider(this);
    layout.Load(slider, this, "OPT_SLIDER");    // art, thumb, text params, height
    slider->Configure(rowWidth, options[i], value);
    ... AddEvent wiring unchanged ...
}
```

`Configure` replaces today's `Initialize`: it keeps the min/max offset and label work but no
longer takes params, because `Load` already applied them. Row width still comes from
`GetViewportWidth() - 2 * GetPadding()`; row height now comes from the template's own extent.

`AddControls(&listOptions, 1, 0, 1)` and the `OptionsListBox` mouse driver are unchanged.

---

## Step 4 — per-type `Load` overrides

`OptionsSlider` and `OptionsEditBox` install `OverrideLoad(memberFuncAddr(&T::_Load))` in
their constructors, mirroring their existing `OverrideDraw` / `OverrideSetExtent`:

```cpp
void OptionsSlider::_Load(CResGFF* gff, CResStruct* item) {
    CSWGuiSlider::LoadFromGff(gff, item);            // BORDER, HILIGHT, THUMB, MAXVALUE
    CExoString label("TEXT");
    nameText->Load(gff, item, &label);               // the name line, from the layout
}

void OptionsEditBox::_Load(CResGFF* gff, CResStruct* item) {
    CSWGuiEditBox::LoadFromGff(gff, item);           // TEXT, BORDER
    CExoString label("HILIGHT");
    hilight->Load(gff, item, &label);                // focus frame the stock Load ignores
}
```

This is what removes the last of the hardcoded styling. Both sub-objects must exist before
`Load` runs, so they are allocated in the constructor rather than in `Initialize`.

---

## Step 5 — cleanups riding along

- **Fix the leak** at `OptionsMenu.h:566-571`: `selectedParams`, `hilightSelectedParams`,
  `protoSelectedBorder`, `protoHilightSelected` are never deleted. Moot once Step 3 lands,
  but worth doing first as its own commit so it is not lost in the refactor.
- `onDefault` keeps calling `populateOptionsListBox()` — with our own GFF held open, that
  now works unchanged.

---

## Verification

1. **Build:** `cd "Patches/ModOptions" && cmd.exe /c ..\create-patch.bat` (needs
   `< /dev/null`, see `[[create-patch-bat-needs-stdin-closed]]`).
2. **Ship the layout:** `additional/` is **not** inside the `.kpatch` — `publish.sh:202-213`
   copies it out as `ModOptions additional files/`, and it must be copied into the game's
   `Override` by hand. A stale `modoptionmenu.gui` there will silently miss the new tags, so
   re-copy it before testing.
3. **In game, with `sample.toml`** (toggle, slider, text):
   - All three row types render with their own art; the slider track is `lbl_optslider`
     with no frame, the toggle still has its checkbox.
   - **Build a menu with a slider before a toggle.** Under the old code the toggle inherited
     the slider's art; with per-type templates it must not. This is the regression the whole
     change is for.
   - Slider: arrows, track clicks and thumb drag all still commit; name and value render.
   - Text: focus frame still highlights, value commits on focus loss.
   - Defaults button resets all three types and the rows redraw — this exercises the held
     `CResGFF` after `StopLoadFromLayout` has run.
4. **Edit `OPT_SLIDER`'s `EXTENT.HEIGHT` in K-GFF**, re-copy, relaunch: the slider row height
   should change with no rebuild. That is the modder story this refactor exists to enable.

---

## Risks

- **A stale `.gui` in Override breaks the menu silently.** `InitControl` / `Load_2` on a
  missing tag simply does nothing, leaving an unloaded control. `OptionsLayout::Load` must
  log the missing tag, and `populateOptionsListBox` should skip that option rather than add
  a blank row. Bumping `manifest.toml` `version` (currently `0.0.2`) is worth doing.
- **`Load_2`'s matching was read from disassembly, not fully decompiled.** It calls
  `GetListCount` / `GetListElement` in a loop over the list; the exact compare is inferred.
  It is proven in practice — `InitControl` resolves five tags today — so the risk is only
  that an odd tag fails to match. Test with the real tags early.
- **MOVETO indices are left raw on template-loaded rows.** `StopLoadFromLayout` walks
  `panel->controls` and converts each navigable's `up`/`left`/`down`/`right` (offsets 92-104,
  loaded from the GFF as *indices*) into control *pointers*. Our rows are never registered in
  `panel->controls`, so that fixup never runs for them — under the old `Initialize` path the
  fields were simply unset, but `Load` will now populate them from the template. Give the
  templates no `MOVETO` fields, and defensively zero all four via
  `CSWGuiNavigable::SetMoveToControl` after loading each row. Left alone, an index would later
  be dereferenced as a pointer.

- **Sub-object `Load` ordering.** `CSWGuiText::Load` and `CSWGuiBorder::Load` copy params
  into the sub-object, the same "baked at load" semantics that caused the empty slider
  label. `RefreshLabel` must keep writing to `nameText->GetTextParams()`, not to a seed.
- The `.gui` edit is a binary blob in git. If iterating on it becomes painful, a
  `tools/gff.py` dump/build round-trip is the escape hatch — the format parses in ~60 lines.
