#include "stdafx.h"

#ifdef _ATL_MIN_CRT


const GUID GUID_ATLVer70 = { 0x394c3de0, 0x3c6f, 0x11d2, { 0x81, 0x7b, 0x0, 0xc0, 0x4f, 0x79, 0x7a, 0xb7 } };

CAtlBaseModule::CAtlBaseModule()
{
	cbSize = sizeof(_ATL_BASE_MODULE);

	m_hInst = m_hInstResource = reinterpret_cast<HINSTANCE>(&__ImageBase);

	m_bNT5orWin98 = false;
	dwAtlBuildVer = _ATL_VER;
	pguidVer = &GUID_ATLVer70;
	m_csResource.Init();
}

CAtlBaseModule::~CAtlBaseModule()
{
	m_csResource.Term();
}


ATL::CAtlBaseModule ATL::_AtlBaseModule;

#endif //_ATL_MIN_CRT
