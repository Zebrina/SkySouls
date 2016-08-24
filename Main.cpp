/*************************************************
	 ____  _          ____              _
	/ ___|| | ___   _/ ___|  ___  _   _| |___
	\___ \| |/ / | | \___ \ / _ \| | | | / __|
	 ___) |   <| |_| |___) | (_) | |_| | \__ \
	|____/|_|\_\\__, |____/ \___/ \__,_|_|___/
				|___/

*************************************************/

#pragma warning(disable : 4731)

#include "GamePauseHandler.h"
#include "MenuCreator.h"

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameAPI.h"

#include "skse/GameThreads.h"
#include "skse/GameData.h"
#include "skse/GameExtraData.h"
#include "skse/GameMenus.h"

#include "skse/PapyrusVM.h"
#include "skse/PapyrusNativeFunctions.h"

#include "skse/ScaleformAPI.h"

#include "skse/SafeWrite.h"

#include "SKSEMemUtil/SKSEMemUtil.h"

using namespace SKSEMemUtil;

typedef void (UIManager::*UIManager_AddMessage)(StringCache::Ref* strData, UInt32 msgID, void* objData);
typedef void (ActorProcessManager::*ActorProcessManager_UpdateEquipment)(Actor*);

UIManager_AddMessage Original_UIManager_AddMessage = nullptr;
ActorProcessManager_UpdateEquipment Original_ActorProcessManager_UpdateEquipment = nullptr;

IDebugLog gLog("SkySouls.log");
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

SKSEMessagingInterface*	g_messaging = nullptr;
SKSETaskInterface*		g_task = nullptr;
SKSEPapyrusInterface*	g_papyrus = nullptr;

CreateTweenMenu			g_tweenMenuCreator;
CreateInventoryMenu		g_inventoryMenuCreator;
CreateContainerMenu		g_containerMenuCreator;
CreateMagicMenu			g_magicMenuCreator;
CreateFavoritesMenu		g_favoritesMenuCreator;
CreateLockpickingMenu	g_lockpickingMenuCreator;
CreateBookMenu			g_bookMenuCreator;
CreateBarterMenu		g_barterMenuCreator;
CreateGiftMenu			g_giftMenuCreator;
CreateTrainingMenu		g_trainingMenuCreator;
CreateMessageBoxMenu	g_messageBoxMenuCreator;

/* fixes */

UInt32 g_lockpickingAndBookMenuOpenDelay = 0;

/* hooks */

void __fastcall UIManager_AddMessage_Hook(UIManager* thisPtr, void*, StringCache::Ref* strData, UInt32 msgID, void* objData) {
	static BSFixedString lockpickingMenu = UIStringHolder::GetSingleton()->lockpickingMenu;
	static BSFixedString bookMenu = UIStringHolder::GetSingleton()->bookMenu;

	if ((msgID == UIMessage::kMessage_Open || msgID == UIMessage::kMessage_Close) &&
		(*strData == lockpickingMenu || *strData == bookMenu)) {
		GamePauseHandler::GetSingleton()->disableTemporary(2);
	}
	_CallMemberFunction(thisPtr, (UIManager_AddMessage)Original_UIManager_AddMessage)(strData, msgID, objData);
}

void __fastcall ActorProcessManager_UpdateEquipment_Hook(ActorProcessManager* thisPtr, void*, Actor* actor) {
	static BSFixedString inventoryMenu = UIStringHolder::GetSingleton()->inventoryMenu;
	static BSFixedString containerMenu = UIStringHolder::GetSingleton()->containerMenu;
	static BSFixedString favoritesMenu = UIStringHolder::GetSingleton()->favoritesMenu;

	MenuManager* menuManager = MenuManager::GetSingleton();

	if (!menuManager->IsMenuOpen(&inventoryMenu) && !menuManager->IsMenuOpen(&containerMenu) && !menuManager->IsMenuOpen(&favoritesMenu)) {
		_CallMemberFunction(thisPtr, Original_ActorProcessManager_UpdateEquipment)(actor);
	}
}

typedef void (IMenu::*IMenu_Accept)(FxDelegateHandler::CallbackProcessor*);
typedef UInt32(IMenu::*IMenu_ProcessUnkData1)(void*);

/*
IMenu_Accept Original_InventoryMenu_Accept = nullptr;
IMenu_ProcessUnkData1 Original_InventoryMenu_ProcessUnkData1 = nullptr;

void __fastcall InventoryMenu_Accept_Hook(IMenu* thisPtr, void*, FxDelegateHandler::CallbackProcessor* processor) {
	_MESSAGE("YES WE DID IT!");

	_CallMemberFunction(thisPtr, Original_InventoryMenu_Accept)(processor);
}
UInt32 __fastcall InventoryMenu_ProcessUnkData1_Hook(IMenu* thisPtr, void*, void* data) {
	_MESSAGE("NOW WE DID IT AGAIN!");

	return _CallMemberFunction(thisPtr, Original_InventoryMenu_ProcessUnkData1)(data);
}
*/

void SkySouls_Messaging_Callback(SKSEMessagingInterface::Message* msg) {
	if (msg->type == SKSEMessagingInterface::kMessage_DataLoaded) {
		// All forms are loaded

		_MESSAGE("kMessage_DataLoaded begin");

		static GamePauseHandler* gamePauseHandler = GamePauseHandler::GetSingleton();

		UIStringHolder* uiStringHolder = UIStringHolder::GetSingleton();

		// register menus
		gamePauseHandler->registerMenu(uiStringHolder->tweenMenu, g_tweenMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->inventoryMenu, g_inventoryMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->containerMenu, g_containerMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->magicMenu, g_magicMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->favoritesMenu, g_favoritesMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->lockpickingMenu, g_lockpickingMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->bookMenu, g_bookMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->barterMenu, g_barterMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->giftMenu, g_giftMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->trainingMenu, g_trainingMenuCreator);
		gamePauseHandler->registerMenu(uiStringHolder->messageBoxMenu, g_messageBoxMenuCreator);

		__asm {
			_AsmBasicHookPrefix(_GameUpdate, 0x0069cc6e, 6);

			pushad;

			add ecx, 0x94;
			push ecx; // menu stack
			add ecx, 0x34;
			push ecx; // game pause counter (as pointer/reference)
			mov ecx, gamePauseHandler;
			call GamePauseHandler::update;

			popad;

			// replaced code
			cmp[ecx + 0xc8], ebx;

			_AsmHookSuffix(_GameUpdate);
		}

		//WriteOpCode16WithImmediate(0x0056ef86, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride()); // ??
		WriteOpCode16WithImmediate(0x00772a5a, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride()); // player controls
		WriteOpCode16WithImmediate(0x008792ea, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride()); // menu controls

		// disable game pause handler for a few "ticks" after requesting to open a menu
		// solves lockpicking and book menu not showing up
		__asm {
			_AsmRedirectHookPrefix(_AddMessage, 0x00431b00, 10, UIManager_AddMessage_Hook, Original_UIManager_AddMessage);

			// original code
			push eax;
			push esi;
			mov esi, ecx;
			mov eax, [esi + 0x000001c8];

			_AsmHookSuffix(_AddMessage);
		}

		// prevent worn armor from updating inside menus
		__asm {
			_AsmRedirectHookPrefix(_UpdateEquipment, 0x7031a0, 6, ActorProcessManager_UpdateEquipment_Hook, Original_ActorProcessManager_UpdateEquipment);

			// original code
			sub esp, 0x10;
			push ebp;
			mov ebp, ecx;

			_AsmHookSuffix(_UpdateEquipment);
		}

		_MESSAGE("kMessage_DataLoaded end");
	}
}

namespace SkySoulsPapyrus {
	bool IsEnabled(StaticFunctionTag*) {
		return GamePauseHandler::GetSingleton()->isEnabled();
	}
	void Enable(StaticFunctionTag*, bool flag) {
		GamePauseHandler::GetSingleton()->enable(flag);
	}

	void SetMenuPauseBehavior(StaticFunctionTag*, BSFixedString menuName, UInt32 pauseBehavior) {
		if (pauseBehavior >= MenuCreator::GamePauseBehavior::Never && pauseBehavior < MenuCreator::GamePauseBehavior::NumBehaviorTypes) {
			UIStringHolder* uiStringHolder = UIStringHolder::GetSingleton();
			if (menuName == uiStringHolder->tweenMenu) {
				g_tweenMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->inventoryMenu) {
				g_inventoryMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->containerMenu) {
				g_containerMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->magicMenu) {
				g_magicMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->favoritesMenu) {
				g_favoritesMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->lockpickingMenu) {
				g_lockpickingMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->bookMenu) {
				g_bookMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->barterMenu) {
				g_barterMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->giftMenu) {
				g_giftMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->trainingMenu) {
				g_trainingMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
			else if (menuName == uiStringHolder->messageBoxMenu) {
				g_messageBoxMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)pauseBehavior;
			}
		}
	}

	bool RegisterAPI(VMClassRegistry* registry) {
		constexpr const char* modName = "SkySouls";

		registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("IsEnabled", modName, IsEnabled, registry));
		registry->SetFunctionFlags(modName, "IsEnabled", VMClassRegistry::kFunctionFlag_NoWait);
		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("Enable", modName, Enable, registry));
		registry->SetFunctionFlags(modName, "Enable", VMClassRegistry::kFunctionFlag_NoWait);
		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, UInt32>("SetMenuPauseBehavior", modName, SetMenuPauseBehavior, registry));
		registry->SetFunctionFlags(modName, "SetMenuPauseBehavior", VMClassRegistry::kFunctionFlag_NoWait);

		return true;
	}
};

extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info) {
		_MESSAGE("SKSEPlugin_Query begin");

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "SkySouls Plugin";
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

		// get the messaging interface and query its version
		g_messaging = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
		if (!g_messaging) {
			_MESSAGE("\tcouldn't get messaging interface");

			return false;
		}
		if (g_messaging->kInterfaceVersion < SKSEMessagingInterface::kInterfaceVersion) {
			_MESSAGE("\tmessaging interface too old (%d expected %d)", g_messaging->interfaceVersion, SKSEMessagingInterface::kInterfaceVersion);

			return false;
		}

		// get the messaging interface and query its version
		g_task = (SKSETaskInterface*)skse->QueryInterface(kInterface_Task);
		if (!g_task) {
			_MESSAGE("\tcouldn't get task interface");

			return false;
		}
		if (g_task->kInterfaceVersion < SKSETaskInterface::kInterfaceVersion) {
			_MESSAGE("\task interface too old (%d expected %d)", g_task->interfaceVersion, SKSETaskInterface::kInterfaceVersion);

			return false;
		}

		// get the papyrus interface and query its version
		g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);
		if (!g_papyrus) {
			_MESSAGE("\tcouldn't get papyrus interface");

			return false;
		}
		if (g_papyrus->interfaceVersion < SKSEPapyrusInterface::kInterfaceVersion) {
			_MESSAGE("\tpapyrus interface too old (%d expected %d)", g_papyrus->interfaceVersion, SKSEPapyrusInterface::kInterfaceVersion);

			return false;
		}

		_MESSAGE("SKSEPlugin_Query end");

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse) {
		_MESSAGE("SKSEPlugin_Load begin");

		// Configuration
		constexpr const char* configFile = "Data\\SKSE\\Plugins\\SkySouls.ini";
		constexpr const char* gamePauseSection = "GamePause";
		constexpr const char* fixesSection = "Fixes";

		g_lockpickingAndBookMenuOpenDelay = GetPrivateProfileInt(fixesSection, "iLockpickingAndBookMenuOpenDelay", 2, configFile);

		g_tweenMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseTweenMenu", 0, configFile);
		g_inventoryMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseInventoryMenu", 0, configFile);
		g_containerMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseContainerMenu", 0, configFile);
		g_magicMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseMagicMenu", 0, configFile);
		g_favoritesMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseFavoritesMenu", 0, configFile);
		g_lockpickingMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseLockpickingMenu", 0, configFile);
		g_bookMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseBookMenu", 0, configFile);
		g_barterMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseBarterMenu", 0, configFile);
		g_giftMenuCreator.pauseBehavior = (MenuCreator::GamePauseBehavior)GetPrivateProfileInt(gamePauseSection, "iDisableGamePauseGiftMenu", 0, configFile);



		//Original_InventoryMenu_Accept = WriteVTableHook(0x010e5b90, 1, InventoryMenu_Accept_Hook);
		//Original_InventoryMenu_ProcessUnkData1 = WriteVTableHook(0x010e5b90, 4, InventoryMenu_ProcessUnkData1_Hook);



		g_messaging->RegisterListener(g_pluginHandle, "SKSE", SkySouls_Messaging_Callback);

		g_papyrus->Register(SkySoulsPapyrus::RegisterAPI);

		_MESSAGE("SKSEPlugin_Load end");

		return true;
	}
};

#pragma warning(default : 4731)
