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
