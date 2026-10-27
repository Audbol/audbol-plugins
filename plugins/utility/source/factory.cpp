#include "UtlController.h"
#include "UtlParams.h"
#include "UtlProcessor.h"

#include "suite/Brand.h"

#include "projectversion.h"
#include "public.sdk/source/main/pluginfactory.h"

#define PLUGIN_NAME SUITE_NAME " Utility"

BEGIN_FACTORY_DEF (SUITE_COMPANY_NAME, SUITE_COMPANY_WEB, SUITE_COMPANY_EMAIL)

DEF_CLASS2 (INLINE_UID_FROM_FUID (utl::kProcessorUID),
            PClassInfo::kManyInstances,
            kVstAudioEffectClass,
            PLUGIN_NAME,
            Vst::kDistributable,
            Vst::PlugType::kFxTools,
            FULL_VERSION_STR,
            kVstVersionString,
            utl::UtlProcessor::create)

DEF_CLASS2 (INLINE_UID_FROM_FUID (utl::kControllerUID),
            PClassInfo::kManyInstances,
            kVstComponentControllerClass,
            PLUGIN_NAME " Controller",
            0,
            "",
            FULL_VERSION_STR,
            kVstVersionString,
            utl::UtlController::create)

END_FACTORY
