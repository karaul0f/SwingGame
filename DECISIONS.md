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
