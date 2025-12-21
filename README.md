# Light-Based Awareness System for AI Detection (Unreal Engine)

> A light-aware AI detection system for stealth gameplay, implemented in Unreal Engine using C++ and Blueprints.

---

## Overview

Traditional game AI often relies on simple vision cones or distance checks to determine whether a player has been detected. While cheap to implement, these approaches ignore one of the most important factors in stealth gameplay: **lighting**.

This project introduces a **Light-Based Awareness System** that samples real-time illumination around characters and feeds the result into AI detection logic. Instead of binary visibility checks, enemies gradually accumulate awareness based on how well-lit the player is, creating more intuitive and believable stealth behavior.

The system is fully implemented in **Unreal Engine 5**, designed to be reusable, configurable, and designer-friendly.

---

## Design Goals

- Make stealth gameplay feel **fair and intuitive**
- React naturally to **lighting conditions** (bright vs dark areas)
- Work with **existing Unreal PointLight and SpotLight actors**
- Avoid engine modifications or custom shaders
- Provide **configurable tuning parameters**
- Offer strong **debugging and visualization tools**

---

## System Architecture

The Light-Based Awareness System is implemented as a reusable C++ component that can be attached to AI characters.

Each update cycle performs the following steps:

1. Collect nearby `PointLight` and `SpotLight` actors within a scan radius
2. Compute raw light contribution based on intensity and distance
3. Apply attenuation, normalization, and clamping
4. Aggregate results into a single `VisibilityFactor` in `[0, 1]`
5. Feed `VisibilityFactor` into AI detection logic

All tuning values are stored in a shared **configuration data asset**, allowing designers to create different detection profiles per mission or enemy type.

---

## 🔬 Light Sampling Pipeline

For each AI agent, the system performs a sphere scan centered on the character’s position and evaluates nearby light sources.

### Contribution Formula

For each light:

This ensures:
- Distant lights contribute minimally
- Bright, nearby lights dominate visibility

The raw value is then normalized and clamped to avoid extreme spikes caused by very strong lights or short distances.

### Supported Light Types
- ✅ Point Lights
- ✅ Spot Lights (with angular falloff)
- ✅ Occlusion checks via line traces

---

## ⚙️ Configuration & Tuning

All scalar parameters are stored in a `LightAwarenessConfig` data asset:

- Detection range
- Light normalization scale
- Distance falloff coefficient
- Occlusion penalty
- Spot light smoothing
- Sampling interval
- Detection gain & decay rates

This makes tuning fast and safe, without touching code.

### Example Presets

| Preset        | Contribution Scale | Max Contribution | Notes |
|--------------|-------------------|------------------|------|
| Default      | 1.0               | 1.0              | Balanced gameplay |
| NightMission | 0.6               | 0.8              | Strong shadow cover |
| HorrorMode   | 1.5               | 1.0              | Aggressive detection |

---

## 👁️ AI Detection Model

Visibility is not treated as a binary state. Instead, AI maintains a continuous **DetectionLevel** in `[0, 100]`.

Each update:
- Detection increases based on **proximity** and **light visibility**
- Detection decays over time if visibility is low
- Full detection triggers behavior tree transitions

### Detection States
- **Idle** – No awareness
- **Suspicious** – Investigating
- **Alerted** – Fully detected, chase/combat

Lighting primarily affects how quickly the AI moves between these states.

---

## 🤖 Integration with Unreal AI

- Detection values are written to **Blackboard keys**
- Behavior Tree tasks read `LightVisibility` and `IsDetected`
- Works alongside Unreal’s built-in sight & hearing systems
- Flashlight implemented as a `SpotLight` instantly affects detection

This modular approach allows easy integration into existing AI setups.

---

## 🧩 Implementation Highlights

### Core Light Sampling (C++)

```cpp
float ALightAwarenessDemoCharacter::CalculateLightLevel()
{
    if (!Config) return 0.f;

    float Sum = 0.f;
    int32 Count = 0;

    // Sample PointLights and SpotLights
    // Apply distance falloff, angular attenuation, and occlusion

    float Avg = (Count > 0 ? Sum / Count : 0.f);
    return FMath::Clamp(Avg, 0.f, 1.f);
}

float DetectionChance =
    0.6f * LightLvl +
    0.4f * Proximity;

DetectionLevel +=
    DetectionChance * Config->GainScale * Config->SampleInterval;

```

