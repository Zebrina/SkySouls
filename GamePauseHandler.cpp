#include "GamePauseHandler.h"

#include "skse/GameMenus.h"
#include "skse/SafeWrite.h"

using namespace MemUtil;

GamePauseHandler** g_gamePauseHandler = (GamePauseHandler**)0x00400011;

BSFixedString MainMenu("Main Menu");
BSFixedString LoadingMenu("Loading Menu");
BSFixedString Console("Console");
BSFixedString TutorialMenu("Tutorial Menu");
BSFixedString MessageBoxMenu("MessageBoxMenu");
BSFixedString TweenMenu("TweenMenu");
BSFixedString InventoryMenu("InventoryMenu");
BSFixedString MagicMenu("MagicMenu");
BSFixedString ContainerMenu("ContainerMenu");
BSFixedString FavoritesMenu("FavoritesMenu");
BSFixedString CraftingMenu("Crafting Menu");
BSFixedString BarterMenu("BarterMenu");
BSFixedString TrainingMenu("Training Menu");
BSFixedString LockpickingMenu("Lockpicking Menu");
BSFixedString BookMenu("Book Menu");
BSFixedString MapMenu("MapMenu");
BSFixedString StatsMenu("StatsMenu");

const IntPtr IMenuVTable_MessageBoxMenu		= 0x010e6b34;
const IntPtr IMenuVTable_TweenMenu			= 0x010e8140;
const IntPtr IMenuVTable_InventoryMenu		= 0x010e5b90;
const IntPtr IMenuVTable_MagicMenu			= 0x010e6594;
const IntPtr IMenuVTable_ContainerMenu		= 0x010e4098;
const IntPtr IMenuVTable_FavoritesMenu		= 0x010e4e18;
const IntPtr IMenuVTable_CraftingMenu		= 0x010e4324;
const IntPtr IMenuVTable_BarterMenu			= nullptr;
const IntPtr IMenuVTable_TrainingMenu		= nullptr;
const IntPtr IMenuVTable_LockpickingMenu	= 0x010e6308;
const IntPtr IMenuVTable_BookMenu			= 0x010e3aa4;
const IntPtr IMenuVTable_MapMenu			= 0x010e95b4;
const IntPtr IMenuVTable_StatsMenu			= 0x010e7ad4;

const IntPtr IMenuVTable_HUDMenu			= 0x010e537c;
const IntPtr IMenuVTable_ConsoleMenu		= 0x010e3cb4;
const IntPtr IMenuVTable_CursorMenu			= 0x010e4bd4;

GamePauseHandler* GamePauseHandler::GetSingleton() {
	static GamePauseHandler instance;
	return &instance;
}

GamePauseHandler::GamePauseHandler() :
	disableCounter(0), tempDisableCounter(0), gamePauseOverride(0) {
	//_MESSAGE("numRealPauseRequests: 0x%.8x", &numRealPauseRequests);

	SafeWrite32((UInt32)g_gamePauseHandler, (UInt32)this);
}
GamePauseHandler::~GamePauseHandler() {
}

void GamePauseHandler::disableTemporary(UInt32 ticks) {
	if (ticks > tempDisableCounter) {
		tempDisableCounter = ticks;
	}
}
void GamePauseHandler::disableConditionally(GamePauseCondition* condition) {
	disableConditions.push_front(condition);
}

void GamePauseHandler::update(UInt32 gamePauseCounter, tArray<IMenu*>* menuStack) {
	struct EvaluateCondition {
		bool operator()(GamePauseCondition* condition) const {
			if (condition->Evaluate()) {
				condition->Dispose();
				return true;
			}
			return false;
		}
	};

	gamePauseOverride = gamePauseCounter > 0 ? 1 : 0;

	if (gamePauseOverride == 0 && menuStack->count > 1) {
		for (UInt32 i = 0; gamePauseOverride == 0 && i < menuStack->count; ++i) {
			if (gamePauseDisabledSet.count(_GetObjectVTable((*menuStack)[i]))) {
				gamePauseOverride = 1;
			}
		}
	}

	if (!disableConditions.empty()) {
		disableConditions.remove_if(EvaluateCondition());
	}

	if (tempDisableCounter > 0) {
		--tempDisableCounter;
	}
}

void GamePauseHandler::disable() {
	++disableCounter;
}
void GamePauseHandler::enable() {
	--disableCounter;
}
