/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// DECLARE_CLASS_IID in ipslcontextinfo.h only declares each interface's IID; exactly one
// translation unit has to define it, same as Steinberg's own interfaces do in vstinitiids.cpp.

#include "pluginterfaces/base/funknown.h"
#include "ipslcontextinfo.h"

DEF_CLASS_IID(Presonus::IContextInfoProvider)
DEF_CLASS_IID(Presonus::IContextInfoHandler)
DEF_CLASS_IID(Presonus::IContextInfoHandler2)
