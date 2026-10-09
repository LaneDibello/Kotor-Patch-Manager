# Anti-Crash Prophylactics

## Overview
**Anti-Crash Prophylactics** comprises a collection of adjustments meant to make some engines more tolerant of flaws in various mods. These flaws sometimes trigger crashes on specific versions of the game far more often than other versions of the game.

This patch addresses known engine vulnerabilities and mod-authoring defects across both Star Wars: Knights of the Old Republic (KotOR 1) and Star Wars: Knights of the Old Republic II: The Sith Lords (KotOR 2) on macOS (Aspyr 64-bit binaries). Rather than modifying user-installed mod assets or module archives directly, this patch hardens the native game binaries to tolerate and neutralize these flaws at runtime.

---

## Crash Trigger1 (K1): Model Root Node Type `0x0000` (Crash)
Models with dummy or root transform nodes exported with type `0x0000` instead of `0x0001` (such as `PLC_Kiosk3.mdl` in "Kiosk Model Fix K1") trigger an immediate crash upon loading on 64-bit macOS.

*Why it did not cause a crash on Windows:* In 32-bit Windows builds, node structs were manipulated in-place within the loaded memory buffer (`base + offset != NULL`). `ResetMdlNode` encountered the unrecognized type, fell through the type switch, and returned without crashing.

### .app Error
- In 64-bit macOS KotOR 1 (`k1_mac_aspyr_swkotor.app_x64.bin`), `AllocateMdlNode` (`0x1001ff0ae`) translates 32-bit disk nodes into 64-bit heap objects.
- Because `0x0000` does not match any recognized node type (`0x0001` Dummy, `0x0003` Light, `0x0005` Emitter, `0x0021` Trimesh, `0x0061` Skin, etc.), `AllocateMdlNode` falls through to `0x1001ff413` and returns `NULL` (`%rax = 0`).
- `InputBinary::Reset` assigns `anim->root_node = NULL` at `0x1001beff0`, and then immediately calls `ResetMdlNode(anim->root_node, mdlBase)` at `0x1001beffb`.
- `ResetMdlNode` (`0x1001bf162`) begins at `0x1001bf17e` with `movzwl (%rbx), %r15d`, dereferencing NULL address `0x0` -> **Instant `EXC_BAD_ACCESS (SIGSEGV)` crash**.

### .app Fix
- **Target**: `AllocateMdlNode` (`0x1001ff0bd`)
- **Hook Type**: `replace`
- **Original Bytes** (6 bytes): `49 89 F6 49 89 FC` (`movq %rsi, %r14; movq %rdi, %r12`)
- **Replacement Bytes** (22 bytes):
  ```assembly
  testq %rsi, %rsi
  jz .skip
  cmpw $0, (%rsi)
  jne .skip
  movw $1, (%rsi)        ; Coerce type 0x0000 -> 0x0001 (Dummy node)
  .skip:
  movq %rsi, %r14
  movq %rdi, %r12
  ```
  Bytes: `[0x48, 0x85, 0xF6, 0x74, 0x0B, 0x66, 0x83, 0x3E, 0x00, 0x75, 0x05, 0x66, 0xC7, 0x06, 0x01, 0x00, 0x49, 0x89, 0xF6, 0x49, 0x89, 0xFC]`
- **Mechanism**: A node with no geometry or emitters in a transform hierarchy is functionally a dummy node. By coercing `0x0000` to `0x0001` in memory, `AllocateMdlNode` allocates a valid `0x68`-byte dummy node, `InitDummyNode` sets `node->type = 1`, and `ResetMdlNode` initializes its transform cleanly.

---

## Crash Trigger2 (K1): Model Self-Parented Root Cycle (Hang)
Models exported where the root node's parent offset points back to the root node itself (`parent_offset == root_offset`, such as in `PLC_Kiosk3.mdl`) cause a 100% CPU infinite loop hang upon model instantiation on 64-bit macOS.

### .app Error
- In `SynchronizeNodes` (`0x1001c07cc`), the engine resolves parent node pointers using disk offsets. At `0x1001c081f`, it writes the resolved parent pointer to `node->parent` (`0x10(%r12)`). Because `parent_offset == root_offset`, `node->parent` is set to the node itself.
- During model instantiation (`0x1001d547f`), after `CreateInstanceDispatch`, the engine calls `ComputeNodeBounds` (`0x1001c0f56`) to calculate world-space bounding boxes and cumulative matrices.
- `ComputeNodeBounds` traverses up the parent chain at `0x1001c105e`:
  ```assembly
  0x1001c105e: movq 0x10(%r15), %r15   ; r15 = r15->parent
  0x1001c1062: testq %r15, %r15
  0x1001c1065: jne 0x1001c1018         ; LOOPS INFINITELY because r15->parent == r15!
  ```
  Because `r15->parent` points to `r15`, `r15` never becomes `NULL` -> **100% CPU infinite loop / hang**.

### .app Fix
- **Target**: `SynchronizeNodes` (`0x1001c081f`)
- **Hook Type**: `replace`
- **Original Bytes** (5 bytes): `49 89 44 24 10` (`movq %rax, 0x10(%r12)`)
- **Replacement Bytes** (12 bytes):
  ```assembly
  cmpq %r12, %rax        ; Is resolved parent (rax) equal to node itself (r12)?
  jne .not_self
  xorl %eax, %eax        ; Yes -> clear parent to NULL (valid root node)
  .not_self:
  movq %rax, 0x10(%r12)  ; Store parent pointer
  ```
  Bytes: `[0x4C, 0x39, 0xE0, 0x75, 0x02, 0x31, 0xC0, 0x49, 0x89, 0x44, 0x24, 0x10]`
- **Mechanism**: In tree hierarchy theory, the root node has no parent (`parent = NULL`). Clearing the parent pointer when `parent == self` converts the corrupt circular reference into a well-formed root node, allowing `ComputeNodeBounds` and all animation updates to terminate cleanly.

---

## Crash Trigger3 (K1): Walkmesh Root & Leaf AABB Invalid Offsets (Crash)
Models containing walkmesh nodes where the root AABB offset or leaf child offsets store `0xFFFFFFFF` (`-1`) (such as room model `m99ac_01a.mdl` in module `liv_m99ac`) cause an immediate crash on 64-bit macOS upon loading the model or entering the area.

*Why it did not cause a crash on Windows:* In 32-bit Windows builds, node structs and walkmesh trees were processed in-place within 32-bit address space buffers; arithmetic and signed offsets did not produce 64-bit out-of-bounds pointer allocations, or leaf checks used signed conditions (`offset > 0`).

### .app Error
1. **Root AABB Offset Defect in `ResetMdlNode` (`0x1001bf24f`)**:
   - In walkmesh trimesh nodes (type `0x0221`), `ResetMdlNode` reads the root AABB disk offset from `0x1e0(%rbx)`. In flawed models like `m99ac_01a.mdl`, this offset is `0xFFFFFFFF` (`-1`).
   - The unpatched engine unreservedly adds `mdl_data_base` to `0xFFFFFFFF` (`0x1001bf255: addq %r14, %rsi`) and calls `CreateAABB(s_pointerMap, mdlBase + 0xFFFFFFFF)`.
   - `CreateAABB` executes `0x1001ff6fa: movl 0x8(%r15), %eax`, dereferencing an address ~4 GB out-of-bounds -> **Instant `EXC_BAD_ACCESS (SIGSEGV)` crash at `0x1001ff6fa`**.
2. **Leaf Child Offset Defect in `ResetAABBTree` (`0x1001bffb4`)**:
   - In models with AABB trees whose leaf records store `0xFFFFFFFF` child pointers, `CreateAABB` (`0x1001ff716`) sign-extends them to `0xFFFFFFFFFFFFFFFF` (`-1`).
   - `ResetAABBTree` checks `if (node->left != 0)`. Because `-1 != 0`, it adds `mdl_data_base + 0xFFFFFFFF` and calls `CreateAABB`, crashing at `0x1001ff6fa`.

### .app Fix
1. **Root AABB Guard in `ResetMdlNode` (`0x1001bf24f`)**:
   - **Hook Type**: `replace`
   - **Original Bytes** (39 bytes): `8B B3 E0 01 00 00 4C 01 F6 48 8D 3D E9 69 4B 00 E8 2C 04 04 00 48 89 83 E0 01 00 00 48 89 C7 4C 89 F6 E8 AE 0C 00 00`
   - **Replacement Bytes** (73 bytes):
     ```assembly
     movl    0x1e0(%rbx), %esi      ; load root AABB disk offset
     testl   %esi, %esi             ; is it <= 0 (e.g. 0xFFFFFFFF / -1 or 0)?
     jle     .no_aabb               ; yes -> skip AABB creation entirely!
     addq    %r14, %rsi
     movabsq $0x100675c48, %rdi     ; s_pointerMap
     movabsq $0x1001ff690, %rax     ; CreateAABB
     callq   *%rax
     movq    %rax, 0x1e0(%rbx)
     movq    %rax, %rdi
     movq    %r14, %rsi
     movabsq $0x1001bff24, %rax     ; ResetAABBTree
     callq   *%rax
     jmp     .exit
     .no_aabb:
     movq    $0, 0x1e0(%rbx)        ; set root_aabb = NULL
     .exit:
     ```
     Bytes: `[0x8B, 0xB3, 0xE0, 0x01, 0x00, 0x00, 0x85, 0xF6, 0x7E, 0x34, 0x4C, 0x01, 0xF6, 0x48, 0xBF, 0x48, 0x5C, 0x67, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0xB8, 0x90, 0xF6, 0x1F, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xD0, 0x48, 0x89, 0x83, 0xE0, 0x01, 0x00, 0x00, 0x48, 0x89, 0xC7, 0x4C, 0x89, 0xF6, 0x48, 0xB8, 0x24, 0xFF, 0x1B, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xD0, 0xEB, 0x0B, 0x48, 0xC7, 0x83, 0xE0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]`
   - **Mechanism**: Inspects the disk offset before arithmetic. If non-positive (`<= 0`), sets `node->root_aabb = NULL` and resumes execution cleanly, avoiding any out-of-bounds pointer reads.
2. **Leaf Child Sanitization in `CreateAABB` (`0x1001ff716`)**:
   - **Hook Type**: `replace`
   - **Original Bytes** (16 bytes): `49 63 47 18 48 89 43 18 49 63 47 1C 48 89 43 20`
   - **Replacement Bytes** (30 bytes):
     ```assembly
     movslq 0x18(%r15), %rax    ; load left child offset
     testq  %rax, %rax          ; is it <= 0?
     jg     .left_valid
     xorl   %eax, %eax          ; clamp to 0 (NULL)
     .left_valid:
     movq   %rax, 0x18(%rbx)    ; store left child

     movslq 0x1c(%r15), %rax    ; load right child offset
     testq  %rax, %rax          ; is it <= 0?
     jg     .right_valid
     xorl   %eax, %eax          ; clamp to 0 (NULL)
     .right_valid:
     movq   %rax, 0x20(%rbx)    ; store right child
     ```
     Bytes: `[0x49, 0x63, 0x47, 0x18, 0x48, 0x85, 0xC0, 0x7F, 0x02, 0x31, 0xC0, 0x48, 0x89, 0x43, 0x18, 0x49, 0x63, 0x47, 0x1C, 0x48, 0x85, 0xC0, 0x7F, 0x02, 0x31, 0xC0, 0x48, 0x89, 0x43, 0x20]`
   - **Mechanism**: Clamps child offsets `<= 0` to `0` (`NULL`) in memory so leaf nodes never trigger child allocation during tree traversal.

---

## General Safeguards

### 1. K1: NULL Node Check in `ResetMdlNode` (`0x1001bf162`)
- **Hook Type**: `replace`
- **Original Bytes** (6 bytes): `55 48 89 E5 41 57` (`pushq %rbp; movq %rsp, %rbp; pushq %r15`)
- **Replacement Bytes** (12 bytes):
  ```assembly
  testq %rdi, %rdi
  jnz .valid
  retq                   ; If node pointer is NULL, return immediately
  .valid:
  pushq %rbp
  movq %rsp, %rbp
  pushq %r15
  ```
  Bytes: `[0x48, 0x85, 0xFF, 0x75, 0x01, 0xC3, 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57]`
- **Mechanism**: Guarantees that `ResetMdlNode` will never dereference NULL under any circumstances.

### 2. K1: Left Child Non-Positive Guard in `ResetAABBTree` (`0x1001bffb8`)
- **Hook Type**: `simple`
- **Original Bytes** (5 bytes): `48 85 C0 74 1C` (`testq %rax, %rax; je 0x1001bffd9`)
- **Replacement Bytes** (5 bytes): `48 85 C0 7E 1C` (`testq %rax, %rax; jle 0x1001bffd9`)
- **Mechanism**: Ensures traversal is skipped if the left child pointer is 0 or negative.

### 3. K1: Right Child Non-Positive Guard in `ResetAABBTree` (`0x1001bffdd`)
- **Hook Type**: `simple`
- **Original Bytes** (9 bytes): `48 85 C0 0F 85 5B FF FF FF` (`testq %rax, %rax; jne 0x1001bff41`)
- **Replacement Bytes** (9 bytes): `48 85 C0 0F 8F 5B FF FF FF` (`testq %rax, %rax; jg 0x1001bff41`)
- **Mechanism**: Ensures the right child tail-call loop only executes if the offset is strictly positive (`> 0`).

### 4. K1: NULL Node Guard in `AABB::Traverse` / `AABB::IntersectRay` (`0x1001daffb`)
- **Hook Type**: `replace`
- **Original Bytes** (8 bytes): `55 48 89 E5 41 57 41 56` (`pushq %rbp; movq %rsp, %rbp; pushq %r15; pushq %r14`)
- **Replacement Bytes** (16 bytes):
  ```assembly
  testq %rdi, %rdi
  jnz   .valid
  xorl  %eax, %eax        ; Return 0 (no collision hit)
  retq
  .valid:
  pushq %rbp
  movq  %rsp, %rbp
  pushq %r15
  pushq %r14
  ```
  Bytes: `[0x48, 0x85, 0xFF, 0x75, 0x03, 0x31, 0xC0, 0xC3, 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56]`
- **Mechanism**: When scene rendering, line-of-sight checks, or raycasts traverse area geometry, `0x1001db2f1` loads `0x1e0(%node)` (`root_aabb`) into `%rdi` and calls `0x1001daffb`. If a model has no AABB tree or has corrupt/missing AABBs that were sanitized to `NULL`, the function receives `%rdi == NULL`. Without this guard, `0x1001daffb` dereferences `%rdi` via `0x1003717db`, crashing at `0x0`. Returning `0` safely indicates no intersection and allows the scene to render and play without crashing.



