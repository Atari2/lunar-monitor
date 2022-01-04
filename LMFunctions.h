#pragma once
#include <Windows.h>

constexpr uintptr_t LM_MAP16_SAVE_FUNCTION = 0x441E10;
using saveMap16Function = BOOL(*)();

constexpr uintptr_t LM_LEVEL_SAVE_FUNCTION = 0x46A6F0;
using saveLevelFunction = BOOL(*)(DWORD x, DWORD y);

constexpr uintptr_t LM_OW_SAVE_FUNCTION = 0x50E310;
using saveOWFunction = BOOL(*)();

constexpr uintptr_t LM_NEW_ROM_FUNCTION = 0x465F70;
using newRomFunction = BOOL(*)(DWORD a, DWORD b);

constexpr uintptr_t LM_TITLESCREEN_SAVE_FUNCTION = 0x4A53A0;
using saveTitlescreenFunction = BOOL(*)();

constexpr uintptr_t LM_CREDITS_SAVE_FUNCTION = 0x4A5890;
using saveCreditsFunction = BOOL(*)();

constexpr uintptr_t LM_SHARED_PALETTES_SAVE_FUNCTION = 0x451630;
using saveSharedPalettesFunction = BOOL(*)(BOOL x);

template <typename T>
constexpr T AddressToFnPtr(const uintptr_t address) {
	return reinterpret_cast<T>(address);
}