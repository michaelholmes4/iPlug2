/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <windows.h>

#include <vector>

#include "IGraphicsPrivate.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

// Fonts

class InstalledWinFont
{
public:
  InstalledWinFont(void* data, int resSize)
  : mFontHandle(nullptr)
  {
    if (data)
    {
      DWORD numFonts = 0;
      mFontHandle = AddFontMemResourceEx(data, resSize, NULL, &numFonts);
    }
  }
  
  ~InstalledWinFont()
  {
    if (IsValid())
      RemoveFontMemResourceEx(mFontHandle);
  }
  
  InstalledWinFont(const InstalledWinFont&) = delete;
  InstalledWinFont& operator=(const InstalledWinFont&) = delete;
    
  bool IsValid() const { return mFontHandle; }
  
private:
  HANDLE mFontHandle;
};

struct HFontHolder
{
  HFontHolder(HFONT hfont) : mHFont(nullptr)
  {
    LOGFONTW lFont = { 0 };
    GetObjectW(hfont, sizeof(LOGFONTW), &lFont);
    mHFont = CreateFontIndirectW(&lFont);
  }
  
  HFONT mHFont;
};

class WinFont : public PlatformFont
{
public:
  WinFont(HFONT font, const char* styleName, bool system)
  : PlatformFont(system), mFont(font), mStyleName(styleName) {}
  ~WinFont()
  {
    DeleteObject(mFont);
  }
  
  FontDescriptor GetDescriptor() override { return mFont; }
  
  IFontDataPtr GetFontData() override
  {
    HDC hdc = CreateCompatibleDC(NULL);
    IFontDataPtr fontData(new IFontData());

    if (hdc != NULL)
    {
      SelectObject(hdc, mFont);

      // If the font belongs to a TrueType Collection the face offsets index into the whole
      // collection, so the buffer must be sized for the 'ttcf' table, not the single face -
      // otherwise GetFontData returns a truncated collection that parses out of bounds
      const DWORD ttcfTag = 0x66637474;
      DWORD table = ttcfTag;
      DWORD size = ::GetFontData(hdc, table, 0, NULL, 0);

      if (size == GDI_ERROR)
      {
        table = 0;
        size = ::GetFontData(hdc, table, 0, NULL, 0);
      }

      if (size != GDI_ERROR && size > 0)
      {
        fontData = std::make_unique<IFontData>((int) size);

        if (fontData->GetSize() == (int) size)
        {
          const DWORD result = ::GetFontData(hdc, table, 0, fontData->Get(), size);
          if (result == size)
            fontData->SetFaceIdx(GetFaceIdx(fontData->Get(), fontData->GetSize(), mStyleName.Get()));
        }
      }

      DeleteDC(hdc);
    }

    return fontData;
  }
    
private:
  HFONT mFont;
  WDL_String mStyleName;
};

/** Font backed by raw font bytes rather than GDI. Used for all bundled/resource fonts so the
 * rendering data always comes from the exact bytes we shipped - GDI maps fonts by face name
 * and can silently substitute another installed font with the same name (e.g. a TrueType
 * Collection registered by the host), so ::GetFontData output cannot be trusted. Also serves
 * as the fallback when GDI font installation fails entirely (e.g. GDI handle exhaustion or
 * font-blocking security policies). The optional HFONT is owned by this object and is used
 * only as the descriptor for platform text entry; without one, text entry falls back to a
 * default GDI font. */
class MemoryWinFont : public PlatformFont
{
public:
  MemoryWinFont(const void* pData, int size, HFONT font = nullptr)
  : PlatformFont(false)
  , mFont(font)
  , mData(reinterpret_cast<const unsigned char*>(pData), reinterpret_cast<const unsigned char*>(pData) + size)
  {}

  ~MemoryWinFont()
  {
    if (mFont)
      DeleteObject(mFont);
  }

  FontDescriptor GetDescriptor() override { return mFont; }

  IFontDataPtr GetFontData() override
  {
    const int faceIdx = GetFaceIdx(mData.data(), static_cast<int>(mData.size()), "");
    return std::make_unique<IFontData>(mData.data(), static_cast<int>(mData.size()), faceIdx);
  }

private:
  HFONT mFont;
  std::vector<unsigned char> mData;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
