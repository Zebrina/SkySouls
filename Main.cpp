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

#include "skse/SafeWrite.h"

#include "MemUtil.h"

using namespace MemUtil;

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
/*
CreateLockpickingMenu	g_lockpickingMenuCreator;
CreateBookMenu			g_bookMenuCreator;
*/

/* hooks */

/*
void __fastcall UIManager_AddMessage_Hook(UIManager* thisPtr, void*, StringCache::Ref* strData, UInt32 msgID, void* objData) {
	if (msgID == UIMessage::kMessage_Open) {
		GamePauseHandler::GetSingleton()->disableTemporary(5);
	}
	_CallMemberFunction(thisPtr, (UIManager_AddMessage)Original_UIManager_AddMessage)(strData, msgID, objData);
}
*/


void __fastcall ActorProcessManager_UpdateEquipment_Hook(ActorProcessManager* thisPtr, void*, Actor* actor) {
	static BSFixedString inventoryMenu = UIStringHolder::GetSingleton()->inventoryMenu;
	static BSFixedString containerMenu = UIStringHolder::GetSingleton()->containerMenu;
	static BSFixedString favoritesMenu = UIStringHolder::GetSingleton()->favoritesMenu;

	MenuManager* menuManager = MenuManager::GetSingleton();

	if (!menuManager->IsMenuOpen(&inventoryMenu) && !menuManager->IsMenuOpen(&containerMenu) && !menuManager->IsMenuOpen(&favoritesMenu)) {
		_CallMemberFunction(thisPtr, Original_ActorProcessManager_UpdateEquipment)(actor);
	}
}



void SkySouls_Messaging_Callback(SKSEMessagingInterface::Message* msg) {
	if (msg->type == SKSEMessagingInterface::kMessage_DataLoaded) {
		// All forms are loaded

		_MESSAGE("kMessage_DataLoaded begin");

		static GamePauseHandler* gamePauseHandler = GamePauseHandler::GetSingleton();

		UIStringHolder* uiStringHolder = UIStringHolder::GetSingleton();

		// Configuration
		constexpr const char* configFileName = "Data\\SKSE\\Plugins\\SkySouls.ini";
		constexpr const char* sectionName = "GamePause";

		if (GetPrivateProfileInt(sectionName, "bDisableGamePauseTweenMenu", 0, configFileName) != 0) {
			gamePauseHandler->registerMenu(uiStringHolder->tweenMenu, g_tweenMenuCreator);
		}
		if (GetPrivateProfileInt(sectionName, "bDisableGamePauseInventoryMenu", 0, configFileName) != 0) {
			gamePauseHandler->registerMenu(uiStringHolder->inventoryMenu, g_inventoryMenuCreator);
		}
		if (GetPrivateProfileInt(sectionName, "bDisableGamePauseContainerMenu", 0, configFileName) != 0) {
			gamePauseHandler->registerMenu(uiStringHolder->containerMenu, g_containerMenuCreator);
		}
		if (GetPrivateProfileInt(sectionName, "bDisableGamePauseMagicMenu", 0, configFileName) != 0) {
			gamePauseHandler->registerMenu(uiStringHolder->magicMenu, g_magicMenuCreator);
		}
		if (GetPrivateProfileInt(sectionName, "bDisableGamePauseFavoritesMenu", 0, configFileName) != 0) {
			gamePauseHandler->registerMenu(uiStringHolder->favoritesMenu, g_favoritesMenuCreator);
		}
		//gamePauseHandler->registerMenu(uiStringHolder->lockpickingMenu, g_lockpickingMenuCreator);
		//gamePauseHandler->registerMenu(uiStringHolder->bookMenu, g_bookMenuCreator);

		__asm {
			_AsmBasicHookPrefix(_GameUpdate, 0x0069cc6e, 6);

			pushad;

			add ecx, 0x94;
			push ecx; // menu stack
			push[ecx + 0x34]; // game pause counter
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

		//WriteOpCode16WithImmediate(0x0069ce24, Cmp_BytePtrImm_BL, gamePauseHandler->getGamePauseOverride()); // projectiles
		//WriteOpCode16WithImmediate(0x0069490a, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride()); // invincible inside menu, die on exit

		// ??
		/*
		WriteOpCode16WithImmediate(0x006c7b55, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0073e241, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x008648d5, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0073f358, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x004c839a, Cmp_DwordPtrImm_EBP, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0086490b, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00a5d9db, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00a5d6c8, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		*/


		/*
		WriteOpCode16WithImmediate(0x0069cca3, Cmp_BytePtrImm_BL, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x006944f0, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00694550, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00694aa5, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0069ce79, Cmp_BytePtrImm_BL, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0069cea0, Cmp_BytePtrImm_BL, gamePauseHandler->getGamePauseOverride());
		WriteOpCode24WithImmediate(0x0069cf39, Movzx_BytePtrImm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0069cf59, Cmp_BytePtrImm_BL, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0069cf80, Cmp_BytePtrImm_BL, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00694680, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0069cfc3, Cmp_BytePtrImm_BL, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00694ad0, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x008d41e0, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00694880, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x006946f0, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0069474f, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x0069484d, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x006910b0, Cmp_BytePtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		*/


		/*
		WriteOpCode16WithImmediate(0x00a5d480, Mov_ECX_DwordPtrImm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00878b80, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x008799b1, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		WriteOpCode16WithImmediate(0x00879abc, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		*/

		// after game pause increment/decrement
		//WriteOpCode16WithImmediate(0x00a5d9e8, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		//WriteOpCode16WithImmediate(0x00a5d6d5, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());


		// when equipping/unequipping armor
		//WriteOpCode16WithImmediate(0x00737acd, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());
		//WriteOpCode16WithImmediate(0x00737bb5, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());




		//WriteOpCode16WithImmediate(0x005d1e03, Cmp_DwordPtrImm_Imm, gamePauseHandler->getGamePauseOverride());

		// disable game pause handler for a few "ticks" after requesting to open a menu
		/*
		__asm {
			_AsmRedirectHookPrefix(_AddMessage, 0x00431b00, 10, UIManager_AddMessage_Hook, Original_UIManager_AddMessage);

			// original code
			push eax;
			push esi;
			mov esi, ecx;
			mov eax, [esi + 0x000001c8];

			_AsmHookSuffix(_AddMessage);
		}
		*/

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
		GamePauseHandler* gamePauseHandler = GamePauseHandler::GetSingleton();
		if (flag) {
			gamePauseHandler->enable();
		}
		else {
			gamePauseHandler->disable();
		}
	}

	/*
	bool DisableMenuGamePause(StaticFunctionTag*, BSFixedString menuName, bool flag) {
		UIStringHolder* uiStringHolder = UIStringHolder::GetSingleton();
		GamePauseHandler* gamePauseHandler = GamePauseHandler::GetSingleton();

		return false;
	}
	*/

	bool RegisterAPI(VMClassRegistry* registry) {
		constexpr const char* modName = "SkySouls";

		registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("IsEnabled", modName, IsEnabled, registry));
		registry->SetFunctionFlags(modName, "IsEnabled", VMClassRegistry::kFunctionFlag_NoWait);
		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("Enable", modName, Enable, registry));
		registry->SetFunctionFlags(modName, "Enable", VMClassRegistry::kFunctionFlag_NoWait);
		/*
		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, BSFixedString, bool>("DisableMenuGamePause", modName, DisableMenuGamePause, registry));
		registry->SetFunctionFlags(modName, "DisableMenuGamePause", VMClassRegistry::kFunctionFlag_NoWait);
		*/

		return true;
	}
};

extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info) {
		_MESSAGE("SKSEPlugin_Query begin");

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "SkySouls Plugin";
		info->version = 1;

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

		g_messaging->RegisterListener(g_pluginHandle, "SKSE", SkySouls_Messaging_Callback);

		g_papyrus->Register(SkySoulsPapyrus::RegisterAPI);

		_MESSAGE("SKSEPlugin_Load end");

		return true;
	}
};

#pragma warning(default : 4731)
