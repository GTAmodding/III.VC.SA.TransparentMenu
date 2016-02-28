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

int gtaversion = -1;
HMODULE dllModule;

#define ID_BLURPS                            101
#define IDR_POSTFXVS                         102

void *blurps = NULL;
void *postfxVS = NULL;

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
WRAPPER RwBool RwD3D9CreateVertexShaderSA(const RwUInt32 *function, void **shader) { EAXJMP(0x7FAC60); }
WRAPPER RwBool RwD3D9CreateVertexDeclarationSA(const void *elements, void **vertexdeclaration) { EAXJMP(0x7FAA30); }
void *quadVertexDecl;
void setpsSA()
{
	if (blurps == NULL)
	{
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(ID_BLURPS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShaderSA(shader, &blurps);
		assert(blurps);
		FreeResource(shader);

		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_POSTFXVS), RT_RCDATA);
		shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreateVertexShaderSA(shader, &postfxVS);
		FreeResource(shader);

		static const D3DVERTEXELEMENT9 vertexElements[] =
		{
			{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
			{ 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
			{ 0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			{ 0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
			D3DDECL_END()
		};
		RwD3D9CreateVertexDeclarationSA(vertexElements, &quadVertexDecl);
	}
}

struct RsGlobalType
{
	DWORD			AppName;
	DWORD			MaximumWidth;
	DWORD			MaximumHeight;
	/*snip*/
};
struct QuadVertex
{
	RwReal      x, y, z;
	RwReal      rhw;
	RwUInt32    emissiveColor;
	RwReal      u, v;
	RwReal      u1, v1;
};
RsGlobalType *RsGlobal = (RsGlobalType*)0xC17040;
RwRaster *&pRasterFrontBuffer = *(RwRaster**)0xC402D8;
static QuadVertex quadVertices[4];
static RwImVertexIndex* quadIndices = (RwImVertexIndex*)0x8D5174;
WRAPPER RwBool RwD3D9SetTextureSA(RwTexture *texture, RwUInt32 stage) { EAXJMP(0x7FDE70); }
WRAPPER void RwD3D9SetVertexDeclarationSA(void *vertexDeclaration) { EAXJMP(0x7F9F70); }
WRAPPER void RwD3D9SetVertexShaderSA(void *shader) { EAXJMP(0x7F9FB0); }
WRAPPER void RwD3D9SetPixelShaderSA(void *shader) { EAXJMP(0x7F9FF0); }
WRAPPER void RwD3D9SetRenderStateSA(RwUInt32 state, RwUInt32 value) { EAXJMP(0x7FC2D0); }
WRAPPER void RwD3D9DrawIndexedPrimitive(RwUInt32 primitiveType, RwInt32 baseVertexIndex, RwUInt32 minIndex,
	RwUInt32 numVertices, RwUInt32 startIndex, RwUInt32 primitiveCount) {
	EAXJMP(0x7FA320);
}
WRAPPER void RwD3D9DrawIndexedPrimitiveUP(RwUInt32 primitiveType, RwUInt32 minIndex, RwUInt32 numVertices, RwUInt32 primitiveCount,
	const void *indexData, const void *vertexStreamZeroData, RwUInt32 VertexStreamZeroStride) {
	EAXJMP(0x7FA1F0);
}
void applyShaderSA()
{
	float rasterWidth = (float)RwRasterGetWidth(pRasterFrontBuffer);
	float rasterHeight = (float)RwRasterGetHeight(pRasterFrontBuffer);
	float halfU = 0.5f / rasterWidth;
	float halfV = 0.5f / rasterHeight;
	float uMax = RsGlobal->MaximumWidth / rasterWidth;
	float vMax = RsGlobal->MaximumHeight / rasterHeight;
	int i = 0;

	float leftOff, rightOff, topOff, bottomOff;
	float scale = RsGlobal->MaximumWidth / 640.0f;
	leftOff = 8.0f*scale / 16.0f / rasterWidth;
	rightOff = 8.0f*scale / 16.0f / rasterWidth;
	topOff = 8.0f*scale / 16.0f / rasterHeight;
	bottomOff = 8.0f*scale / 16.0f / rasterHeight;

	quadVertices[i].x = 1.0f;
	quadVertices[i].y = -1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = uMax + halfU;
	quadVertices[i].v = vMax + halfV;
	quadVertices[i].u1 = quadVertices[i].u + rightOff;
	quadVertices[i].v1 = quadVertices[i].v + bottomOff;
	i++;

	quadVertices[i].x = -1.0f;
	quadVertices[i].y = -1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = 0.0f + halfU;
	quadVertices[i].v = vMax + halfV;
	quadVertices[i].u1 = quadVertices[i].u + leftOff;
	quadVertices[i].v1 = quadVertices[i].v + bottomOff;
	i++;

	quadVertices[i].x = -1.0f;
	quadVertices[i].y = 1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = 0.0f + halfU;
	quadVertices[i].v = 0.0f + halfV;
	quadVertices[i].u1 = quadVertices[i].u + leftOff;
	quadVertices[i].v1 = quadVertices[i].v + topOff;
	i++;

	quadVertices[i].x = 1.0f;
	quadVertices[i].y = 1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = uMax + halfU;
	quadVertices[i].v = 0.0f + halfV;
	quadVertices[i].u1 = quadVertices[i].u + rightOff;
	quadVertices[i].v1 = quadVertices[i].v + topOff;

	RwTexture tempTexture;
	tempTexture.raster = pRasterFrontBuffer;
	RwD3D9SetTextureSA(&tempTexture, 0);

	RwD3D9SetVertexDeclarationSA(quadVertexDecl);

	RwD3D9SetVertexShaderSA(postfxVS);
	RwD3D9SetPixelShaderSA(blurps);


	RwD3D9SetRenderStateSA(D3DRS_ALPHABLENDENABLE, 0);
	RwD3D9SetRenderStateSA(D3DRS_CULLMODE, D3DCULL_NONE);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, quadIndices, quadVertices, sizeof(QuadVertex));

	RwD3D9SetRenderStateSA(D3DRS_ALPHABLENDENABLE, 1);

	RwD3D9SetVertexShaderSA(NULL);
	RwD3D9SetPixelShaderSA(NULL);
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
			injector::WriteMemory(0x53E288, 0x832050FF, true);
			funcCHudDraw();
		}
	});
}

template<uintptr_t addr>
void CSprite2dDrawRectHookSA()
{
	using func_hook = injector::function_hooker<addr, void(float *, unsigned int)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type funcCSprite2dDrawRect, float *coords, unsigned int rgbaColor)
	{
		funcCSprite2dDrawRect(coords, rgbaColor);
		applyShaderSA();
	});
}

template<uintptr_t addr>
void CRendererConstructRenderListHook()
{
	using func_hook = injector::function_hooker<addr, void()>;
	injector::make_static_hook<func_hook>([](func_hook::func_type CRendererConstructRenderList)
	{
		if (isVC() ? *(bool*)0x869668 == 0 : *(bool*)0x8F5AE9 == 0) //bMenuVisible
		{
			CRendererConstructRenderList();
		}
		else
		{
			isVC() ? injector::cstd<void()>::call<0x4CA260>() : injector::cstd<void()>::call<0x4A76B0>();
		}
	});
}

template<uintptr_t addr>
void RsMouseSetPosHookSA()
{
	using func_hook = injector::function_hooker<addr, void(RwV2d*)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type RsMouseSetPos, RwV2d *pos)
	{
		if (*(bool*)0xBA67A4 == 0) //bMenuVisible
		{
			
			RsMouseSetPos(pos);
		}
		else
			injector::WriteMemory(0x53E288, 0x83909090, true);
		return;
	});
}

void CSprite2dDrawHook()
{
	__asm ret 8
}

void patchIII()
{
	injector::MakeInline<0x582E71, 0x582E78>([](injector::reg_pack&)
	{
		injector::WriteMemory<char>(0x8F5AEE, 0, true); // bGameStarted

		injector::MakeNOP(0x48D02C, 5, true);

		injector::MakeNOP(0x48E520, 6, true);

		//gta3 doesn't care about alpha without cleo, so noping draw calls is necessary just in case
		injector::WriteMemory<char>(0x47A635 + 0x1, 0x00, true); injector::MakeNOP(0x47A63E, 2, true); injector::MakeCALL(0x47A695, CSprite2dDrawHook, true);
		injector::WriteMemory<char>(0x47A7B3 + 0x1, 0x00, true); injector::MakeCALL(0x47A812, CSprite2dDrawHook, true);
		injector::WriteMemory<unsigned char>(0x47A829, 0x90u, true); injector::WriteMemory(0x47A829 + 0x1, 0x000000B8, true); injector::MakeCALL(0x47A891, CSprite2dDrawHook, true);

		injector::WriteMemory<char>(0x47A8A7 + 0x1, 0x00, true);
		injector::MakeCALL(0x47A904, CSprite2dDrawHook, true);

		CRendererConstructRenderListHook<0x48E539>();
		CHudDrawHook<(0x48E420)>();

		setps();
	});

	static int nForceBlur = 1;
	injector::WriteMemory(0x50AE87, &nForceBlur, true);
	injector::WriteMemory(0x50B072, &nForceBlur, true);
	injector::WriteMemory(0x50B079, &nForceBlur, true);
	injector::MakeNOP(0x48B000, 5, true);

	LoadLibrary("rwd3d9");
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

		CRendererConstructRenderListHook<0x4A5E45>();
		CHudDrawHook<(0x4A64D0)>();

		setps();
	});

	static int nForceBlur = 1;
	injector::WriteMemory(0x55CE81, &nForceBlur, true);
	injector::WriteMemory(0x55D062, &nForceBlur, true);
	injector::WriteMemory(0x55D069, &nForceBlur, true);
	injector::MakeNOP(0x498F48, 5, true);

	//CMBlur::AddRenderFx Type 4
	injector::MakeNOP(0x560FF9, 5, true);
	injector::MakeNOP(0x561259, 5, true);

	LoadLibrary("rwd3d9");
}

void patchSA()
{
	injector::MakeInline<0x748D0F, 0x748D15>([](injector::reg_pack&)
	{
		injector::WriteMemory<char>(0xBA6831, 0, true); // FrontEndMenuManager.bUseGameMenu

		injector::MakeNOP(0x53E9B3, 6, true);

		//injector::WriteMemory<char>(0x57B982 + 0x1, 0x00, true);

		RsMouseSetPosHookSA<0x53E9F1>();
		CHudDrawHookSA<(0x53E4FF)>();
		CSprite2dDrawRectHookSA<(0x57B9BB)>(); //fastloader compatibility
		injector::MakeNOP(0x576E23, 5, true);
		injector::MakeNOP(0x576E38, 5, true);// menu flash
		injector::WriteMemory<char>(0x57B9F5, 0x50, true); //frontend

		setpsSA();
	});
}

void Init()
{
	AddressByVersion<uint32_t>(0, 0, 0, 0, 0, 0);
	auto gvm = injector::address_manager::singleton();
	
	if (gvm.IsIII())
	{
		patchIII();
	}
	else
	{
		if (gvm.IsVC())
		{
			patchVC();
		}
		else
		{
			if (gvm.IsSA())
			{
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