#ifndef __MAIN_H__
#define __MAIN_H__

#define WINVER 0x600
#define _WIN32_IE 0x600

#include <vector>
#include <string>

#define _MMSYSTEM_H
#define _INC_MMSYSTEM

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <psapi.h>
#include <oleacc.h>
#include <process.h>

// 定义MWORD为机器字长
#include <stdint.h>
#ifdef _WIN64
typedef uint64_t MWORD;
#else
typedef uint32_t MWORD;
#endif

void GreenChrome();

HMODULE hInstance;

#include "util.h"
#include "winmm.h"
#include "GetParent.h"
#include "Loader.h"
#include "NoRegisteredClass.h"
#include "TabBookmark.h"
#include "Bosskey.h"

#endif // __MAIN_H__
