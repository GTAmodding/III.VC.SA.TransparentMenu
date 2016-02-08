#include "stdafx.h"
#include <d3d9.h>
#include "d3d9types.h"
#include "includes\injector\injector.hpp"
#include "includes\injector\assembly.hpp"
#include "includes\injector\hooking.hpp"
#include "includes\injector\calling.hpp"
#include "includes\MemoryMgr.h"
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include <rtbmp.h>
#include "includes\rwd3d9\rwd3d9.h"
#pragma comment(lib, "delayimp")
#pragma comment(lib, "rwd3d9")

//#define SA 1

int gtaversion = -1;
HMODULE dllModule;

#define ID_BLURPS                            101

void *blurps = NULL;

static uint32_t DefinedState_A = AddressByVersion<uint32_t>(0x526330, 0x526570, 0x526500, 0x57F9C0, 0x57F9E0, 0x57F7F0);
WRAPPER void DefinedState(void) { VARJMP(DefinedState_A); }

RwD3D8Vertex *blurVertices = AddressByVersion<RwD3D8Vertex*>(0x62F780, 0x62F780, 0x63F780, 0x7097A8, 0x7097A8, 0x7087A8);
RwImVertexIndex *blurIndices = AddressByVersion<RwImVertexIndex*>(0x5FDD90, 0x5FDB78, 0x60AB70, 0x697D48, 0x697D48, 0x696D50);

static uint32_t CreateImmediateModeData_A = AddressByVersion<uint32_t>(0x50A800, 0, 0, 0x55F1D0, 0, 0);
WRAPPER void CreateImmediateModeData(RwCamera*, RwRect*) { EAXJMP(CreateImmediateModeData_A); }

void setps()
{
	if (blurps == NULL)
	{
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(ID_BLURPS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &blurps);
		assert(blurps);
		FreeResource(shader);
	}
}

void applyShader()
{
	static RwRaster *rampRaster = NULL;
	RwCamera *cam = *(RwCamera**)(0x7E4E90);
	if (isIII())
	{
		cam = *(RwCamera**)(0x6FACF8 + 0x7A0);
	}

	if (rampRaster == NULL)
	{
		int w, h;
		for (w = 1; w < cam->frameBuffer->width; w <<= 1);
		for (h = 1; h < cam->frameBuffer->height; h <<= 1);
		rampRaster = RwRasterCreate(w, h, 0, 5);
		RwRect rect = { 0, 0, w, h };
		CreateImmediateModeData(cam, &rect);
	}

	RwRasterPushContext(rampRaster);
	RwRasterRenderFast(cam->frameBuffer, 0, 0);
	RwRasterPopContext();

	DefinedState();
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, rampRaster);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	RwD3D9SetIm2DPixelShader(blurps);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
	RwD3D9SetIm2DPixelShader(NULL);
	DefinedState();
	RwD3D8SetTexture(NULL, 1);
}


WRAPPER RwBool RwD3D9CreatePixelShaderSA(const RwUInt32 *function, void **shader) { EAXJMP(0x7FACC0); }
WRAPPER void _rwD3D9SetPixelShader(void *shader) { EAXJMP(0x7F9FF0); }
WRAPPER RwRaster *RwRasterCreateSA(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) { EAXJMP(0x7FB230); }
void setpsSA()
{
	if (blurps == NULL)
	{
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(ID_BLURPS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShaderSA(shader, &blurps);
		assert(blurps);
		FreeResource(shader);
	}
}


void **rwengine = *(void***)0x58FFC0;
#define RwEngineInstance (*rwengine)
#define RwRenderStateSetMacroSA(_state, _value)   \
	(RWSRCGLOBAL(dOpenDevice).fpRenderStateSet(_state, _value))
#define RwRenderStateSetSA(_state, _value) \
		RwRenderStateSetMacroSA(_state, _value)

WRAPPER RwCamera *RwCameraBeginUpdateSA(RwCamera *camera) { EAXJMP(0x7EE190); }
WRAPPER RwCamera *RwCameraEndUpdateSA(RwCamera *camera) { EAXJMP(0x7EE180); }
WRAPPER RwRaster *RwRasterPushContextSA(RwRaster *raster) { EAXJMP(0x7FB060); }
WRAPPER RwRaster *RwRasterPopContextSA(void) { EAXJMP(0x7FB110); }
WRAPPER RwRaster *RwRasterRenderFastSA(RwRaster *raster, RwInt32 x, RwInt32 y) { EAXJMP(0x7FAF50); }
WRAPPER void DefinedStateSA(void) { EAXJMP(0x734650); }
WRAPPER RwBool RwIm2DRenderIndexedPrimitiveSA(RwPrimitiveType, RwIm2DVertex*, RwInt32, RwImVertexIndex*, RwInt32) { EAXJMP(0x734EA0); }
WRAPPER RwBool RwD3D9SetTextureSA(RwTexture*, RwUInt32) { EAXJMP(0x7FDE70); }
WRAPPER void ImmediateModeRenderStatesStore(void) { EAXJMP(0x700CC0); }
WRAPPER void ImmediateModeRenderStatesSet(void) { EAXJMP(0x700D70); }
WRAPPER void ImmediateModeRenderStatesReStore(void) { EAXJMP(0x700E00); }
WRAPPER void DrawQuad(float x1, float y1, float x2, float y2, char r, char g, char b, char alpha, RwRaster *ras) { EAXJMP(0x700EC0); }
void applyShaderSA()
{
	RwCamera *&Camera = *(RwCamera**)0xC1703C;
	RwRaster* camraster = RwCameraGetRaster(Camera);

	RwRasterPushContextSA(camraster);
	RwRasterRenderFastSA(Camera->frameBuffer, 0, 0);
	RwRasterPopContextSA();

	DefinedStateSA();

	RwRenderStateSetSA(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
	RwRenderStateSetSA(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	RwRenderStateSetSA(rwRENDERSTATESRCBLEND, (void*)D3DBLEND_DESTCOLOR);
	RwRenderStateSetSA(rwRENDERSTATEDESTBLEND, (void*)D3DBLEND_SRCALPHA);
	setpsSA();
	_rwD3D9SetPixelShader(blurps);
	DrawQuad(0, 0, RwRasterGetWidth(camraster) / 2, RwRasterGetHeight(camraster), 0xFF, 0xFF, 0xFF, 0xFF, camraster);
	_rwD3D9SetPixelShader(NULL);
	DefinedStateSA();
	RwD3D9SetTextureSA(NULL, 1);
}

template<uintptr_t addr>
void CHudDrawHook()
{
	using func_hook = injector::function_hooker<addr, void()>;
	injector::make_static_hook<func_hook>([](func_hook::func_type funcCHudDraw)
	{
		if (isVC() ? *(bool*)0x869668 == 0 : *(bool*)0x8F5AE9 == 0) //bMenuVisible
		{
			funcCHudDraw();
		}
		else
		{
			applyShader();
		}
	});
}

template<uintptr_t addr>
void CHudDrawHookSA()
{
	using func_hook = injector::function_hooker<addr, void()>;
	injector::make_static_hook<func_hook>([](func_hook::func_type funcCHudDraw)
	{
		if (*(bool*)0xBA67A4 == 0) //bMenuVisible
		{
			funcCHudDraw();
		}
		else
		{
			applyShaderSA();
		}
		return;
	});
}

template<uintptr_t addr>
void CRendererConstructRenderListHook()
{
	using func_hook = injector::function_hooker<addr, void()>;
	injector::make_static_hook<func_hook>([](func_hook::func_type CRendererConstructRenderList)
	{
		if (*(bool*)0x8F5AE9 == 0) //bMenuVisible
		{
			CRendererConstructRenderList();
		}
		else
		{
			injector::cstd<void()>::call<0x4A76B0>();
		}
	});
}

template<uintptr_t addr>
void RsMouseSetPosHook()
{
	using func_hook = injector::function_hooker<addr, void(RwV2d*)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type RsMouseSetPos, RwV2d *pos)
	{
		if (*(bool*)0x8F5AE9 == 0) //bMenuVisible
		{
			RsMouseSetPos(pos);
		}
		return;
	});
}

void patchIII()
{
	injector::MakeInline<0x582E71, 0x582E78>([](injector::reg_pack&)
	{
		injector::WriteMemory<char>(0x8F5AEE, 0, true); // bGameStarted

		injector::MakeNOP(0x48D02C, 5, true);

		injector::MakeNOP(0x48E520, 6, true);

		injector::WriteMemory<char>(0x47A635 + 0x1, 0x00, true); injector::MakeNOP(0x47A63E, 2, true);
		injector::WriteMemory<char>(0x47A7B3 + 0x1, 0x00, true);
		injector::WriteMemory<char>(0x47A829, 0x90, true); injector::WriteMemory(0x47A829 + 0x1, 0x000000B8, true);
		injector::WriteMemory<char>(0x47A8A7 + 0x1, 0x00, true);

		CRendererConstructRenderListHook<0x48E539>();
		CHudDrawHook<(0x48E420)>();
	});
}

void patchVC()
{
	injector::MakeInline<0x600416, 0x60041D>([](injector::reg_pack&)
	{
		injector::WriteMemory<char>(0x86969C, 0, true); // bGameStarted

		injector::MakeNOP(0x4A73C9, 5, true);
		injector::MakeNOP(0x4A21E0, 5, true);

		injector::MakeNOP(0x4A5E27, 6, true);

		injector::MakeNOP(0x4A212D + 0x704, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
		injector::MakeNOP(0x4A212D + 0x7FE, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
		injector::MakeNOP(0x4A212D + 0x907, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
		injector::MakeNOP(0x4A212D + 0xC8C, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
		injector::MakeNOP(0x4A212D + 0xD86, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
		injector::MakeNOP(0x4A212D + 0xE95, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
		injector::MakeNOP(0x4A212D + 0xFA4, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)


		injector::WriteMemory<char>(0x4A26D8 + 0x1, 0x00, true); injector::MakeNOP(0x4A26D6, 2, true);
		injector::WriteMemory<char>(0x4A2C60 + 0x1, 0x00, true); injector::MakeNOP(0x4A2C5E, 2, true);
		injector::WriteMemory<char>(0x4A35A2 + 0x1, 0x00, true);

		CHudDrawHook<(0x4A64D0)>();
	});
}

void patchSA()
{
	injector::MakeInline<0x748D0F, 0x748D15>([](injector::reg_pack&)
	{
		injector::WriteMemory<char>(0xBA6831, 0, true); // FrontEndMenuManager.bUseGameMenu

		injector::MakeNOP(0x53E9B3, 6, true);

		injector::WriteMemory<char>(0x57B982 + 0x1, 0x00, true);

		RsMouseSetPosHook<0x53E9F1>();
		CHudDrawHookSA<(0x53E4FF)>();
		//53E288 menu flash
	});
}

void Init()
{
	AddressByVersion<uint32_t>(0, 0, 0, 0, 0, 0);
	auto gvm = injector::address_manager::singleton();
	
	if (gvm.IsIII())
	{
		setps();
		patchIII();
	}
	else
	{
		if (gvm.IsVC())
		{
			setps();
			patchVC();
		}
		else
		{
			if (gvm.IsSA())
			{
				//setpsSA();
				patchSA();
			}
		}
	}
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		dllModule = hInst;
		Init();
	}
	return TRUE;
}