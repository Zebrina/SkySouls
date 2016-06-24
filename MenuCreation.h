#pragma once

#include "skse/GameMenus.h"

#include <string>

typedef tHashSet<MenuTableItem, BSFixedString> MenuTable;

template <UInt32 ID>
static MenuManager::CreatorFunc OriginalMenuCreatorFunction;

#define GetObjectVTable2(ptr) (*(UInt32*)(ptr))

template <UInt32 ID>
IMenu* NoPauseMenuCreatorFunction() {
	static UInt32 myID = ID;

	IMenu* m = OriginalMenuCreatorFunction<ID>();

	//m->flags ^= IMenu::kType_PauseGame;

	char menuID[5]{
		((char*)menuID)[3],
		((char*)menuID)[2],
		((char*)menuID)[1],
		((char*)menuID)[0],
		'\0',
	};

	_MESSAGE("%s vtable: 0x%.8x", menuID, GetObjectVTable2(m));

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