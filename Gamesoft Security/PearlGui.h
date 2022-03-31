#pragma once
#include "stdafx.h" 
#include <cstdint>
#include <memory>
#include <thread>
#include <string>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "Pearl Engine.h"
void UIMain(PearlEngine* e);
void UpdateUI(string nick, uint32 np, uint32 cash, uint32 bonuscash, uint8 level, uint8 reblevel, uint8 dd, uint8 axe, uint8 sword, uint8 mace, uint8 spear, uint8 bow, vector<string> _notices, uint32 _moneyreq);
void UpdateUICashInfo(uint32 cash, uint32 bonuscash);
void UpdateInventory(vector<ITEM_DATA> inv);
void LoadPUS(vector<PUSItem> _data);
void ReleaseUI();