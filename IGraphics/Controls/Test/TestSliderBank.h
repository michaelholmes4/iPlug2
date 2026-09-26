/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestSliderBank
 */

#include "IControl.h"

#include <vector>

/** A bank of labelled horizontal sliders drawn and handled inside a single test control, for tweaking a test's
 * parameters live without attaching child controls (which would outlive the test control when it is removed).
 * Call Draw() from the owning control's Draw() and forward its mouse events.
 *   @ingroup TestControls */
class TestSliderBank
{
public:
  /** Add a slider
   * @param name Label drawn to the left of the slider
   * @param min Minimum value
   * @param max Maximum value
   * @param value Initial value
   * @return The slider's index, for Get()/Set() */
  int Add(const char* name, float min, float max, float value)
  {
    mSliders.push_back({ name, min, max, value });
    return static_cast<int>(mSliders.size()) - 1;
  }

  float Get(int idx) const { return mSliders[idx].value; }
  void Set(int idx, float value) { Slider& s = mSliders[idx]; s.value = Clip(value, s.min, s.max); }

  /** Lay the sliders out in rows within bounds, filling each column top to bottom */
  void SetBounds(const IRECT& bounds, int nColumns = 1)
  {
    mBounds = bounds;
    const int n = static_cast<int>(mSliders.size());
    const int nRows = (n + nColumns - 1) / nColumns;

    for (int i = 0; i < n; i++)
      mSliders[i].bounds = bounds.GetGridCell(i % nRows, i / nRows, nRows, nColumns).GetHPadded(-4.f);
  }

  const IRECT& GetBounds() const { return mBounds; }

  void Draw(IGraphics& g)
  {
    g.FillRect(IColor(220, 30, 30, 34), mBounds);

    const IText labelText = IText(12.f, COLOR_LIGHT_GRAY, nullptr, EAlign::Near);
    const IText valueText = IText(12.f, COLOR_WHITE, nullptr, EAlign::Far);

    for (const Slider& s : mSliders)
    {
      const IRECT track = GetTrack(s);
      const float frac = (s.value - s.min) / (s.max - s.min);

      g.DrawText(labelText, s.name, s.bounds.GetFromLeft(kLabelWidth));
      g.FillRoundRect(IColor(255, 70, 70, 76), track, track.H() * 0.5f);
      g.FillRoundRect(IColor(255, 0, 150, 255), track.GetFromLeft(track.W() * frac), track.H() * 0.5f);
      g.FillCircle(COLOR_WHITE, track.L + track.W() * frac, track.MH(), 5.f);

      WDL_String str;
      str.SetFormatted(32, "%.2f", s.value);
      g.DrawText(valueText, str.Get(), s.bounds.GetFromRight(kValueWidth));
    }
  }

  /** @return true if the mouse down hit a slider, which then tracks subsequent drags until OnMouseUp() */
  bool OnMouseDown(float x, float y)
  {
    mActive = -1;

    for (int i = 0; i < static_cast<int>(mSliders.size()); i++)
    {
      if (mSliders[i].bounds.Contains(x, y))
      {
        mActive = i;
        SetFromX(x);
        return true;
      }
    }

    return false;
  }

  /** @return true if a slider is being dragged */
  bool OnMouseDrag(float x)
  {
    if (mActive < 0)
      return false;

    SetFromX(x);
    return true;
  }

  /** @return true if a slider was being dragged */
  bool OnMouseUp()
  {
    const bool wasActive = mActive >= 0;
    mActive = -1;
    return wasActive;
  }

private:
  static constexpr float kLabelWidth = 76.f;
  static constexpr float kValueWidth = 40.f;

  struct Slider
  {
    const char* name;
    float min, max, value;
    IRECT bounds;
  };

  IRECT GetTrack(const Slider& s) const
  {
    return s.bounds.GetReducedFromLeft(kLabelWidth).GetReducedFromRight(kValueWidth).GetHPadded(-8.f).GetMidVPadded(2.f);
  }

  void SetFromX(float x)
  {
    Slider& s = mSliders[mActive];
    const IRECT track = GetTrack(s);
    const float frac = Clip((x - track.L) / track.W(), 0.f, 1.f);
    s.value = s.min + frac * (s.max - s.min);
  }

  std::vector<Slider> mSliders;
  IRECT mBounds;
  int mActive = -1;
};
