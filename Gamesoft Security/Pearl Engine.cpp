#include "Pearl Engine.h"
#include "md5.h"
#include "PearlGui.h"

PearlEngine* __Engine = nullptr;
bool uiINIT = false;
bool pusINIT = false;

vector<ProcessInfo> processTMP;
string tmpGraphics = "<unknown>";
string tmpProcessor = "<unknown>";

typedef int (WINAPI* MyOldRecv) (SOCKET, uint8*, int, int);
typedef int (WSAAPI* MyRecv) (SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef int (WINAPI* MySend) (SOCKET, char*, int, int);
typedef int (WINAPI* MyConnect) (SOCKET, const sockaddr*, int);
typedef int (WINAPI* MyClose) (IN SOCKET);
typedef int (WSAAPI* MyWSAConnect) (SOCKET, const sockaddr*, int, LPWSABUF, LPWSABUF, LPQOS, LPQOS);
typedef int (WSAAPI* MyWSAStartup) (WORD, LPWSADATA);
typedef int (WSAAPI* MyWSACleanup) ();
typedef int (WINAPI* MyTerminateProcess) (HANDLE hProcess, UINT uExitCode);
typedef int (WINAPI* MyExitProcess) (UINT uExitCode);

MyOldRecv OrigOldRecv = NULL;
MyRecv OrigRecv = NULL;
MySend OrigSend = NULL;
MyConnect OrigConnect = NULL;
MyClose OrigClose = NULL;
MyWSAConnect OrigWSAConnect = NULL;
MyWSAStartup OrigWSAStartup = NULL;
MyWSACleanup OrigWSACleanup = NULL;
MyTerminateProcess OrigTerminateProcess = NULL;
MyExitProcess OrigExitProcess = NULL;

bool allowAlive = true;
bool gameStarted = false;
bool isAlive = false;

HANDLE thisProc = NULL;
DWORD KO_ADR = 0x0;
DWORD KO_DLG = 0x0;
const DWORD KO_WH = 0x6C0;
const DWORD KO_PTR_PKT = 0x00E47844;
const DWORD KO_SND_FNC = 0x00497E40;
const DWORD KO_ACC = 0x00C957B8;

void LM_Send(Packet* pkt);
void __stdcall LM_Shutdown(std::string log, std::string graphicCards = tmpGraphics, std::string processor = tmpProcessor);
void LM_StayAlive();
void LM_SendProcess(uint16 toWHO);

void PearlEngine::Update() {
	
	if (isAlive) {
		if (thisProc == NULL) thisProc = GetCurrentProcess();
		if (KO_ADR == 0x0) ReadProcessMemory(thisProc, (LPCVOID)KO_PTR_CHR, &KO_ADR, sizeof(DWORD), 0);
		FreeLibrary(GetModuleHandleA("dinput8.dll"));
		if (gameStarted) {
			uint8 auth;
			ReadProcessMemory(thisProc, (LPCVOID)(KO_ADR + KO_WH), &auth, sizeof(auth), 0);
			if (this->Player.Authority != auth) {
				if (auth == USER || auth == GAMEMASTER || auth == BANNED) {
					tmpGraphics = "";
					for (string gpu : Player.GPU)
						tmpGraphics += " | " + gpu;
					this->Player.Authority = (UserAuthority)auth;
					Packet pkt(PL_AUTHINFO);
					pkt << auth << tmpGraphics << Player.CPU;
					this->Send(&pkt);
				}
			}
		}
	}
}

void __stdcall LM_Shutdown(std::string log, std::string graphicCards, std::string processor) {
	if (isAlive && gameStarted) {
		Packet pkt(PL_LOG);
		pkt << log << graphicCards << processor;
		LM_Send(&pkt);
	}
	string fname = "GameSoft\\engine\\helper.exe";
	string params = log;
	string result = "start " + fname + " \"" + params + "\"";
	system(result.c_str());
	exit(0);
	FreeLibrary(GetModuleHandle(NULL));
	__asm {
		MOV AL, 1
		MOV EBX, 0
		INT 80h
	}
	TerminateProcess(GetCurrentProcess(), 0);
}

void PearlEngine::Shutdown(std::string log) {
	tmpGraphics = "";
	for (string gpu : Player.GPU)
		tmpGraphics += " | " + gpu;
	LM_Shutdown(log, tmpGraphics, Player.CPU);
}

void LM_Send(Packet* pkt) {

	/*if (!allowAlive) {
		TerminateProcess(GetCurrentProcess(), 0);
		return;
	}*/

	if (isAlive) {

		uint8 opcode = pkt->GetOpcode();
		uint8* out_stream = nullptr;
		uint16 len = (uint16)(pkt->size() + 1);

		out_stream = new uint8[len];
		out_stream[0] = pkt->GetOpcode();

		if (pkt->size() > 0)
			memcpy(&out_stream[1], pkt->contents(), pkt->size());

		BYTE* ptrPacket = out_stream;
		SIZE_T tsize = len;

		__asm
		{
			mov ecx, KO_PTR_PKT
			mov ecx, DWORD ptr ds : [ecx]
			push tsize
			push ptrPacket
			call KO_SND_FNC
		}

		delete[] out_stream;
	}
}


bool dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	return false;
}


std::string getHWID() {
	HW_PROFILE_INFO hwProfileInfo;
	GetCurrentHwProfile(&hwProfileInfo);
	wstring hwidWString = hwProfileInfo.szHwProfileGuid;
	string hwid(hwidWString.begin(), hwidWString.end());
	return hwid;
}

void GetMacHash(uint16& MACOffset1, uint16& MACOffset2);
uint16 GetCPUHash();

uint16 GetVolumeHash()
{
	DWORD SerialNum = 0;
	GetVolumeInformationA("C:\\", NULL, 0, &SerialNum, NULL, NULL, NULL, 0);
	uint16 nHash = (uint16)((SerialNum + (SerialNum >> 16)) & 0xFFFF);
	return nHash;
}

int64 GetHardwareID()
{
	uint16 MACData1, MACData2 = 0;
	GetMacHash(MACData1, MACData2);
	return _atoi64(string_format("%d%d%d%d", MACData1, MACData2, GetCPUHash(), GetVolumeHash()).c_str());
}

uint16 HashMacAddress(PIP_ADAPTER_INFO info)
{
	uint16 nHash = 0;
	for (uint32 i = 0; i < info->AddressLength; i++)
		nHash += (info->Address[i] << ((i & 1) * 8));
	return nHash;
}


void GetMacHash(uint16& MACOffset1, uint16& MACOffset2)
{
	IP_ADAPTER_INFO AdapterInfo[32];
	DWORD dwBufLen = sizeof(AdapterInfo);

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS)
		return;

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
	MACOffset1 = HashMacAddress(pAdapterInfo);
	if (pAdapterInfo->Next)
		MACOffset2 = HashMacAddress(pAdapterInfo->Next);

	if (MACOffset1 > MACOffset2)
	{
		uint16 TempOffset = MACOffset2;
		MACOffset2 = MACOffset1;
		MACOffset1 = TempOffset;
	}
}

uint16 GetCPUHash()
{
	int CPUInfo[4] = { 0, 0, 0, 0 };
	__cpuid(CPUInfo, 0);
	uint16 nHash = 0;
	uint16* nPointer = (uint16*)(&CPUInfo[0]);
	for (uint16 i = 0; i < 8; i++)
		nHash += nPointer[i];

	return nHash;
}

static bool lock = false;

void HandlePacket(Packet pkt) {
	uint8 cmd = pkt.GetOpcode();
	lock = true;
	switch (cmd) {
	case PL_ITEM_PROCESS:
	{
		
	}
	break;
	case PL_DROP_LIST:
	{
		uint32 resultCount;
		pkt >> resultCount;
		vector<DROP_RESULT> result;
		for (int i = 0; i < resultCount; i++) {
			uint8 dropCount;
			string mobname, mobzone;
			vector<DROP> drops;
			pkt >> mobname >> mobzone >> dropCount;
			for (int j = 0; j < dropCount; j++) {
				string itemname;
				uint16 droprate;
				pkt >> itemname >> droprate;
				drops.push_back(DROP(itemname, droprate));
				printf("%s, %d\n", itemname.c_str(), droprate);
			}
			result.push_back(DROP_RESULT(mobname, drops, mobzone));
		}
		
	}
	break;
	case PL_CASHCHANGE:
	{
		uint32 cash, bonuscash;
		pkt >> cash >> bonuscash;
		UpdateUICashInfo(cash, bonuscash);
	}
	break;
	case PL_PUS:
	{
		
		uint32 itemCount;
		vector<PUSItem> items;
		pkt >> itemCount;
		for (uint32 i = 0; i < itemCount; i++) {
			uint32 sid;
			uint32 price;
			uint8 cat;
			pkt >> sid >> price >> cat;
			items.push_back(PUSItem(sid, price, (PUS_CATEGORY)cat));
		}
		pusINIT = true;
		LoadPUS(items);
	}
	break;
	case PL_PROCINFO:
	{
		uint16 toWHO = 0;
		pkt >> toWHO;
		printf("toWHO: %d\n", toWHO);
		LM_SendProcess(toWHO);
	}
	break;
	case PL_ALIVE:
		LM_StayAlive();
		break;
	case PL_OPEN: {
		string address;
		pkt >> address;
		ShellExecuteA(NULL, "open", address.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
				break;
	case(0x0D):
	{
		Packet pkt(PL_UIINFO);
		LM_Send(&pkt);
		gameStarted = true;
	}
		
		break;
	case(0x0E):
	{
		Packet pkt(PL_UIINFO);
		LM_Send(&pkt);
		gameStarted = true;
	}
		break;
	case(0x2E):
	{
		Packet pkt(PL_UIINFO);
		LM_Send(&pkt);
		gameStarted = true;
	}
		break;
	case(0x01):
	{
		char AccName[25];
		ReadProcessMemory(GetCurrentProcess(), (LPVOID)KO_ACC, AccName, sizeof(AccName), NULL);
		string m_strAccountID = string(AccName);
		uint16 MACData1, MACData2 = 0;
		GetMacHash(MACData1, MACData2);
		int64 UserHardwareID = GetHardwareID();
		Packet result(0xA1);
		result << uint8(22) << m_strAccountID << uint32(MACData1 + MACData2) << int64(UserHardwareID);
		LM_Send(&result);

	}
	break;
	
	case PL_UIINFO:
	{
		vector<string> notices;
		string nick;
		uint32 np, cash, bonuscash, moneyreq;
		uint8 level, reblevel, dd, axe, sword, mace, spear, bow, noticeCount;
		pkt >> nick >> level >> reblevel >> np >> cash >> bonuscash >> dd >> axe >> sword >> mace >> spear >> bow >> noticeCount >> moneyreq;
		for (int i = 0; i < noticeCount; i++) {
			string notice;
			pkt >> notice;
			notices.push_back(notice);
		}
		/*__Engine->Player.Nick = nick;
		__Engine->Player.Level = level;
		__Engine->Player.RebLevel = reblevel;
		__Engine->Player.NationPoint = np;
		__Engine->Player.KnightCash = cash;
		__Engine->Player.ddAc = dd;
		__Engine->Player.axeAc = axe;
		__Engine->Player.swordAc = sword;
		__Engine->Player.maceAc = mace;
		__Engine->Player.spearAc = spear;
		__Engine->Player.arrowAc = bow;*/

		UpdateUI(nick, np, cash, bonuscash, level, reblevel, dd, axe, sword, mace, spear, bow, notices, moneyreq);
		uiINIT = true;
	}
	break;
	case 0x1F: //WIZ_ITEM_MOVE
	{
		//Clear the packets before.
		uint16 a1, a2, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17;
		uint32 a3;
		uint8 a00, a0, a4;
		short a5, a6;
		pkt >> a00 >> a0 >> a1 >> a2 >> a3 >> a4 >> a5 >> a6 >> a7 >> a8 >> a9 >> a10 >> a11 >> a12 >> a13 >> a14 >> a15 >> a16 >> a17;
		//Handle our new packets
		vector<string> notices;
		string nick;
		uint32 np, cash, bonuscash, moneyreq;
		uint8 level, reblevel, dd, axe, sword, mace, spear, bow, noticeCount;
		pkt >> nick >> level >> reblevel >> np >> cash >> bonuscash >> dd >> axe >> sword >> mace >> spear >> bow >> noticeCount >> moneyreq;
		for (int i = 0; i < noticeCount; i++) {
			string notice;
			pkt >> notice;
			notices.push_back(notice);
		}
		UpdateUI(nick, np, cash, bonuscash, level, reblevel, dd, axe, sword, mace, spear, bow, notices, moneyreq);
		uiINIT = true;
	}
	break;
	}
	lock = false;
}

/* Connection Gain or Lose */


int WINAPI ConnectDetour(SOCKET s, const sockaddr* name, int namelen) {
	struct sockaddr_in* addr_in = (struct sockaddr_in*)name;
	char* _s = inet_ntoa(addr_in->sin_addr);
	string hostAddress = string(_s);
	if(hostAddress != "37.156.246.70") // IP ADDRESS
		return WSAECONNREFUSED;
	return OrigConnect(s, name, namelen);
}
int WINAPI CloseDetour(IN SOCKET s) {
	return OrigClose(s);
}

int WSAAPI WSAStartupDetour(WORD wVersionRequired, LPWSADATA lpWSAData) {
	isAlive = true;
	return OrigWSAStartup(wVersionRequired, lpWSAData);
}
int WSAAPI WSAConnectDetour(SOCKET s, const sockaddr* name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS) {
	struct sockaddr_in* addr_in = (struct sockaddr_in*)name;
	char* _s = inet_ntoa(addr_in->sin_addr);
	string hostAddress = string(_s);
	if (hostAddress != "37.156.246.70") //IP ADDRESS
		return WSAECONNREFUSED;
	return OrigWSAConnect(s, name, namelen, lpCalleeData, lpCalleeData, lpSQOS, lpGQOS);
}

int WSAAPI WSACleanupDetour() {
	//isAlive = false;
	return OrigWSACleanup();
}

/* ----------------------- */

int WINAPI CommonRecv(uint8* buf) {
	Packet pkt;
	uint16 header;
	uint16 length;
	uint16 footer;
	memcpy(&header, buf, 2);
	memcpy(&length, buf + 2, 2);
	memcpy(&footer, buf + 4 + length, 2);
	uint8* in_stream = new uint8[length];
	memcpy(in_stream, buf + 4, length);

	if (length > 0)
		length--;

	pkt = Packet(in_stream[0], (size_t)length);
	if (length > 0)
	{
		pkt.resize(length);
		memcpy((void*)pkt.contents(), &in_stream[1], length);
	}

	delete[]in_stream;

	if (pkt.GetOpcode() == 0x0F)
		ReleaseUI();

	if (gameStarted && !uiINIT)
	{
		Packet pkt(PL_UIINFO);
		LM_Send(&pkt);
	}
	if (gameStarted && !pusINIT && uiINIT) {
		Packet pkt(PL_PUS);
		pkt << uint8(0);
		LM_Send(&pkt);
	}
	if(!lock)
		HandlePacket(pkt);
	return 1;
}

int WINAPI OldRecvDetour(SOCKET s, uint8* buf, int len, int flags) {
	//if (!allowAlive) return 0x90;
	CommonRecv(buf);
	return OrigOldRecv(s, buf, len, flags);
}


int WINAPI RecvDetour(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	uint8* buf = (uint8*)lpBuffers->buf;
	//if (!allowAlive) return 0x90;
	CommonRecv(buf);
	return OrigRecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
}

int WINAPI SendDetour(SOCKET s, char* buf, int len, int flags) {
	Packet pkt;
	uint16 header;
	uint16 length;
	uint16 footer;
	memcpy(&header, buf, 2);
	memcpy(&length, buf + 2, 2);
	memcpy(&footer, buf + 4 + length, 2);
	uint8* in_stream = new uint8[length];
	memcpy(in_stream, buf + 4, length);

	if (length > 0)
		length--;

	pkt = Packet(in_stream[0], (size_t)length);
	if (length > 0)
	{
		pkt.resize(length);
		memcpy((void*)pkt.contents(), &in_stream[1], length);
	}

	delete[]in_stream;

	uint8 cmd = pkt.GetOpcode();
	if(cmd == 0x0F)
		ReleaseUI();
	if (cmd == 0x0D || cmd == 0x0E || cmd == 0x2E && !gameStarted) gameStarted = true;
	return OrigSend(s, buf, len, flags);
}

string strToLower(string str) {
	for (auto& c : str) c = tolower(c);
	return str;
}

string WtoString(WCHAR s[]) {
	wstring ws(s);
	string ret(ws.begin(), ws.end());
	return ret;
}

int currentID = 0;
vector<string> activeWindows;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	if (currentID != dwProcessId) return TRUE;
	char String[255];
	if (!hWnd)
		return TRUE;
	if (!::IsWindowVisible(hWnd))
		return TRUE;
	if (!SendMessage(hWnd, WM_GETTEXT, sizeof(String), (LPARAM)String))
		return TRUE;
	if (IsWindowVisible(hWnd))
	{
		char wnd_title[256];
		GetWindowTextA(hWnd, wnd_title, sizeof(wnd_title));
		activeWindows.push_back(string((wnd_title)));
	}
	return TRUE;
}

bool inArray(vector<string> arr, string obj) {
	for (string i : arr) {
		if (obj == i) return true;
	}
	return false;
}

void PearlEngine::InitPlayer() {
	ofstream logFile;
	if (!dirExists("GameSoft")) {
		_wmkdir(_T("GameSoft"));
	}
	logFile.open("GameSoft\\init_log.txt");
	Player.Nick = "";
	Player.Level = 0;
	Player.RebLevel = 0;
	Player.NationPoint = 0;
	Player.KnightCash = 0;
	Player.ddAc = 0;
	Player.axeAc = 0;
	Player.swordAc = 0;
	Player.maceAc = 0;
	Player.arrowAc = 0;
	Player.spearAc = 0;
	this->Player.Authority = USER;
	uint16 MACData1, MACData2 = 0;
	GetMacHash(MACData1, MACData2);
	this->Player.MAC = uint32(MACData1 + MACData2);
	//Init GPU info
	DISPLAY_DEVICE DevInfo;
	DevInfo.cb = sizeof(DISPLAY_DEVICE);
	DWORD iDevNum = 0;
	logFile << "-------------------------------------------- GAMESOFT Initializing --------------------------------------------" << endl;
	while (EnumDisplayDevices(NULL, iDevNum, &DevInfo, 0))
	{
		if (inArray(this->Player.GPU, WtoString(DevInfo.DeviceString))) {
			iDevNum++;
			continue;
		}
		this->Player.GPU.push_back(WtoString(DevInfo.DeviceString));
		iDevNum++;
		logFile << "------ GPU: " << WtoString(DevInfo.DeviceString).c_str() << endl;
	}
	tmpGraphics = "";
	for (string gpu : Player.GPU)
		tmpGraphics += " | " + gpu;
	//Init processor info
	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);
	int CPUInfo[4] = { -1 };
	__cpuid(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];
	char CPUBrandString[0x40] = { 0 };
	for (unsigned int i = 0x80000000; i <= nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		if (i == 0x80000002)
		{
			memcpy(CPUBrandString,
				CPUInfo,
				sizeof(CPUInfo));
		}
		else if (i == 0x80000003)
		{
			memcpy(CPUBrandString + 16,
				CPUInfo,
				sizeof(CPUInfo));
		}
		else if (i == 0x80000004)
		{
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
		}
	}
	this->Player.CPU = string(CPUBrandString) + " | " + to_string(siSysInfo.dwNumberOfProcessors) + " Core(s)";
	logFile << "------ CPU: " << this->Player.CPU.c_str() << endl;
	tmpProcessor = Player.CPU;
	//Init hwid info
	this->Player.HWID = GetHardwareID();
	//Init screen info
	ScreenInfo* screen = new ScreenInfo();
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	screen->height = desktop.bottom;
	screen->width = desktop.right;
	this->Player.Screen = screen;
	//Init processes
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;
	if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		cProcesses = cbNeeded / sizeof(DWORD);
		for (i = 0; i < cProcesses; i++)
		{
			if (aProcesses[i] != 0) {
				char szProcessName[MAX_PATH];
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
				if (NULL != hProcess)
				{
					HMODULE hMod;
					DWORD cbNeeded;
					if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
					{
						GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
						ProcessInfo procInfo;
						procInfo.id = aProcesses[i];
						procInfo.name = szProcessName;
						currentID = aProcesses[i];
						activeWindows.clear();
						EnumWindows(EnumWindowsProc, NULL);
						for (string windowName : activeWindows) {
							procInfo.windows.push_back(windowName);
						}
						this->Player.Processes.push_back(procInfo);
					}
				}
			}
		}
	}
	for (ProcessInfo procInfo : this->Player.Processes) {
		if (procInfo.windows.size() == 0) continue;
		logFile << "------ Proccess: [" << procInfo.name.c_str() << "]" << endl;
		vector<string> windows = procInfo.windows;
		for (string wnd : windows) {
			logFile << "------------- Window: [" << wnd.c_str() << "]" << endl;
		}
	}
	logFile << "---------------------------------------------------------------------------------------------------------------" << endl;
	logFile.close();

	processTMP = Player.Processes;

	OrigOldRecv = (MyOldRecv)DetourFunction((PBYTE)recv, (PBYTE)OldRecvDetour);
	OrigRecv = (MyRecv)DetourFunction((PBYTE)WSARecv, (PBYTE)RecvDetour);
	OrigSend = (MySend)DetourFunction((PBYTE)send, (PBYTE)SendDetour);
	OrigConnect = (MyConnect)DetourFunction((PBYTE)connect, (PBYTE)2ConnectDetour);
	OrigClose = (MyClose)DetourFunction((PBYTE)closesocket, (PBYTE)CloseDetour);
	OrigWSAConnect = (MyWSAConnect)DetourFunction((PBYTE)WSAConnect, (PBYTE)WSAConnectDetour);
	OrigWSAStartup = (MyWSAStartup)DetourFunction((PBYTE)WSAStartup, (PBYTE)WSAStartupDetour);
	OrigWSACleanup = (MyWSACleanup)DetourFunction((PBYTE)WSACleanup, (PBYTE)WSACleanupDetour);
}

void LM_SendProcess(uint16 toWHO) {
	processTMP.clear();
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;
	if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		cProcesses = cbNeeded / sizeof(DWORD);
		for (i = 0; i < cProcesses; i++)
		{
			if (aProcesses[i] != 0) {
				char szProcessName[MAX_PATH];
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
				if (NULL != hProcess)
				{
					HMODULE hMod;
					DWORD cbNeeded;
					if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
					{
						GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
						ProcessInfo procInfo;
						procInfo.id = aProcesses[i];
						procInfo.name = szProcessName;
						currentID = aProcesses[i];
						activeWindows.clear();
						EnumWindows(EnumWindowsProc, NULL);
						for (string windowName : activeWindows) {
							procInfo.windows.push_back(windowName);
						}
						processTMP.push_back(procInfo);
					}
				}
			}
		}
	}

	Packet pkt(PL_PROCINFO);
	pkt << uint16(toWHO) << uint32(processTMP.size());
	for (ProcessInfo proc : processTMP) {
		pkt << int(proc.id) << string(proc.name) << int(proc.windows.size());
		for (string window : proc.windows)
			pkt << string(window);
	}
	LM_Send(&pkt);
}

void PearlEngine::SendProcess(uint16 toWHO) {
	LM_SendProcess(toWHO);
}

void PearlEngine::Disconnect() {
	allowAlive = false;
}

void PearlEngine::Send(Packet* pkt) {
	LM_Send(pkt);
}


void DriverHandle(PearlEngine _classobj) {
	PearlDriver driver;
}

void LM_StayAlive() {
	if (isAlive && allowAlive) {
		char AccName[25];
		ReadProcessMemory(GetCurrentProcess(), (LPVOID)KO_ACC, AccName, sizeof(AccName), NULL);
		string m_strAccountID = string(AccName);
		Packet pkt(PL_ALIVE);
		string public_key = md5("0x" + std::to_string(PL_VERSION) + "10001" + m_strAccountID);
		pkt << public_key;
		LM_Send(&pkt);
	}
}

void PearlEngine::StayAlive() {
	LM_StayAlive();
}

int WINAPI hTerminateProcess(HANDLE hProcess, UINT uExitCode) {
	if (hProcess == GetCurrentProcess())
		ReleaseUI();
	return OrigTerminateProcess(hProcess, uExitCode);
}

int WINAPI hExitProcess(UINT uExitCode) {
	ReleaseUI();
	return OrigExitProcess(uExitCode);
}

PearlEngine::PearlEngine() {
	this->InitPlayer();
	OrigTerminateProcess = (MyTerminateProcess)DetourFunction((PBYTE)TerminateProcess, (PBYTE)hTerminateProcess);
	OrigExitProcess = (MyExitProcess)DetourFunction((PBYTE)ExitProcess, (PBYTE)hExitProcess);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)DriverHandle, this, NULL, NULL);
	__Engine = this;
}

PearlEngine::~PearlEngine() {

}