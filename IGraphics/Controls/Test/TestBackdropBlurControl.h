/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestBackdropBlurControl
 */

#include "IControl.h"
#include "TestSliderBank.h"

/** Control to test IGraphics::DrawBackdropBlur(), which snapshots the canvas drawn so far this frame and blurs it.
 * Drag the frosted panel over the busy backdrop - it should stay aligned with what is behind it, with no smearing
 * at its edges. A second, smaller panel is nested inside it, so its backdrop blur captures the first panel's
 * blurred result (stacked blurs within one frame). Use the sliders to change the blur radius, corner radius and panel tint.
 *   @ingroup TestControls */
class TestBackdropBlurControl : public IControl
{
public:
  TestBackdropBlurControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetTooltip("TestBackdropBlurControl");

    // Added in ESlider order, so the enum values are the slider indices
    mSliders.Add("Blur", 1.f, 64.f, 8.f);
    mSliders.Add("Tint", 0.f, 1.f, 0.2f);
    mSliders.Add("Corner", 0.f, 1.f, 0.f);
    OnResize();
  }

  void OnResize() override
  {
    mSliders.SetBounds(mRECT.GetFromBottom(kSliderHeight), 2);
  }

  void Draw(IGraphics& g) override
  {
    DrawBackdrop(g, GetBackdropRect());

    const float blurSize = mSliders.Get(kBlur);
    const int tint = static_cast<int>(mSliders.Get(kTint) * 255.f);
    const float corner = mSliders.Get(kCorner);
    const IRECT panel = GetPanelRect();
    const IRECT inner = panel.GetFromBRHC(panel.W() * 0.5f, panel.H() * 0.5f).GetTranslated(-10.f, -10.f);

    const float panelRadius = corner * std::min(panel.W(), panel.H()) * 0.5f;
    g.DrawBackdropBlur(panel, blurSize, nullptr, panelRadius);
    g.FillRoundRect(IColor(tint, 255, 255, 255), panel, panelRadius);
    g.DrawRoundRect(IColor(160, 255, 255, 255), panel, panelRadius);

    WDL_String str;
    str.SetFormatted(64, "%s\nblur %.1f", g.GetDrawingAPIStr(), blurSize);
    g.DrawMultiLineText(IText(14.f, COLOR_BLACK), str.Get(), panel.GetFromTop(panel.H() * 0.5f));

    // Nested blur: captures the outer panel's blurred output drawn above
    const float innerRadius = corner * std::min(inner.W(), inner.H()) * 0.5f;
    g.DrawBackdropBlur(inner, blurSize, nullptr, innerRadius);
    g.FillRoundRect(IColor(60, 0, 0, 0), inner, innerRadius);
    g.DrawRoundRect(IColor(160, 255, 255, 255), inner, innerRadius);
    g.DrawText(IText(12.f, COLOR_WHITE), "nested", inner);

    mSliders.Draw(g);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mSliders.OnMouseDown(x, y))
      SetDirty(false);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    if (!mSliders.OnMouseDrag(x))
    {
      const IRECT backdrop = GetBackdropRect();
      mPanelX = Clip(mPanelX + dX / backdrop.W(), 0.f, 1.f);
      mPanelY = Clip(mPanelY + dY / backdrop.H(), 0.f, 1.f);
    }

    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mSliders.OnMouseUp();
  }

private:
  enum ESlider { kBlur, kTint, kCorner };

  static constexpr float kSliderHeight = 60.f;

  IRECT GetBackdropRect() const
  {
    return mRECT.GetReducedFromBottom(kSliderHeight);
  }

  IRECT GetPanelRect() const
  {
    const IRECT backdrop = GetBackdropRect();
    const float w = backdrop.W() * 0.5f;
    const float h = backdrop.H() * 0.4f;
    const float l = backdrop.L + mPanelX * (backdrop.W() - w);
    const float t = backdrop.T + mPanelY * (backdrop.H() - h);
    return IRECT(l, t, l + w, t + h);
  }

  // Hard-edged stripes, a checkerboard, dots and text make blur amount and misalignment easy to see
  void DrawBackdrop(IGraphics& g, const IRECT& r)
  {
    g.FillRect(IColor(255, 245, 240, 230), r);

    const float stripe = 12.f;
    int i = 0;
    for (float x = r.L; x < r.R; x += stripe, i++)
    {
      if (i % 2)
        g.FillRect(IColor(255, 30, 30, 40), IRECT(x, r.T, std::min(x + stripe, r.R), r.MH()));
    }

    const float cell = 24.f;
    const IRECT checker = r.GetFromBottom(r.H() * 0.5f);
    int row = 0;
    for (float y = checker.T; y < checker.B; y += cell, row++)
    {
      int col = 0;
      for (float x = checker.L; x < checker.R; x += cell, col++)
      {
        if ((row + col) % 2)
          g.FillRect(IColor(255, 0, 120, 255), IRECT(x, y, std::min(x + cell, checker.R), std::min(y + cell, checker.B)));
      }
    }

    const IColor dots[] = { COLOR_RED, IColor(255, 255, 170, 0), COLOR_GREEN };
    for (int d = 0; d < 3; d++)
      g.FillCircle(dots[d], r.L + r.W() * (d + 0.5f) / 3.f, checker.MH(), checker.H() * 0.2f);

    g.DrawText(IText(22.f, COLOR_BLACK), "Backdrop blur", r.GetFromBottom(30.f));
  }

  TestSliderBank mSliders;
  float mPanelX = 0.5f;
  float mPanelY = 0.4f;
};
