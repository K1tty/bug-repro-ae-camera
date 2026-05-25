#pragma once

#define PLUGIN_NAME         "TestPlugin"
#define PLUGIN_MATCH_NAME   "KITTY TestPlugin"
#define PLUGIN_CATEGORY     "Repro Samples"
#define PLUGIN_ENTRY_POINT  "EffectMain"

// PF_VERSION(0,1,0,PF_Stage_ALPHA,0) = 33280
#define PLUGIN_RC_VERSION       33280

// PF_OutFlag_NON_PARAM_VARY(1<<2) | PF_OutFlag_PIX_INDEPENDENT(1<<10)
// = 4 + 1024 = 1028
#define PLUGIN_RC_GLOBAL_OUTFLAGS   1028

// PF_OutFlag2_I_USE_3D_CAMERA(1<<1) | PF_OutFlag2_SUPPORTS_SMART_RENDER(1<<10)
// = 2 + 1024 = 1026
#define PLUGIN_RC_GLOBAL_OUTFLAGS2  1026
