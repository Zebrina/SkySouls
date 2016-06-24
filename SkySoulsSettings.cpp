#include "SkySoulsSettings.h"

SkySoulsSettings* SkySoulsSettings::GetSingleton() {
	static SkySoulsSettings instance;
	return &instance;
}

bool SkySoulsSettings::IsMenuUnpaused(BSFixedString* menuName) const {
	UIStringHolder* uiStringHolder = UIStringHolder::GetSingleton();
	if (uiStringHolder) {
		return *menuName == uiStringHolder->tweenMenu ||
			*menuName == uiStringHolder->magicMenu ||
			*menuName == uiStringHolder->inventoryMenu ||
			*menuName == uiStringHolder->containerMenu ||
			*menuName == uiStringHolder->favoritesMenu ||
			*menuName == uiStringHolder->lockpickingMenu ||
			*menuName == uiStringHolder->bookMenu;
	}
	return false;
	
}