/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Shader source for the NanoVG liquid glass lens, shared by the GL and Metal backends.
 *
 * This is a port of the Skia backend's SkSL lens (see kLiquidGlassSkSL in IGraphicsSkia.cpp) - keep the two in step.
 * The body is written once using float2/float3/float4, which the GLSL prelude #defines to vec2/vec3/vec4, and a few
 * macros each backend's prelude provides:
 *   P(i)       - parameter i, see ELiquidGlassParam
 *   SAMPLE(pt) - sample the backdrop at a point in graphics context coordinates (points)
 *   PIX        - the pixel being shaded, relative to the top-left of the output, in pixels (with the +0.5 centre)
 * The body leaves its premultiplied result, already masked to the glass shape, in `result`.
 */

/** Indices of the float parameters passed to the lens shader. All scalars, so the array has the same layout in C++,
 * GLSL (a uniform float array) and MSL (a constant float buffer) */
enum ELiquidGlassParam
{
  kLGOriginX, kLGOriginY,   // graphics context coordinates of the output's top-left
  kLGScale,                 // pixels per point
  kLGTexW, kLGTexH,         // backdrop / output size in pixels
  kLGCenterX, kLGCenterY,
  kLGHalfW, kLGHalfH,
  kLGRadius,
  kLGNormalRadius,
  kLGDepth,
  kLGRefraction,
  kLGDispersion,
  kLGLightX, kLGLightY,
  kLGLightIntensity,
  kLGSaturation,
  kLGBrightness,
  kLGTintR, kLGTintG, kLGTintB, kLGTintA, // premultiplied
  kNumLGParams
};

static_assert(kNumLGParams == 23, "Update the parameter unpacking at the top of kLiquidGlassBody to match");

static const char kLiquidGlassFunctions[] = R"(
float sdRoundBox(float2 p, float2 b, float r)
{
  float2 q = abs(p) - b + r;
  return length(max(q, float2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

// Outward normal of a rounded box. Taken from a box whose radius is at least the bezel depth, so normals
// turn smoothly around corners tighter than the bezel instead of creasing along the diagonal.
float2 roundBoxNormal(float2 p, float2 b, float r)
{
  float2 q = abs(p) - b + r;
  float2 n = (q.x > 0.0 && q.y > 0.0) ? normalize(q) : (q.x > q.y ? float2(1.0, 0.0) : float2(0.0, 1.0));
  return n * float2(p.x < 0.0 ? -1.0 : 1.0, p.y < 0.0 ? -1.0 : 1.0);
}
)";

static const char kLiquidGlassBody[] = R"(
  // Unpack the parameters, in ELiquidGlassParam order
  float2 origin = float2(P(0), P(1));
  float scale = P(2);
  float2 center = float2(P(5), P(6));
  float2 halfSize = float2(P(7), P(8));
  float radius = P(9);
  float normalRadius = P(10);
  float depth = P(11);
  float refraction = P(12);
  float dispersion = P(13);
  float2 lightDir = float2(P(14), P(15));
  float lightIntensity = P(16);
  float saturation = P(17);
  float brightness = P(18);
  float4 tint = float4(P(19), P(20), P(21), P(22));

  float2 coord = origin + PIX / scale;
  float2 p = coord - center;
  float d = sdRoundBox(p, halfSize, radius);
  float2 n = roundBoxNormal(p, halfSize, normalRadius);

  // x runs 0 at the edge to 1 where the bezel meets the flat top. The bezel is a convex squircle,
  // height = (1 - (1 - x)^4)^(1/4); its slope sets the angle of incidence for a vertical view ray.
  float x = clamp(-d / depth, 0.0, 1.0);
  float u = 1.0 - x;
  float slope = u * u * u * pow(max(1.0 - u * u * u * u, 1e-4), -0.75);
  float theta1 = atan(slope);
  float theta2 = asin(sin(theta1) / 1.5);
  float2 offset = -n * (refraction * depth * tan(theta1 - theta2));

  float4 col = SAMPLE(coord + offset);

  if (dispersion > 0.0)
  {
    // Blue bends more than red
    float k = dispersion * 0.3;
    col.r = SAMPLE(coord + offset * (1.0 - k)).r;
    col.b = SAMPLE(coord + offset * (1.0 + k)).b;
  }

  // Vibrancy: glass makes what's behind it more saturated and a touch brighter
  float luma = dot(col.rgb, float3(0.2126, 0.7152, 0.0722));
  col.rgb = clamp(mix(float3(luma), col.rgb, float3(saturation)) + float3(brightness * col.a), float3(0.0), float3(col.a));

  col = tint + col * (1.0 - tint.a);

  // Edge light: a faint Fresnel glow all the way round, plus a thin specular line gathered onto the parts of
  // the edge square-on to the light, with a weaker internal reflection on the opposite side.
  float facing = dot(n, lightDir);
  float fresnel = exp(d / 4.0) * 0.12;
  float rim = 1.0 - smoothstep(0.0, 0.8, -d);
  float sheen = 1.0 - smoothstep(0.0, 4.0, -d);
  float spec = pow(max(facing, 0.0), 3.0) + 0.5 * pow(max(-facing, 0.0), 3.0);
  float lit = clamp(lightIntensity * (fresnel + rim * (0.1 + spec) + 0.2 * sheen * spec), 0.0, 1.0);

  // Anti-aliased coverage of the glass shape, one pixel wide. Everything outside it is transparent.
  float coverage = clamp(0.5 - d * scale, 0.0, 1.0);
  float4 result = (col + float4(lit) * (1.0 - col)) * coverage;
)";
