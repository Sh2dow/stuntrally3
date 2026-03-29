# SR3 Carbon HDR Luminance Fix Plan

## Problem

The current Carbon-style HDR path is only partially correct.

What works:

- SR3 now creates a dedicated `hdrLumDummy` target.
- `hdrLumDummy` is cleared to white before the HDR passes.
- `HDR/BrightPass_Start` and `HDR/FinalToneMapping` now read `hdrLumDummy` instead of reusing scene color as `lumRt`.
- bloom settings are reapplied after compositor rebuild via `gcom->slHdrBloom(nullptr)`.

What is still wrong:

- `hdrLumDummy` is only a stabilization placeholder, not a real luminance source.
- both shaders still expect `lumRt` to contain inverse average luminance at texel `(0, 0)`.
- without real scene luminance, exposure stays effectively constant and the image response becomes flat or washed out.
- pushing brightness and contrast further over-whitens the image instead of adapting exposure cleanly.

This means the startup crash was solved, but the actual Carbon-style HDR pipeline is still incomplete.

## Current Technical State

Relevant files:

- `src/common/AppGui_Compositor.cpp`
- `src/common/GuiCom_Options.cpp`
- `src/OgreCommon/Utils/HdrUtils.cpp`
- `data/materials/Common/HDR.material`
- `data/materials/Common/GLSL/BrightPass_Start_ps.glsl`
- `data/materials/Common/GLSL/FinalToneMapping_ps.glsl`
- `data/materials/Common/HLSL/BrightPass_Start_ps.hlsl`
- `data/materials/Common/HLSL/FinalToneMapping_ps.hlsl`
- `data/materials/Common/Metal/BrightPass_Start_ps.metal`
- `data/materials/Common/Metal/FinalToneMapping_ps.metal`

Important shader behavior:

- `BrightPass_Start` samples `lumRt` and multiplies scene color by `fInvLumAvg`.
- `FinalToneMapping` also samples `lumRt` and multiplies scene color by `fInvLumAvg` before bloom combine and filmic mapping.
- `FinalToneMapping` currently has an additional hardcoded lift/contrast adjustment after tonemapping.

Result:

- the current path is stable,
- but exposure is fake,
- so grading and bloom tuning are compensating for missing luminance data.

## Long-Term Goal

Replace the dummy luminance placeholder with a real auto-exposure chain that behaves like the original SR3/Ogre HDR intent and can then be tuned toward Carbon.

The final pipeline should:

1. compute scene luminance from HDR scene color,
2. reduce it down to a small luminance texture or single-pixel average,
3. optionally smooth luminance over time,
4. feed the resulting inverse average luminance into both `BrightPass_Start` and `FinalToneMapping`,
5. keep bloom threshold and bloom intensity as artistic controls, not exposure substitutes.

## Implementation Plan

## Phase A: Recover a Real Luminance Path

Goal:

- stop using `hdrLumDummy` as the real `lumRt`.

Tasks:

- inspect the older SR3/Ogre HDR path and recover how luminance textures were originally built and consumed.
- locate whether SR3 previously relied on Ogre sample HDR resources such as `lumRt`, `lumRt0`, `lumRt1`, or similar mip/downsample textures.
- document the exact expected semantic of `lumRt`:
  - whether it stores average luminance,
  - inverse average luminance,
  - or exposure-scaled luminance.

Expected output:

- one concrete definition of what `lumRt` must contain for the current shaders to be mathematically correct.

## Phase B: Build the Luminance Reduction Chain in `AppGui_Compositor.cpp`

Goal:

- add a real compositor path for luminance generation.

Tasks:

- add one HDR luminance extraction target from `rtt_final`.
- add successive downsample passes until the chain reaches a tiny texture or 1x1 texture.
- ensure all luminance targets are non-MSAA.
- bind the final luminance result as `lumRt` for:
  - `HDR/BrightPass_Start`
  - `HDR/FinalToneMapping`
- keep `hdrLumDummy` only as a temporary fallback during bring-up, then remove it.

Constraints:

- do not repeat the failed custom MSAA resolve experiment.
- keep the luminance chain independent from race-window present logic.
- avoid mutating target counts after definitions have already been added.

Expected output:

- a deterministic luminance texture chain owned by the compositor.

## Phase C: Add Temporal Exposure Adaptation

Goal:

- prevent harsh brightness pumping between tunnels, open streets, headlights, and canyon lights.

Tasks:

- add a previous-frame luminance texture or previous-frame scalar exposure state.
- blend previous and current luminance values with separate dark-to-bright and bright-to-dark adaptation speeds.
- expose adaptation speed settings in code first, UI later if needed.

Carbon target behavior:

- quick but not instant adaptation,
- headlights and bright signage should bloom aggressively,
- dark roads should stay moody instead of being lifted to grey.

Expected output:

- exposure that feels authored and cinematic rather than static.

## Phase D: Rebalance the Shader Math

Goal:

- stop compensating for missing luminance with hardcoded image lifting.

Tasks:

- audit `FinalToneMapping_ps.*` after real luminance is active.
- evaluate whether this line should be reduced or removed:
  - `( vSample.xyz - 0.5 ) * 1.25 + 0.5 + 0.11`
- re-evaluate `brightThreshold` defaults after real exposure exists.
- confirm that `bloomIntensity` is no longer being used to fake overall brightness.

Expected output:

- brightness, contrast, and bloom behaving as separate controls.

## Phase E: Carbon-Specific Artistic Tuning

Goal:

- once the HDR math is correct, tune it toward Carbon instead of generic SR3 HDR.

Tasks:

- compare SR3 output against Carbon references:
  - `D:\Games\NFSC Redux\FX\MODULES\tonemap_variants.fx`
  - night race footage and canyon footage
  - Carbon sky/fog/headlight mood from validation captures
- tune for:
  - darker mids,
  - strong specular highlights,
  - restrained but visible bloom,
  - warmer street lights,
  - cooler ambient shadows,
  - less rally-style clear daylight separation.

Expected output:

- one stable Carbon night-grade preset for urban tracks,
- one stronger canyon preset if needed.

## MSAA Considerations

The luminance chain must be built from resolved/non-MSAA HDR color inputs.

Rules:

- do not sample multisampled scene color from ad hoc custom shaders unless the resolve path is fully understood and implemented.
- if luminance is built from `rtt_final`, make sure `rtt_final` is already a valid non-MSAA texture for shader sampling.
- if the render backend needs explicit resolve, use the existing proven SR3/Ogre path rather than inventing a custom one without shader support.

This is critical because MSAA corruption was already observed during failed resolve experiments.

## Validation Plan

Validation scenes:

- one night urban track with headlights, reflective paint, and street lights,
- one tunnel or dark enclosed section,
- one brighter open section,
- one canyon-style dusk/night scene.

Validation checks:

- startup with HDR enabled works,
- race loads with HDR already enabled,
- no washed-out baseline at neutral brightness/contrast,
- no immediate over-whitening when brightness/contrast increase moderately,
- bloom responds to highlights rather than lifting the whole frame,
- dark scenes keep readable contrast without turning grey,
- MSAA behavior is no worse than before the luminance work.

## Recommended Execution Order

1. recover/document the original luminance semantics,
2. implement luminance reduction chain,
3. wire real `lumRt` into bright pass and final tone mapping,
4. add temporal adaptation,
5. remove or reduce hardcoded post-tonemap lift,
6. tune Carbon night look.

## Success Criteria

This fix is complete when:

- `hdrLumDummy` is no longer acting as the real luminance source,
- exposure responds to actual scene brightness,
- bloom threshold and intensity are artistic controls instead of exposure hacks,
- neutral brightness/contrast no longer looks washed out,
- Carbon night scenes read as dark, glossy, and aggressive without clipping to white.
