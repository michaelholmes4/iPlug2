/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "base/source/fstring.h"

#include "IPlugParameter.h"

BEGIN_IPLUG_NAMESPACE

/** VST3 parameter helper */
class IPlugVST3Parameter : public Steinberg::Vst::Parameter
{
public:
  IPlugVST3Parameter(IParam* pParam, Steinberg::Vst::ParamID tag, Steinberg::Vst::UnitID unitID)
  : mIPlugParam(pParam)
  {
    Steinberg::UString(info.title, str16BufferSize(Steinberg::Vst::String128)).assign(pParam->GetName());
    Steinberg::UString(info.units, str16BufferSize(Steinberg::Vst::String128)).assign(pParam->GetLabel());

    precision = pParam->GetDisplayPrecision();

    if (pParam->Type() != IParam::kTypeDouble)
      info.stepCount = pParam->GetRange();
    else
      info.stepCount = 0; // continuous

    Steinberg::int32 flags = 0;

    if (pParam->GetCanAutomate()) flags |= Steinberg::Vst::ParameterInfo::kCanAutomate;
    if (pParam->Type() == IParam::kTypeEnum) flags |= Steinberg::Vst::ParameterInfo::kIsList;

    info.defaultNormalizedValue = valueNormalized = pParam->ToNormalized(pParam->GetDefault());
    info.flags = flags;
    info.id = tag;
    info.unitId = unitID;
  }

  void toString(Steinberg::Vst::ParamValue valueNormalized, Steinberg::Vst::String128 string) const override
  {
    WDL_String display;
    mIPlugParam->GetDisplay(valueNormalized, true, display);
    Steinberg::UString(string, 128).fromAscii(display.Get());
  }

  bool fromString(const Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& valueNormalized) const override
  {
    Steinberg::String str((Steinberg::Vst::TChar*) string);
    valueNormalized = mIPlugParam->ToNormalized(mIPlugParam->StringToValue(str.text8()));

    return true;
  }

  Steinberg::Vst::ParamValue toPlain(Steinberg::Vst::ParamValue normValue) const override
  {
    return mIPlugParam->FromNormalized(normValue);
  }

  Steinberg::Vst::ParamValue toNormalized(Steinberg::Vst::ParamValue plainValue) const override
  {
    return mIPlugParam->ToNormalized(plainValue);
  }

  OBJ_METHODS(IPlugVST3Parameter, Parameter)

protected:
  IParam* mIPlugParam = nullptr;
};

/** VST3 preset parameter helper */
class IPlugVST3PresetParameter : public Steinberg::Vst::Parameter
{
public:
  IPlugVST3PresetParameter(int nPresets)
  : Steinberg::Vst::Parameter(STR16("Preset"), kPresetParam, STR16(""), 0, nPresets - 1, Steinberg::Vst::ParameterInfo::kIsProgramChange)
  {}
  
  Steinberg::Vst::ParamValue toPlain(Steinberg::Vst::ParamValue valueNormalized) const override
  {
    return std::round(valueNormalized * info.stepCount);
  }
  
  Steinberg::Vst::ParamValue toNormalized(Steinberg::Vst::ParamValue plainValue) const override
  {
    return plainValue / info.stepCount;
  }
  
  OBJ_METHODS(IPlugVST3PresetParameter, Steinberg::Vst::Parameter)
};

/** VST3 bypass parameter helper */
class IPlugVST3BypassParameter : public Steinberg::Vst::StringListParameter
{
public:
  IPlugVST3BypassParameter()
  : Steinberg::Vst::StringListParameter(STR16("Bypass"), kBypassParam, 0, Steinberg::Vst::ParameterInfo::kCanAutomate | Steinberg::Vst::ParameterInfo::kIsBypass | Steinberg::Vst::ParameterInfo::kIsList)
  {
    appendString(STR16("off"));
    appendString(STR16("on"));
  }

  OBJ_METHODS(IPlugVST3BypassParameter, StringListParameter)
};

/** VST3 gain reduction parameter helper - read-only parameter for reporting GR to host */
class IPlugVST3GainReductionParameter : public Steinberg::Vst::Parameter
{
public:
  IPlugVST3GainReductionParameter()
  : Steinberg::Vst::Parameter(STR16("Gain Reduction"), kGainReductionParam, STR16("dB"), 0, 0, Steinberg::Vst::ParameterInfo::kIsReadOnly)
  {
    precision = 2;
  }

  void toString(Steinberg::Vst::ParamValue valueNormalized, Steinberg::Vst::String128 string) const override
  {
    // Convert normalized value (0.0 to 1.0) to dB (0 to -60 dB)
    double grDB = valueNormalized * -60.0;
    char text[32];
    snprintf(text, 32, "%.2f", grDB);
    Steinberg::UString(string, 128).fromAscii(text);
  }

  Steinberg::Vst::ParamValue toPlain(Steinberg::Vst::ParamValue valueNormalized) const override
  {
    // Convert normalized (0.0-1.0) to dB (0 to -60)
    return valueNormalized * -60.0;
  }

  Steinberg::Vst::ParamValue toNormalized(Steinberg::Vst::ParamValue plainValue) const override
  {
    // Convert dB (0 to -60) to normalized (0.0-1.0)
    return plainValue / -60.0;
  }

  OBJ_METHODS(IPlugVST3GainReductionParameter, Parameter)
};

END_IPLUG_NAMESPACE

