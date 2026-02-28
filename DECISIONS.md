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

## 4. Character rotation around grip point during swing
**Date:** 2026-02-28
**Decision:** Character visually rotates around the grip socket (hand) during the swing, creating a more dynamic and realistic hanging/swinging animation.
**Implementation:**
- Added `GripSocketName` property (default: "hand_r") to specify which socket acts as the grip point
- Added `bRotateAroundGripPoint` boolean flag (default: false) to enable/disable this behavior
- Added `GripSocketLocalOffset` to store the local-space offset from character root to the grip socket, calculated on grab
- In `OnGrabbed`: When `bRotateAroundGripPoint` is true, calculate grip socket world location from skeletal mesh and store as local offset
- In `TickSwinging`:
  - Calculate target rotation based on swing direction (facing direction of motion)
  - Character leans forward/backward with swing angle (Pitch rotation multiplied by 0.8x for balanced effect)
  - Apply target rotation to grip socket offset to get world-space position
  - **Position character so grip socket is hard-pinned to the pivot (beam grab point):** `NewPos = PivotWorldLocation - RotatedGripOffset`
  - This keeps the grip socket fixed at the beam while the body hangs below and rotates/leans with pendulum motion
- When disabled (default), behavior is 100% identical to original code
**Rationale:** Previous implementation positioned the character's center around the pole, making the grip point feel disconnected. With this feature enabled, the character visually grips the pole with a specific socket, leans dynamically with the swing motion, and rotates naturally around that point as it swings, improving physical realism and visual feedback. Feature is opt-in to avoid breaking existing setups.

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
**Decision:** Character tilts/leans to align with ground surface slope when walking on uneven terrain.
**Implementation:**
- Added `TickWalkingSlope(float DeltaTime)` private method to `USwingComponent`
- In `TickComponent`: Call `TickWalkingSlope()` when character is on ground and not swinging
- `TickWalkingSlope` logic:
  - Performs a **line trace** downward 200 units from character to detect ground surface
  - Extracts surface normal from hit result
  - Calculates **pitch angle** from X component of normal: `atan2(-SurfaceNormal.X, SurfaceNormal.Z)`
  - Calculates **roll angle** from Y component of normal: `atan2(SurfaceNormal.Y, SurfaceNormal.Z)`
  - Creates target rotation: pitch and roll from surface, **yaw preserved from current character rotation**
  - Uses smooth interpolation (speed 5.0) to gradually rotate from current to target rotation
- Only active when `IsMovingOnGround()` returns true — disabled in air or while swinging
**Rationale:** Walking on slopes with fixed upright orientation looks stiff and unnatural. Slope-following creates natural leaning that matches the terrain and improves visual feedback during movement.
