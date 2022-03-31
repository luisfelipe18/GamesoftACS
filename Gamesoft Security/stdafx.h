#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#pragma execution_character_set("utf-8")

#include <Windows.h>
#include <VirtualizerSDK.h>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <detours.h>
#include <vector>
#include <tchar.h>
#include <psapi.h>
#include <atlstr.h>
#include <fstream>
#include "Packet.h"
#include <shellapi.h>
#include <winnetwk.h>
#include <Iphlpapi.h>
#include <locale>
#include "md5.h"
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "detours.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "detours.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mpr.lib")
#pragma comment(lib, "VirtualizerSDK32.lib")

#define __ASSERT(expr, expMessage) \
if(!(expr)) {\
	printf("ERROR-> %s\n%s: %d\n\n", expMessage, __FILE__, __LINE__);\
}

#define ARRAY_SIZE 1024

using namespace std;

enum UserAuthority {
	BANNED = 255,
	USER = 1,
	GAMEMASTER = 0
};

struct ScreenInfo {
	int height;
	int width;
};

struct ProcessInfo {
	int id;
	string name;
	vector<string> windows;
};

struct User
{
	string Nick;
	UserAuthority Authority;
	uint32 KnightCash;
	uint32 NationPoint;
	uint8 Level;
	uint8 RebLevel;
	uint8 ddAc;
	uint8 axeAc;
	uint8 swordAc;
	uint8 maceAc;
	uint8 arrowAc;
	uint8 spearAc;
	int64 HWID;
	vector<string> GPU;
	string CPU;
	ScreenInfo * Screen;
	vector<ProcessInfo> Processes;
	uint32 MAC;
};

enum PUS_CATEGORY
{
	SPECIAL = 1,
	POWER_UP = 2,
	COSTUME = 3,
	BUNDLE = 4
};

struct PUSItem
{
	uint32 sID;
	uint32 Price;
	PUS_CATEGORY Category;
	PUSItem(uint32 sid, uint32 price, PUS_CATEGORY category) {
		sID = sid;
		Price = price;
		Category = category;
	}
};

struct ITEM_DATA
{
	string itemName;
	uint32 itemID;
	uint16 sCount, sDuration;
	ITEM_DATA(string name, uint32 id, uint16 count, uint16 duration)
	{
		itemName = name;
		itemID = id;
		sCount = count;
		sDuration = duration;
	}
};

struct DROP
{
	string itemName;
	uint16 dropRate;
	DROP(string n, uint16 d) {
		itemName = n;
		dropRate = d;
	}
};

struct DROP_RESULT
{
	string mName;
	vector<DROP> drops;
	string mZone;
	DROP_RESULT(string n, vector<DROP> d, string z) {
		mName = n;
		drops = d;
		mZone = z;
	}
};

#define PL_VERSION 1

#define PL_COM 0xA1
#define PL_AUTHINFO 0xA2
#define PL_PROCINFO 0xA3
#define PL_OPEN 0xA4
#define PL_LOG 0xA5
#define PL_ALIVE 0xA6
#define PL_UIINFO 0xA7
#define PL_PUS 0xA8
#define PL_CASHCHANGE 0xA9
#define PL_KESN 0xAA
#define PL_DROP_LIST 0xAB
#define PL_ITEM_PROCESS 0xAC
#define PL_RESET 0xAD

#define KO_PTR_CHR		0x00E61920
#define KO_PTR_DLG		0x00E47878
#define KO_PTR_HWD		0x00CB3E64
#define KO_FLDB			0x00E6191C
#define KO_ITOB			0x00E61754
#define KO_ITEB			0x00E6175C
#define KO_SMMB			0x00000000
#define KO_FMBS			0x004F91A0
#define KO_FPBS			0x004FA110
#define KO_FNCZ			0x00523590
#define KO_FNCB			0x00523700
#define KO_FNSB			0x004FC300
#define KO_FINDITEM		0x004BF960
#define KO_FINDITEM2	0x004BF9C0
#define KO_INFO_MSG		0x006B24F0
#define KO_EXCEPTION	0x009DDD60
#define KO_SH_HOOK		0x004ECABB
#define KO_SH_VALUE		0x00B95760
#define KO_RECV_PTR		0x00A7D987
#define KO_BASE_CON		0x0057DE50
#define KO_BASE_DES		0x0057DF80
#define KO_M_TIMEOUT	0x00B8FE2C
#define KO_PERI_TAK		0x00579560
#define KO_NODC			0x00B95610
#define KO_PTR_OL1		0x00E47854
#define KO_ADR_OL1		0x004DB050
#define KO_ADR_OL2		0x004DF9C0
#define KO_ADR_OL3		0x004E2240
#define KO_ADR_OL4		0x004DB590
#define KO_ADR_OL5		0x004C6660
#define KO_ADR_OL6		0x004CB420
#define KO_ADR_OL7		0x004CFD8C
#define KO_ADR_ROTA1	0x00557060
#define KO_OFF_ZONE		0x00000C30
#define KO_OFF_NAME		0x00000688
#define KO_OFF_NAMELEN	0x00000698
#define KO_OFF_CLASS	0x000006B0
#define KO_OFF_ID		0x00000680
#define KO_OFF_SWIFT	0x000007CC
#define KO_OFF_MP		0x00000000
#define KO_OFF_X		0x000000D8
#define KO_OFF_Y		0x000000E0
#define KO_OFF_Z		0x000000DC
#define KO_OFF_EXP		0x00000BA4
#define KO_OFF_MAXEXP	0x00000BA0
#define KO_OFF_NATION	0x000006A8