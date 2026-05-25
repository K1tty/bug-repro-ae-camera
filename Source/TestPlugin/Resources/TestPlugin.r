#include "AE_EffectVers.h"
#include "Resources/PluginInfo.h"


resource 'PiPL' (16000)
{
    {
        Kind { AEEffect },

        Name { PLUGIN_NAME },
        Category { PLUGIN_CATEGORY },

        AE_PiPL_Version { 2, 0 },
        AE_Effect_Spec_Version { PF_PLUG_IN_VERSION, PF_PLUG_IN_SUBVERS },
        AE_Effect_Version { PLUGIN_RC_VERSION },

        AE_Effect_Info_Flags { 0 },
        AE_Effect_Global_OutFlags { PLUGIN_RC_GLOBAL_OUTFLAGS },
        AE_Effect_Global_OutFlags_2 { PLUGIN_RC_GLOBAL_OUTFLAGS2 },

        AE_Effect_Match_Name { PLUGIN_MATCH_NAME },
        AE_Reserved_Info { 0 },

        CodeWin64X86 { PLUGIN_ENTRY_POINT }
    }
};
