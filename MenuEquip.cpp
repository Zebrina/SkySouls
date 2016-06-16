#include "MenuEquip.h"

#include "skse/GameData.h"
#include "skse/GameMenus.h"

static void Scaleform_RefreshInventoryLists(BSFixedString* menuName) {
	MenuManager* menuManager = MenuManager::GetSingleton();
	if (menuManager) {
		IMenu* menu = menuManager->GetMenu(menuName);
		if (menu) {
			menu->InitMovie();
			menuManager->GetMovieView(menuName)->Render();
		}

		/*

		UIManager* uiManager = UIManager::GetSingleton();

		//CALL_MEMBER_FN(uiManager, AddMessage)(menuName, UIMessage::kMessage_Close, nullptr);
		//CALL_MEMBER_FN(uiManager, AddMessage)(menuName, UIMessage::kMessage_Refresh, nullptr);

		GFxMovieView* view = mm->GetMovieView(menuName);

		/*
		GFxValue fxValue;
		SetGFxValue<T>(&fxValue, value);

		GFxValue * value = NULL;
		if (args.size() > 0)
			value = &args[0];

		view->Invoke(target_.c_str(), NULL, value, args.size());
		*/

		/*
		if (view) {
			view->GotoFrame(0);
			view->Unk_01

			bool result = view->Invoke("_root.Menu_mc.inventoryLists.updateList", nullptr, nullptr, 0);
			_MESSAGE("Invoke: %i", result);
		}

		CALL_MEMBER_FN(uiManager, AddMessage)(menuName, UIMessage::kMessage_Refresh, nullptr);
		*/
	}
}
static void RefreshMenu() {
	UIStringHolder* uiStringHolder = UIStringHolder::GetSingleton();
	//UIManager* uiManager = UIManager::GetSingleton();
	if (uiStringHolder) {
		//CALL_MEMBER_FN(uiManager, AddMessage)(&uiStringHolder->inventoryMenu, UIMessage::kMessage_Refresh , nullptr);
		//CALL_MEMBER_FN(uiManager, AddMessage)(&uiStringHolder->favoritesMenu, UIMessage::kMessage_Refresh, nullptr);

		Scaleform_RefreshInventoryLists(&uiStringHolder->inventoryMenu);
		Scaleform_RefreshInventoryLists(&uiStringHolder->magicMenu);
		Scaleform_RefreshInventoryLists(&uiStringHolder->containerMenu);
		Scaleform_RefreshInventoryLists(&uiStringHolder->favoritesMenu);

		_MESSAGE("menus refresh maybe?");
	}
}

MenuEquipManager* MenuEquipManager::GetSingleton() {
	static MenuEquipManager instance;
	return &instance;
}

void MenuEquipManager::queueEquipItem(MenuEquipData* data, UInt32 ticks) {
	data->equipAt = internalCounter + ticks  * 2;
	equipQueue.push(data);

	//_MESSAGE("EquipItem queued");
}
void MenuEquipManager::queueUnequipItem(MenuUnequipData* data, UInt32 ticks) {
	data->unequipAt = internalCounter + ticks * 2;
	unequipQueue.push(data);

	//_MESSAGE("UnequipItem queued");
}

bool MenuEquipManager::processEquipQueue(EquipItemMethod originalEquipItem) {
	//_MESSAGE("originalEquipItem: 0x%.8x", originalEquipItem);

	++internalCounter;

	if (equipQueue.empty()) {
		//_MESSAGE("empty");

		if (unequipQueue.empty()) {
			internalCounter = 0u;
		}
		return false;
	}
	else {
		//_MESSAGE("not empty (%i)", equipQueue.size());

		MenuEquipData* data = equipQueue.top();
		//_MESSAGE("data: 0x%.8x", data);
		if (data->equipAt <= internalCounter) {
			((data->equipManager)->*originalEquipItem)(data->actor, data->item, data->extraData, data->count, data->equipSlot, data->withEquipSound, data->preventEquip, data->showMsg, data->unk);
			delete data;
			equipQueue.pop();

			RefreshMenu();
		}

		return true;
	}
}
bool MenuEquipManager::processUnequipQueue(UnequipItemMethod originalUnequipItem) {
	//_MESSAGE("originalUnequipItem: 0x%.8x", originalUnequipItem);

	++internalCounter;

	if (unequipQueue.empty()) {
		//_MESSAGE("empty");

		if (equipQueue.empty()) {
			internalCounter = 0u;
		}
		return false;
	}
	else {
		//_MESSAGE("not empty (%i)", unequipQueue.size());

		MenuUnequipData* data = unequipQueue.top();
		//_MESSAGE("data: 0x%.8x", data);
		if (data->unequipAt <= internalCounter) {
			((data->equipManager)->*originalUnequipItem)(data->actor, data->item, data->extraData, data->count, data->equipSlot, data->unkFlag1, data->preventEquip, data->unkFlag2, data->unkFlag3, data->unk);
			delete data;
			unequipQueue.pop();

			RefreshMenu();
		}

		return true;
	}
}