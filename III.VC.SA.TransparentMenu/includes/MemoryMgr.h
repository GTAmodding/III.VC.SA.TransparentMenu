#ifndef __MEMORYMGR
#define __MEMORYMGR

#define WRAPPER __declspec(naked)
#define DEPRECATED __declspec(deprecated)
#define EAXJMP(a) { _asm mov eax, a _asm jmp eax }
#define VARJMP(a) { _asm jmp a }
#define WRAPARG(a) UNREFERENCED_PARAMETER(a)

#define NOVMT __declspec(novtable)
#define SETVMT(a) *((DWORD_PTR*)this) = (DWORD_PTR)a

enum
{
	PATCH_CALL,
	PATCH_JUMP,
	PATCH_NOTHING,
};

enum
{
	III_10 = 1,
	III_11,
	III_STEAM,
	VC_10,
	VC_11,
	VC_STEAM
};

extern int gtaversion;

//template<typename T>
//inline T AddressByVersion(uint32_t addressIII10, uint32_t addressIII11, uint32_t addressvc10)
//{
//	if(gtaversion == -1){
//		     if(*(uint32_t*)0x5C1E75 == 0xB85548EC) gtaversion = III_10;
//		else if(*(uint32_t*)0x5C2135 == 0xB85548EC) gtaversion = III_11;
//		else if(*(uint32_t*)0x5C6FD5 == 0xB85548EC) gtaversion = III_STEAM;
//		else if(*(uint32_t*)0x667BF5 == 0xB85548EC) gtaversion = VC_10;
//		else if(*(uint32_t*)0x667C45 == 0xB85548EC) gtaversion = VC_11;
//		else if(*(uint32_t*)0x666BA5 == 0xB85548EC) gtaversion = VC_STEAM;
//		else gtaversion = 0;
//	}
//	switch(gtaversion){
//	case III_10:
//		return (T)addressIII10;
//	case III_11:
//		return (T)addressIII11;
//	case VC_10:
//		return (T)addressvc10;
//	default:
//		return (T)0;
//	}
//}

template<typename T>
inline T AddressByVersion(uint32_t addressIII10, uint32_t addressIII11, uint32_t addressIIISteam, uint32_t addressvc10, uint32_t addressvc11, uint32_t addressvcSteam)
{
	if(gtaversion == -1){
		     if(*(uint32_t*)0x5C1E75 == 0xB85548EC) gtaversion = III_10;
		else if(*(uint32_t*)0x5C2135 == 0xB85548EC) gtaversion = III_11;
		else if(*(uint32_t*)0x5C6FD5 == 0xB85548EC) gtaversion = III_STEAM;
		else if(*(uint32_t*)0x667BF5 == 0xB85548EC) gtaversion = VC_10;
		else if(*(uint32_t*)0x667C45 == 0xB85548EC) gtaversion = VC_11;
		else if(*(uint32_t*)0x666BA5 == 0xB85548EC) gtaversion = VC_STEAM;
		else gtaversion = 0;
	}
	switch(gtaversion){
	case III_10:
		return (T)addressIII10;
	case III_11:
		return (T)addressIII11;
	case III_STEAM:
		return (T)addressIIISteam;
	case VC_10:
		return (T)addressvc10;
	case VC_11:
		return (T)addressvc11;
	case VC_STEAM:
		return (T)addressvcSteam;
	default:
		return (T)0;
	}
}

inline bool
isIII(void)
{
	return gtaversion >= III_10 && gtaversion <= III_STEAM;
}

inline bool
isVC(void)
{
	return gtaversion >= VC_10 && gtaversion <= VC_STEAM;
}

template<typename AT>
inline AT DynBaseAddress(AT address)
{
	return (AT)GetModuleHandle(nullptr) - 0x400000 + address;
}

namespace Memory
{
	template<typename T, typename AT>
	inline void		Patch(AT address, T value)
	{*(T*)address = value; }

	template<typename AT>
	inline void		Nop(AT address, unsigned int nCount)
	// TODO: Finish multibyte nops
	{ memset((void*)address, 0x90, nCount); }

	template<typename AT, typename HT>
	inline void		InjectHook(AT address, HT hook, unsigned int nType=PATCH_NOTHING)
	{
		DWORD		dwHook;
		_asm
		{
			mov		eax, hook
			mov		dwHook, eax
		}

		switch ( nType )
		{
		case PATCH_JUMP:
			*(BYTE*)address = 0xE9;
			break;
		case PATCH_CALL:
			*(BYTE*)address = 0xE8;
			break;
		}

		*(ptrdiff_t*)((DWORD)address + 1) = dwHook - (DWORD)address - 5;
	}
};

namespace MemoryVP
{
	template<typename T, typename AT>
	inline void		Patch(AT address, T value)
	{
		DWORD		dwProtect[2];
		VirtualProtect((void*)address, sizeof(T), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
		*(T*)address = value;
		VirtualProtect((void*)address, sizeof(T), dwProtect[0], &dwProtect[1]);
	}

	template<typename AT>
	inline void		Nop(AT address, unsigned int nCount)
	{
		DWORD		dwProtect[2];
		VirtualProtect((void*)address, nCount, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
		memset((void*)address, 0x90, nCount);
		VirtualProtect((void*)address, nCount, dwProtect[0], &dwProtect[1]);
	}

	template<typename AT, typename HT>
	inline void		InjectHook(AT address, HT hook, unsigned int nType=PATCH_NOTHING)
	{
		DWORD		dwProtect[2];
		switch ( nType )
		{
		case PATCH_JUMP:
			VirtualProtect((void*)address, 5, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			*(BYTE*)address = 0xE9;
			break;
		case PATCH_CALL:
			VirtualProtect((void*)address, 5, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			*(BYTE*)address = 0xE8;
			break;
		default:
			VirtualProtect((void*)((DWORD)address + 1), 4, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			break;
		}
		DWORD		dwHook;
		_asm
		{
			mov		eax, hook
			mov		dwHook, eax
		}

		*(ptrdiff_t*)((DWORD)address + 1) = (DWORD)dwHook - (DWORD)address - 5;
		if ( nType == PATCH_NOTHING )
			VirtualProtect((void*)((DWORD)address + 1), 4, dwProtect[0], &dwProtect[1]);
		else
			VirtualProtect((void*)address, 5, dwProtect[0], &dwProtect[1]);
	}

	namespace DynBase
	{
		template<typename T, typename AT>
		inline void		Patch(AT address, T value)
		{
			MemoryVP::Patch(DynBaseAddress(address), value);
		}

		template<typename AT>
		inline void		Nop(AT address, unsigned int nCount)
		{
			MemoryVP::Nop(DynBaseAddress(address), nCount);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook, unsigned int nType=PATCH_NOTHING)
		{
			MemoryVP::InjectHook(DynBaseAddress(address), hook, nType);
		}
	};
};

#endif