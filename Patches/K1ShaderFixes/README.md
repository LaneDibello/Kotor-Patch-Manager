# KotOR 1 Shader Fixes (macOS)

Ports and expands the OpenGL `ARB_fragment_program` / `ARB_vertex_program` shader, distance fog, grass fog, environment cubemap reflection, lightmap saturation, bump-mapping, and client-state fixes from **J, Synchro and Vriff**'s *KotOR II Aspyr Shader Fixes* and *Grass Fog Fix* to **Star Wars: Knights of the Old Republic** (Steam, Aspyr macOS 64-bit).

---

## Patch Options

In **KotOR Patch Manager**, this patch provides five independent toggles (all enabled by default):

1. **Fog Fix (`fog-fix`)** *(Default: Enabled)*
   Restores missing distance fog on walls/floors, outdoor grass, bump-mapped surfaces, and transparent foliage, prevents indoor rooms from staying foggy after leaving a foggy area, and fixes characters and creatures turning pitch-black in fog.
2. **Reflections Fix (`reflections-fix`)** *(Default: Enabled)*
   Fixes broken shiny and metallic reflections on character armor, visors, clothing, and reflective room surfaces so they sample cubemaps properly and add reflections after diffuse lighting (`MAD_SAT`) instead of glowing solid white/grey or washing out diffuse detail.
3. **Lightmap Fix (`lightmap-fix`)** *(Default: Enabled)*
   Prevents brightly lit rooms and reflective floors or walls from looking overly bright, washed-out, or blown out.
4. **Particle Texture Fix (`particle-texture-fix`)** *(Default: Enabled)*
   Fixes a bug where particle effects like smoke, sparks, and explosions can accidentally turn off textures on the next object drawn on screen.
5. **Bump Mapping Fix (`bumpmap-fix`)** *(Default: Enabled)*
   Restores hardware normal-map and specular bump mapping on creatures, characters, animated water ripples (e.g., Manaan and Dantooine), and world surfaces that Aspyr stubbed out in the macOS port, and fixes broken specular/diffuse math and uninitialized vertex color attributes in the bump-mapping shaders.

---

## Included Fixes (28 Non-Overlapping Hooks Across 5 Options)

Every shader and code path is partitioned into non-overlapping byte slices so that all five options can be toggled in any of the 32 combinations without hook address conflicts:

| # | Address | Size | Option (`when`) | Target Function / Shader | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | `0x100525F2B` | 177 B | `fog-fix` | **Fragment Shader 0** (`0x100525F20`, Lightmap + Diffuse) | Adds `OPTION ARB_fog_linear;` so opaque lightmapped room geometry receives distance fog. |
| **2** | `0x100525FDC` | 38 B | `lightmap-fix` | **Fragment Shader 0** (`0x100525F20`, Lightmap + Diffuse) | Replaces `ADD` with `ADD_SAT` when summing vertex color and lightmap lighting to prevent washed-out surfaces. |
| **3** | `0x100526CBC` | 265 B | `fog-fix` | **Fragment Shader 7** (`0x100526CB1`, Lightmap + 2D Envmap) | Adds `OPTION ARB_fog_linear;` so lightmapped environment-mapped room surfaces receive distance fog. |
| **4** | `0x100526DC5` | 28 B | `reflections-fix` | **Fragment Shader 7** (`0x100526CB1`, Lightmap + 2D Envmap) | Changes `TEX r2, T2, texture[2], 2D;` to `CUBE` to match the bound cubemap texture. |
| **5** | `0x100526DF5` | 91 B | `lightmap-fix` | **Fragment Shader 7** (`0x100526CB1`, Lightmap + 2D Envmap) | Clamps both the lightmap and reflection sums with `MAD_SAT` and `ADD_SAT`. |
| **6** | `0x100526E88` | 265 B | `fog-fix` | **Fragment Shader 8** (`0x100526E7D`, Lightmap + Cubemap Envmap) | Adds `OPTION ARB_fog_linear;` so lightmapped cubemap-reflective room surfaces receive distance fog. |
| **7** | `0x100526FC3` | 91 B | `lightmap-fix` | **Fragment Shader 8** (`0x100526E7D`, Lightmap + Cubemap Envmap) | Clamps both the lightmap and cubemap-reflection sums with `MAD_SAT` and `ADD_SAT`. |
| **8** | `0x10052632C` | 182 B | `fog-fix` | **Fragment Shader 3** (`0x100526321`, Vertex-Lit + 2D Envmap) | Adds `OPTION ARB_fog_linear;` on vertex-lit environment-mapped meshes. |
| **9** | `0x1005263E2` | 116 B | `reflections-fix` | **Fragment Shader 3** (`0x100526321`, Vertex-Lit + 2D Envmap) | Changes `TEX r1, T1, texture[1], 2D;` to `CUBE` and upgrades reflection blending from pre-lighting `LRP` to post-lighting `MAD_SAT`. |
| **10** | `0x100526462` | 182 B | `fog-fix` | **Fragment Shader 4** (`0x100526457`, Vertex-Lit + Cubemap Envmap) | Adds `OPTION ARB_fog_linear;` on vertex-lit cubemap-mapped meshes. |
| **11** | `0x100526518` | 118 B | `reflections-fix` | **Fragment Shader 4** (`0x100526457`, Vertex-Lit + Cubemap Envmap) | Upgrades cubemap reflection blending from pre-lighting `LRP` to post-lighting `MAD_SAT` (`SUB r1.a,1,r0.a; MUL r0,r0,v0; MAD_SAT r0,r1,r1.a,r0;`). |
| **12** | `0x10019898C` | 174 B | `fog-fix` | **`Scene::DoFog`** (`0x1001988B0`) | When `scene->m_bFogEnabled` (`+0x70`) is `0`, calls `glDisable(GL_FOG)` and sets `GL_FOG_END = 1000000.0f` (`0x49742400`) via `glFogf` so `ARB_fragment_program` shaders with `OPTION ARB_fog_linear;` do not render stale fog in unfogged modules. |
| **13** | `0x1001E380C` | 5 B | `particle-texture-fix` | **Particle Cleanup** (`0x1001E3770`) | Replaces the tail-call `jmp _glDisableClientState` (`E9 D7 CD 2C 00`) with `ret; nop; nop; nop; nop` (`C3 90 90 90 90`) so `GL_TEXTURE_COORD_ARRAY` remains enabled on texture unit 0 after particle rendering. |
| **14** | `0x100196184` | 50 B | `fog-fix` | **`Scene::DoGrass`** (`0x100196128`) | Compacts the grass setup block after the initial `glDisable(GL_FOG)` call to invoke `Scene::DoFog(this)` (`0x1001988B0`), restoring area distance fog for grass and all subsequent transparent mesh passes. |
| **15** | `0x10052603C` | 191 B | `fog-fix` | **Fragment Shader 1** (`0x100526031`, Skinned 2D Envmap in Fog) | Binds `state.fog.color` to `c[1]` so fogged environment-mapped characters do not read an uninitialized black `program.env[1]`. |
| **16** | `0x100526117` | 144 B | `reflections-fix` | **Fragment Shader 1** (`0x100526031`, Skinned 2D Envmap in Fog) | Changes `TEX r1, T1, texture[1], 2D` to `CUBE` and upgrades reflection blending from pre-lighting `LRP` to post-lighting `MAD_SAT`. |
| **17** | `0x1005261B3` | 191 B | `fog-fix` | **Fragment Shader 2** (`0x1005261A8`, Skinned Cubemap Envmap in Fog) | Binds `state.fog.color` to `c[1]` so fogged cubemap-mapped characters do not turn black when `program.env[1]` is clobbered. |
| **18** | `0x10052628E` | 146 B | `reflections-fix` | **Fragment Shader 2** (`0x1005261A8`, Skinned Cubemap Envmap in Fog) | Upgrades cubemap reflection blending from pre-lighting `LRP` to post-lighting `MAD_SAT`. |
| **19** | `0x10052658F` | 221 B | `fog-fix` | **Fragment Shader 5** (`0x10052658F`, `BumpShinyA`) | Adds `OPTION ARB_fog_linear;` so bump-mapped specular surfaces receive distance fog. |
| **20** | `0x100526736` | 203 B | `bumpmap-fix` | **Fragment Shader 5** (`0x10052658F`, `BumpShinyA`) | Clamps normal-map dot products (`DP3_SAT`) and fixes overwritten `r5`/`r4` registers (`MUL r5.rgb,r5,r5;` and `MUL r4.rgb,r4,c[1];`). |
| **21** | `0x100526921` | 221 B | `fog-fix` | **Fragment Shader 6** (`0x100526921`, `BumpShinyB`) | Adds `OPTION ARB_fog_linear;` so bump-mapped specular surfaces receive distance fog. |
| **22** | `0x100526AC8` | 203 B | `bumpmap-fix` | **Fragment Shader 6** (`0x100526921`, `BumpShinyB`) | Clamps normal-map dot products (`DP3_SAT`) and fixes overwritten `r5`/`r4` registers (`MUL r5.xyz,r5,r5;` and `MUL r4.xyz,r4,c[1];`). |
| **23** | `0x10052704B` | 204 B | `fog-fix` | **Fragment Shader 9** (`0x10052704B`, `EnvBumpMap`) | Adds `OPTION ARB_fog_linear;` so environment-bump-mapped surfaces receive distance fog. |
| **24** | `0x1001B906A` | 64 B | `fog-fix` | **`PartTriMesh::SetupLighting`** (`0x1001B8FC2`) | Honors `PartTriMesh::m_bAffectedByFog` (`0x99(%r14)`): enables `GL_FOG` and restores `GL_FOG_END = scene->m_fFogEnd` (`0x78`) for fogged meshes (`fog 1`) without falling through into `glDisable(GL_FOG)`, and disables `GL_FOG` while setting `GL_FOG_END = 1000000.0f` (`0x49742400`) for skyboxes/backdrops (`fog 0`) so neither fixed-function nor `ARB_fog_linear` shaders fog the skybox. |
| **25** | `0x10052742E` | 288 B | `fog-fix` | **Fragment Shader 11** (`0x10052742E`, Skinned Mesh Fog Shader) | Binds `state.fog.color` so non-envmapped skinned characters and creatures read `GL_FOG_COLOR` directly instead of fading to pitch black `(0, 0, 0)`. |
| **26** | `0x10019DF48` | 6 B | `bumpmap-fix` | **`getATIFragmentShadersDisabled`** (`0x10019DF48`) | Returns `1` (`mov eax, 1; ret`) so all 5 bump-mapping checks in `PartTriMesh::SelectRenderPath` and `SkinMesh::Render` succeed on macOS. |
| **27** | `0x1005C8E5C` | 279 B | `bumpmap-fix` | **`skinBumpyVP`** (`0x1005C7B10`) | Appends `MOV result.color,{0,0,0,1};` in-place so bump-mapped skinned meshes do not leave `fragment.color.primary` undefined. |
| **28** | `0x1005CA440` | 330 B | `bumpmap-fix` | **`renderSkinnedVPSkinVP2`** (`0x1005C93B0`) | Appends `MOV result.color,{0,0,0,1};` in-place so bump-mapped skinned meshes in fog do not leave `fragment.color.primary` undefined. |

---

## Target Executable

- **Game:** *Star Wars: Knights of the Old Republic* (Steam, Aspyr macOS 64-bit)
- **Executable:** `Knights of the Old Republic.app/Contents/MacOS/KOTOR_Exe`
- **Architecture:** `x86_64`
- **SHA-256:** `C1FCB8D37C702849882A17751C63EE0AF7C2B9CBBC3B31B98A5F0EDBC27C6D71`

## Credits

- **J, Synchro and Vriff** — Original research and creation of *KotOR II Aspyr Shader Fixes*
