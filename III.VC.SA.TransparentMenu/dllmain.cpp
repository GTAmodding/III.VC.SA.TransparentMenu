#include "stdafx.h"
#include "includes\injector\injector.hpp"
#include "includes\injector\assembly.hpp"
#include "includes\injector\hooking.hpp"
#include "includes\injector\calling.hpp"

injector::address_manager& gvm;

void Init()
{
	gvm = injector::address_manager::singleton();

}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}