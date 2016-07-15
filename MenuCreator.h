#pragma once

#include "MemUtil.h"

#include "skse/GameMenus.h"

struct MenuCreator {
	MenuManager::CreatorFunc originalMenuCreator = nullptr;

	//MenuCreateFunctorBase()

	operator MenuManager::CreatorFunc() const {
		return originalMenuCreator;
	}
	void operator()(MenuManager::CreatorFunc _originalMenuCreator) {
		originalMenuCreator = _originalMenuCreator;
	}

	void operator()(IMenu* menu) const {
		_ClearFlags(menu->flags, IMenu::kType_PauseGame);
	}
};

struct CreateTweenMenu : public MenuCreator {};
struct CreateInventoryMenu : public MenuCreator {};
struct CreateContainerMenu : public MenuCreator {};
struct CreateMagicMenu : public MenuCreator {};
struct CreateFavoritesMenu : public MenuCreator {};
struct CreateLockpickingMenu : public MenuCreator {};
struct CreateBookMenu : public MenuCreator {};
