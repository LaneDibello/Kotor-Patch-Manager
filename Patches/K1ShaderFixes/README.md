# KotOR 1 Shader Fixes (macOS)

Ports the applicable OpenGL `ARB_fragment_program` shader, distance fog, grass fog, environment cubemap, lightmap saturation, and client-state fixes from **J**'s *KotOR II Aspyr Shader Fixes* and *Grass Fog Fix* to **Star Wars: Knights of the Old Republic** (Steam, Aspyr macOS 64-bit).

---

## Background: Why Fog and Reflections Are Broken in Aspyr's Mac Port

When Aspyr ported *Knights of the Old Republic* to macOS, they replaced several legacy Direct3D/OpenGL fixed-function multi-texture combiner paths with 23 embedded `GL_ARB_fragment_program` assembly shaders (`!!ARBfp1.0` strings stored at `0x100525F20`–`0x100536E8F`). Doing so introduced several rendering bugs across the game:

1. **Missing Distance Fog on Lightmapped and Environment-Mapped Surfaces:**
   Under the OpenGL `ARB_fragment_program` specification, fixed-function `GL_FOG` state is **ignored** whenever a fragment program is bound unless the shader explicitly declares `OPTION ARB_fog_linear;` (or `ARB_fog_exp` / `ARB_fog_exp2`). Aspyr omitted `OPTION ARB_fog_linear;` from every single fragment program in the binary. As a result, all opaque lightmapped room geometry (Render Paths 10, 11, 12) and vertex-lit environment-mapped meshes (Render Paths 6, 7) render completely unfogged, breaking atmospheric depth on Kashyyyk (`kas_m24aa`, `kas_m25aa`), Taris Undercity (`tar_m04aa`), Dantooine (`danm14*`), Korriban (`korr_m36aa`), and the Unknown World (`unk_m41*`).
2. **Stale Fragment-Program Fog When Entering Unfogged Modules (`Scene::DoFog`):**
   On macOS OpenGL drivers, `ARB_fragment_program` shaders compiled with `OPTION ARB_fog_linear;` continue interpolating linear fog using the last uploaded `GL_FOG_START` and `GL_FOG_END` distances even after `glDisable(GL_FOG)` is called. Without resetting `GL_FOG_END` when fog is disabled, transitioning from a fogged exterior module into an unfogged interior module leaves the interior shrouded in stale fog.
3. **Malformed Environment Reflections (`2D` vs. `CUBE` Sampler Bug):**
   In Fragment Shaders 1 (`0x100526031`), 3 (`0x100526321`), and 7 (`0x100526CB1`), Aspyr sampled the bound 6-sided environment cubemap texture using `2D` (`TEX r1, T1, texture[1], 2D;` and `TEX r2, T2, texture[2], 2D;`) instead of `CUBE`, even though the corresponding enabler paths (`RenderEnvmapSkinnedMesh` at `0x1001B346C`, `0x1001B0F48`, and `0x1001B1068`) bind a cubemap texture via `CTexture::UseCubeMap` (`0x1001FA5C6`) and `BindFragmentShader` (`0x1001A8E4E`) selects the `is_cubemap == 0` branch whenever `CTexture` vtable `+0x20` returns `0`. On environment-mapped skinned characters (such as Canderous's armor and visor, Jedi robes, Twi'leks, and Taris citizens), sampling the unbound `2D` target on texture unit 1 reads the default white fallback texture `(1, 1, 1)`, causing diffuse alpha masks (`r0.a < 1.0`) to glow solid white/grey instead of reflecting the cubemap.
4. **Overbright Lightmaps and Additive Cubemap Blowout:**
   In Fragment Shader 0 (`0x100525F20`), vertex lighting (`v0`) and the lightmap (`r1`) were summed with an unclamped `ADD` instruction before multiplying by the diffuse texture, washing out brightly lit surfaces. Similarly, Fragment Shaders 3, 4, 7, and 8 used unclamped sums (`ADD` / `MAD`) or additive cubemap blending instead of clamped sums (`ADD_SAT` / `MAD_SAT`) and alpha interpolation (`LRP`).
5. **Disabled Primary UV Array After Particle Cleanup:**
   At the end of the particle rendering cleanup function (`0x1001E3770`), `0x1001E380C` tail-calls `glDisableClientState(GL_TEXTURE_COORD_ARRAY)` on texture unit 0, leaving primary texture coordinates disabled for subsequent draw calls that assume unit 0's UV array remains enabled.
6. **`Scene::DoGrass` Disables `GL_FOG` Before Grass & Transparent Mesh Passes:**
   In modules with grass (such as Kashyyyk Upper Shadowlands `kas_m24aa`, Dantooine plains, Taris Undercity, and Lehon), `Scene::Render` executes `Scene::DoGrass` (`0x100196128`) immediately after the opaque room pass (`0x100195D00`) and **before** the transparent mesh passes (`0x100196298` / `0x100196F58` / `0x1001972D6`). `Scene::DoGrass` unconditionally calls `glDisable(GL_FOG)` at `0x10019616C` and never calls `Scene::DoFog(this)`, which leaves both the grass itself and all subsequent fixed-function transparent meshes (hanging vines, foliage canopies, holograms, and transparent placeables) unfogged.
7. **Pitch-Black Fog on Non-Envmapped Skinned Characters & Creatures (`SkinMesh::Render` / Fragment Shader 11):**
   In `SkinMesh::Render` (`0x10017FDB7`), when an environment-mapped skinned character is rendered (`0x1001808EB`–`0x10018092B`), the engine uploads `scene->m_cSunFogColor` (`+0x84`) to `program.env[1]` (`c[1]`) via `glProgramEnvParameter4fvARB` before calling `RenderEnvmapSkinnedMesh` (`0x1001B346C`). However, when a **non-envmapped** skinned character or creature is rendered in a fogged module (`0x100180965`–`0x1001809D4`, such as Ajuur the Hutt in `tar_m02ae` or creatures in `kas_m25aa`), `SkinMesh::Render` binds Fragment Shader 11 (`0x10052742E`, Case 3) which interpolates toward `c[1]` (`LRP r0.rgb, v1.r, c[1], r0;`), **without ever uploading the fog color to `program.env[1]`**. Because bump-mapped meshes (`0x100180FA2`, `0x1001B58B1`) overwrite `program.env[1]` with `(0.0, 0.0, 0.0, 1.0)`, `c[1]` is pitch black and every non-envmapped skinned character fades to black `(0, 0, 0)` instead of the area fog color.

---

## Included Fixes (10 Hooks)

| # | Address | Size | Target Function / Shader | Description |
| :--- | :--- | :--- | :--- | :--- |
| **1** | `0x100525F20` | 272 B | **Fragment Shader 0** (Render Path 12: Lightmap + Diffuse) | Adds `OPTION ARB_fog_linear;` so opaque lightmapped room geometry receives distance fog, and replaces `ADD` with `ADD_SAT` when summing vertex color and lightmap lighting to prevent washed-out surfaces. |
| **2** | `0x100526CB1` | 459 B | **Fragment Shader 7** (Render Path 10: Lightmap + 2D Envmap) | Changes `TEX r2, T2, texture[2], 2D` to `CUBE` to match the bound cubemap texture, clamps both lighting sums with `MAD_SAT` and `ADD_SAT`, and enables `OPTION ARB_fog_linear;`. |
| **3** | `0x100526E7D` | 461 B | **Fragment Shader 8** (Render Path 11: Lightmap + Cubemap Envmap) | Clamps both lighting sums with `MAD_SAT` and `ADD_SAT` and enables `OPTION ARB_fog_linear;` on lightmapped cubemap-reflective room surfaces. |
| **4** | `0x100526321` | 309 B | **Fragment Shader 3** (Render Path 6: Vertex-Lit + 2D Envmap) | Changes `TEX r1, T1, texture[1], 2D` to `CUBE`, replaces additive cubemap blending (`SUB` + `MAD`) with `LRP r0.rgb, r0.a, r0, r1`, and enables `OPTION ARB_fog_linear;`. |
| **5** | `0x100526457` | 311 B | **Fragment Shader 4** (Render Path 7: Vertex-Lit + Cubemap Envmap) | Replaces additive cubemap blending (`SUB` + `MAD`) with `LRP r0.rgb, r0.a, r0, r1` and enables `OPTION ARB_fog_linear;`. |
| **6** | `0x10019898C` | 174 B | **`Scene::DoFog`** (`0x1001988B0`) | When `scene->m_bFogEnabled` (`+0x70`) is `0`, calls `glDisable(GL_FOG)` and sets `GL_FOG_END = 1000000.0f` (`0x49742400`) via `glFogf` so `ARB_fragment_program` shaders with `OPTION ARB_fog_linear;` do not render stale fog in unfogged modules. Preserves the internal branch target at `0x100198A3A`. |
| **7** | `0x1001E380C` | 5 B | **Particle Cleanup** (`0x1001E3770`) | Replaces the tail-call `jmp _glDisableClientState` (`E9 D7 CD 2C 00`) with `ret; nop; nop; nop; nop` (`C3 90 90 90 90`) so `GL_TEXTURE_COORD_ARRAY` remains enabled on texture unit 0 after particle rendering. |
| **8** | `0x100196184` | 50 B | **`Scene::DoGrass`** (`0x100196128`) | Compacts the grass setup block after the initial `glDisable(GL_FOG)` call to invoke `Scene::DoFog(this)` (`0x1001988B0`), restoring area distance fog for grass and all subsequent transparent mesh passes. |
| **9** | `0x100526031` | 374 B | **Fragment Shader 1** (`BindFragmentShader` Case 20: Skinned Mesh + Envmap) | Changes `TEX r1, T1, texture[1], 2D` to `CUBE` so environment-mapped skinned characters (Canderous's armor/visor, Jedi robes, Twi'leks, armored NPCs) sample the bound cubemap texture instead of the unbound white 2D fallback texture. |
| **10** | `0x10052742E` | 288 B | **Fragment Shader 11** (`BindFragmentShader` Case 3: Skinned Mesh Fog Shader) | Replaces `PARAM c[2] = { program.env[0..1] };` with `PARAM c1 = state.fog.color;` and `LRP r0.rgb, v1.r, c[1], r0;` with `LRP r0.rgb, v1.r, c1, r0;` so non-envmapped skinned characters and creatures read `GL_FOG_COLOR` directly instead of fading to pitch black `(0, 0, 0)`. |

---

## Differences Between KotOR 1 and KotOR 2 (`K2AspyrShaderFixes`)

During reverse engineering of the KotOR 1 macOS binary (`KOTOR_Exe`), every hook from **J**'s `K2AspyrShaderFixes` and `K2GrassFogFix` was audited against KotOR 1:

- **Included (10 Hooks above):** Fragment Shaders 0, 1, 3, 4, 7, 8, 11, `Scene::DoFog` fog-disable neutralization, the particle `GL_TEXTURE_COORD_ARRAY` state fix, and `Scene::DoGrass` fog state restoration.
- **KotOR 1-Specific Fix in Fragment Shader 11 (`0x10052742E`):**
  - In KotOR 2, Obsidian/Aspyr wrote `MOV r0.a, v0.x;` (copying the red channel into alpha), whereas KotOR 1 already had the correct `MOV r0.a, v0;`. However, in KotOR 1's `SkinMesh::Render` (`0x10017FDB7`), Aspyr forgot to call `glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, &fog_color)` on the non-envmapped skinned-mesh branch (`0x100180965`–`0x1001809D4`), causing `c[1]` to retain `(0, 0, 0, 1)` from bump-mapped meshes and turning non-envmapped characters/creatures pitch-black in fog. Binding `state.fog.color` in Fragment Shader 11 (`0x10052742E`) fixes this directly.
- **Not Needed in KotOR 1 (Already Correct in Stock K1):**
  - **Gamma `+15` Offset:** Aspyr's hardcoded `+15` brightness offset in KotOR 2 does not exist in KotOR 1's gamma handlers.
  - **KotOR 2 Extra Lightmap/Envmap Shaders (K2 Shaders 23 & 24):** KotOR 2 added two extra fragment programs (25 total vs. K1's 23) that do not exist in KotOR 1.
  - **`SceneMeshNode::SelectRenderPath` (`0x1001B6474`) Bump-Map Reordering:** In KotOR 2, Aspyr broke bumpy-shiny water/surfaces by branching on `alpha < 1.0` before testing for normal-mapped environment maps. In KotOR 1, `SelectRenderPath` (`0x1001B6474`) uses a different render-path dispatch table and does not suffer from K2's regression.

---

## Target Executable

- **Game:** *Star Wars: Knights of the Old Republic* (Steam, Aspyr macOS 64-bit)
- **Executable:** `Knights of the Old Republic.app/Contents/MacOS/KOTOR_Exe`
- **Architecture:** `x86_64`
- **SHA-256:** `C1FCB8D37C702849882A17751C63EE0AF7C2B9CBBC3B31B98A5F0EDBC27C6D71`

## Credits

- **J** — Original research and creation of *KotOR II Aspyr Shader Fixes*
