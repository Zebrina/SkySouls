#pragma once

#include "skse/GameMenus.h"

typedef tHashSet<MenuTableItem, BSFixedString> MenuTable;

template <UInt32 ID>
static MenuManager::CreatorFunc OriginalMenuCreatorFunction;

template <UInt32 ID>
IMenu* NoPauseMenuCreatorFunction() {
	IMenu* m = OriginalMenuCreatorFunction<ID>();

	m->flags ^= IMenu::kType_PauseGame;

	return m;
}

template <UInt32 ID>
void HookNoPauseMenuCreation(BSFixedString* menu) {
	MenuTable* mtable = (MenuTable*)((UInt32)MenuManager::GetSingleton() + 0xa4);

	MenuTableItem* item = mtable->Find(menu);
	if (item) {
		OriginalMenuCreatorFunction<ID> = (MenuManager::CreatorFunc)item->menuConstructor;
		item->menuConstructor = (void*)NoPauseMenuCreatorFunction<ID>;
	}
}