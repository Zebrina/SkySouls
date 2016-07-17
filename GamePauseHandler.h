#pragma once

#include "SKSEMemUtil.h"

#include "skse/GameTypes.h"
#include "skse/GameMenus.h"

#include <forward_list>
#include <unordered_map>
#include <unordered_set>

class IMenu;
class GamePauseHandler;

extern const SKSEMemUtil::IntPtr IMenuVTable_MessageBoxMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_TweenMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_InventoryMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_MagicMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_ContainerMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_FavoritesMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_CraftingMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_BarterMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_TrainingMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_LockpickingMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_BookMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_MapMenu;
extern const SKSEMemUtil::IntPtr IMenuVTable_StatsMenu;

template <>
struct std::hash<BSFixedString> {
	size_t operator()(const BSFixedString& str) const {
		return (size_t)str.data;
	}
};

class GamePauseCondition {
public:
	virtual bool Evaluate() = 0;
	virtual void Dispose() {
		delete this;
	}
};

class GamePauseHandler {
public:
	static GamePauseHandler* GetSingleton();

	~GamePauseHandler();

	bool isEnabled() const {
		return disableCounter == 0;
	}

	const UInt32* getGamePauseOverride() const {
		return &gamePauseOverride;
	}

	void disableTemporary(UInt32 ticks);
	void disableConditionally(GamePauseCondition* condition);

	void update(UInt32 gamePauseCounter, tArray<IMenu*>* menuStack);

	void disable();
	void enable();

	template <class F>
	void registerMenu(BSFixedString menuName, F& menuCreator) {
		tHashSet<MenuTableItem, BSFixedString>* mtable = SKSEMemUtil::IntPtr(MenuManager::GetSingleton()) + 0xa4;

		MenuTableItem* item = mtable->Find(&menuName);
		if (item) {
			F& thisMenuCreator = getMenuCreator<F>();
			thisMenuCreator = menuCreator;
			thisMenuCreator((MenuManager::CreatorFunc)item->menuConstructor);
			item->menuConstructor = (void*)CreateMenu<F>;
		}
	}

private:
	static std::unordered_map<BSFixedString, SKSEMemUtil::IntPtr> menuNameToVTableLookup;

	std::unordered_set<SKSEMemUtil::IntPtr> gamePauseDisabledSet;

	UInt32 disableCounter;
	UInt32 tempDisableCounter;
	std::forward_list<GamePauseCondition*> disableConditions;

	UInt32 gamePauseOverride;

	GamePauseHandler();

	template <class F>
	static F& getMenuCreator() {
		static F f;
		return f;
	}

	template <class F>
	static IMenu* CreateMenu() {
		F& menuCreateFunctor = getMenuCreator<F>();

		IMenu* menu = ((MenuManager::CreatorFunc)menuCreateFunctor)();

		GamePauseHandler* gamePauseHandler = GetSingleton();
		if (gamePauseHandler->disableCounter == 0 && gamePauseHandler->disableConditions.empty()) {
			bool pausedByDefault = _TestFlags(menu->flags, IMenu::kType_PauseGame);

			menuCreateFunctor(menu);

			if (pausedByDefault && !_TestFlags(menu->flags, IMenu::kType_PauseGame)) {
				gamePauseHandler->gamePauseDisabledSet.insert(SKSEMemUtil::IntPtr(_GetObjectVTable(menu)));
			}
			else {
				gamePauseHandler->gamePauseDisabledSet.erase(SKSEMemUtil::IntPtr(_GetObjectVTable(menu)));
			}
		}

		return menu;
	}
};

