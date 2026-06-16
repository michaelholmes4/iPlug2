/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Metal-specific BlurLayer implementation using MPSImageGaussianBlur.
// Compiled only when building with IGRAPHICS_NANOVG + IGRAPHICS_METAL.

#if defined IGRAPHICS_NANOVG && defined IGRAPHICS_METAL

#include "IGraphicsNanoVG.h"
#include "nanovg_mtl.h"
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

using namespace iplug;
using namespace igraphics;

ILayerPtr IGraphicsNanoVG::_BlurLayerMetal(const ILayerPtr& layer, float blurSize)
{
  const APIBitmap* pSrcBmp = layer->GetAPIBitmap();
  if (!pSrcBmp)
    return IGraphics::BlurLayer(layer, blurSize);

  const int w             = pSrcBmp->GetWidth();
  const int h             = pSrcBmp->GetHeight();
  const float scale       = pSrcBmp->GetScale();
  const float drawScale   = (float)pSrcBmp->GetDrawScale();
  const int   srcImageID  = pSrcBmp->GetBitmap(); // NVG image ID

  // CreateAPIBitmap handles nvgEndFrame/nvgBeginFrame internally (guarded by mInDraw),
  // so we don't need an explicit nvgEndFrame here.
  APIBitmap* pDstBmp = CreateAPIBitmap(w, h, scale, drawScale);
  if (!pDstBmp)
    return IGraphics::BlurLayer(layer, blurSize);

  @autoreleasepool
  {
    id<MTLDevice>       dev    = (__bridge id<MTLDevice>)      mnvgDevice(mVG);
    id<MTLCommandQueue> queue  = (__bridge id<MTLCommandQueue>)mnvgCommandQueue(mVG);
    id<MTLTexture>      srcTex = (__bridge id<MTLTexture>)     mnvgImageHandle(mVG, srcImageID);
    id<MTLTexture>      dstTex = (__bridge id<MTLTexture>)     mnvgImageHandle(mVG, pDstBmp->GetBitmap());

    if (!dev || !queue || !srcTex || !dstTex)
    {
      delete pDstBmp;
      return IGraphics::BlurLayer(layer, blurSize);
    }

    const float sigma = blurSize * GetBackingPixelScale() * 0.5f;
    MPSImageGaussianBlur* blur = [[MPSImageGaussianBlur alloc] initWithDevice:dev sigma:sigma];
    blur.edgeMode = MPSImageEdgeModeClamp;

    id<MTLCommandBuffer> cmd = [queue commandBuffer];
    cmd.label = @"IGraphics::BlurLayer";
    [blur encodeToCommandBuffer:cmd sourceTexture:srcTex destinationTexture:dstTex];
    [cmd commit];
    [cmd waitUntilCompleted];
  }

  return std::make_unique<ILayer>(pDstBmp, layer->Bounds(), nullptr, IRECT());
}

#endif // IGRAPHICS_NANOVG && IGRAPHICS_METAL
