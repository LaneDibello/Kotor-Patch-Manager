# Anti-Crash Prophylactics

A collection of engine hardening patches for Star Wars: Knights of the Old Republic (KotOR 1) and Knights of the Old Republic II: The Sith Lords (KotOR 2) on macOS (Aspyr 64-bit AMD64 binary).

Many popular community mods contain subtle asset flaws (such as non-standard model headers, circular parent pointers, out-of-bounds walkmesh offsets, or unmapped controllers) that ran without issue on the original 32-bit Windows engine due to differences in memory layout and implicit fault tolerance. On the 64-bit macOS port, these flaws trigger immediate crashes or infinite loops.

Rather than modifying user-installed mod assets or module archives directly, **Anti-Crash Prophylactics** hardens the native game executable to neutralize these flaws at runtime.

---

## Hardened Engine Systems
### KotOR 1
#### 1. Model Root Node Type `0x0000` (`AllocateMdlNode` `0x1001ff0bd`)
- **Trigger Mod**: *Kiosk Model Fix K1* (`PLC_Kiosk3.mdl`)
- **Issue**: Models with dummy transform root nodes exported with type `0x0000` instead of `0x0001` cause `AllocateMdlNode` to return `NULL`. `InputBinary::Reset` then calls `ResetMdlNode(NULL)`, dereferencing address `0x0` (`EXC_BAD_ACCESS`).
- **Fix**: Coerces node type `0x0000` to `0x0001` (Dummy node) before allocation, creating a valid `0x68`-byte dummy node whose transform initializes cleanly.
  ```assembly
  testq   %rsi, %rsi
  jz      .skip
  cmpw    $0, (%rsi)
  jne     .skip
  movw    $1, (%rsi)             ; Coerce 0x0000 -> 0x0001 (Dummy)
  .skip:
  movq    %rsi, %r14
  movq    %rdi, %r12
  ```

#### 2. Self-Parented Root Cycle (`SynchronizeNodes` `0x1001c081f`)
- **Trigger Mod**: *Kiosk Model Fix K1* (`PLC_Kiosk3.mdl`)
- **Issue**: Models where the root node's parent offset points to itself (`parent_offset == root_offset`) create a circular reference (`node->parent == node`). When `ComputeNodeBounds` traverses up the parent hierarchy (`r15 = r15->parent`), the traversal never reaches `NULL`, resulting in a 100% CPU infinite loop hang.
- **Fix**: Compares the resolved parent pointer against the node itself; if equal, sets the parent to `NULL`.
  ```assembly
  cmpq    %r12, %rax             ; Is parent (rax) equal to node (r12)?
  jne     .valid
  xorl    %eax, %eax             ; Clear to NULL (valid root node)
  .valid:
  movq    %rax, 0x10(%r12)
  ```

#### 3. Walkmesh Root AABB Invalid Offsets (`ResetMdlNode` `0x1001bf24f`, `0x1001bf162`)
- **Trigger Mod**: *Yavin IV* (`liv_m99ac` / `m99ac_01a.mdl`)
- **Issue**: In walkmesh trimesh nodes (type `0x0221`), flawed models store `0xFFFFFFFF` (`-1`) as the root AABB disk offset at `0x1e0(%rbx)`. The engine adds `mdl_data_base + 0xFFFFFFFF` (~4 GB out of bounds) and attempts to read `0x8(%r15)` inside `CreateAABB`, crashing immediately.
- **Fix**:
  - `0x1001bf24f`: Checks if the root AABB offset is $\le 0$. If so, skips `CreateAABB` and sets `node->root_aabb = NULL`.
  - `0x1001bf162`: Safeguard at entry of `ResetMdlNode` to return immediately if `node` (`%rdi`) is `NULL`.
  ```assembly
  movl    0x1e0(%rbx), %esi      ; Load root AABB disk offset
  testl   %esi, %esi             ; Is offset <= 0 (-1 or 0)?
  jle     .no_aabb               ; Skip AABB creation
  addq    %r14, %rsi
  ...                            ; CreateAABB & ResetAABBTree
  jmp     .exit
  .no_aabb:
  movq    $0, 0x1e0(%rbx)        ; root_aabb = NULL
  .exit:
  ```

#### 4. Walkmesh Leaf AABB Invalid Offsets & Traversal (`CreateAABB` `0x1001ff716`, `ResetAABBTree` `0x1001bffb8`, `0x1001bffdd`)
- **Trigger Mod**: Complex area walkmeshes with `-1` leaf child pointers
- **Issue**: Leaf AABB records storing `0xFFFFFFFF` child pointers are sign-extended to `-1`. Subsequent tree traversal interprets `-1 != 0` as valid children and attempts out-of-bounds reads.
- **Fix**:
  - `0x1001ff716`: Clamps child offsets $\le 0$ to `0` (`NULL`) during AABB construction.
  - `0x1001bffb8`: Changes left-child branch from `je` $\rightarrow$ `jle` to prevent traversing non-positive offsets.
  - `0x1001bffdd`: Changes right-child tail loop from `jne` $\rightarrow$ `jg` to only follow strictly positive offsets.

#### 5. Raycast / LOS Missing AABB Guard (`AABB::Traverse` `0x1001daffb`)
- **Trigger Mod**: Geometry without AABB trees or models with pruned AABB roots
- **Issue**: Line-of-sight checks, camera collision, and raycasts call `AABB::Traverse` with `root_aabb` (`0x1e0(%node)`). If a model has no AABB tree or had corrupt AABBs sanitized to `NULL`, the function dereferenced `%rdi == NULL` through `0x1003717db`, causing an instant crash at address `0x0`.
- **Fix**: Inspects `%rdi` at function entry; if `NULL`, returns `0` (no collision hit) cleanly.
  ```assembly
  testq   %rdi, %rdi
  jnz     .valid
  xorl    %eax, %eax             ; Return 0 (no hit)
  retq
  .valid:
  pushq   %rbp
  movq    %rsp, %rbp
  ...
  ```

#### 6. Action Queue on Unallocated Object (`CSWSObject::AddActionToQueue` `0x1003948ec`)
- **Trigger Mod**: *Brotherhood of Shadow* (`ActionPauseConversation()`)
- **Issue**: Script commands queuing actions onto area or trigger objects without an action queue (`this->action_nodes == NULL` at `0x148(%rdi)`) pass `NULL` into `CExoLinkedListInternal::AddTail`, crashing at `0x8(%rbx)`. On 32-bit Windows, the engine checked `pObject->m_pActionQueue != NULL` and exited cleanly.
- **Fix**: Restores the Windows check at the entry of `AddActionToQueue`, returning cleanly if `this` or `this->action_nodes` is `NULL`.
  ```assembly
  testq   %rdi, %rdi
  jz      .exit
  cmpq    $0, 0x148(%rdi)        ; Is action_nodes NULL?
  jnz     .valid
  .exit:
  retq
  .valid:
  pushq   %rbp
  movq    %rsp, %rbp
  ...
  ```

#### 7. Unmapped Controller Vtable Corruption (`ApplyController` `0x1001adba1`, `0x1001adc1c`, `0x1001adccf`)
- **Trigger Mod**: *Yavin IV* (`sith_lnd4.mdl` in `yav_99.mod`, `plc_acd.mdl` in `yav_92.mod`)
- **Issue**: In 32-bit Windows KotOR, controller numbers (`8`, `20`, `36`, `132`, etc.) were direct byte offsets into 32-bit `CModelNode` structs. On 64-bit macOS, Aspyr added `MapControllerTypeToNodeOffset` (`0x1001d43b4`) to translate controller numbers into 64-bit struct offsets via `std::map`. When a controller is attached to an unsupported node type (e.g. `Alpha` controller `132` attached to a Dummy node `0x0001`), the map lookup fails and returns `0`. `ApplyController` (`0x1001adb22`) then writes the controller float value (`0.0f = 0x00000000`) to offset `0x0` of the node—overwriting the lower 32 bits of the node's C++ vtable pointer (`0x00000001005a2cc0` $\rightarrow$ `0x0000000100000000`, the Mach-O header). Subsequent virtual calls in `InitHierarchy` and `TraverseModelHierarchy` crash immediately.
- **Fix**: At all three controller branches in `ApplyController` (4-column quaternion, 1-column float, 3-column vector), tests if the resolved struct offset `%eax` is $\le 0$. If unmapped, branches directly to the function epilogue (`0x1001ade47`) without writing to offset `0x0`.
  ```assembly
  testl   %eax, %eax             ; Is resolved struct offset > 0?
  jg      .valid
  movabsq $0x1001ade47, %rax     ; Unmapped controller -> skip to epilogue
  jmpq    *%rax
  .valid:
  ...
  ```

---

### KotOR 2
#### 1. Action Queue on Area / Non-Action Object (`CSWSObject::AddDoCommandAction` `0x1003a990e`)
- **Trigger Mod**: *KotOR 3: The Jedi Masters 2.1* (`templestart.ncs` in `001ebo.mod`, `504ond.mod`, `506ond.mod`, `710kor.mod`, etc.)
- **Issue**: Calling NWScript `ActionDoCommand()` directly in Area OnEnter or other non-object script contexts executes on `OBJECT_SELF`, which is an Area (`CSWSArea`, object type 4), not a game object with an action queue (`CSWSObject`). Unlike other script action commands that verify downcasting via `pGameObject->AsSWSObject()` (`*0x28(%vtable)`), `ExecuteCommandActionDoCommand` (`0x10030046a`) unconditionally casts `CGameObject*` to `CSWSObject*` and calls `CSWSObject::AddDoCommandAction(pSituation)`. On 32-bit Windows KotOR 2, offset `0xEC` on `CSWSArea` was zero, branching cleanly to `0x6bd54b` to delete `pSituation` via `DeleteScriptSituation` and return without crashing. On 64-bit macOS, struct alignment and 64-bit pointer expansion shifted members such that `0x130(%CSWSArea)` hits non-zero data, bypassing the original `cmpl $0, 0x130(%rdi); je 0x1003a9a14` guard. The engine then reads `0x148(%CSWSArea)` (`0x00000000ffffff00`) as a linked list pointer and calls `CExoLinkedListInternal::AddTail`, attempting to read `0x8(%rbx)` (`0x00000000ffffff08`) and crashing (`EXC_BAD_ACCESS` / `SIGSEGV`).
- **Fix**: In `CSWSObject::AddDoCommandAction` at `0x1003a990e`, verifies polymorphic downcasting by checking that `%rdi != NULL` and that `0x28(%vtable)` matches `CSWSObject::AsSWSObject` (`0x10042bf90`). On non-action objects (or if `0x130(%rdi) == 0`), branches cleanly to `0x1003a9a14` where `CVirtualMachine::DeleteScriptSituation(pSituation)` is invoked to cleanly discard the command situation and return 0.
  ```assembly
  testq   %rdi, %rdi
  jz      .error
  movq    (%rdi), %r10
  movabsq $0x10042bf90, %r11      ; CSWSObject::AsSWSObject
  cmpq    %r11, 0x28(%r10)
  jne     .error
  cmpl    $0, 0x130(%rdi)
  jne     .valid
  .error:
  movabsq $0x1003a9a14, %r11      ; DeleteScriptSituation exit
  jmpq    *%r11
  .valid:
  ```

#### 2. Action Queue Downcast & NULL Pointer Guard (`CSWSObject::AddAction` `0x1003ae994`)
- **Trigger Mod**: General KotOR 2 modding safeguard (any script calling actions on an area or invalid target)
- **Issue**: Parallels KotOR 1's `CSWSObject::AddActionToQueue` hardening. If any script command attempts to queue an action onto a non-`CSWSObject` or an object whose action queue is NULL (`0x148(%rdi) == NULL`), `AddAction` passes the invalid list pointer into `CExoLinkedListInternal::AddTail`, crashing at `0x8(%rbx)`.
- **Fix**: At the entry of `CSWSObject::AddAction` (`0x1003ae994`), validates that `%rdi != NULL`, `0x28(%vtable) == CSWSObject::AsSWSObject` (`0x10042bf90`), and `0x148(%rdi) != NULL`. If invalid, returns immediately via `retq` without attempting to allocate or queue the action node.
  ```assembly
  testq   %rdi, %rdi
  jz      .exit
  movq    (%rdi), %r10
  movabsq $0x10042bf90, %r11      ; CSWSObject::AsSWSObject
  cmpq    %r11, 0x28(%r10)
  jne     .exit
  cmpq    $0, 0x148(%rdi)         ; Is action_nodes NULL?
  jz      .exit
  pushq   %rbp
  movq    %rsp, %rbp
  pushq   %r15
  jmp     .continue
  .exit:
  retq
  .continue:
  ```

---

## Summary of All K1 Hooks

| # | Address | Function | Type | Mod / Trigger Example | What It Does |
|---|---------|----------|------|-----------------------|--------------|
| 1 | `0x1001ff0bd` | `AllocateMdlNode` | `replace` | *Kiosk Model Fix K1* (`PLC_Kiosk3.mdl`) | Coerces invalid node type `0x0000` $\rightarrow$ `0x0001` (Dummy) so `AllocateMdlNode` doesn't return `NULL`. |
| 2 | `0x1001c081f` | `SynchronizeNodes` | `replace` | *Kiosk Model Fix K1* (`PLC_Kiosk3.mdl`) | Clears self-parented root cycle (`node->parent == node` $\rightarrow$ `NULL`) to prevent infinite loop in `ComputeNodeBounds`. |
| 3 | `0x1001bf24f` | `ResetMdlNode` | `replace` | *Yavin IV* (`liv_m99ac` / `m99ac_01a.mdl`) | Skips `CreateAABB` if walkmesh root AABB disk offset at `0x1e0(%rbx)` is $\le 0$ (e.g. `0xFFFFFFFF` / `-1`). |
| 4 | `0x1001ff716` | `CreateAABB` | `replace` | Walkmesh leaf AABB nodes | Clamps negative leaf child offsets (`-1`) to `0` (`NULL`) at allocation time. |
| 5 | `0x1001bffb8` | `ResetAABBTree` | `simple` | Walkmesh AABB traversal | Changes `je` $\rightarrow$ `jle` so non-positive left child offsets are never traversed. |
| 6 | `0x1001bffdd` | `ResetAABBTree` | `simple` | Walkmesh AABB traversal | Changes `jne` $\rightarrow$ `jg` so right child tail-loop only runs when offset $> 0$. |
| 7 | `0x1001bf162` | `ResetMdlNode` | `replace` | General MDL loading safeguard | Returns early if `node` (`%rdi`) is `NULL`. |
| 8 | `0x1001daffb` | `AABB::Traverse` | `replace` | Raycasting / LOS on models without AABB | Returns `0` (no collision hit) if `root_aabb` (`%rdi`) is `NULL`. |
| 9 | `0x1003948ec` | `CSWSObject::AddActionToQueue` | `replace` | *Brotherhood of Shadow* (`ActionPauseConversation`) | Returns early if `this` or `this->action_nodes` (`0x148(%rdi)`) is `NULL`, matching 32-bit Windows behavior. |
| 10 | `0x1001adba1` | `ApplyController` (4-col) | `replace` | *Yavin IV* (`sith_lnd4.mdl`, `plc_acd.mdl`) | Skips 4-column controller write if resolved struct offset $\le 0$ (prevents overwriting `CModelNode` vtable at `0x0`). |
| 11 | `0x1001adc1c` | `ApplyController` (1-col) | `replace` | *Yavin IV* (`sith_lnd4.mdl`, `plc_acd.mdl`) | Skips 1-column float controller write (e.g. `Alpha` 132 on Dummy node) if resolved struct offset $\le 0$. |
| 12 | `0x1001adccf` | `ApplyController` (3-col) | `replace` | *Yavin IV* (`sith_lnd4.mdl`, `plc_acd.mdl`) | Skips 3-column vector/color controller write if resolved struct offset $\le 0$. |

## Summary of All K2 Hooks

| # | Address | Function | Type | Mod / Trigger Example | What It Does |
|---|---------|----------|------|-----------------------|--------------|
| 1 | `0x1003a990e` | `CSWSObject::AddDoCommandAction` | `replace` | *The Jedi Masters 2.1* (`templestart.ncs`) | Replaces naive `0x130` offset check with polymorphic `AsSWSObject()` check; cleanly discards and deletes script situations queued on Area objects. |
| 2 | `0x1003ae994` | `CSWSObject::AddAction` | `replace` | General K2 Action Queue Safeguard | Early return `retq` if target is `NULL`, not a `CSWSObject`, or `action_nodes` (`0x148(%rdi)`) is `NULL`. |
