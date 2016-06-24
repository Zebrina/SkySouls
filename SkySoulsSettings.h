#pragma once

#include "skse/GameMenus.h"

class SkySoulsSettings {
public:

	static SkySoulsSettings* GetSingleton();

	bool IsMenuUnpaused(BSFixedString* menuName) const;
};