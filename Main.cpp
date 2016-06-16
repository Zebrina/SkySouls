#include "MenuCreation.h"
#include "MenuEquip.h"

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameAPI.h"

#include "skse/GameMenus.h"
#include "skse/GameData.h"

#include "skse/SafeWrite.h"

IDebugLog gLog("NoMenuPause.log");
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

static constexpr UInt32 hookAddr1 = 0x0069cc6e;
static constexpr UInt32 hookAddr2 = 0x0056ef86;

static constexpr UInt32 EquipManager_EquipItem_Addr = 0x006EF3E0;
static constexpr UInt32 EquipManager_UnequipItem_Addr = 0x006EE560;

static constexpr UInt32 UIManager_AddMessage_Addr = 0x00431B00;

enum {
	kMenuPauseID_MainMenu = 0,
	kMenuPauseID_LoadingMenu,
	kMenuPauseID_Console,
	kMenuPauseID_TutorialMenu,
	kMenuPauseID_MessageBoxMenu,
	kMenuPauseID_TweenMenu,
	kMenuPauseID_MagicMenu,
	kMenuPauseID_InventoryMenu,
	kMenuPauseID_ContainerMenu,
	kMenuPauseID_FavoritesMenu,
	kMenuPauseID_CraftingMenu,
	kMenuPauseID_BarterMenu,
	kMenuPauseID_TrainingMenu,
	kMenuPauseID_LockpickingMenu,
	kMenuPauseID_BookMenu,
	kMenuPauseID_Count,
};

BSFixedString MainMenu("Main Menu");
BSFixedString LoadingMenu("Loading Menu");
BSFixedString Console("Console");
BSFixedString TutorialMenu("Tutorial Menu");
BSFixedString MessageBoxMenu("MessageBoxMenu");
BSFixedString TweenMenu("TweenMenu");
BSFixedString MagicMenu("MagicMenu");
BSFixedString InventoryMenu("InventoryMenu");
BSFixedString ContainerMenu("ContainerMenu");
BSFixedString FavoritesMenu("FavoritesMenu");
BSFixedString CraftingMenu("Crafting Menu");
BSFixedString BarterMenu("BarterMenu");
BSFixedString TrainingMenu("Training Menu");
BSFixedString LockpickingMenu("Lockpicking Menu");
BSFixedString BookMenu("Book Menu");

EquipItemMethod g_originalEquipItem;
UnequipItemMethod g_originalUnequipItem;

bool GetPaused() {
	static const UInt32* pauseMenuStack = (UInt32*)((UInt32)MenuManager::GetSingleton() + 0xc8);
	return *pauseMenuStack > 0u;
}

bool GetUnpausedMenuOpen() {
	MenuManager* mm = MenuManager::GetSingleton();
	if (mm) {
		return mm->IsMenuOpen(&TweenMenu) ||
			mm->IsMenuOpen(&MagicMenu) ||
			mm->IsMenuOpen(&InventoryMenu) ||
			mm->IsMenuOpen(&ContainerMenu) ||
			mm->IsMenuOpen(&FavoritesMenu) ||
			mm->IsMenuOpen(&LockpickingMenu) ||
			mm->IsMenuOpen(&BookMenu);
	}
	return false;
}

bool GetInventoryMenuOpen() {
	MenuManager* mm = MenuManager::GetSingleton();
	if (mm) {
		return mm->IsMenuOpen(&MagicMenu) ||
			mm->IsMenuOpen(&InventoryMenu) ||
			mm->IsMenuOpen(&ContainerMenu) ||
			mm->IsMenuOpen(&FavoritesMenu);
	}
	return false;
}

void __cdecl WriteRedirectionHook(UInt32 targetOfHook, UInt32 sourceBegin, UInt32 sourceEnd, UInt32 asmSegSize) {
	WriteRelJump(targetOfHook, sourceBegin);

	for (int i = 5; i < asmSegSize; ++i) {
		SafeWrite8(targetOfHook + i, 0x90); // nop
	}

	WriteRelJump(sourceEnd, targetOfHook + asmSegSize);
}

class NoMenuPauseController {
public:
	void postpone(UInt32 ticks) {
		counter = (counter >= ticks ? counter : ticks);
	}

	void update() {
		if (counter > 0) {
			--counter;
		}
	}

	bool getNoPauseAllowed() {
		return counter == 0;
	}

private:
	UInt32 counter = 0u;
};
static NoMenuPauseController g_noMenuPauseController;

UInt32 __fastcall GetNoPause(MenuManager* mm) {
	//_MESSAGE("MenuEquipManager: 0x%.8x", MenuEquipManager::GetSingleton());

	bool equipProcessActive = MenuEquipManager::GetSingleton()->processEquipQueue(g_originalEquipItem);
	bool unequipProcessActive = MenuEquipManager::GetSingleton()->processUnequipQueue(g_originalUnequipItem);

	if (equipProcessActive || unequipProcessActive) {
		g_noMenuPauseController.postpone(1);
	}
	else {
		if (g_noMenuPauseController.getNoPauseAllowed()) {
			if (GetPaused()) {
				if (mm->IsMenuOpen(&MainMenu) || mm->IsMenuOpen(&LoadingMenu) || mm->IsMenuOpen(&Console) || mm->IsMenuOpen(&TutorialMenu)) {
					return 1;
				}
				else if (GetUnpausedMenuOpen()) {
					return 0;
				}
			}
		}
		else {
			g_noMenuPauseController.update();
		}
	}

	return 1;
}

bool CanEquip(Actor* actor, TESForm* item) {
	return !GetPaused() || !(actor == *g_thePlayer && item->IsArmor());
}

void __fastcall EquipManager_EquipItem_Hook(EquipManager* thisPtr, void*, Actor* actor, TESForm* item, BaseExtraList* extraData, SInt32 count, BGSEquipSlot* equipSlot, bool withEquipSound, bool preventEquip, bool showMsg, void* unk) {
	//_MESSAGE("EquipManager_EquipItem_Hook");

	//_MESSAGE("0x%.8x", unk);

	if (actor && item && (CanEquip(actor, item) || !GetInventoryMenuOpen())) {
		//_MESSAGE("EquipItem not queued");

		(thisPtr->*g_originalEquipItem)(actor, item, extraData, count, equipSlot, withEquipSound, preventEquip, showMsg, unk);
	}
	else {
		MenuEquipData* data = new MenuEquipData;
		data->equipManager = thisPtr;
		data->actor = actor;
		data->item = item;
		data->extraData = extraData;
		data->count = count;
		data->equipSlot = equipSlot;
		data->withEquipSound = withEquipSound;
		data->preventEquip = preventEquip;
		data->showMsg = showMsg;
		data->unk = unk;
		MenuEquipManager::GetSingleton()->queueEquipItem(data, 1);
	}
}

bool __fastcall EquipManager_UnequipItem_Hook(EquipManager* thisPtr, void*, Actor* actor, TESForm* item, BaseExtraList* extraData, SInt32 count, BGSEquipSlot* equipSlot, bool unkFlag1, bool preventEquip, bool unkFlag2, bool unkFlag3, void * unk) {
	//_MESSAGE("EquipManager_UnequipItem_Hook");

	/*
	_MESSAGE("0x%.8x", unkFlag1);
	_MESSAGE("0x%.8x", unkFlag2);
	_MESSAGE("0x%.8x", unkFlag3);
	_MESSAGE("0x%.8x", unk);
	*/

	if (actor && item && (CanEquip(actor, item) || !GetInventoryMenuOpen())) {
		//_MESSAGE("UnequipItem not queued");

		return (thisPtr->*g_originalUnequipItem)(actor, item, extraData, count, equipSlot, unkFlag1, preventEquip, unkFlag2, unkFlag3, unk);
	}
	else {
		MenuUnequipData* data = new MenuUnequipData;
		data->equipManager = thisPtr;
		data->actor = actor;
		data->item = item;
		data->extraData = extraData;
		data->count = count;
		data->equipSlot = equipSlot;
		data->unkFlag1 = unkFlag1;
		data->preventEquip = preventEquip;
		data->unkFlag2 = unkFlag2;
		data->unkFlag3 = unkFlag3;
		data->unk = unk;
		MenuEquipManager::GetSingleton()->queueUnequipItem(data, 1);

		return true;
	}
}

typedef void (UIManager::*AddMessageMethod)(StringCache::Ref*, UInt32, void*);
AddMessageMethod g_originalAddMessage;
void __fastcall UIManager_AddMessage_Hook(UIManager* thisPtr, void*, StringCache::Ref* strData, UInt32 msgID, void* objData) {
	_MESSAGE("message: 0x%.8x to %s", msgID, strData->data);

	if (msgID == UIMessage::kMessage_Open) {
		g_noMenuPauseController.postpone(1);
	}

	(thisPtr->*g_originalAddMessage)(strData, msgID, objData);
}

extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info) {
		_MESSAGE("SKSEPlugin_Query begin");

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "NoMenuPause Plugin";
		info->version = 2;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor) {
			_MESSAGE("loaded in editor, marking as incompatible");

			return false;
		}
		else if (skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0) {
			_MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);

			return false;
		}

		_MESSAGE("SKSEPlugin_Query end");

		// supported runtime version
		return true;
	}

#pragma warning(disable: 4102)
	bool SKSEPlugin_Load(const SKSEInterface * skse) {
		_MESSAGE("SKSEPlugin_Load begin");

		__asm {
			push eax;
			mov eax, EQUIPITEM_ORIGINAL;
			mov g_originalEquipItem, eax;
			pop eax;
			push dword ptr 8;
			push EQUIPITEM_JUMP_RETURN;
			push EquipManager_EquipItem_Hook;
			push EquipManager_EquipItem_Addr;
			call WriteRedirectionHook;
			add esp, 0x10;

			jmp EQUIPITEM_SKIP_HOOK;

		EQUIPITEM_ORIGINAL:
			sub esp, 0x18;
			push ebx;
			mov ebx, [esp + 0x20];

		EQUIPITEM_JUMP_RETURN:
			nop;
			nop;
			nop;
			nop;
			nop;

		EQUIPITEM_SKIP_HOOK:
		}

		__asm {
			push eax;
			mov eax, UNEQUIPITEM_ORIGINAL;
			mov g_originalUnequipItem, eax;
			pop eax;
			push dword ptr 5;
			push UNEQUIPITEM_JUMP_RETURN;
			push EquipManager_UnequipItem_Hook;
			push EquipManager_UnequipItem_Addr;
			call WriteRedirectionHook;
			add esp, 0x10;

			jmp UNEQUIPITEM_SKIP_HOOK;

		UNEQUIPITEM_ORIGINAL:
			sub esp, 0x18;
			push ebx;
			push ebp;

		UNEQUIPITEM_JUMP_RETURN:
			nop;
			nop;
			nop;
			nop;
			nop;

		UNEQUIPITEM_SKIP_HOOK:
		}

		//HookNoPauseMenuCreation<'twen'>(&TweenMenu);
		//HookNoPauseMenuCreation<'inv_'>(&InventoryMenu);
		//HookNoPauseMenuCreation<'magi'>(&MagicMenu);
		//HookNoPauseMenuCreation<'fav_'>(&FavoritesMenu);
		//HookNoPauseMenuCreation<'fav_'>(&LockpickingMenu);

		//WriteRelCall(0x00730EE0 + 0xFC, GetFnAddr(ActorProcessManager_UpdateEquipment_Hook));

		__asm {
			push dword ptr 6;
			push NOMENUPAUSE_END_OF_HOOK;
			push NOMENUPAUSE_START_OF_HOOK;
			push hookAddr1;
			call WriteRedirectionHook;
			add esp, 0x10;

			jmp NOMENUPAUSE_SKIP_HOOK;

		NOMENUPAUSE_START_OF_HOOK:
			pushad;
			push ebx;
			call GetNoPause;
			pop ebx;
			cmp eax, ebx;
			popad;
			jz NOMENUPAUSE_END_OF_HOOK;

			cmp[ecx + 0x000000C8], ebx;

		NOMENUPAUSE_END_OF_HOOK:
			nop;
			nop;
			nop;
			nop;
			nop;

		NOMENUPAUSE_SKIP_HOOK:
		}

		__asm {
			push eax;
			mov eax, ADDMESSAGE_ORIGINAL;
			mov g_originalAddMessage, eax;
			pop eax;
			push dword ptr 10;
			push ADDMESSAGE_JUMP_RETURN;
			push UIManager_AddMessage_Hook;
			push UIManager_AddMessage_Addr;
			call WriteRedirectionHook;
			add esp, 0x10;

			jmp ADDMESSAGE_SKIP_HOOK;

		ADDMESSAGE_ORIGINAL:
			push ecx;
			push esi;
			mov esi, ecx;
			mov eax, [esi + 0x000001c8];

		ADDMESSAGE_JUMP_RETURN:
			nop;
			nop;
			nop;
			nop;
			nop;

		ADDMESSAGE_SKIP_HOOK:
		}

		_MESSAGE("SKSEPlugin_Load end");

		return true;
	}
#pragma warning(default: 4102)
};
