#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <numbers>

#include "AE_Effect.h"
#include "AE_EffectCBSuites.h"
#include "AE_GeneralPlug.h"
#include "AEConfig.h"
#include "AEFX_SuiteHelper.h"
#include "entry.h"
#include "Param_Utils.h"
#include "Smart_Utils.h"

#include "Resources/PluginInfo.h"


enum { ParamIndex_Input = 0, ParamIndex_COUNT };

namespace PluginInfo
{
	constexpr PF_OutFlags  GlobalOutFlags = PF_OutFlag_NON_PARAM_VARY | PF_OutFlag_PIX_INDEPENDENT;
	constexpr PF_OutFlags2 GlobalOutFlags2 = PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_I_USE_3D_CAMERA;
}

static_assert(PluginInfo::GlobalOutFlags == PLUGIN_RC_GLOBAL_OUTFLAGS, "PiPL GlobalOutFlags mismatch");
static_assert(PluginInfo::GlobalOutFlags2 == PLUGIN_RC_GLOBAL_OUTFLAGS2, "PiPL GlobalOutFlags2 mismatch");

static AEGP_PluginID sPluginID = 0;


static bool ReadCameraRotation(PF_InData* in_data, float& out_x, float& out_y, float& out_z)
{
	AEFX_SuiteScoper<AEGP_PFInterfaceSuite1> pfInterface(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);

	A_Time compTime;
	if (pfInterface->AEGP_ConvertEffectToCompTime(in_data->effect_ref, in_data->current_time, in_data->time_scale, &compTime))
		return false;

	AEGP_LayerH cameraH;
	if (pfInterface->AEGP_GetEffectCamera(in_data->effect_ref, &compTime, &cameraH) || !cameraH)
		return false;

	A_Matrix4 mat;
	A_FpLong dist;
	A_short w, h;
	if (pfInterface->AEGP_GetEffectCameraMatrix(in_data->effect_ref, &compTime, &mat, &dist, &w, &h))
		return false;

	// mat is row-vector convention: row i is local basis vector i expressed in world space
	// Decompose rotation angles from the matrix rows
	constexpr double kToDeg = 180.0 / std::numbers::pi;
	const double pitch = std::asin(std::clamp(mat.mat[0][2], -1.0, 1.0));
	const double yaw = std::atan2(-mat.mat[0][1], mat.mat[0][0]);
	const double roll = std::atan2(-mat.mat[1][2], mat.mat[2][2]);

	// Normalize to [0..360)
	auto norm = [](double deg) -> float
	{
		deg = std::fmod(deg, 360.0);
		if (deg < 0.0) deg += 360.0;
		return static_cast<float>(deg);
	};

	out_x = norm(pitch * kToDeg);
	out_y = norm(yaw * kToDeg);
	out_z = norm(roll * kToDeg);
	return true;
}

static void FillOutput(PF_EffectWorld* world, A_u_char r, A_u_char g, A_u_char b)
{
	for (A_long y = 0; y < world->height; ++y)
	{
		PF_Pixel8* row = reinterpret_cast<PF_Pixel8*>(reinterpret_cast<char*>(world->data) + y * world->rowbytes);
		for (A_long x = 0; x < world->width; ++x)
		{
			row[x].red = r;
			row[x].green = g;
			row[x].blue = b;
			row[x].alpha = 255;
		}
	}
}

static PF_Err EffectMainInternal(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output, void* extra)
{
	PF_Err err = PF_Err_NONE;

	try
	{
		switch (cmd)
		{
			case PF_Cmd_GLOBAL_SETUP:
			{
				out_data->my_version = PF_VERSION(0, 1, 0, PF_Stage_ALPHA, 0);
				out_data->out_flags = PluginInfo::GlobalOutFlags;
				out_data->out_flags2 = PluginInfo::GlobalOutFlags2;

				AEFX_SuiteScoper<AEGP_UtilitySuite6> util(in_data, kAEGPUtilitySuite, kAEGPUtilitySuiteVersion6);
				util->AEGP_RegisterWithAEGP(nullptr, PLUGIN_NAME, &sPluginID);

				break;
			}

			case PF_Cmd_PARAMS_SETUP:
			{
				out_data->num_params = ParamIndex_COUNT;
				break;
			}

			case PF_Cmd_SMART_PRE_RENDER:
			{
				auto* pre = reinterpret_cast<PF_PreRenderExtra*>(extra);
				PF_RenderRequest req = pre->input->output_request;
				PF_CheckoutResult checkout{};

				err = pre->cb->checkout_layer(in_data->effect_ref, ParamIndex_Input, ParamIndex_Input, &req, in_data->current_time, in_data->time_step, in_data->time_scale, &checkout);

				if (!err)
				{
					UnionLRect(&checkout.result_rect, &pre->output->result_rect);
					UnionLRect(&checkout.max_result_rect, &pre->output->max_result_rect);
				}

				break;
			}

			case PF_Cmd_SMART_RENDER:
			{
				auto* sr = reinterpret_cast<PF_SmartRenderExtra*>(extra);
				PF_EffectWorld* input_world = nullptr;
				PF_EffectWorld* output_world = nullptr;

				err = sr->cb->checkout_layer_pixels(in_data->effect_ref, ParamIndex_Input, &input_world);

				if (!err)
					err = sr->cb->checkout_output(in_data->effect_ref, &output_world);

				if (!err && output_world)
				{
					float rx = 0, ry = 0, rz = 0;
					ReadCameraRotation(in_data, rx, ry, rz);

					auto toU8 = [](float deg) -> A_u_char
					{
						return static_cast<A_u_char>(std::clamp(deg / 360.0f * 255.0f, 0.0f, 255.0f));
					};

					FillOutput(output_world, toU8(rx), toU8(ry), toU8(rz));
				}

				sr->cb->checkin_layer_pixels(in_data->effect_ref, ParamIndex_Input);
				break;
			}
		}
	}
	catch (...)
	{
		err = PF_Err_INTERNAL_STRUCT_DAMAGED;
	}

	return err;
}


extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output, void* extra)
{
	return EffectMainInternal(cmd, in_data, out_data, params, output, extra);
}

extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr inPtr, PF_PluginDataCB2 inPluginDataCallBackPtr, SPBasicSuite* inSPBasicSuitePtr, const char* inHostName, const char* inHostVersion)
{
	PF_Err PF_REGISTER_EFFECT_EXT2(inPtr, inPluginDataCallBackPtr, PLUGIN_NAME, PLUGIN_MATCH_NAME, PLUGIN_CATEGORY, AE_RESERVED_INFO, PLUGIN_ENTRY_POINT, "");
	return result;
}
