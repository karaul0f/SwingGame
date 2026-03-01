# Unreal Engine C++ Pro Skills

## Core Principles

### UObject & Garbage Collection
- Always use `UPROPERTY()` for UObject* members to ensure proper garbage collection tracking
- Distinguish between `IsValid()` checks and `nullptr` comparisons
- `IsValid()` safely handles pending kill states

### Reflection System
- Leverage `UCLASS()`, `USTRUCT()`, `UENUM()`, and `UFUNCTION()` macros
- Expose types to reflection system and Blueprints
- Prefer `BlueprintReadOnly` over `BlueprintReadWrite` when state shouldn't be modified externally

### Performance Optimization
- Disable ticking by default (`bCanEverTick = false`); enable only when necessary
- Prefer timers via `GetWorldTimerManager()` or event-driven logic
- Avoid `Cast<T>()` in hot loops—cache references during `BeginPlay`
- Use `F` structs for data-heavy, non-UObject types

## Naming Conventions

Follow Epic Games' standard:
- Templates: `T`
- UObject: `U`
- Actors: `A`
- Slate widgets: `S`
- Structs: `F`
- Enums: `E`
- Interfaces: `I`
- Booleans: `b`

## Key Patterns

1. **Component Lookup:** Find components in `PostInitializeComponents()` or `BeginPlay()`, not in `Tick()`
2. **Interfaces:** Use `Implements<>()` checks to decouple systems
3. **Asset Loading:** Use `TSoftClassPtr` or `TSoftObjectPtr` instead of hard references for massive assets

## Debugging Techniques

- Use `UE_LOG` with custom categories
- On-screen debug messages via `GEngine->AddOnScreenDebugMessage()`
- Visual Logger for AI debugging
