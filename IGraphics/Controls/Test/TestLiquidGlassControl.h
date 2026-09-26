/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestLiquidGlassControl
 */

#include "IControl.h"
#include "TestSliderBank.h"

/** Control to test IGraphics::DrawBackdropLiquidGlass(). Drag the glass over the busy backdrop and tweak every
 * ILiquidGlass parameter with the sliders; click the glass without dragging to load the next preset
 * (clear, frosted, tinted).
 *   @ingroup TestControls */
class TestLiquidGlassControl : public IControl
{
public:
  TestLiquidGlassControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetTooltip("TestLiquidGlassControl");

    // Added in ESlider order, so the enum values are the slider indices
    const ILiquidGlass defaults;
    mSliders.Add("Refraction", 0.f, 2.f, defaults.mRefraction);
    mSliders.Add("Depth", 0.f, 48.f, defaults.mDepth);
    mSliders.Add("Dispersion", 0.f, 1.f, defaults.mDispersion);
    mSliders.Add("Frost", 0.f, 32.f, defaults.mFrost);
    mSliders.Add("Corner", 0.f, 1.f, 1.f);
    mSliders.Add("Light angle", -180.f, 180.f, defaults.mLightAngle);
    mSliders.Add("Light", 0.f, 1.f, defaults.mLightIntensity);
    mSliders.Add("Saturation", 0.f, 2.f, defaults.mSaturation);
    mSliders.Add("Brightness", -1.f, 1.f, defaults.mBrightness);
    mSliders.Add("Tint", 0.f, 1.f, 0.f);

    LoadPreset(0);
    OnResize();
  }

  void OnResize() override
  {
    mSliders.SetBounds(mRECT.GetFromBottom(kSliderHeight), 2);
  }

  void Draw(IGraphics& g) override
  {
    DrawBackdrop(g, GetBackdropRect());

    const IRECT glassRect = GetGlassRect();
    ILiquidGlass glass(mSliders.Get(kRefraction), mSliders.Get(kDepth), mSliders.Get(kDispersion), mSliders.Get(kFrost),
                       mSliders.Get(kLightAngle), mSliders.Get(kLightIntensity),
                       IColor(static_cast<int>(mSliders.Get(kTint) * 255.f), 255, 255, 255));
    glass.mSaturation = mSliders.Get(kSaturation);
    glass.mBrightness = mSliders.Get(kBrightness);

    g.DrawBackdropLiquidGlass(glassRect, glassRect.H() * 0.5f * mSliders.Get(kCorner), glass);
    g.DrawText(IText(14.f, COLOR_WHITE), g.GetDrawingAPIStr(), glassRect);

    mSliders.Draw(g);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mDragged = false;
    mDraggingGlass = false;

    if (mSliders.OnMouseDown(x, y))
      SetDirty(false);
    else
      mDraggingGlass = GetGlassRect().Contains(x, y);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    if (mSliders.OnMouseDrag(x))
    {
      SetDirty(false);
      return;
    }

    if (!mDraggingGlass)
      return;

    const IRECT backdrop = GetBackdropRect();
    mDragged = true;
    mGlassX = Clip(mGlassX + dX / backdrop.W(), 0.f, 1.f);
    mGlassY = Clip(mGlassY + dY / backdrop.H(), 0.f, 1.f);
    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (mSliders.OnMouseUp())
      return;

    if (mDraggingGlass && !mDragged)
    {
      LoadPreset((mPreset + 1) % kNumPresets);
      SetDirty(false);
    }
  }

private:
  enum ESlider { kRefraction, kDepth, kDispersion, kFrost, kCorner, kLightAngle, kLightIntensity, kSaturation, kBrightness, kTint };

  static constexpr float kSliderHeight = 150.f;
  static constexpr int kNumPresets = 3;

  void LoadPreset(int idx)
  {
    static const ILiquidGlass kPresets[kNumPresets] = {
      ILiquidGlass(0.8f, 16.f, 0.4f, 0.f, -45.f, 0.7f),
      ILiquidGlass(0.8f, 16.f, 0.4f, 6.f, -45.f, 0.7f),
      ILiquidGlass(0.9f, 24.f, 0.5f, 3.f, -45.f, 0.8f, IColor(40, 255, 255, 255)),
    };

    const ILiquidGlass& p = kPresets[idx];
    mSliders.Set(kRefraction, p.mRefraction);
    mSliders.Set(kDepth, p.mDepth);
    mSliders.Set(kDispersion, p.mDispersion);
    mSliders.Set(kFrost, p.mFrost);
    mSliders.Set(kCorner, 1.f);
    mSliders.Set(kLightAngle, p.mLightAngle);
    mSliders.Set(kLightIntensity, p.mLightIntensity);
    mSliders.Set(kSaturation, p.mSaturation);
    mSliders.Set(kBrightness, p.mBrightness);
    mSliders.Set(kTint, p.mTint.A / 255.f);
    mPreset = idx;
  }

  IRECT GetBackdropRect() const
  {
    return mRECT.GetReducedFromBottom(kSliderHeight);
  }

  IRECT GetGlassRect() const
  {
    const IRECT backdrop = GetBackdropRect();
    const float w = std::min(backdrop.W() * 0.7f, 220.f);
    const float h = std::min(backdrop.H() * 0.4f, 72.f);
    const float cx = backdrop.L + w * 0.5f + mGlassX * (backdrop.W() - w);
    const float cy = backdrop.T + h * 0.5f + mGlassY * (backdrop.H() - h);
    return IRECT(cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f);
  }

  // High-contrast stripes, dots and text so refraction and dispersion are easy to see
  void DrawBackdrop(IGraphics& g, const IRECT& r)
  {
    g.FillRect(IColor(255, 245, 240, 230), r);

    const float stripe = 12.f;
    int i = 0;
    for (float x = r.L; x < r.R; x += stripe, i++)
    {
      if (i % 2)
        g.FillRect(IColor(255, 30, 30, 40), IRECT(x, r.T, std::min(x + stripe * 0.5f, r.R), r.MH()));
    }

    const IColor dots[] = { COLOR_RED, IColor(255, 255, 170, 0), COLOR_GREEN, IColor(255, 0, 120, 255), IColor(255, 190, 0, 255) };
    for (int d = 0; d < 5; d++)
    {
      const float cx = r.L + r.W() * (d + 0.5f) / 5.f;
      g.FillCircle(dots[d], cx, r.T + r.H() * 0.72f, r.H() * 0.12f);
    }

    g.DrawText(IText(22.f, COLOR_BLACK), "Liquid glass", r.GetFromBottom(30.f));
  }

  TestSliderBank mSliders;
  float mGlassX = 0.5f;
  float mGlassY = 0.35f;
  int mPreset = 0;
  bool mDragged = false;
  bool mDraggingGlass = false;
};
