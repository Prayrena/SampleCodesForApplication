#pragma once
#include "Engine/core/ErrorWarningAssert.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/NamedStrings.hpp"
#include "Engine/core/DevConsole.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/Math/Vec4.hpp"

//----------------------------------------------------------------------------------------------------------------------------------------------------
class EventSystem;
class DevConsole;

//----------------------------------------------------------------------------------------------------------------------------------------------------
#define STATIC // does nothing; used as a CPP maker for class static data & methods
#define UNUSED(x) (void)(x);

//----------------------------------------------------------------------------------------------------------------------------------------------------
#define DX_SAFE_RELEASE(dxObject)\
{								 \
	if((dxObject) != nullptr)	 \
	{							 \
		(dxObject)->Release();	 \
		(dxObject) = nullptr;	 \
	}							 \
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// global variables
//
extern NamedStrings g_gameConfigBlackboard;// global string key-value pairs, shared by game and engine
extern EventSystem* g_theEventSystem;
extern DevConsole*	g_theDevConsole;