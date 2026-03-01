# Design Decisions Log

## 1. Auto-swing instead of manual swing input
**Date:** 2026-02-28
**Decision:** Character swings automatically after grabbing a pole. Player cannot control the swing direction/force — only jump off.
**Implementation:**
- Removed player input routing (`AddSwingInput` is now a no-op)
- Added `AutoSwingForce` parameter (default 3.0) — applies force in the direction of current angular velocity (energy pumping)
- When velocity is near zero, pushes away from equilibrium based on angle sign
- Player retains only Jump to grab/release from pole
**Rationale:** User requested fully automatic swinging with jump-off as the only available action.

## 2. Preserve momentum on swing release
**Date:** 2026-02-28
**Decision:** When the player releases from a pole, the character preserves the actual movement vector from the swing instead of falling straight down.
**Implementation:**
- Added `CachedSwingVelocity` field to `USwingComponent`
- Each frame in `TickSwinging`, velocity is computed as `(NewPos - OldPos) / DeltaTime` — the real positional delta
- `CalcLaunchVelocity` now returns `CachedSwingVelocity * LaunchSpeedMultiplier` instead of an analytical tangent formula
- Old formula had a bug: for negative angular velocity the direction was inverted twice, cancelling out and launching in the wrong direction
**Rationale:** Player should feel the swing momentum carry through on release. Using actual frame velocity is more robust and matches what the player visually sees.

## 3. Fix post-release momentum loss
**Date:** 2026-02-28
**Decision:** Three fixes to make swing release momentum clearly visible:
**Implementation:**
- `BrakingDecelerationFalling` set to 0 on release, restored when character lands — was 1500, which killed horizontal speed in ~0.3s
- `DoJumpStart()` now tracks `bWasSwinging` to prevent `Jump()` call right after release (state was already None, so the old check didn't catch it)
- `LaunchSpeedMultiplier` default raised from 1.1 to 1.8 for a more pronounced launch
**Rationale:** Momentum was technically applied but immediately eaten by air braking deceleration and potentially overridden by Jump(). These fixes ensure the player visibly flies in the swing direction after release.

## 4. [REVERTED] Character rotation around grip point during swing
**Date:** 2026-02-28 | **Reverted:** 2026-03-02
**Decision:** Feature reverted — caused shaking and coordinate jumps during swing. Code restored to pre-grip-point state.

## 5. Replace humanoid character with wolf model
**Date:** 2026-02-28
**Decision:** Replaced the default SK_Mannequin skeleton and mesh with a realistic wolf model (wolf_demo) from Unreal Marketplace.
**Implementation:**
- Downloaded "Realistic_Wolf_3D_Model_2_0_Demo_Free_Download_" from Unreal Marketplace
- Replaced SK_Mannequin with wolf_demo skeletal mesh in BP_ThirdPersonCharacter Blueprint
- Added Fab marketplace content folder containing wolf assets: wolf_demo.uasset, wolf_demo_Skeleton, wolf_demo_Anim, wolf_demo_PhysicsAsset
**Notes:**
- Wolf skeleton likely uses different socket names than humanoid (no "HandGrip_R")
- If `bRotateAroundGripPoint` is enabled, socket name must be updated to a valid wolf socket (or feature should be disabled)
- Wolf animations don't match humanoid AnimBP — may need adjustment or separate animation blueprint
**Rationale:** User requested a wolf model to replace the default humanoid character for thematic variety.

## 6. Slope-following character rotation during walking
**Date:** 2026-03-01
**Decision:** Character tilts/leans to align with ground surface slope AND movement direction when walking on uneven terrain.
**Implementation:**
- Added `TickWalkingSlope(float DeltaTime)` private method to `USwingComponent`
- In `TickComponent`: Call `TickWalkingSlope()` when character is on ground and not swinging
- `TickWalkingSlope` logic:
  - Performs a **line trace** downward 200 units from character to detect ground surface
  - Extracts surface normal from hit result
  - Calculates **base pitch angle** from X component of normal: `atan2(-SurfaceNormal.X, SurfaceNormal.Z)` — character follows terrain contour
    - On downslope: negative pitch (lean forward)
    - On upslope: positive pitch (lean backward)
  - Calculates **roll angle** from Y component of normal: `atan2(SurfaceNormal.Y, SurfaceNormal.Z)` for side-to-side slope tilt
  - **Additional movement-based lean**: When moving, gets horizontal velocity and dot-product with surface normal
    - If moving upslope (positive dot product), add backward lean for balance
    - If moving downslope (negative dot product), add forward lean
    - Movement lean is clamped to ±20° and multiplied by 30° to add to base pitch
  - Creates target rotation: pitch (base + movement) and roll (from surface), **yaw preserved from current character rotation**
  - Uses smooth interpolation (speed 5.0) to gradually rotate from current to target rotation
- Only active when `IsMovingOnGround()` returns true — disabled in air or while swinging
**Rationale:** Walking on slopes with fixed upright orientation looks stiff and unnatural. Combining surface-normal tilt with movement-based lean creates natural leaning that matches both terrain and direction of travel, improving visual feedback and immersion.

## 8. Increase swing intensity (tuned for balance)
**Date:** 2026-03-02
**Decision:** Make swing more dynamic and energetic while avoiding overshooting limits.
**Implementation:**
- `AutoSwingForce`: 3.0 → 3.7 — character pumps more energy into the swing, but not so much it hits the angle limit
- `SwingDamping`: 0.985 → 0.988 — slightly reduced damping, maintains momentum better
- `LaunchSpeedMultiplier`: 1.8 → 2.1 — character launches faster on release
**Rationale:** Initial attempt (4.5 / 0.99 / 2.2) caused character to get stuck at extreme swing angles due to excessive force. Balanced values (3.7 / 0.988 / 2.1) provide more intense feel while keeping the swing oscillating smoothly.

## 7. [REVERTED] Fix swing shaking and coordinate jumps
**Date:** 2026-03-01 | **Reverted:** 2026-03-02
**Decision:** Reverted along with grip point rotation — the fixes were tightly coupled to that feature.
