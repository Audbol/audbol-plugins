#include "SatController.h"
#include "SatParams.h"
#include "SatProcessor.h"

#include "suite/Brand.h"

#include "projectversion.h"
#include "public.sdk/source/main/pluginfactory.h"

#define PLUGIN_NAME SUITE_NAME " Saturation"

BEGIN_FACTORY_DEF (SUITE_COMPANY_NAME, SUITE_COMPANY_WEB, SUITE_COMPANY_EMAIL)

DEF_CLASS2 (INLINE_UID_FROM_FUID (sat::kProcessorUID),
            PClassInfo::kManyInstances,
            kVstAudioEffectClass,
            PLUGIN_NAME,
            Vst::kDistributable,
            Vst::PlugType::kFxDistortion,
            FULL_VERSION_STR,
            kVstVersionString,
            sat::SatProcessor::create)

DEF_CLASS2 (INLINE_UID_FROM_FUID (sat::kControllerUID),
            PClassInfo::kManyInstances,
            kVstComponentControllerClass,
            PLUGIN_NAME " Controller",
            0,
            "",
            FULL_VERSION_STR,
            kVstVersionString,
            sat::SatController::create)

END_FACTORY
