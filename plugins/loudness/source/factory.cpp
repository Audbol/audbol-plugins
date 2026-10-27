#include "LoudController.h"
#include "LoudParams.h"
#include "LoudProcessor.h"

#include "suite/Brand.h"

#include "projectversion.h"
#include "public.sdk/source/main/pluginfactory.h"

#define PLUGIN_NAME SUITE_NAME " Loudness"

BEGIN_FACTORY_DEF (SUITE_COMPANY_NAME, SUITE_COMPANY_WEB, SUITE_COMPANY_EMAIL)

DEF_CLASS2 (INLINE_UID_FROM_FUID (loud::kProcessorUID),
            PClassInfo::kManyInstances,
            kVstAudioEffectClass,
            PLUGIN_NAME,
            Vst::kDistributable,
            Vst::PlugType::kFxAnalyzer,
            FULL_VERSION_STR,
            kVstVersionString,
            loud::LoudProcessor::create)

DEF_CLASS2 (INLINE_UID_FROM_FUID (loud::kControllerUID),
            PClassInfo::kManyInstances,
            kVstComponentControllerClass,
            PLUGIN_NAME " Controller",
            0,
            "",
            FULL_VERSION_STR,
            kVstVersionString,
            loud::LoudController::create)

END_FACTORY
