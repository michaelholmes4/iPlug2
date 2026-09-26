/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Metal-specific BlurLayer implementation using MPSImageGaussianBlur, and the liquid glass lens as a compute kernel.
// Not compiled on its own: like IGraphicsNanoVG.cpp it is #included by IGraphicsMac.mm/IGraphicsIOS.mm,
// so it is built per plug-in format with the same defines as the rest of IGraphics.

#if defined IGRAPHICS_NANOVG && defined IGRAPHICS_METAL

#include "IGraphicsNanoVG.h"
#include "IGraphicsNanoVG_glass.h"
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
#if !__has_feature(objc_arc)
    [blur release];
#endif
  }

  return std::make_unique<ILayer>(pDstBmp, layer->Bounds(), nullptr, IRECT());
}

// Builds the liquid glass kernel from the shader source shared with the GL backend
static NSString* LiquidGlassMSL()
{
  std::string src =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "#define P(i) params[i]\n"
    "#define PIX (float2(gid) + 0.5)\n"
    "#define SAMPLE(pt) src.sample(smp, ((pt) - float2(P(0), P(1))) * P(2) / float2(P(3), P(4)))\n";
  src += kLiquidGlassFunctions;
  src +=
    "kernel void liquidGlass(texture2d<float, access::sample> src [[texture(0)]],\n"
    "                        texture2d<float, access::write> dst [[texture(1)]],\n"
    "                        constant float* params [[buffer(0)]],\n"
    "                        uint2 gid [[thread_position_in_grid]])\n"
    "{\n"
    "  if (gid.x >= dst.get_width() || gid.y >= dst.get_height())\n"
    "    return;\n"
    "  constexpr sampler smp(address::clamp_to_edge, filter::linear);\n";
  src += kLiquidGlassBody;
  src += "  dst.write(result, gid);\n}\n";
  return [NSString stringWithUTF8String:src.c_str()];
}

ILayerPtr IGraphicsNanoVG::_LiquidGlassLayerMetal(const ILayerPtr& layer, const float* params)
{
  const APIBitmap* pSrcBmp = layer->GetAPIBitmap();
  if (!pSrcBmp || mGlassPipelineFailed)
    return nullptr;

  id<MTLDevice> dev = (__bridge id<MTLDevice>) mnvgDevice(mVG);
  id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>) mnvgCommandQueue(mVG);
  if (!dev || !queue)
    return nullptr;

  if (!mGlassPipeline)
  {
    // Compiled from source at runtime, so there is no .metal file for every project to build
    @autoreleasepool
    {
      NSError* error = nil;
      id<MTLLibrary> library = [dev newLibraryWithSource:LiquidGlassMSL() options:nil error:&error];
      id<MTLFunction> function = [library newFunctionWithName:@"liquidGlass"];
      id<MTLComputePipelineState> pipeline = function ? [dev newComputePipelineStateWithFunction:function error:&error] : nil;

      if (!pipeline)
      {
        DBGMSG("Liquid glass shader failed to compile: %s\n", error ? [[error localizedDescription] UTF8String] : "");
        mGlassPipelineFailed = true; // don't retry every frame
      }

#if __has_feature(objc_arc)
      if (pipeline)
        mGlassPipeline = (void*) CFBridgingRetain(pipeline);
#else
      mGlassPipeline = (void*) pipeline; // already +1 from new...
      [function release];
      [library release];
#endif
    }

    if (!mGlassPipeline)
      return nullptr;
  }

  const int w = pSrcBmp->GetWidth();
  const int h = pSrcBmp->GetHeight();

  // CreateAPIBitmap ends NanoVG's frame, committing its pending drawing (including the backdrop) to the queue
  // ahead of the command buffer below, and resumes it afterwards
  APIBitmap* pDstBmp = CreateAPIBitmap(w, h, pSrcBmp->GetScale(), (float) pSrcBmp->GetDrawScale());
  if (!pDstBmp)
    return nullptr;

  @autoreleasepool
  {
    id<MTLTexture> srcTex = (__bridge id<MTLTexture>) mnvgImageHandle(mVG, pSrcBmp->GetBitmap());
    id<MTLTexture> dstTex = (__bridge id<MTLTexture>) mnvgImageHandle(mVG, pDstBmp->GetBitmap());

    if (!srcTex || !dstTex)
    {
      delete pDstBmp;
      return nullptr;
    }

    id<MTLComputePipelineState> pipeline = (__bridge id<MTLComputePipelineState>) mGlassPipeline;
    id<MTLCommandBuffer> cmd = [queue commandBuffer];
    cmd.label = @"IGraphics::DrawBackdropLiquidGlass";
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setComputePipelineState:pipeline];
    [encoder setTexture:srcTex atIndex:0];
    [encoder setTexture:dstTex atIndex:1];
    [encoder setBytes:params length:sizeof(float) * kNumLGParams atIndex:0];
    [encoder dispatchThreadgroups:MTLSizeMake((w + 15) / 16, (h + 15) / 16, 1) threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
    [encoder endEncoding];

    // No wait needed: command buffers on one queue run in order, so NanoVG's later drawing of this
    // layer sees the result
    [cmd commit];
  }

  return std::make_unique<ILayer>(pDstBmp, layer->Bounds(), nullptr, IRECT());
}

void IGraphicsNanoVG::_ReleaseMetalResources()
{
  if (mGlassPipeline)
    CFRelease(mGlassPipeline);

  mGlassPipeline = nullptr;
  mGlassPipelineFailed = false;
}

#endif // IGRAPHICS_NANOVG && IGRAPHICS_METAL
