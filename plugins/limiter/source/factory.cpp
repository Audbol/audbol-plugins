#include "LimController.h"
#include "LimParams.h"
#include "LimProcessor.h"

#include "suite/Brand.h"

#include "projectversion.h"
#include "public.sdk/source/main/pluginfactory.h"

#define PLUGIN_NAME SUITE_NAME " Limiter"

BEGIN_FACTORY_DEF (SUITE_COMPANY_NAME, SUITE_COMPANY_WEB, SUITE_COMPANY_EMAIL)

DEF_CLASS2 (INLINE_UID_FROM_FUID (lim::kProcessorUID),
            PClassInfo::kManyInstances,
            kVstAudioEffectClass,
            PLUGIN_NAME,
            Vst::kDistributable,
            Vst::PlugType::kFxDynamics,
            FULL_VERSION_STR,
            kVstVersionString,
            lim::LimProcessor::create)

DEF_CLASS2 (INLINE_UID_FROM_FUID (lim::kControllerUID),
            PClassInfo::kManyInstances,
            kVstComponentControllerClass,
            PLUGIN_NAME " Controller",
            0,
            "",
            FULL_VERSION_STR,
            kVstVersionString,
            lim::LimController::create)

END_FACTORY
