#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameAPI.h"
#include "skse/GameMenus.h"

#include "skse/SafeWrite.h"

#define NOMENUPAUSE_HOOK_ADDR 0x0069cc6e

IDebugLog gLog("NoMenuPause.log");

PluginHandle g_pluginHandle = kPluginHandle_Invalid;

// These menus cannot be paused!
BSFixedString MainMenu("Main Menu");
BSFixedString LoadingMenu("Loading Menu");
BSFixedString Console("Console");
BSFixedString TutorialMenu("Tutorial Menu");
BSFixedString TweenMenu("TweenMenu");

// These menus can be paused!
BSFixedString MagicMenu("MagicMenu"); // Will not be paused if accessed through the "tween" menu.
BSFixedString InventoryMenu("InventoryMenu"); // Will not be paused if accessed through the "tween" menu.
BSFixedString ContainerMenu("ContainerMenu");
BSFixedString FavoritesMenu("FavoritesMenu");
BSFixedString BookMenu("Book Menu");
//BSFixedString LockpickingMenu("Lockpicking Menu"); // Does not work :(
BSFixedString BarterMenu("BarterMenu");
BSFixedString TrainingMenu("Training Menu");

bool __fastcall GetNoPause(MenuManager* mm) {
	UInt32 menuStackCount = *(UInt32*)((UInt32)mm + 0x9c);

	if (menuStackCount > 1) {
		if (mm->IsMenuOpen(&MainMenu) || mm->IsMenuOpen(&LoadingMenu) || mm->IsMenuOpen(&Console) || mm->IsMenuOpen(&TutorialMenu) || mm->IsMenuOpen(&TweenMenu)) {
			return false;
		}
		return mm->IsMenuOpen(&MagicMenu) || mm->IsMenuOpen(&InventoryMenu) || mm->IsMenuOpen(&ContainerMenu) || mm->IsMenuOpen(&FavoritesMenu) || mm->IsMenuOpen(&BookMenu) || mm->IsMenuOpen(&BarterMenu) || mm->IsMenuOpen(&TrainingMenu);
	}

	return false;
}

extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info) {
		_MESSAGE("SKSEPlugin_Query begin");

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "NoMenuPause Plugin";
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

		_MESSAGE("SKSEPlugin_Query end");

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse) {
		_MESSAGE("SKSEPlugin_Load begin");

		UInt32 hs, he;
		__asm {
			push eax;
			mov eax, START_OF_HOOK;
			mov hs, eax;
			mov eax, END_OF_HOOK;
			mov he, eax;
			pop eax;

			jmp SKIP_HOOK;

		START_OF_HOOK:
			pushad;
			call GetNoPause;
			cmp eax, 0x1;
			popad;
			jz END_OF_HOOK;

			cmp[ecx + 0x000000C8], ebx;

		END_OF_HOOK:
			nop;
			nop;
			nop;
			nop;
			nop;

		SKIP_HOOK:
		}

		WriteRelJump(NOMENUPAUSE_HOOK_ADDR, hs);
		SafeWrite8(NOMENUPAUSE_HOOK_ADDR + 5, 0x90);
		WriteRelJump(he, NOMENUPAUSE_HOOK_ADDR + 6);

		_MESSAGE("SKSEPlugin_Load end");

		return true;
	}

};
