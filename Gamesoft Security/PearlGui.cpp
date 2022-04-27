#include "PearlGui.h"
#include <codecvt>
#include <iterator>
#include "UIBackground.h"
#include "UINoitem.h"
#include "UITool.h"
#include "imgui/imgui_internal.h"
#include "UpgradeImages.h"


PearlEngine * _Engine = nullptr;

static bool tools = false;

using namespace std;

static string Nick = "<unknown>";
static uint32 KnightCash = 0;
static uint32 KnightCashBonus = 0;
static uint32 NationPoint = 0;
static uint8 Level = 0;
static uint8 RebLevel = 0;
static uint8 ddAc = 0;
static uint8 axeAc = 0;
static uint8 swordAc = 0;
static uint8 maceAc = 0;
static uint8 arrowAc = 0;
static uint8 spearAc = 0;
static int moneyreq = 0;

static vector<string> notices;
//Inventory
static vector<ITEM_DATA> inventory;
//PUS
static vector<PUSItem> pusItems;
static vector<PUSItem> specialItems;
static vector<PUSItem> powerItems;
static vector<PUSItem> costumeItems;
static vector<PUSItem> bundles;
//Static PUS Data
struct ItemData {
	uint32 id;
	char* name;
	char* description;
	PDIRECT3DTEXTURE9 icon;
	ItemData(uint32 _id, char* _name, char* _description) {
		icon = NULL;
		id = _id;
		name = _name;
		description = _description;
	}
};
extern string strToLower(string str);
ItemData getItemDataByID(uint32 id, vector<ItemData> data) {
	for (ItemData i : data)
		if (i.id == id)
			return i;
	return ItemData(0, (char*)"<unknown>", (char*)"<unknown>");
}

static vector<ItemData> itemData;

#define HOOK(func,addy)	o##func = (t##func)DetourFunction((PBYTE)addy,(PBYTE)hk##func)
#define D3D_RELEASE(D3D_PTR) if( D3D_PTR ){ D3D_PTR->Release(); D3D_PTR = NULL; }
#define ES	0
#define DIP	1
#define RES 2
DWORD VTable[3] = { 0 };
DWORD D3DEndScene;
DWORD D3DReset;
DWORD D3DDrawIndexedPrimitive;
DWORD WINAPI VirtualMethodTableRepatchingLoopToCounterExtensionRepatching(LPVOID);
PDWORD Direct3D_VMTable = NULL;

HRESULT WINAPI Direct3DCreate9_VMTable(VOID);
HRESULT WINAPI hkReset(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT(WINAPI* tEndScene)(LPDIRECT3DDEVICE9 LeHaxzor);
typedef HRESULT(WINAPI* DrawIndexedPrimitive_)(LPDIRECT3DDEVICE9 Device, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
typedef HRESULT(WINAPI* tDrawIndexedPrimitive)(LPDIRECT3DDEVICE9 LeHaxzor, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
typedef HRESULT(WINAPI* tReset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
tReset oReset;
DrawIndexedPrimitive_ oDrawIndexedPrimitive = NULL;
tEndScene oEndScene;


typedef HRESULT(WINAPI* mySendMessage) (HWND, UINT, WPARAM, LPARAM);
mySendMessage origSendMessage;

inline std::string toMultiByte(UINT codePage, const wchar_t* wstr, int size /*= -1*/)
{
	std::string str;
	if (size < 0)
	{
		size = (int)wcslen(wstr);
	}
	int bytesNeed = WideCharToMultiByte(codePage, NULL, wstr, size, NULL, 0, NULL, FALSE);
	str.resize(bytesNeed);
	WideCharToMultiByte(codePage, NULL, wstr, size, const_cast<char*>(str.c_str()), bytesNeed, NULL, FALSE);
	return str;
}


inline std::string toUtf8(const wchar_t* wstr, int size /*= -1*/)
{
	return toMultiByte(CP_UTF8, wstr, size);
}

inline std::string toUtf8(const std::wstring& str)
{
	return toUtf8(str.c_str(), str.length());
}

HRESULT WINAPI hkDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
	//DIP
	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
}


static bool lowPower = false;

WNDPROC oWndProc;
LRESULT oSndMessage;


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

typedef SHORT(WINAPI* pfnGetAsyncKeyState)(int vKey);
typedef BOOL(WINAPI* pfnGetKeyboardState)(__out_ecount(256) PBYTE lpKeyState);
typedef UINT(WINAPI* pfnGetRawInputData)(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);
typedef UINT(WINAPI* pfnGetRawInputBuffer)(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader);
pfnGetAsyncKeyState T_GetAsyncKeyState = nullptr;
pfnGetAsyncKeyState T_GetKeyState = nullptr;
pfnGetKeyboardState T_GetKeyboardState = nullptr;
pfnGetRawInputData T_GetRawInputData = nullptr;
pfnGetRawInputBuffer T_GetRawInputBuffer = nullptr;


bool disableGameMouse[2] = { false, false };
static bool activeBoxes[3] = { false,false,false };

SHORT WINAPI H_GetAsyncKeyState(__in int vKey) {
	for(int i = 0; i < sizeof(disableGameMouse) / sizeof(disableGameMouse[0]); i++) 
		if (dGameMouse || (disableGameMouse[i] && (vKey == VK_LBUTTON || vKey == VK_RBUTTON || vKey == VK_MBUTTON || vKey == VK_XBUTTON1 || vKey == VK_XBUTTON2 || vKey == VK_CANCEL || vKey == VK_SCROLL)) )
			return 0;
	for (int i = 0; i < sizeof(activeBoxes) & sizeof(activeBoxes[0]); i++)
		if (activeBoxes[i] == true)
			return 0;
	if (dGameKey)
		return 0;
	return T_GetAsyncKeyState(vKey);
}

SHORT WINAPI H_GetKeyState(__in int vKey) {
	for (int i = 0; i < sizeof(activeBoxes) & sizeof(activeBoxes[0]); i++)
		if (activeBoxes[i] == true)
			return 0;
	if (dGameKey)
		return 0;
	return T_GetKeyState(vKey);
}

BOOL WINAPI H_GetKeyboardState(__out_ecount(256) PBYTE lpKeyState) {
	for (int i = 0; i < sizeof(activeBoxes) & sizeof(activeBoxes[0]); i++)
		if (activeBoxes[i] == true)
			return 0;
	if (dGameKey)
		return 0;
	return T_GetKeyboardState(lpKeyState);
}

BOOL WINAPI H_GetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) {
	for (int i = 0; i < sizeof(activeBoxes) & sizeof(activeBoxes[0]); i++)
		if (activeBoxes[i] == true)
			return 0;
	if (dGameKey)
		return 0;
	return T_GetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
}

BOOL WINAPI H_GetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader) {
	for (int i = 0; i < sizeof(activeBoxes) & sizeof(activeBoxes[0]); i++)
		if (activeBoxes[i] == true)
			return 0;
	if (dGameKey)
		return 0;
	return T_GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
}


HWND koHWND = NULL;


static bool show = false;

std::string wstring_to_utf8(const std::wstring& str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

bool LoadTextureFromFile(LPDIRECT3DDEVICE9 g_pd3dDevice, const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height)
{
	PDIRECT3DTEXTURE9 texture;
	HRESULT hr = D3DXCreateTextureFromFileA(g_pd3dDevice, filename, &texture);
	if (hr != S_OK)
		return false;
	D3DSURFACE_DESC my_image_desc;
	texture->GetLevelDesc(0, &my_image_desc);
	*out_texture = texture;
	*out_width = (int)my_image_desc.Width;
	*out_height = (int)my_image_desc.Height;
	return true;
}

static LPDIRECT3DTEXTURE9 pTextureInterface;
static LPD3DXSPRITE pSpriteInterface;
static D3DXVECTOR3 Position;

static LPDIRECT3DTEXTURE9 ToolInterface;
static LPD3DXSPRITE ToolSprite;


static int itemiconW = 0;
static int itemiconH = 0;

static ImFont* pFont;

bool is_file_exist(const char* fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

static bool inited = true;

PDIRECT3DTEXTURE9 lowClassTexture = NULL;
PDIRECT3DTEXTURE9 middleClassTexture = NULL;
PDIRECT3DTEXTURE9 highClassTexture = NULL;
PDIRECT3DTEXTURE9 greenBusTexture = NULL;
PDIRECT3DTEXTURE9 trinaTexture = NULL;
PDIRECT3DTEXTURE9 rebirthTexture = NULL;
PDIRECT3DTEXTURE9 karidivsTexture = NULL;
PDIRECT3DTEXTURE9 accesoryTexture = NULL;

HRESULT WINAPI hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* PresP)
{
	
	pTextureInterface->Release();
	pSpriteInterface->Release();
	ToolInterface->Release();
	ToolSprite->Release();
	lowClassTexture->Release();
	middleClassTexture->Release();
	highClassTexture->Release();
	greenBusTexture->Release();
	trinaTexture->Release();
	rebirthTexture->Release();
	karidivsTexture->Release();
	accesoryTexture->Release();
	pTextureInterface = NULL;
	pSpriteInterface = NULL;
	ToolInterface = NULL;
	ToolSprite = NULL;
	pFont = NULL;
	lowClassTexture = NULL;
	middleClassTexture = NULL;
	highClassTexture = NULL;
	greenBusTexture = NULL;
	trinaTexture = NULL;
	rebirthTexture = NULL;
	karidivsTexture = NULL;
	accesoryTexture = NULL;


	for (int i = 0; i < itemData.size(); i++) {
		itemData[i].icon->Release();
		itemData[i].icon = NULL;
	}

	ImGui_ImplDX9_InvalidateDeviceObjects();

	inited = true;

	return oReset(pDevice, PresP);
}

static int myfpslimit = 60;
static bool isfpslimit = false;

static string strSearch;
static string strESN;

static int PusSearchEditCallbackStub(ImGuiInputTextCallbackData* data)
{
	strSearch = data->Buf;
	return 1;
}

static int ESNEditCallbackStub(ImGuiInputTextCallbackData* data)
{
	strESN = data->Buf;
	return 1;
}


bool bCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	return (*szMask) == NULL;
}

DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE* bMask, char* szMask)
{
	for (DWORD i = 0; i < dwLen; i++)
		if (bCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (DWORD)(dwAddress + i);
	return 0;
}



HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	
	if (inited)
	{
		D3DXCreateTextureFromFileInMemoryEx(pDevice, lowClassImage, sizeof(lowClassImage), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &lowClassTexture);
		D3DXCreateTextureFromFileInMemoryEx(pDevice, middleClassImage, sizeof(middleClassImage), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &middleClassTexture);
		D3DXCreateTextureFromFileInMemoryEx(pDevice, highClassImage, sizeof(highClassImage), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &highClassTexture);
		D3DXCreateTextureFromFileInMemoryEx(pDevice, greenBusImage, sizeof(greenBusImage), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &greenBusTexture);
		D3DXCreateTextureFromFileInMemoryEx(pDevice, trinaImage, sizeof(trinaImage), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &trinaTexture);
		D3DXCreateTextureFromFileInMemoryEx(pDevice, rebirthImage, sizeof(rebirthImage), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &rebirthTexture);
		D3DXCreateTextureFromFileInMemoryEx(pDevice, karidivsImage, sizeof(karidivsImage), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &karidivsTexture);
		D3DXCreateTextureFromFileInMemoryEx(pDevice, accesoryImage, sizeof(accesoryImage), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &accesoryTexture);

		for (int i = 0; i < itemData.size(); i++) {
			const char* path = ("GameSoft\\ui\\pus\\" + std::to_string(itemData[i].id) + ".png").c_str();
			bool ret;
			if(is_file_exist(path))
				ret = LoadTextureFromFile(pDevice, path, &itemData[i].icon, &itemiconW, &itemiconH);
			else if(is_file_exist(("GameSoft\\ui\\pus\\" + std::to_string(itemData[i].id) + ".jpg").c_str()))
				ret = LoadTextureFromFile(pDevice, ("GameSoft\\ui\\pus\\" + std::to_string(itemData[i].id) + ".jpg").c_str(), &itemData[i].icon, &itemiconW, &itemiconH);
			else if (is_file_exist(("GameSoft\\ui\\pus\\" + std::to_string(itemData[i].id) + ".bmp").c_str()))
				ret = LoadTextureFromFile(pDevice, ("GameSoft\\ui\\pus\\" + std::to_string(itemData[i].id) + ".bmp").c_str(), &itemData[i].icon, &itemiconW, &itemiconH);
			else {
				D3DXCreateTextureFromFileInMemoryEx(pDevice, UINoitemRAW, sizeof(UINoitemRAW), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
					D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
					D3DX_DEFAULT, 0, NULL, NULL, &itemData[i].icon);
			}
			IM_ASSERT(ret);
		}



		Position.x = 150.0f;
		Position.y = 200.0f;
		Position.z = 1.0f;

		D3DXCreateTextureFromFileInMemoryEx(pDevice, UIBackgroundRAW, sizeof(UIBackgroundRAW), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &pTextureInterface);

		D3DXCreateTextureFromFileInMemoryEx(pDevice, UIToolRAW, sizeof(UIToolRAW), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT,
			D3DX_DEFAULT, 0, NULL, NULL, &ToolInterface);

		D3DXCreateSprite(pDevice, &pSpriteInterface);
		D3DXCreateSprite(pDevice, &ToolSprite);


		koHWND = FindWindowA(NULL, "Knight OnLine Client");
		inited = false;
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		pFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", 13.0f);

		ImGui_ImplWin32_Init(koHWND);
		ImGui_ImplDX9_Init(pDevice);

		ImGuiStyle* style = &ImGui::GetStyle();

		style->WindowBorderSize = 0.0f;
		style->WindowPadding = ImVec2(20, 15);
		style->WindowRounding = 10.0f;
		style->FramePadding = ImVec2(8, 5.5F);
		style->FrameRounding = 4.0f;
		style->ItemSpacing = ImVec2(12, 8);
		style->IndentSpacing = 25.0f;
		style->ScrollbarSize = 15.0f;
		style->ScrollbarRounding = 9.0f;
		style->GrabMinSize = 5.0f;
		style->GrabRounding = 3.0f;
		style->FrameBorderSize = 1.0f;
		style->TabBorderSize = 1.0f;
		style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style->ItemInnerSpacing = ImVec2(10, 10);
	

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.87f, 0.87f, 0.50f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.461f, 0.461f, 0.461f, 1.000f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.10f, 0.03f, 0.0f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.11f, 0.05f, 0.60f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.17f, 0.09f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.79f, 0.72f, 0.56f, 0.28f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.17f, 0.19f, 0.14f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.30f, 0.13f, 0.80f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.11f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.31f, 0.31f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.29f, 0.21f, 0.15f, 0.75f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.14f, 0.08f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.59f, 0.62f, 0.52f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.92f, 0.00f, 0.31f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.123f, 0.123f, 0.101f, 1.000f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.251f, 0.288f, 0.158f, 1.000f);
		colors[ImGuiCol_Header] = ImVec4(0.301f, 0.263f, 0.153f, 1.000f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.347f, 0.311f, 0.152f, 1.000f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.358f, 0.361f, 0.178f, 1.000f);
		colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.43f, 0.47f, 0.19f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.56f, 0.46f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.19f, 0.19f, 0.19f, 0.86f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.43f, 0.41f, 0.23f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.36f, 0.37f, 0.22f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.15f, 0.07f, 0.97f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.40f, 0.42f, 0.14f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.71f, 0.72f, 0.21f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.27f, 0.32f, 0.14f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.28f, 0.32f, 0.14f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.461f, 0.466f, 0.106f, 0.000f);

	}
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	/*imgui kullanma alan*/
	ImGuiIO& io = ImGui::GetIO();
	pDevice->SetRenderState(D3DRS_ZENABLE, false);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	if (show) {
		// arkaplan resmi
		pSpriteInterface->Begin(D3DXSPRITE_ALPHABLEND);
		pSpriteInterface->Draw(pTextureInterface, NULL, NULL, &Position, D3DCOLOR_ARGB(245,255,255,255) );
		pSpriteInterface->End();
		// ---------------
		ImGui::SetNextWindowSize(ImVec2(547, 370));
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		if (!ImGui::Begin("遊戲進階##mainwindow", &show, 1 | 2 | 32 | 256 | ImGuiWindowFlags_NoMove)) {
			ImGui::End();
		}
		ImGui::PushFont(pFont);
		if (ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_Reorderable))
		{
			if (ImGui::BeginTabItem("角色詳情"))
			{
				ImGui::Text("缺口: %s\t", Nick.c_str());
				ImGui::SameLine();
				ImGui::Text("水平: %d\t國家點: %d\t", Level, NationPoint);
				ImGui::SameLine();
				ImGui::Text("重生等級: %d", RebLevel);
				ImGui::Spacing();
				ImGui::Text("匕首防禦: %d\t斧頭防禦: %d\n\n劍防: %d\t狼牙棒防禦: %d\n\n箭防禦: %d\n\n矛防禦: %d", ddAc, axeAc, swordAc, maceAc, arrowAc, spearAc);
				ImGui::Separator();
				ImGui::Text("殺: 0\t死亡: 0\tKD: 0");
				ImGui::Separator();
				if (ImGui::Button("統計重置")) {
					ImGui::OpenPopup("Stat Reset##statresetbox");
				}
				ImGui::SameLine();
				if (ImGui::Button("技能重置")) {
					ImGui::OpenPopup("Skill Reset##skillresetbox");
				}

				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
				if (ImGui::BeginPopupModal("Skill Reset##skillresetbox", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
				{
					ImGui::Text("您想重新分配您的技能點嗎？");
					ImGui::Text(string_format("它的成本 %d 硬幣.", moneyreq).c_str());
					ImGui::Separator();
					if (ImGui::Button("Yes##yesskillreset", ImVec2(120, 0))) {
						Packet pkt(PL_RESET);
						pkt << uint8(1);
						_Engine->Send(&pkt);
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					ImGui::SetItemDefaultFocus();
					if (ImGui::Button("No##closeskillreset", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
					ImGui::EndPopup();
				}
				ImGui::PopStyleVar();
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
				if (ImGui::BeginPopupModal("Stat Reset##statresetbox", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
				{
					ImGui::Text("您想重新分配統計點嗎？");
					ImGui::Text(string_format("它的成本 %d 硬幣.", moneyreq).c_str());
					ImGui::Separator();
					if (ImGui::Button("Yes##yesstatreset", ImVec2(120, 0))) {
						Packet pkt(PL_RESET);
						pkt << uint8(2);
						_Engine->Send(&pkt);
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					ImGui::SetItemDefaultFocus();
					if (ImGui::Button("No##closestatreset", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
					ImGui::EndPopup();
				}
				ImGui::PopStyleVar();

				ImGui::EndTabItem();
			}
			/*if (ImGui::BeginTabItem("Drop Search"))
			{
				static char searchquery[128] = "";
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
				if (ImGui::InputTextWithHint("##searchbox", "Enter an item name...", searchquery, IM_ARRAYSIZE(searchquery), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory)) {
					Packet pkt(PL_DROP_LIST);
					pkt << uint8(0) << string(searchquery);
					_Engine->Send(&pkt);
				}
				if (ImGui::IsItemActive())
					activeBoxes[0] = true;
				else
					activeBoxes[0] = false;
				ImGui::SameLine();
				const char* items[] = { "Moradon", "Elmorad Castle", "Luferson Castle", "Eslant", "Ronark Land", "Ardream", "Bifrost", "Krowaz", "Under the Castle" };
				static const char* item_current = items[0];
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.35f);
				if (ImGui::BeginCombo("Zone", item_current, 0)) {
					for (int n = 0; n < IM_ARRAYSIZE(items); n++)
					{
						bool is_selected = (item_current == items[n]);
						if (ImGui::Selectable(items[n], is_selected))
							item_current = items[n];
						if (is_selected)
							ImGui::SetItemDefaultFocus(); 
					}
					ImGui::EndCombo();
				}
				ImGui::EndTabItem();
			}*/
			if (ImGui::BeginTabItem("Guide"))
			{
				if (ImGui::CollapsingHeader("Upgrade Oranları##guideupgrade"))
				{
					ImGui::Columns(6,0,false);
					ImGui::Text("#");
					ImGui::NextColumn();
					ImGui::Image((void*)lowClassTexture, ImVec2(32,32));
					ImGui::NextColumn();
					ImGui::Image((void*)middleClassTexture, ImVec2(32, 32));
					ImGui::NextColumn();
					ImGui::Image((void*)highClassTexture, ImVec2(32, 32));
					ImGui::NextColumn();
					ImGui::Image((void*)trinaTexture, ImVec2(32, 32));
					ImGui::NextColumn();
					ImGui::Image((void*)greenBusTexture, ImVec2(32, 32));
					ImGui::NextColumn();
					for (int i = 1; i < 10; i++) {
						ImGui::Text("+%d -> +%d", i, i+1);
						ImGui::NextColumn();
						for (int j = 0; j < 5; j++)
						{
							ImGui::Text("0%%");
							ImGui::NextColumn();
						}
					}
					ImGui::Columns(1);
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
					ImGui::Columns(4,0,false);
					ImGui::Text("#");
					ImGui::NextColumn();
					ImGui::Image((void*)rebirthTexture, ImVec2(32, 32));
					ImGui::NextColumn();
					ImGui::Image((void*)karidivsTexture, ImVec2(32, 32));
					ImGui::NextColumn();
					ImGui::Image((void*)accesoryTexture, ImVec2(32, 32));
					ImGui::NextColumn();
					for (int i = 1; i < 10; i++) {
						ImGui::Text("+%d -> +%d", i, i + 1);
						ImGui::NextColumn();
						for (int j = 0; j < 3; j++)
						{
							ImGui::Text("0%%");
							ImGui::NextColumn();
						}
					}
					ImGui::Columns(1);
				}
				if (ImGui::CollapsingHeader("Maden & Balık")) {

				}
				if (ImGui::CollapsingHeader("Clan Sistemi")) {

				}
				if (ImGui::CollapsingHeader("Para Nasıl Kasarım")) {

				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("PUS"))
			{
				static char search[30];
				static char esn[17];
				static bool searchActive = true;
				ImGui::Text("Knight Cash: %d\tBonus: %d", KnightCash, KnightCashBonus);
				//Search
				ImGui::Text("Search: ");
				ImGui::SameLine();
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.22f);
				if (ImGui::InputText("##pussearchbox", search, IM_ARRAYSIZE(search), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &PusSearchEditCallbackStub)) {
					strSearch = search;
				}
				if (ImGui::IsItemActive())
					activeBoxes[1] = true;
				else
					activeBoxes[1] = false;

				ImGui::SameLine();
				//ESN
				ImGui::SameLine();
				ImGui::Text("ESN: ");
				ImGui::SameLine();

				if(ImGui::InputText("##esnbox", esn, IM_ARRAYSIZE(esn), /*ImGuiInputTextFlags_EnterReturnsTrue |*/ ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &ESNEditCallbackStub)) {
					strESN = esn;
				}
				if (ImGui::IsItemActive())
					activeBoxes[2] = true;
				else
					activeBoxes[2] = false;

				ImGui::SameLine();
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.22f);
				if (ImGui::Button("USE ESN")) {
					if (strESN.size() == 16) {
						Packet pkt(PL_PUS);
						pkt << uint8(2) << string(strESN);
						_Engine->Send(&pkt);
					}
					else ImGui::OpenPopup("Error##esnerror");
				}

				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
				if (ImGui::BeginPopupModal("Error##esnerror", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
				{
					ImGui::Text("Invalid K-ESN format.");
					ImGui::Separator();
					ImGui::SetItemDefaultFocus();
					if (ImGui::Button("Confirm##closeesn", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
					ImGui::EndPopup();
				}
				ImGui::PopStyleVar();

				if (ImGui::BeginTabBar("#pus", ImGuiTabBarFlags_None))
				{
					if (ImGui::BeginTabItem("All")) {
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Columns(6, NULL, false);
						for (int i = 0; i < pusItems.size(); i++) {
							ItemData thisData = getItemDataByID(pusItems[i].sID, itemData);
							bool isOK = false;
							if (searchActive)
								if (string(strToLower(thisData.name)).find(strToLower(strSearch)) != std::string::npos)
									isOK = true;
								else
									isOK = false;
							else isOK = true;

							if (isOK) {
								ImGui::Text("");
								ImGui::SameLine();
								ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
								if (ImGui::IsItemHovered()) {
									ImGui::BeginTooltip();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
									ImGui::Spacing();
									ImGui::Text("%s", thisData.description);
									ImGui::Spacing();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", pusItems[i].Price);
									ImGui::EndTooltip();
								}
								ImGui::Text("%s", thisData.name);
								ImGui::Text("Price: ");
								ImGui::SameLine();
								ImGui::TextColored(ImVec4(1.0f, 0.43f, 0.6f, 1.0f), "%d", pusItems[i].Price);
								string btnID = "Purchase##btn" + std::to_string(i);
								static bool disabled = false;
								if (KnightCash < pusItems[i].Price && KnightCashBonus < pusItems[i].Price)
									disabled = true;
								else
									disabled = false;
								if (disabled)
									ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								if (ImGui::Button(btnID.c_str())) {
									ImGui::OpenPopup(string("Confirm Shopping##confirm" + std::to_string(i)).c_str());
								}
								ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
								if (ImGui::BeginPopupModal(string("Confirm Shopping##confirm" + std::to_string(i)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
								{
									ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
									if (ImGui::IsItemHovered()) {
										ImGui::BeginTooltip();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
										ImGui::Spacing();
										ImGui::Text("%s", thisData.description);
										ImGui::Spacing();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", pusItems[i].Price);
										ImGui::EndTooltip();
									}
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "%s", thisData.name);
									ImGui::Text("Item Price: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d", pusItems[i].Price);
									ImGui::Text("Current Knight Cash: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus);
									ImGui::Text("After Shopping: ");
									ImGui::SameLine();
									if(KnightCash >= pusItems[i].Price)
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash - pusItems[i].Price, KnightCashBonus);
									else
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus - pusItems[i].Price);

									ImGui::Text("\nDo you approve of the purchase?\n\n", pusItems[i].Price);
									ImGui::Separator();
									if (ImGui::Button(string("Purchase##popup" + std::to_string(i)).c_str(), ImVec2(120, 0))) {
										Packet pkt(PL_PUS);
										pkt << uint8(1) << uint32(pusItems[i].sID);
										_Engine->Send(&pkt);
										ImGui::CloseCurrentPopup();
									}
									ImGui::SetItemDefaultFocus();
									ImGui::SameLine();
									if (ImGui::Button(string("Cancel##popup" + std::to_string(i)).c_str(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
									ImGui::EndPopup();
								}
								ImGui::PopStyleVar();

								if (disabled)
									ImGui::PopItemFlag();
								ImGui::Spacing();
								ImGui::NextColumn();
							}
						}
						ImGui::Columns(1);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Specials")) {
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Columns(6, NULL, false);
						for (int i = 0; i < specialItems.size(); i++) {
							ItemData thisData = getItemDataByID(specialItems[i].sID, itemData);
							bool isOK = false;
							if (searchActive)
								if (string(strToLower(thisData.name)).find(strToLower(strSearch)) != std::string::npos)
									isOK = true;
								else
									isOK = false;
							else isOK = true;
							if (isOK) {
								ImGui::Text("");
								ImGui::SameLine();
								ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
								if (ImGui::IsItemHovered()) {
									ImGui::BeginTooltip();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
									ImGui::Spacing();
									ImGui::Text("%s", thisData.description);
									ImGui::Spacing();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", specialItems[i].Price);
									ImGui::EndTooltip();
								}
								ImGui::Text("%s", thisData.name);
								ImGui::Text("Price: ");
								ImGui::SameLine();
								ImGui::TextColored(ImVec4(1.0f, 0.43f, 0.6f, 1.0f), "%d", specialItems[i].Price);
								string btnID = "Purchase##btns" + std::to_string(i);
								static bool disabled = false;
								if (KnightCash < specialItems[i].Price && KnightCashBonus < specialItems[i].Price)
									disabled = true;
								else
									disabled = false;
								if (disabled)
									ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								if (ImGui::Button(btnID.c_str())) {
									ImGui::OpenPopup(string("Confirm Shopping##confirms" + std::to_string(i)).c_str());
								}
								ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
								if (ImGui::BeginPopupModal(string("Confirm Shopping##confirms" + std::to_string(i)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
								{
									ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
									if (ImGui::IsItemHovered()) {
										ImGui::BeginTooltip();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
										ImGui::Spacing();
										ImGui::Text("%s", thisData.description);
										ImGui::Spacing();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", specialItems[i].Price);
										ImGui::EndTooltip();
									}
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "%s", thisData.name);
									ImGui::Text("Item Price: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d", specialItems[i].Price);
									ImGui::Text("Current Knight Cash: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus);
									ImGui::Text("After Shopping: ");
									ImGui::SameLine();
									if (KnightCash >= specialItems[i].Price)
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash - specialItems[i].Price, KnightCashBonus);
									else
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus - specialItems[i].Price);

									ImGui::Text("\nDo you approve of the purchase?\n\n", specialItems[i].Price);
									ImGui::Separator();
									if (ImGui::Button(string("Purchase##popups" + std::to_string(i)).c_str(), ImVec2(120, 0))) {
										Packet pkt(PL_PUS);
										pkt << uint8(1) << uint32(specialItems[i].sID);
										_Engine->Send(&pkt);
										ImGui::CloseCurrentPopup();
									}
									ImGui::SetItemDefaultFocus();
									ImGui::SameLine();
									if (ImGui::Button(string("Cancel##popups" + std::to_string(i)).c_str(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
									ImGui::EndPopup();
								}
								ImGui::PopStyleVar();

								if (disabled)
									ImGui::PopItemFlag();
								ImGui::Spacing();
								ImGui::NextColumn();
							}
						}
						ImGui::Columns(1);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Power-Up")) {
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Columns(6, NULL, false);
						for (int i = 0; i < powerItems.size(); i++) {
							ItemData thisData = getItemDataByID(powerItems[i].sID, itemData);
							bool isOK = false;
							if (searchActive)
								if (string(strToLower(thisData.name)).find(strToLower(strSearch)) != std::string::npos)
									isOK = true;
								else
									isOK = false;
							else isOK = true;
							if (isOK) {
								ImGui::Text("");
								ImGui::SameLine();
								ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
								if (ImGui::IsItemHovered()) {
									ImGui::BeginTooltip();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
									ImGui::Spacing();
									ImGui::Text("%s", thisData.description);
									ImGui::Spacing();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", powerItems[i].Price);
									ImGui::EndTooltip();
								}
								ImGui::Text("%s", thisData.name);
								ImGui::Text("Price: ");
								ImGui::SameLine();
								ImGui::TextColored(ImVec4(1.0f, 0.43f, 0.6f, 1.0f), "%d", powerItems[i].Price);
								string btnID = "Purchase##btnp" + std::to_string(i);
								static bool disabled = false;
								if (KnightCash < powerItems[i].Price && KnightCashBonus < powerItems[i].Price)
									disabled = true;
								else
									disabled = false;
								if (disabled)
									ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								if (ImGui::Button(btnID.c_str())) {
									ImGui::OpenPopup(string("Confirm Shopping##confirmp" + std::to_string(i)).c_str());
								}
								ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
								if (ImGui::BeginPopupModal(string("Confirm Shopping##confirmp" + std::to_string(i)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
								{
									ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
									if (ImGui::IsItemHovered()) {
										ImGui::BeginTooltip();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
										ImGui::Spacing();
										ImGui::Text("%s", thisData.description);
										ImGui::Spacing();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", powerItems[i].Price);
										ImGui::EndTooltip();
									}
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "%s", thisData.name);
									ImGui::Text("Item Price: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d", powerItems[i].Price);
									ImGui::Text("Current Knight Cash: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus);
									ImGui::Text("After Shopping: ");
									ImGui::SameLine();
									if (KnightCash >= powerItems[i].Price)
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash - powerItems[i].Price, KnightCashBonus);
									else
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus - powerItems[i].Price);

									ImGui::Text("\nDo you approve of the purchase?\n\n", powerItems[i].Price);
									ImGui::Separator();
									if (ImGui::Button(string("Purchase##popupp" + std::to_string(i)).c_str(), ImVec2(120, 0))) {
										Packet pkt(PL_PUS);
										pkt << uint8(1) << uint32(powerItems[i].sID);
										_Engine->Send(&pkt);
										ImGui::CloseCurrentPopup();
									}
									ImGui::SetItemDefaultFocus();
									ImGui::SameLine();
									if (ImGui::Button(string("Cancel##popupp" + std::to_string(i)).c_str(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
									ImGui::EndPopup();
								}
								ImGui::PopStyleVar();

								if (disabled)
									ImGui::PopItemFlag();
								ImGui::Spacing();
								ImGui::NextColumn();
							}
						}
						ImGui::Columns(1);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Costumes")) {
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Columns(6, NULL, false);
						for (int i = 0; i < costumeItems.size(); ++i) {
							ItemData thisData = getItemDataByID(costumeItems[i].sID, itemData);
							bool isOK = false;
							if (searchActive)
								if (string(strToLower(thisData.name)).find(strToLower(strSearch)) != std::string::npos)
									isOK = true;
								else
									isOK = false;
							else isOK = true;
							if (isOK) {
								ImGui::Text("");
								ImGui::SameLine();
								ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
								if (ImGui::IsItemHovered()) {
									ImGui::BeginTooltip();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
									ImGui::Spacing();
									ImGui::Text("%s", thisData.description);
									ImGui::Spacing();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", costumeItems[i].Price);
									ImGui::EndTooltip();
								}
								ImGui::Text("%s", thisData.name);
								ImGui::Text("Price: ");
								ImGui::SameLine();
								ImGui::TextColored(ImVec4(1.0f, 0.43f, 0.6f, 1.0f), "%d", costumeItems[i].Price);
								string btnID = "Purchase##btnc" + std::to_string(i);
								static bool disabled = false;
								if (KnightCash < costumeItems[i].Price && KnightCashBonus < costumeItems[i].Price)
									disabled = true;
								else
									disabled = false;
								if (disabled)
									ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								if (ImGui::Button(btnID.c_str())) {
									ImGui::OpenPopup(string("Confirm Shopping##confirmc" + std::to_string(i)).c_str());
								}
								ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
								if (ImGui::BeginPopupModal(string("Confirm Shopping##confirmc" + std::to_string(i)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
								{
									ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
									if (ImGui::IsItemHovered()) {
										ImGui::BeginTooltip();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
										ImGui::Spacing();
										ImGui::Text("%s", thisData.description);
										ImGui::Spacing();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", costumeItems[i].Price);
										ImGui::EndTooltip();
									}
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "%s", thisData.name);
									ImGui::Text("Item Price: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d", costumeItems[i].Price);
									ImGui::Text("Current Knight Cash: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus);
									ImGui::Text("After Shopping: ");
									ImGui::SameLine();
									if (KnightCash >= costumeItems[i].Price)
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash - costumeItems[i].Price, KnightCashBonus);
									else
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus - costumeItems[i].Price);

									ImGui::Text("\nDo you approve of the purchase?\n\n", costumeItems[i].Price);
									ImGui::Separator();
									if (ImGui::Button(string("Purchase##popupc" + std::to_string(i)).c_str(), ImVec2(120, 0))) {
										Packet pkt(PL_PUS);
										pkt << uint8(1) << uint32(costumeItems[i].sID);
										_Engine->Send(&pkt);
										ImGui::CloseCurrentPopup();
									}
									ImGui::SetItemDefaultFocus();
									ImGui::SameLine();
									if (ImGui::Button(string("Cancel##popupc" + std::to_string(i)).c_str(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
									ImGui::EndPopup();
								}
								ImGui::PopStyleVar();

								if (disabled)
									ImGui::PopItemFlag();
								ImGui::Spacing();
								ImGui::NextColumn();
							}
						}
						ImGui::Columns(1);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Bundles")) {
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Columns(6, NULL, false);
						for (int i = 0; i < bundles.size(); i++) {
							ItemData thisData = getItemDataByID(bundles[i].sID, itemData);
							bool isOK = false;
							if (searchActive)
								if (string(strToLower(thisData.name)).find(strToLower(strSearch)) != std::string::npos)
									isOK = true;
								else
									isOK = false;
							else isOK = true;
							if (isOK) {
								ImGui::Text("");
								ImGui::SameLine();
								ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
								if (ImGui::IsItemHovered()) {
									ImGui::BeginTooltip();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
									ImGui::Spacing();
									ImGui::Text("%s", thisData.description);
									ImGui::Spacing();
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", bundles[i].Price);
									ImGui::EndTooltip();
								}
								ImGui::Text("%s", thisData.name);
								ImGui::Text("Price: ");
								ImGui::SameLine();
								ImGui::TextColored(ImVec4(1.0f, 0.43f, 0.6f, 1.0f), "%d", bundles[i].Price);
								string btnID = "Purchase##btnb" + std::to_string(i);
								static bool disabled = false;
								if (KnightCash < bundles[i].Price && KnightCashBonus < bundles[i].Price)
									disabled = true;
								else
									disabled = false;
								if (disabled)
									ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								if (ImGui::Button(btnID.c_str())) {
									ImGui::OpenPopup(string("Confirm Shopping##confirmb" + std::to_string(i)).c_str());
								}
								ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
								if (ImGui::BeginPopupModal(string("Confirm Shopping##confirmb" + std::to_string(i)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
								{
									ImGui::Image((void*)thisData.icon, ImVec2(38, 38));
									if (ImGui::IsItemHovered()) {
										ImGui::BeginTooltip();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", thisData.name);
										ImGui::Spacing();
										ImGui::Text("%s", thisData.description);
										ImGui::Spacing();
										ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Price: %d", bundles[i].Price);
										ImGui::EndTooltip();
									}
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "%s", thisData.name);
									ImGui::Text("Item Price: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d", bundles[i].Price);
									ImGui::Text("Current Knight Cash: ");
									ImGui::SameLine();
									ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus);
									ImGui::Text("After Shopping: ");
									ImGui::SameLine();
									if (KnightCash >= bundles[i].Price)
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash - bundles[i].Price, KnightCashBonus);
									else
										ImGui::TextColored(ImVec4(0.7F, 0.4F, 1.0F, 1.0F), "%d + (%d)", KnightCash, KnightCashBonus - bundles[i].Price);

									ImGui::Text("\nDo you approve of the purchase?\n\n", bundles[i].Price);
									ImGui::Separator();
									if (ImGui::Button(string("Purchase##popupb" + std::to_string(i)).c_str(), ImVec2(120, 0))) {
										Packet pkt(PL_PUS);
										pkt << uint8(1) << uint32(bundles[i].sID);
										_Engine->Send(&pkt);
										ImGui::CloseCurrentPopup();
									}
									ImGui::SetItemDefaultFocus();
									ImGui::SameLine();
									if (ImGui::Button(string("Cancel##popupb" + std::to_string(i)).c_str(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
									ImGui::EndPopup();
								}
								ImGui::PopStyleVar();

								if (disabled)
									ImGui::PopItemFlag();
								ImGui::Spacing();
								ImGui::NextColumn();
							}
						}
						ImGui::Columns(1);
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::EndTabItem();
			}
			/*if (ImGui::BeginTabItem("User Market"))
			{
				if (ImGui::BeginTabBar("#market", ImGuiTabBarFlags_None))
				{
					if (ImGui::BeginTabItem("On Sale")) {
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Sell Item")) {
						ImGui::Text("Inventory");
						ImGui::Separator();
						ImGui::Columns(4, NULL, false);
						ImGui::Text("Item");
						ImGui::NextColumn();
						ImGui::Text("Count");
						ImGui::NextColumn();
						ImGui::Text("Duration");
						ImGui::NextColumn();
						ImGui::Text("#");
						ImGui::NextColumn();
						for (int i = 0; i < inventory.size(); i++) {
							ITEM_DATA item = inventory[i];
							if ((item.sCount < 1 && item.sDuration < 1) || !(item.itemName.length() > 0))
								continue;
							ImGui::Text("%s", item.itemName.c_str());
							ImGui::NextColumn();
							ImGui::Text("%d", item.sCount);
							ImGui::NextColumn();
							ImGui::Text("%d", item.sDuration);
							ImGui::NextColumn();
							ImGui::Button(string("Sell for Knight Cash##sell" + to_string(i)).c_str());
							ImGui::NextColumn();
						}
						ImGui::Columns(1);
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::EndTabItem();
			}*/
			if (ImGui::BeginTabItem("Notice"))
			{
				ImGui::Columns(1);
				for (int i = 0; i < notices.size(); ++i) {
					char* notice = (char*)notices[i].c_str();
					wchar_t converted[256];
					MultiByteToWideChar(CP_UTF8, 0, notice, -1, converted, IM_ARRAYSIZE(converted));

					ImGui::Text("%s", toUtf8(converted));
					ImGui::NextColumn();
				}
				ImGui::Columns(1);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Settings"))
			{
				
				ImGui::Text("If you'r mining or AFK you can set power mode on for reducing cpu usage.");
				ImGui::Checkbox("Low Power Mode", &lowPower);
				ImGui::Text("If you want to set the fps limit by yourself. The best performance can be gottan when it is is off.");
				ImGui::SliderInt("##fpslimitor", &myfpslimit, 1, 240);
				ImGui::SameLine();
				ImGui::Checkbox("ON/OFF FPS Limit", &isfpslimit);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		
		/* Implement Game Overlay */

		

		ImVec2 windowSize = ImGui::GetWindowSize();
		ImVec2 windowPos = ImGui::GetWindowPos();
		Position.x = windowPos.x - 25;
		Position.y = windowPos.y - 40;

	
	}
 else disableGameMouse[0] = false;
	ImGui::End();

	static D3DXVECTOR3 toolPos;
	toolPos.z = 1;

	if (tools) {

		ToolSprite->Begin(D3DXSPRITE_ALPHABLEND);
		ToolSprite->Draw(ToolInterface, NULL, NULL, &toolPos, D3DCOLOR_ARGB(245, 255, 255, 255));
		ToolSprite->End();

		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.095f, io.DisplaySize.y * 0.123f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin("Game Tools##tools", &tools, 1 | 2 | 32 | 256);
		if (ImGui::Button("Advanced")) {
			show = !show;
		}
		ImVec2 toolPosition = ImGui::GetWindowPos();
		ImVec2 toolSize = ImGui::GetWindowSize();
		toolPos.x = toolPosition.x + 10;
		toolPos.y = toolPosition.y + 5;


		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	/* Implement gaming overlay */
	POINT p;
	RECT kwr;
	GetWindowRect(koHWND, &kwr);
	static int onCount = 0;
	if (GetCursorPos(&p))
	{
		ImGuiContext& g = *GImGui;
		for (int i = g.Windows.Size - 1; i >= 0; i--)
		{
			ImGuiWindow* window = g.Windows[i];
			int curPosX = p.x - kwr.left - 3;
			int curPosY = p.y - kwr.top - 3;
			if (window->Active && (curPosX > window->Pos.x&& curPosX <= window->Pos.x + window->Size.x) && (curPosY >= window->Pos.y && curPosY <= window->Pos.y + window->Size.y))
				onCount++;
		}
	}
	if (onCount > 0)
		dGameMouse = true;
	else
		dGameMouse = false;
	onCount = 0;
	/* ---------------------- */

	if (lowPower && !isfpslimit) {
		int lowFPS = 10;
		Sleep(1000 / lowFPS);
	}
	else if (isfpslimit) {
		Sleep(1000 / myfpslimit);
	}
	

	return oEndScene(pDevice);
}

LRESULT CALLBACK MsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void loopUI() {
	while (true) {
		if (GetAsyncKeyState(VK_ESCAPE) & 1)
			show = false;
		Sleep(100);
	}
}

void DX_Init(DWORD* table)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"DX", NULL };
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindow(L"DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), NULL, wc.hInstance, NULL);
	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	LPDIRECT3DDEVICE9 pd3dDevice;
	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice);
	DWORD* pVTable = (DWORD*)pd3dDevice;
	pVTable = (DWORD*)pVTable[0];

	table[ES] = pVTable[42];
	table[DIP] = pVTable[82];
	table[RES] = pVTable[16];

	DestroyWindow(hWnd);

	
	//msgBox.LoadFromFile("GameSoft\\ui\\co_messagebox_us.uif", N3FORMAT_VER_1264);
	//msgBox.ShowWindow();

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)loopUI, NULL, NULL, NULL);
}

DWORD WINAPI InitGUI()
{
	Nick = "";
	NationPoint = 0;
	KnightCash = 0;
	Level = 0;
	RebLevel = 0;
	ddAc = 0;
	axeAc = 0;
	swordAc = 0;
	maceAc = 0;
	spearAc = 0;
	arrowAc = 0;

	while (GetModuleHandle(L"d3d9.dll") == NULL) {
		Sleep(250);
	}

	DX_Init(VTable);

	T_GetAsyncKeyState = (pfnGetAsyncKeyState)DetourFunction((PBYTE)GetAsyncKeyState, (PBYTE)H_GetAsyncKeyState);
	T_GetKeyState = (pfnGetAsyncKeyState)DetourFunction((PBYTE)GetKeyState, (PBYTE)H_GetKeyState);
	T_GetKeyboardState = (pfnGetKeyboardState)DetourFunction((PBYTE)GetKeyboardState, (PBYTE)H_GetKeyboardState);
	T_GetRawInputData = (pfnGetRawInputData)DetourFunction((PBYTE)GetRawInputData, (PBYTE)H_GetRawInputData);
	T_GetRawInputBuffer = (pfnGetRawInputBuffer)DetourFunction((PBYTE)GetRawInputBuffer, (PBYTE)H_GetRawInputBuffer);

	HOOK(EndScene, VTable[ES]);
	HOOK(DrawIndexedPrimitive, VTable[DIP]);
	HOOK(Reset, VTable[RES]);

	HWND window = NULL;
	while (window == NULL) window = FindWindowA(NULL, "Knight OnLine Client");

	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
	return 0;
}

void UIMain(PearlEngine* e) {
	_Engine = e;
	itemData.push_back(ItemData(399282686, (char*)"EXP Premium", (char*)"  1. Preferred game access 24/7\n  2. Level 1-50 +400% EXP\n     Level 51-60 +300% EXP\n     Level 61-70 +200% EXP\n     Level 71-83 +120% EXP\n  3. Ability to use EXP Flash Items up to level 80 (obtained by fishing) for an additional 50%-80% boost\n  4. Item drop rate increased\n  5. EXP loss on death reduced to 1%\n  6. NPC item sell price increased to 150%\n  7. 50% discount on item repair\n  8. 100% more Noah boost for all levels\n  9. Additional 12NP for PvP kills\n  10. Golden Fishing Pole\n  11. Trinas Piece (increased upgrade success"));
	itemData.push_back(ItemData(399281685, (char*)"DISC Premium", (char*)"  1. Preferred game access 24/7\n  2. 30% EXP Boost for levels 1-83\n  3. Item drop rate increased\n  4. Reduced experience loss\n      2.5% upon  death\n  5. NPC item sale price \n      increased to 150%\n  6. 50% discount on item repair price\n  7. 100% Noah Boost for all levels\n  8. Whenever the user to kill +12 NP\n  9. Golden Mattock Voucher\n      (1 Month of usage)\n 10. Ability of DC Flash items usage\n 11. DC Store available. (potion & buff)\n 12. 30x Magic Hammer\n 13. A Trinas Piece (increase upgrade\n       success a 800 BP Credit Value) \n\nPlease note that preferred game access may be unavailable when the server is at capacity.\n"));
	itemData.push_back(ItemData(399292764, (char*)"WAR Premium", (char*)"1. 100% EXP boost for levels 1-83.\n 2. Item drop rate increased.\n 3. Experience loss on death reduced to 2.5%.\n 4. NPC item sale price increased.\n 5. 50% discount on item repair price.\n 6. 30% Noah boost for all levels.\n 7. Additional 15 NP for PvP kills.\n 8. The ability to us War Flash items (obtained by fishing) for an additional 1-10NP per kill.\n 9. Golden Fishing Pole. (1 month)\n 10. FREE Redistribution.\n 11. NPC [sirin] from providing daily Ancient Text and Chaos Map Coupon. \n\nPlease note that preferred game access may be unavailable when the server is at capacity.\n"));
	itemData.push_back(ItemData(700002000, (char*)"Trinas Piece", (char*)"Item used in weapon and armor upgrading. This does not guarantee that the item will not be destroyed.  This just increases the chance of the upgrade. Not Purchasable with Gift Cash. "));
	itemData.push_back(ItemData(700009000, (char*)"Shadow Piece", (char*)"Item used in weapon and armor crafting. This does not guarantee that the item will not be destroyed.  This increases the chance to successfully create the item. "));
	itemData.push_back(ItemData(810164000, (char*)"Dragons Wing", (char*)"Allows you to obtain a Dragons Wing item which changes your characters appearance while adding one chosen attribute selected by you. Adds ONE of the following attributes: HP+300, DEF +50, EXP +9%, Nation Point +3"));
	itemData.push_back(ItemData(389390000, (char*)"Premium Potion HP", (char*)"Premium Potion Voucher HP 5000."));
	itemData.push_back(ItemData(389400000, (char*)"Premium Potion MP", (char*)"Premium Potion Voucher MP 5000."));
	itemData.push_back(ItemData(810227000, (char*)"Genie Hammer", (char*)"All of the equipments that need reparing will be repaired. (available with Knight Genie) "));
	itemData.push_back(ItemData(800086000, (char*)"Clan Name Change Scroll", (char*)"Allows Clans name to be changed. "));
	itemData.push_back(ItemData(810594000, (char*)"Gender Change", (char*)"Current character’s gender and appearance will be changed.  Cannot be used by Portu, Kurian, Ark Tuarek Warrior, Tuarek Rogue.  "));
	itemData.push_back(ItemData(810163000, (char*)"Menicias official list coupon(30Days) ", (char*)"Item lists out the all the [Selling/Buying] merchants in Moradon, El Morad Castle, and Luferson Castle. Items can be browsed using search tool. Player can [Whisper] or [Move] to the Seller/Buyer. "));
	itemData.push_back(ItemData(810323000, (char*)"Clan Contribution Certificate", (char*)"You can receive all the Nation Points contributed by carrying [Nation Point Return Scroll] when leaving the clan. ?  need to use [Nation Point Return Scroll] within 24 hours from exchange   ?You must be in Training Knight Squad or higher rank clan to receive a Nation Point return. "));
	itemData.push_back(ItemData(800078000, (char*)"2000 Health+ Scroll", (char*)"It increases the characters max Health "));
	itemData.push_back(ItemData(800076000, (char*)"Scroll of Armor 350", (char*)"Increase Defense "));
	itemData.push_back(ItemData(800079000, (char*)"%60 HP Scroll", (char*)"Scroll which will increase health by 60%. "));;
	itemData.push_back(ItemData(800008000, (char*)"Power of Lion Scroll(Stat)(L)", (char*)"It increases the characters STR, HP, DEX, INT, MP stats. "));
	itemData.push_back(ItemData(800003000, (char*)"STR+ Scroll(Stat)(L)", (char*)"Increases STR. "));
	itemData.push_back(ItemData(800004000, (char*)"HP+ Scroll(Stat)(L)", (char*)"Increases HP. "));
	itemData.push_back(ItemData(800005000, (char*)"DEX+ Scroll(Stat)(L)", (char*)"Increases DEX. "));
	itemData.push_back(ItemData(800006000, (char*)"INT+ Scroll(Stat)(L)", (char*)"Increases INT. "));
	itemData.push_back(ItemData(800007000, (char*)"MP+ Scroll(Stat)(L)", (char*)"It increases the characters MP "));
	itemData.push_back(ItemData(379258000, (char*)"Tears of Karivdis", (char*)"Item used in weapon and armor upgrading. This does not guarantee that the item will not be destroyed.  This increases the chance for a successful upgrade with the Rebirthed Item. "));
	itemData.push_back(ItemData(800036000, (char*)"60% Re-Spawn Scroll", (char*)"60% of EXP is recovered when re-spawning. "));
	itemData.push_back(ItemData(800021000, (char*)"Teleportation Item", (char*)"You can teleport to your friend. "));
	itemData.push_back(ItemData(800022000, (char*)"Duration Item", (char*)"It increases the duration on the skills cast on you by other players. "));
	itemData.push_back(ItemData(810075000, (char*)"Symbol of Gladiator[10 ea]", (char*)"Increases experience gain by 70%  - The level requirement is between 1 to 50 Only."));;
	itemData.push_back(ItemData(800200000, (char*)"Menissiah Transform Scroll(Karus)", (char*)"Transforms your character to look like   NPC Mennissiah(Karus) "));
	itemData.push_back(ItemData(800110000, (char*)"Menissiah Transform Scroll(El Morad)", (char*)"Transforms your character to look like   NPC Mennissiah(El Morad) "));
	itemData.push_back(ItemData(800220000, (char*)"Hera Transform Scroll(Karus)", (char*)"Transforms your character to look like   NPC Hera(Karus) "));
	itemData.push_back(ItemData(800130000, (char*)"Hera Transform Scroll(El Morad)", (char*)"Transforms your character to look like   NPC Hera(El Morad) "));
	itemData.push_back(ItemData(800190000, (char*)"Patrick Transform Scroll(Karus)", (char*)"Transforms your character to look like   NPC Patrick(Karus) "));
	itemData.push_back(ItemData(800100000, (char*)"Patrick Transform Scroll(El Morad)", (char*)"Transforms your character to look like   NPC Patrick(El Morad) "));
	itemData.push_back(ItemData(800210000, (char*)"Cougar Transform Scroll (Karus)", (char*)"Transforms your character to look like   NPC Cougar(Karus) "));
	itemData.push_back(ItemData(800120000, (char*)"Cougar Transform Scroll(El Morad)", (char*)"Transforms your character to look like   NPC Cougar(El Morad) "));
	itemData.push_back(ItemData(810185000, (char*)"Elmoradian Man Transform Scroll", (char*)"Transforms your character to look like   NPC Elmoradian Man"));
	itemData.push_back(ItemData(810186000, (char*)"Elmoradian Woman Transform Scroll", (char*)"Transforms your character to look like   NPC Elmoradian Woman"));
	itemData.push_back(ItemData(810187000, (char*)"Karus Man Transform Scroll", (char*)"Transforms your character to look like   NPC Karus Man"));
	itemData.push_back(ItemData(810188000, (char*)"Karus Woman Transform Scroll", (char*)"Transforms your character to look like   NPC Karus Woman"));
	itemData.push_back(ItemData(800060000, (char*)"Weight Scroll", (char*)"Increases the characters [Weight] add (+500)"));
	itemData.push_back(ItemData(508117000, (char*)"Yeniceri Armor", (char*)"It allows you to obtain a Yeniçeri Armor piece that changes your characters body appearance while adding one chosen attribute selected by you. "));
	itemData.push_back(ItemData(508116000, (char*)"Yeniceri Helmet", (char*)"It allows you to obtain a Yeniçeri Helmetpiece that changes your characters body appearance while adding one chosen attribute selected by you. "));
	itemData.push_back(ItemData(800170000, (char*)"Valkyrie Helmet Certificate", (char*)"It allows you to obtain a Valkyrie Helmet piece that changes your characters body appearance while adding one chosen attribute selected by you. "));
	itemData.push_back(ItemData(800180000, (char*)"Valkyrie Armor Certificate", (char*)"It allows you to obtain a Valkyrie Armor piece that changes your characters body appearance while adding one chosen attribute selected by you. "));
	itemData.push_back(ItemData(800230000, (char*)"Gryphon Helmet Certificate", (char*)"Allows you to obtain a Gryphon Helmet piece that changes your characters body appearance while adding one chosen attribute selected by you. "));
	itemData.push_back(ItemData(800240000, (char*)"Gryphon Armor Certificate", (char*)"Allows you to obtain a Gryphon Armor piece that changes your characters body appearance while adding one chosen attribute selected by you."));
	itemData.push_back(ItemData(800270000, (char*)"Bahamut Armor Certificate", (char*)"It allows you to obtain a Valkyrie Helmet piece that changes your characters body appearance while adding one chosen attribute selected by you."));
	itemData.push_back(ItemData(800260000, (char*)"Bahamut Helmet Certificate", (char*)"It allows you to obtain a Valkyrie Helmet piece that changes your characters body appearance while adding one chosen attribute selected by you. "));
	itemData.push_back(ItemData(900129000, (char*)"Red Potion", (char*)"Increases attack by 10% Attack Increase by 10%. Can be stacked with Cospre Gear and Dragon Wings. "));
	itemData.push_back(ItemData(900128000, (char*)"Blue Potion", (char*)"Potion that will increase defense by 60 points. Increases defense by 60 points. Can be stacked with Cospre Gear and Dragon Wings. "));
	itemData.push_back(ItemData(508112000, (char*)"Minerva Package", (char*)"It allows you to obtain a Minerva piece that changes your characters body appearance while adding one chosen attribute selected by you. "));
	itemData.push_back(ItemData(508074000, (char*)"Pathos Package", (char*)"Allows you to obtain a Pathos Glove that changes your characters body appearance while adding one chosen attribute selected by you. "));
	itemData.push_back(ItemData(810160000, (char*)"Ancient Text", (char*)"Contains clues to deciphering the ancient tongue. A good wing can be received by interpreting the Voucher of Oracle. [White Shadow Commander] Sirin. "));
	itemData.push_back(ItemData(900067000, (char*)"Unidentified Potion", (char*)"When you use it you will be invisible for 60 seconds."));
	itemData.push_back(ItemData(508122000, (char*)"Golden Mattock Voucher", (char*)"Exchange the item with [Vendor] Iruta in Moradon."));
	itemData.push_back(ItemData(508121000, (char*)"Golden Fishing Rod Voucher", (char*)"Exchange the item with [Vendor] Iruta in Moradon."));
	itemData.push_back(ItemData(800032000, (char*)"Name Change Scroll", (char*)"A scroll to change the character name."));
	itemData.push_back(ItemData(800440000, (char*)"Voucher for Magic Bag", (char*)"This voucher can be turned in for a Magic Bag that can be used to carry more items on your character. "));
	itemData.push_back(ItemData(800111000, (char*)"Character Seal Scroll", (char*)"Scroll for sealing a character. "));
	itemData.push_back(ItemData(800250000, (char*)"Voucher for a Pathos Glove", (char*)"Allows you to obtain a Pathos Glove that changes your characters body appearance while adding one chosen attribute selected by you."));
	itemData.push_back(ItemData(810051000, (char*)"Hyper Noah Scroll", (char*)"Increases Noah (in-game coin) drop amount by 100%."));
	itemData.push_back(ItemData(900435000, (char*)"Lesson of Master", (char*)"Increases the characters STR, HP, DEX, INT, MP stats. "));
	itemData.push_back(ItemData(810305000, (char*)"Spirit of Genie(10ea)", (char*)"-"));
	itemData.push_back(ItemData(810115000, (char*)"HP Maestro Voucher(30 days)", (char*)"With [Familliar Trainer] Kate, exchange ite for an item that will allow recovery of HP 720 for the cost of 4,900 Noah (no DC premium status) or 3,500 Noah (with the DC premium) Must have enough Noah in the inventory \n\n*can only be purchased by Npoints."));
	itemData.push_back(ItemData(800360000, (char*)"Nation Transfer", (char*)"Allows a player to switch nations and change the appearance of characters on the server, with a partial refund of his NP contributed to previous clan. If he is a clan leader, he may also receive the [Clan Restoration] item. NOTE: Cannot be used by Kings, Kings Inspection Team, or Sheriffs."));
	itemData.push_back(ItemData(800074000, (char*)"Np Increase", (char*)"Increase Nation Point gain by 50% "));
	itemData.push_back(ItemData(800087000, (char*)"Merchant Concentration", (char*)"Utilize a special merchant table to sell items.  More item slots so you can have more stock. "));
	itemData.push_back(ItemData(810340000, (char*)"Appearance Change", (char*)"Modifies features. It will last 30 days. [Makeup Specialist] Ulku. "));
	itemData.push_back(ItemData(800444000, (char*)"Arrange Line", (char*)"At [Trainer] Kate, permanently change characters location on character selection window. \n\n*Cannot be traded and can only be purchased by Npoints."));
	itemData.push_back(ItemData(800450000, (char*)"Voucher for Automatic Pet Looting", (char*)"This voucher can be turned in for the ability to have your pet automatically loot fallen enemies for you."));
	itemData.push_back(ItemData(810520000, (char*)"Seal Exchange Voucher", (char*)"its an exchange to receive 10 seal items. Seal item will seal characters affributet item."));
	itemData.push_back(ItemData(900705000, (char*)"Punishment Stick", (char*)"Wield the mighty Punishment Stick plus a little more! Wings are always a nice touch of the latest Knight Crown fashion, so if you happen to need the stick go grab the bundle and own your very own pair of Dragon Wings!"));
	itemData.push_back(ItemData(700033015, (char*)"Giga Hammer Pet", (char*)"Transforms pets appearance in to Giga Hammer."));
	itemData.push_back(ItemData(700034016, (char*)"Krowaz Pet", (char*)"Transforms pets appearance in to Krowaz"));
	itemData.push_back(ItemData(900386000, (char*)"Oread Voucher (30 days)", (char*)"Exchange it with [Merchant] Kayra for Automatic looting. \n\n*Cannot be traded and can only be purchased by Npoints."));
	itemData.push_back(ItemData(900385000, (char*)"Dyads Voucher (30 days)", (char*)"Exchange it with [Merchant] Kayra for [Dryad spirit] which will increase Noah Drop 100%. \n\n*Cannot be traded and can only be purchased by Npoints."));
	itemData.push_back(ItemData(800123000, (char*)"Ibex(Karus)", (char*)"Transforms your character to look like NPC Ibex(Karus)."));
	itemData.push_back(ItemData(800122000, (char*)"Crisis(El Morad)", (char*)"Transforms your character to look like NPC Crisis(ElMorad) "));
	itemData.push_back(ItemData(810511000, (char*)"Alencia Wing Exchange(Blue)", (char*)"[Wing of Alencia] is available from a trade with [Merchant] Kaira in Moradon. Only 1 option will be applied among followings: Defense +50, Earring EXP +9%, Devotion +3"));
	itemData.push_back(ItemData(810641000, (char*)"Wings of Hellfire Dragon Exchange Voucher(30 days)  ", (char*)"[Wing of Hellfire] is available from a trade with [VIP Manage] Juliane in Moradon. Only 1 option will be applied among followings: Defense +50, Earring EXP +9%, Devotion +3"));
	itemData.push_back(ItemData(700043000, (char*)"Dragons scale", (char*)"Gives a 30% increase in success rate when combining accessories. This does not guarantess that the item will not be destroyed"));
	itemData.push_back(ItemData(700035017, (char*)"Cuff Binder Pet", (char*)"Transforms pets appearance into Cuff binder."));
	itemData.push_back(ItemData(810700000, (char*)"Medium Level Seal Exchange Coupon", (char*)"its an exchange to receive 50 seal items. Seal item will seal characters affributet item."));
	itemData.push_back(ItemData(810116000, (char*)"MP Maestro Voucher(30 days)", (char*)"With [Familliar Trainer] Kate, exchange ite for an item that will allow recovery of MP 1920 for the cost of 10,500 Noah (no DC premium status) or 5,400 Noah (with the DC premium) Must have enough Noah in the inventory \n\n*can only be purchased by Npoints."));
	//itemData.push_back(ItemData(810507000, (char*)"Alencia Wing Exchange(Red)", (char*)"[Wing of Alencia] is available from a trade with [Merchant] Kaira in Moradon. Only 1 option will be applied among followings: Defense +50, Earring EXP +9%, Devotion +3"));
	//itemData.push_back(ItemData(800010000, (char*)"300 Defense+ Scroll(L)", (char*)"Increases Defense."));
	itemData.push_back(ItemData(800015000, (char*)"Speed+ Potion", (char*)"It increases characters speed.  It increases chracters speed. "));
	itemData.push_back(ItemData(518011000, (char*)"Magpie (ElMorad)", (char*)"Transforms your character to look like the cute NPC Magpie. Transforms your character to look like NPC Magpie (Elmorad)."));
	itemData.push_back(ItemData(518012000, (char*)"Magpie (Karus)", (char*)"Transforms your character to look like the cute NPC Magpie. Transforms your character to look like NPC Magpie (Karus)."));
	itemData.push_back(ItemData(501710000, (char*)"A. Mining (Golden)", (char*)"Automatic Mining (Golden)"));
	itemData.push_back(ItemData(501610000, (char*)"A. Mining", (char*)"Automatic Mining"));
	itemData.push_back(ItemData(501620000, (char*)"A. Fishing", (char*)"Automatic Fishing"));
	itemData.push_back(ItemData(501720000, (char*)"A. Fishing (Golden)", (char*)"Automatic Fishing (Golden)"));
	itemData.push_back(ItemData(810125000, (char*)"Robin Loot Voucher", (char*)"Robin Loot Voucher"));
	itemData.push_back(ItemData(914041877, (char*)"Offline Merchant", (char*)"Offline Merchant"));
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InitGUI, NULL, NULL, NULL);
}

int itrr = 0;

void UpdateUI(string nick, uint32 np, uint32 cash, uint32 bonuscash, uint8 level, uint8 reblevel, uint8 dd, uint8 axe, uint8 sword, uint8 mace, uint8 spear, uint8 bow, vector<string> _notices, uint32 _moneyreq) {
	Nick = nick;
	NationPoint = np;
	KnightCash = cash;
	KnightCashBonus = bonuscash;
	Level = level;
	RebLevel = reblevel;
	ddAc = dd;
	axeAc = axe;
	swordAc = sword;
	maceAc = mace;
	spearAc = spear;
	arrowAc = bow;
	notices = _notices;
	moneyreq = _moneyreq;
	if (itrr >= 2)
		tools = true;
	else
		itrr++;
}

void UpdateUICashInfo(uint32 cash, uint32 bonuscash) {
	KnightCash = cash;
	KnightCashBonus = bonuscash;
}

void UpdateInventory(vector<ITEM_DATA> inv)
{
	inventory = inv;
}

void LoadPUS(vector<PUSItem> _data) {
	pusItems = _data;
	specialItems.clear();
	powerItems.clear();
	costumeItems.clear();
	bundles.clear();
	for (PUSItem i : _data) {
		if (i.Category == SPECIAL) {
			specialItems.push_back(i);
			continue;
		}
		if (i.Category == POWER_UP) {
			powerItems.push_back(i);
			continue;
		}
		if (i.Category == COSTUME) {
			costumeItems.push_back(i);
			continue;
		}
		if (i.Category == BUNDLE) {
			bundles.push_back(i);
			continue;
		}
	}
	if (itrr >= 2)
		tools = true;
	else
		itrr++;
}

void ReleaseUI() {
	show = false;
	tools = false;
}