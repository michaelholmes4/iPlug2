/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestBlurLayerControl
 */

#include "IControl.h"

/** Control to test IGraphics::BlurLayer(). A cached layer is drawn on the left and a blurred copy of it on the right.
 * The blurred copy should sit exactly over the same content, with the same bounds. Drag or use the value
 * slider to change the blur radius.
 *   @ingroup TestControls */
class TestBlurLayerControl : public IKnobControlBase
{
public:
  TestBlurLayerControl(const IRECT& bounds, int paramIdx)
  : IKnobControlBase(bounds, paramIdx)
  {
    SetTooltip("TestBlurLayerControl");
  }

  void Draw(IGraphics& g) override
  {
    const IRECT top = mRECT.GetFromTop(mRECT.H() - 30.f);
    const IRECT left = top.GetGridCell(0, 1, 2).GetPadded(-10.f);
    const IRECT right = top.GetGridCell(1, 1, 2).GetPadded(-10.f);
    const float blurSize = 1.f + static_cast<float>(GetValue()) * (kMaxBlur - 1.f);

    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, left);
      DrawContent(g, left);
      mLayer = g.EndLayer();
      mBlurredLayer = nullptr;
    }

    if (!mBlurredLayer || blurSize != mBlurredSize)
    {
      mBlurredLayer = g.BlurLayer(mLayer, blurSize);
      mBlurredSize = blurSize;
    }

    g.DrawLayer(mLayer);
    g.DrawFittedLayer(mBlurredLayer, right, nullptr);
    g.DrawRect(COLOR_BLACK, left);
    g.DrawRect(COLOR_BLACK, right);

    WDL_String str;
    str.SetFormatted(64, "%s - BlurLayer %.1f", g.GetDrawingAPIStr(), blurSize);
    g.DrawText(IText(16.f, COLOR_BLACK), str.Get(), mRECT.GetFromBottom(30.f));
  }

private:
  static constexpr float kMaxBlur = 40.f;

  void DrawContent(IGraphics& g, const IRECT& r)
  {
    g.FillRect(COLOR_WHITE, r);
    g.FillCircle(COLOR_RED, r.MW(), r.MH(), r.W() * 0.3f);
    g.FillRect(COLOR_BLUE, r.GetCentredInside(r.W() * 0.15f, r.H() * 0.8f));
    g.DrawLine(COLOR_BLACK, r.L, r.T, r.R, r.B, nullptr, 3.f);
    g.DrawText(IText(28.f, COLOR_BLACK), "Blur", r.GetFromTop(40.f));
  }

  ILayerPtr mLayer;
  ILayerPtr mBlurredLayer;
  float mBlurredSize = -1.f;
};
