#include "TransController.h"
#include "TransParams.h"
#include "TransProcessor.h"

#include "suite/Brand.h"

#include "projectversion.h"
#include "public.sdk/source/main/pluginfactory.h"

#define PLUGIN_NAME SUITE_NAME " Transient Shaper"

BEGIN_FACTORY_DEF (SUITE_COMPANY_NAME, SUITE_COMPANY_WEB, SUITE_COMPANY_EMAIL)

DEF_CLASS2 (INLINE_UID_FROM_FUID (trans::kProcessorUID),
            PClassInfo::kManyInstances,
            kVstAudioEffectClass,
            PLUGIN_NAME,
            Vst::kDistributable,
            Vst::PlugType::kFxDynamics,
            FULL_VERSION_STR,
            kVstVersionString,
            trans::TransProcessor::create)

DEF_CLASS2 (INLINE_UID_FROM_FUID (trans::kControllerUID),
            PClassInfo::kManyInstances,
            kVstComponentControllerClass,
            PLUGIN_NAME " Controller",
            0,
            "",
            FULL_VERSION_STR,
            kVstVersionString,
            trans::TransController::create)

END_FACTORY
