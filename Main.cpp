/*************************************************
	 ____  _          ____              _
	/ ___|| | ___   _/ ___|  ___  _   _| |___
	\___ \| |/ / | | \___ \ / _ \| | | | / __|
	 ___) |   <| |_| |___) | (_) | |_| | \__ \
	|____/|_|\_\\__, |____/ \___/ \__,_|_|___/
				|___/

*************************************************/

#include "MenuCreation.h"
#include "MenuEquip.h"

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameAPI.h"

#include "skse/GameData.h"
#include "skse/GameExtraData.h"
#include "skse/GameMenus.h"
#include "skse/ScaleformCallbacks.h"

#include "skse/SafeWrite.h"

#include <forward_list>

IDebugLog gLog("SkySouls.log");
PluginHandle g_pluginHandle = kPluginHandle_Invalid;
SKSEMessagingInterface* g_messaging = nullptr;

UInt8* g_staticMenuPauseByte = (UInt8*)0x01b2e85f;

UInt32* g_menuPauseNumRequestsFaked; // Pointer to the variable which controls whether the game should be paused or not.
UInt32 g_menuPauseNumRequestsActual;

constexpr const UInt32* IMenuVTable_MessageBoxMenu	= (UInt32*)0x010e6b34;
constexpr const UInt32* IMenuVTable_TweenMenu		= (UInt32*)0x010e8140;
constexpr const UInt32* IMenuVTable_InventoryMenu	= (UInt32*)0x010e5b90;
constexpr const UInt32* IMenuVTable_MagicMenu		= (UInt32*)0x010e6594;
constexpr const UInt32* IMenuVTable_ContainerMenu	= (UInt32*)0x010e4098;
constexpr const UInt32* IMenuVTable_FavoritesMenu	= (UInt32*)0x010e4e18;
constexpr const UInt32* IMenuVTable_CraftingMenu	= nullptr;
constexpr const UInt32* IMenuVTable_BarterMenu		= nullptr;
constexpr const UInt32* IMenuVTable_TrainingMenu	= nullptr;
constexpr const UInt32* IMenuVTable_LockpickingMenu	= (UInt32*)0x010e6308;
constexpr const UInt32* IMenuVTable_BookMenu		= (UInt32*)0x010e3aa4;
constexpr const UInt32* IMenuVTable_MapMenu			= (UInt32*)0x010e95b4;
constexpr const UInt32* IMenuVTable_StatsMenu		= (UInt32*)0x010e7ad4;
constexpr const UInt32* IMenuVTable_CursorMenu		= (UInt32*)0x010e4bd4;

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
BSFixedString CursorMenu("Cursor Menu");

#define GetObjectVTable(ptr) (*(UInt32**)(ptr))

template<typename T>
T GetUndefinedMethodPtr(unsigned int addr) {
	union AddrToPtr {
		unsigned int addr;
		T ptr;
	};

	AddrToPtr atp;
	atp.addr = addr;
	return atp.ptr;
}
#define CallUndefinedMemberFunction(thisPtr, method) ((thisPtr)->*(method))

void __stdcall WriteRedirectionHook(UInt32 targetOfHook, UInt32 sourceBegin, UInt32 sourceEnd, UInt32 asmSegSize) {
	WriteRelJump(targetOfHook, sourceBegin);

	for (int i = 5; i < asmSegSize; ++i) {
		SafeWrite8(targetOfHook + i, 0x90); // nop
	}

	WriteRelJump(sourceEnd, targetOfHook + asmSegSize);
}

#define AsmSimpleHookBegin(id, addr, retnOffset)		\
		__asm push dword ptr retnOffset					\
		__asm push NOMENUPAUSE_##id##_END_OF_HOOK		\
		__asm push NOMENUPAUSE_##id##_START_OF_HOOK		\
		__asm push addr									\
		__asm call WriteRedirectionHook					\
		__asm add esp, 0x10								\
		__asm jmp NOMENUPAUSE_##id##_SKIP_HOOK			\
	__asm NOMENUPAUSE_##id##_START_OF_HOOK:

#define AsmRedirectHookBegin(id, addr, retnOffset, newFunc, oldFuncPtr)	\
		__asm push eax													\
		__asm mov eax, NOMENUPAUSE_##id##_ORIGINAL_CODE					\
		__asm mov oldFuncPtr, eax										\
		__asm pop eax													\
		__asm push dword ptr retnOffset									\
		__asm push NOMENUPAUSE_##id##_END_OF_HOOK						\
		__asm push newFunc												\
		__asm push addr													\
		__asm call WriteRedirectionHook									\
		__asm add esp, 0x10												\
		__asm jmp NOMENUPAUSE_##id##_SKIP_HOOK							\
	__asm NOMENUPAUSE_##id##_ORIGINAL_CODE:

#define AsmHookEnd(id)									\
	__asm NOMENUPAUSE_##id##_END_OF_HOOK:				\
		__asm nop										\
		__asm nop										\
		__asm nop										\
		__asm nop										\
		__asm nop										\
	__asm NOMENUPAUSE_##id##_SKIP_HOOK:

bool __stdcall IsMenuInstanceUnpaused(IMenu* menu) {
	UInt32* vtbl = GetObjectVTable(menu);
	if (vtbl == IMenuVTable_InventoryMenu ||
		vtbl == IMenuVTable_MagicMenu ||
		vtbl == IMenuVTable_ContainerMenu ||
		vtbl == IMenuVTable_FavoritesMenu ||
		vtbl == IMenuVTable_BookMenu ||
		vtbl == IMenuVTable_LockpickingMenu) {
		return true;
	}
	return false;
}
bool __stdcall IsMenuUnpaused(StringCache::Ref& menuName) {
	return menuName == InventoryMenu ||
		menuName == MagicMenu ||
		menuName == ContainerMenu ||
		menuName == FavoritesMenu ||
		menuName == LockpickingMenu ||
		menuName == BookMenu;
}

class IDelayedProcess {
public:
	virtual void execute() = 0;
	virtual void cleanUp() { delete this; }
};

class NoMenuPauseController {
private:
	class Entry {
	public:
		struct RemoveIfPred {
			SInt32 counter;

			RemoveIfPred(SInt32 _counter) :
				counter(_counter) {
			}

			bool operator()(Entry& entry) {
				return entry.test(counter);
			}
		};

		Entry(IDelayedProcess* _process, SInt32 _delayBefore, SInt32 _delayAfter) :
			process(_process), delayBefore(_delayBefore), delayAfter(_delayAfter) {
		}
		Entry(Entry&) = delete;
		Entry(Entry&& other) :
			process(other.process), delayBefore(other.delayBefore), delayAfter(other.delayAfter) {
			other.process = nullptr;
		}

		bool test(SInt32 counter) {
			if (counter >= delayBefore) {
				if (process) {
					process->execute();
					process->cleanUp();
					process = nullptr;
				}
				return counter >= (delayBefore + delayAfter);
			}
			return false;
		}

	private:
		IDelayedProcess* process;
		SInt32 delayBefore;
		SInt32 delayAfter;
	};

public:
	static NoMenuPauseController& GetSingleton() {
		static NoMenuPauseController instance;
		return instance;
	}

	NoMenuPauseController() = default;
	NoMenuPauseController(const NoMenuPauseController&) = default;

	operator bool() const {
		return processes.empty();
	}

	void update() {
		if (!processes.empty()) {
			processes.remove_if(Entry::RemoveIfPred(counter));
			++counter;
		}
	}

	void queueProcess(IDelayedProcess* process, SInt32 delayBefore, SInt32 delayAfter) {
		if (processes.empty()) {
			counter = 0;
		}
		processes.emplace_front(process, delayBefore, delayAfter);
	}

private:
	std::forward_list<Entry> processes;
	SInt32 counter;
};

NoMenuPauseController g_noMenuPauseController;

/* hooks */

bool __stdcall PreDefaultGameProcessing() {
	g_noMenuPauseController.update();
	return g_noMenuPauseController;
}

typedef void (UIManager::*UIManager_AddMessage)(StringCache::Ref* strData, UInt32 msgID, void* objData);
UIManager_AddMessage Original_UIManager_AddMessage;

class UIManagerAddMessageProcess : public IDelayedProcess {
public:
	UIManagerAddMessageProcess(UIManager* _uiManager, StringCache::Ref* _strData, UInt32 _msgID, void* _objData) :
		uiManager(_uiManager), strData(_strData), msgID(_msgID), objData(_objData) {}

	virtual void execute() {
		CallUndefinedMemberFunction(uiManager, Original_UIManager_AddMessage)(strData, msgID, objData);
	}

	static void __fastcall UIManager_AddMessage_Hook(UIManager* thisPtr, void*, StringCache::Ref* strData, UInt32 msgID, void* objData) {
		if (msgID == UIMessage::kMessage_Open && g_menuPauseNumRequestsActual == 0 && strData && (*strData == LockpickingMenu || *strData == BookMenu)) {
			g_noMenuPauseController.queueProcess(new UIManagerAddMessageProcess(thisPtr, strData, msgID, objData), 10, 10);
			_MESSAGE("AddMessage queued");
		}
		else {
			CallUndefinedMemberFunction(thisPtr, Original_UIManager_AddMessage)(strData, msgID, objData);
		}
	}

private:
	UIManager* uiManager;
	StringCache::Ref* strData;
	UInt32 msgID;
	void* objData;
};

typedef void (IMenu::*IMenu_Accept)(FxDelegateHandler::CallbackProcessor* processor);
IMenu_Accept Original_IMenu_Accept;

class IMenuAcceptProcess : public IDelayedProcess {
public:
	IMenuAcceptProcess(IMenu* _menu, FxDelegateHandler::CallbackProcessor* _processor) :
		menu(_menu), processor(_processor) {}

	virtual void execute() {
		CallUndefinedMemberFunction(menu, Original_IMenu_Accept)(processor);
	}

	static void __fastcall IMenu_Accept_Hook(IMenu* thisPtr, void*, FxDelegateHandler::CallbackProcessor* processor) {
		if (IsMenuInstanceUnpaused(thisPtr) && g_menuPauseNumRequestsActual > 0) {
			g_noMenuPauseController.queueProcess(new IMenuAcceptProcess(thisPtr, processor), 1, 1);
		}
		else {
			CallUndefinedMemberFunction(thisPtr, Original_IMenu_Accept)(processor);
		}
	}

private:
	IMenu* menu;
	FxDelegateHandler::CallbackProcessor* processor;
};

void SkySouls_Messaging_Callback(SKSEMessagingInterface::Message* msg) {
	if (msg->type == SKSEMessagingInterface::kMessage_DataLoaded) {
		// All forms are loaded

		_MESSAGE("kMessage_DataLoaded begin");

		g_menuPauseNumRequestsFaked = (UInt32*)((UInt32)MenuManager::GetSingleton() + 0xc8);
		g_menuPauseNumRequestsActual = *g_menuPauseNumRequestsFaked;

		//WriteNoMenuPauseComplexHook(0x0069cc6e, ecx, ebx);

		__asm {
			AsmSimpleHookBegin(PreDefaultGameProcessing, 0x0069cc6e, 6);

			pushad;
			call PreDefaultGameProcessing;
			test al, al;
			popad;

			jnz DelayedProcessing;

			/* original code */
			cmp[ecx + 0x000000C8], ebx;
			jmp ReturnToDefaultProcessing;

		DelayedProcessing:
			cmp g_menuPauseNumRequestsActual, ebx;

		ReturnToDefaultProcessing:
			AsmHookEnd(PreDefaultGameProcessing);
		}

		__asm {
			AsmSimpleHookBegin(SetMenuPause, 0x00a5d917, 6);

			pushad;

			push eax;
			call IsMenuInstanceUnpaused;
			test al, al;

			popad;

			jnz IgnoreSetMenuPauseRequest;

			/* original code */
			add dword ptr[esi + 0x000000c8], 1;

		IgnoreSetMenuPauseRequest:
			add g_menuPauseNumRequestsActual, ecx;

			AsmHookEnd(SetMenuPause);
		}

		__asm {
			AsmSimpleHookBegin(ClearMenuPause, 0x00a5d5f4, 6);

			pushad;

			push edi;
			call IsMenuInstanceUnpaused;
			test al, al;

			popad;

			jnz IgnoreClearMenuPauseRequest;

			/* original code */
			add dword ptr[esi + 0x000000c8], -1;

		IgnoreClearMenuPauseRequest:
			add g_menuPauseNumRequestsActual, eax;

			AsmHookEnd(ClearMenuPause);
		}

		// Disable "player controls" inside unpaused menus.
		SafeWrite16(0x00772a5a, 0x3d83); // cmp [reg + offset], 0 -> cmp [ptr], 0
		SafeWrite32(0x00772a5a + 2, (UInt32)&g_menuPauseNumRequestsActual);

		// Prevent favorite hotkeys being usable inside menus.
		SafeWrite16(0x00879abc, 0x3d83); // cmp [reg + offset], 0 -> cmp [ptr], 0
		SafeWrite32(0x00879abc + 2, (UInt32)&g_menuPauseNumRequestsActual);

		// Fix for lockpicking/book menus not opening.
		__asm {
			AsmRedirectHookBegin(LockpickingBookMenuFix, 0x00431b00, 10, UIManagerAddMessageProcess::UIManager_AddMessage_Hook, Original_UIManager_AddMessage);

			/* original code */
			push ecx;
			push esi;
			mov esi, ecx;
			mov eax, [esi + 0x000001c8];

			AsmHookEnd(LockpickingBookMenuFix);
		}

		//WriteRelCall(0x00a5c192, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));
		//WriteRelCall(0x00a5c266, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));
		//WriteRelCall(0x0069495c, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));
		//WriteRelCall(0x00a5c192, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));
		//WriteRelCall(0x00a5c266, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));
		//WriteRelCall(0x0069495c, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));
		//WriteRelCall(0x00a5c192, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));
		//WriteRelCall(0x00a5c266, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));
		//WriteRelCall(0x0069495c, GetFnAddr(UIManager_AddMessage_DelayedProcess::UIManager_AddMessage_Hook));


		// Fix for equip/unequip in inventory menus.
		WriteRelCall(0x0086a4c9, GetFnAddr(IMenuAcceptProcess::IMenu_Accept_Hook));

		_MESSAGE("kMessage_DataLoaded end");
	}
}

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

		_MESSAGE("SKSEPlugin_Query end");

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse) {
		_MESSAGE("SKSEPlugin_Load begin");

		__asm {
			/* initialize static function pointers */

			push eax;

			mov eax, 0x00869db0;
			mov Original_IMenu_Accept, eax;

			pop eax;
		}

		HookNoPauseMenuCreation<'bart'>(&BarterMenu);
		HookNoPauseMenuCreation<'crft'>(&CraftingMenu);
		HookNoPauseMenuCreation<'trng'>(&TrainingMenu);

		g_messaging->RegisterListener(g_pluginHandle, "SKSE", SkySouls_Messaging_Callback);

		_MESSAGE("SKSEPlugin_Load end");

		return true;
	}
};