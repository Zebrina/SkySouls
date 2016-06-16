#pragma once

#include <queue>

class EquipManager;
class Actor;
class TESForm;
class BaseExtraList;
class BGSEquipSlot;

typedef void (EquipManager::*EquipItemMethod)(Actor*, TESForm*, BaseExtraList*, SInt32 count, BGSEquipSlot*, bool, bool, bool, void*);
typedef bool (EquipManager::*UnequipItemMethod)(Actor*, TESForm*, BaseExtraList*, SInt32 count, BGSEquipSlot*, bool, bool, bool, bool, void*);

struct MenuEquipData {
	UInt32 equipAt;
	EquipManager* equipManager;
	Actor* actor;
	TESForm* item;
	BaseExtraList* extraData;
	SInt32 count;
	BGSEquipSlot* equipSlot;
	bool withEquipSound;
	bool preventEquip;
	bool showMsg;
	void* unk;
};

struct MenuUnequipData {
	UInt32 unequipAt;
	EquipManager* equipManager;
	Actor* actor;
	TESForm* item;
	BaseExtraList* extraData;
	SInt32 count;
	BGSEquipSlot* equipSlot;
	bool unkFlag1;
	bool preventEquip;
	bool unkFlag2;
	bool unkFlag3;
	void* unk;
};

class MenuEquipManager {
private:
	struct MenuEquipQueuePriority {
		bool operator()(MenuEquipData* a, MenuEquipData* b) {
			return a->equipAt < b->equipAt;
		}
	};
	struct MenuUnequipQueuePriority {
		bool operator()(MenuUnequipData* a, MenuUnequipData* b) {
			return a->unequipAt < b->unequipAt;
		}
	};

public:
	~MenuEquipManager() = default;

	static MenuEquipManager* GetSingleton();

	void queueEquipItem(MenuEquipData* data, UInt32 ticks);
	void queueUnequipItem(MenuUnequipData* data, UInt32 ticks);

	bool processEquipQueue(EquipItemMethod originalEquipItem);
	bool processUnequipQueue(UnequipItemMethod originalUnequipItem);

private:
	UInt32 internalCounter;
	std::priority_queue<MenuEquipData*, std::vector<MenuEquipData*>, MenuEquipQueuePriority> equipQueue;
	std::priority_queue<MenuUnequipData*, std::vector<MenuUnequipData*>, MenuUnequipQueuePriority> unequipQueue;

	MenuEquipManager() : internalCounter(0u) {
	}
	MenuEquipManager(const MenuEquipManager&) = delete;
	MenuEquipManager(MenuEquipManager&&) = delete;
};