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

/** Control to test IGraphics::DrawBackdropLiquidGlass(). Drag the glass over the busy backdrop;
 * click without dragging to cycle between clear, frosted and tinted glass.
 *   @ingroup TestControls */
class TestLiquidGlassControl : public IControl
{
public:
  TestLiquidGlassControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetTooltip("TestLiquidGlassControl");
  }

  void Draw(IGraphics& g) override
  {
    DrawBackdrop(g);

    const IRECT glassRect = GetGlassRect();
    static const ILiquidGlass kPresets[] = {
      ILiquidGlass(0.8f, 16.f, 0.4f, 0.f, -45.f, 0.7f),
      ILiquidGlass(0.8f, 16.f, 0.4f, 6.f, -45.f, 0.7f),
      ILiquidGlass(0.9f, 24.f, 0.5f, 3.f, -45.f, 0.8f, IColor(40, 255, 255, 255)),
    };

    g.DrawBackdropLiquidGlass(glassRect, glassRect.H() * 0.5f, kPresets[mPreset]);
    g.DrawText(IText(14.f, COLOR_WHITE), g.GetDrawingAPIStr(), glassRect);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mDragged = false;
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    mDragged = true;
    mGlassX = Clip(mGlassX + dX / mRECT.W(), 0.f, 1.f);
    mGlassY = Clip(mGlassY + dY / mRECT.H(), 0.f, 1.f);
    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (!mDragged)
    {
      mPreset = (mPreset + 1) % 3;
      SetDirty(false);
    }
  }

private:
  IRECT GetGlassRect() const
  {
    const float w = std::min(mRECT.W() * 0.7f, 220.f);
    const float h = std::min(mRECT.H() * 0.4f, 72.f);
    const float cx = mRECT.L + w * 0.5f + mGlassX * (mRECT.W() - w);
    const float cy = mRECT.T + h * 0.5f + mGlassY * (mRECT.H() - h);
    return IRECT(cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f);
  }

  // High-contrast stripes, dots and text so refraction and dispersion are easy to see
  void DrawBackdrop(IGraphics& g)
  {
    g.FillRect(IColor(255, 245, 240, 230), mRECT);

    const float stripe = 12.f;
    int i = 0;
    for (float x = mRECT.L; x < mRECT.R; x += stripe, i++)
    {
      if (i % 2)
        g.FillRect(IColor(255, 30, 30, 40), IRECT(x, mRECT.T, std::min(x + stripe * 0.5f, mRECT.R), mRECT.MH()));
    }

    const IColor dots[] = { COLOR_RED, IColor(255, 255, 170, 0), COLOR_GREEN, IColor(255, 0, 120, 255), IColor(255, 190, 0, 255) };
    for (int d = 0; d < 5; d++)
    {
      const float cx = mRECT.L + mRECT.W() * (d + 0.5f) / 5.f;
      g.FillCircle(dots[d], cx, mRECT.T + mRECT.H() * 0.72f, mRECT.H() * 0.12f);
    }

    g.DrawText(IText(22.f, COLOR_BLACK), "Liquid glass", mRECT.GetFromBottom(30.f));
  }

  float mGlassX = 0.5f;
  float mGlassY = 0.35f;
  int mPreset = 0;
  bool mDragged = false;
};
