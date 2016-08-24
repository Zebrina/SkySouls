#pragma once

#include "SKSEMemUtil/SKSEMemUtil.h"

#include "skse/GameMenus.h"

struct MenuCreator {
	enum GamePauseBehavior {
		Never,
		Always,
		CombatOnly,
		NoCombatOnly,
		NumBehaviorTypes,
	};

	GamePauseBehavior pauseBehavior = Never;
	MenuManager::CreatorFunc originalMenuCreator = nullptr;

	operator MenuManager::CreatorFunc() const {
		return originalMenuCreator;
	}
	void operator()(MenuManager::CreatorFunc _originalMenuCreator) {
		originalMenuCreator = _originalMenuCreator;
	}

	void operator()(IMenu* menu) const {
		_MESSAGE("GetPause(): %i", menu->view->GetPause());

		if (pauseBehavior != Never) {
			if (pauseBehavior != Always) {
				bool inCombat = (*g_thePlayer)->IsInCombat();
				if ((pauseBehavior == CombatOnly && !inCombat) ||
					(pauseBehavior == NoCombatOnly && inCombat)) {
					return;
				}
			}

			//menu->view->SetPause(0);
			_ClearFlags(menu->flags, IMenu::kType_PauseGame);
		}
	}
};

struct CreateTweenMenu : public MenuCreator {};
struct CreateInventoryMenu : public MenuCreator {};
struct CreateContainerMenu : public MenuCreator {};
struct CreateMagicMenu : public MenuCreator {};
struct CreateFavoritesMenu : public MenuCreator {};
struct CreateLockpickingMenu : public MenuCreator {};
struct CreateBookMenu : public MenuCreator {};
struct CreateBarterMenu : public MenuCreator {};
struct CreateGiftMenu : public MenuCreator {};
struct CreateTrainingMenu : public MenuCreator {};
struct CreateMessageBoxMenu : public MenuCreator {};
