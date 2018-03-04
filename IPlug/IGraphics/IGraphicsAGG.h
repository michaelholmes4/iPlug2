#pragma once

#include <stack>

/*

 AGG 2.4 should be modified to avoid bringing carbon headers on mac, which can cause conflicts

 in "agg_mac_pmap.h" ...
 //#include <ApplicationServices/ApplicationServices.h>
 #include <CoreGraphics/CoreGraphics.h>

 */

#include "IGraphicsPathBase.h"
#include "IGraphicsAGG_src.h"

class AGGBitmap : public APIBitmap
{
public:
  AGGBitmap(agg::pixel_map* pPixMap, int scale) : APIBitmap (pPixMap, pPixMap->width(), pPixMap->height(), scale) {}
  virtual ~AGGBitmap() { delete ((agg::pixel_map*) GetBitmap()); }
};

inline const agg::rgba8 AGGColor(const IColor& color, const IBlend* pBlend = nullptr)
{
  return agg::rgba8(color.R, color.G, color.B, (BlendWeight(pBlend) * color.A));
}

inline agg::comp_op_e AGGBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
    return agg::comp_op_src;

  switch (pBlend->mMethod)
  {
    case kBlendClobber: return agg::comp_op_src_over;
    case kBlendAdd: return agg::comp_op_plus;
    case kBlendColorDodge: return agg::comp_op_color_dodge;
    case kBlendNone:
    default:
      return agg::comp_op_src_over;
  }
}

inline const agg::cover_type AGGCover(const IBlend* pBlend = nullptr)
{
  if (!pBlend)
    return 255;

  return std::max(agg::cover_type(0), std::min(agg::cover_type(roundf(pBlend->mWeight * 255.f)), agg::cover_type(255)));
}

/** IGraphics draw class using Antigrain Geometry
*   @ingroup DrawClasses*/
class IGraphicsAGG : public IGraphicsPathBase
{
public:
  struct LineInfo
  {
    int mStartChar;
    int mEndChar;
    double mWidth;
    LineInfo() : mWidth(0.0), mStartChar(0), mEndChar(0) {}
  };

#ifdef OS_WIN
  typedef agg::order_bgra PixelOrder;
#else
  typedef agg::order_argb PixelOrder;
#endif
  typedef agg::comp_op_adaptor_rgba<agg::rgba8, PixelOrder> BlenderType;
  typedef agg::comp_op_adaptor_rgba_pre<agg::rgba8, PixelOrder> BlenderTypePre;
  typedef agg::pixfmt_custom_blend_rgba<BlenderType, agg::rendering_buffer> PixfmtType;
  typedef agg::pixfmt_custom_blend_rgba<BlenderTypePre, agg::rendering_buffer> PixfmtTypePre;
  typedef agg::renderer_base <PixfmtType> RenbaseType;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> RendererSolid;
  typedef agg::renderer_scanline_bin_solid<RenbaseType> RendererBin;
  typedef agg::font_engine_freetype_int32 FontEngineType;
  typedef agg::font_cache_manager <FontEngineType> FontManagerType;
  typedef agg::span_interpolator_linear<> InterpolatorType;
  typedef agg::span_allocator<agg::rgba8> SpanAllocatorType;
  typedef agg::gradient_lut<agg::color_interpolator<agg::rgba8>, 512> ColorArrayType;
  typedef agg::image_accessor_clip<PixfmtType> imgSourceType;
  typedef agg::span_image_filter_rgba_bilinear_clip <PixfmtType, InterpolatorType> spanGenType;
  typedef agg::renderer_base<agg::pixfmt_gray8> maskRenBase;
  typedef agg::scanline_u8_am<agg::alpha_mask_gray8> scanlineType;
  typedef agg::rasterizer_scanline_aa<> RasterizerType;
  
  IGraphicsAGG(IDelegate& dlg, int w, int h, int fps);
  ~IGraphicsAGG();

  void SetDisplayScale(int scale) override;

  void Draw(const IRECT& rect) override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend) override;
  
  void PathClear() override { mPath.remove_all(); }
  void PathStart() override { mPath.start_new_path(); }
  void PathClose() override { mPath.close_polygon(); }
  
  void PathArc(float cx, float cy, float r, float aMin, float aMax) override;

  void PathMoveTo(float x, float y) override { mPath.move_to(x, y); }
  void PathLineTo(float x, float y) override { mPath.line_to(x, y);}
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { mPath.curve4(x1, y1, x2, y2, x3, y3); }
  
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  void PathStateSave() override { mState.push(mTransform); }
  
  void PathStateRestore() override
  {
    mTransform = mState.top();
    mState.pop();
  }
  
  void PathTransformTranslate(float x, float y) override { mTransform /= agg::trans_affine_translation(x, y); }
  void PathTransformScale(float scale) override { mTransform /= agg::trans_affine_scaling(scale); }
  void PathTransformRotate(float angle) override { mTransform /= agg::trans_affine_rotation(DegToRad(angle)); }

  bool DrawText(const IText& text, const char* str, IRECT& rect, bool measure = false) override;
  bool MeasureText(const IText& text, const char* str, IRECT& destRect) override;

  IColor GetPoint(int x, int y) override;
  void* GetData() override { return 0; } //todo
  const char* GetDrawingAPIStr() override { return "AGG"; }

 // IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* cacheName, int scale) override;
 //  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override;

  void RenderDrawBitmap() override;

private:

  template <typename GradientFuncType>
  void GradientRasterize(RasterizerType& rasterizer, const GradientFuncType& gradientFunc, agg::trans_affine& xform, ColorArrayType& colorArray)
  {
    agg::scanline_p8 scanline;
    SpanAllocatorType spanAllocator;
    InterpolatorType spanInterpolator(xform);

    // Gradient types
    
    typedef agg::span_gradient<agg::rgba8, InterpolatorType, GradientFuncType, ColorArrayType> SpanGradientType;
    typedef agg::renderer_scanline_aa<RenbaseType, SpanAllocatorType, SpanGradientType> RendererGradientType;
    
    // Gradient objects
    
    SpanGradientType spanGradient(spanInterpolator, gradientFunc, colorArray, 0, 512);
    RendererGradientType renderer(mRenBase, spanAllocator, spanGradient);
    
    agg::render_scanlines(rasterizer, scanline, renderer);
  }
  
  template <typename GradientFuncType>
  void GradientRasterizeAdapt(EPatternExtend extend, RasterizerType& rasterizer, const GradientFuncType& gradientFunc, agg::trans_affine& xform, ColorArrayType& colorArray)
  {
    // FIX extend none

    switch (extend)
    {
      case kExtendNone:
      case kExtendPad:
        GradientRasterize(rasterizer, gradientFunc, xform, colorArray);
        break;
      case kExtendReflect:
        GradientRasterize(rasterizer, agg::gradient_reflect_adaptor<GradientFuncType>(gradientFunc), xform, colorArray);
        break;
      case kExtendRepeat:
        GradientRasterize(rasterizer, agg::gradient_repeat_adaptor<GradientFuncType>(gradientFunc), xform, colorArray);
          break;
    }
  }
    
  template <typename PathType>
  void Rasterize(const IPattern& pattern, PathType& path, const IBlend* pBlend = nullptr, EFillRule rule = kFillWinding)
  {
    agg::rasterizer_scanline_aa<> rasterizer;
    rasterizer.reset();
    rasterizer.filling_rule(rule == kFillWinding ? agg::fill_non_zero : agg::fill_even_odd );
    agg::conv_curve<PathType> convertedPath(path);
    rasterizer.add_path(convertedPath);
    
    switch (pattern.mType)
    {
      case kSolidPattern:
      {
        agg::scanline_p8 scanline;
        RendererSolid renderer(mRenBase);
        
        const IColor &color = pattern.GetStop(0).mColor;
        renderer.color(AGGColor(color, pBlend));
        
        // Rasterize

        agg::render_scanlines(rasterizer, scanline, renderer);
      }
        break;
        
      case kLinearPattern:
      case kRadialPattern:
      {
        // Common gradient objects
        
        const float* xform = pattern.mTransform;
        
        agg::trans_affine       gradientMTX(xform[0], xform[1] , xform[2], xform[3], xform[4], xform[5]);
        ColorArrayType          colorArray;
       
        // Scaling
  
        gradientMTX = agg::trans_affine_scaling(1.0 / GetDisplayScale()) * mTransform * gradientMTX * agg::trans_affine_scaling(512.0);

        // Make gradient lut
      
        colorArray.remove_all();

        for (int i = 0; i < pattern.NStops(); i++)
        {
          const IColorStop& stop = pattern.GetStop(i);
          float offset = stop.mOffset;
          colorArray.add_color(offset, AGGColor(stop.mColor, pBlend));
        }
        
        colorArray.build_lut();

        // Rasterize
        
        if (pattern.mType == kLinearPattern)
        {
          GradientRasterizeAdapt(pattern.mExtend, rasterizer, agg::gradient_x(), gradientMTX, colorArray);
        }
        else
        {
          GradientRasterizeAdapt(pattern.mExtend, rasterizer, agg::gradient_radial_d(), gradientMTX, colorArray);
        }
      }
      break;
    }
  }

  RenbaseType mRenBase;
  PixfmtType mPixf;
  FontEngineType mFontEngine;
  FontManagerType mFontManager;
  agg::rendering_buffer mRenBuf;
  agg::path_storage mPath;
  agg::trans_affine mTransform;
  
  // TODO Oli probably wants this to not be STL but there's nothing in WDL for this...
  
  std::stack<agg::trans_affine> mState;
  
#ifdef OS_MAC
  agg::pixel_map_mac mPixelMap;
#else
  //TODO:
#endif
  
  void SetAGGSourcePattern(RendererSolid &renderer, const IPattern& pattern, const IBlend* pBlend);

  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  agg::pixel_map* CreateAPIBitmap(int w, int h);
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int s) override;

  //pipeline to process the vectors glyph paths(curves + contour)
  agg::conv_curve<FontManagerType::path_adaptor_type> mFontCurves;
  agg::conv_contour<agg::conv_curve<FontManagerType::path_adaptor_type> > mFontContour;

  
  void CalculateTextLines(WDL_TypedBuf<LineInfo>* pLines, const IRECT& rect, const char* str, FontManagerType& manager);
  void ToPixel(float & pixel);
};
