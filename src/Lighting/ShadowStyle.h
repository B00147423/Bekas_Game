#pragma once
#include <raylib.h>

namespace Lighting {
enum class ShadowMode { Occlusion, Footprint, ContactOnly, None };

// World-pixel base dimensions, independent of sprite alpha and collision geometry.
// Offset is from sprite bottom-center to base center. Occlusion ignores this base.
struct ShadowStyle {
    ShadowMode mode = ShadowMode::ContactOnly;
    Vector2 baseSize{12, 6};
    Vector2 baseOffset{0, -3};
    float maxProjection = 24;
    float nearFadeDistance = 24; // Distance beyond base radius until projection is full.
    float projectionOpacity = 0.45f;
    float contactOpacity = 0.65f;
};
inline Rectangle ShadowBase(Rectangle sprite, const ShadowStyle& style) {
    return {sprite.x + sprite.width*0.5f + style.baseOffset.x - style.baseSize.x*0.5f,
            sprite.y + sprite.height + style.baseOffset.y - style.baseSize.y*0.5f,
            style.baseSize.x, style.baseSize.y};
}
}
