#include "stdafx.h"
#include "PearlGui.h"
#include "Splash.h"

PearlEngine Engine;

std::string forbiddenModules[] ={ "dbk64", "dbk32", "hunter", "PROCEXP152", "BlackBoneDrv10", "since", "ntice", "winice", "syser", "77fba431"  };
char* sModule[] = { (char*)"SbieDll.dll", (char*)"dbghelp.dll" };
unsigned char	idtr[6];
unsigned long	idt = 0;
unsigned long pnsize = 0x1000;
unsigned int reax = 0;
unsigned char* pMem = NULL;
int it = 0;

extern string WtoString(WCHAR s[]);
extern string strToLower(string str);

LPVOID LM_CreateThread(LPVOID Thread)
{
	DWORD ThreadAdresi = 0x60000 + it;
	DWORD Old;
	VirtualProtect((LPVOID)ThreadAdresi, 0x100, PAGE_EXECUTE_READWRITE, &Old);
	CONTEXT ctx;
	HANDLE tHand = CreateRemoteThread(GetCurrentProcess(), 0, 0, (LPTHREAD_START_ROUTINE)ThreadAdresi, 0, 0, 0);
	SuspendThread(tHand);
	ctx.ContextFlags = CONTEXT_INTEGER;
	GetThreadContext(tHand, &ctx);
	ctx.Eax = (DWORD)Thread;
	ctx.ContextFlags = CONTEXT_INTEGER;
	SetThreadContext(tHand, &ctx);
	ResumeThread(tHand);
	it++;
	return (LPVOID)ctx.Eax;
}

void LM_DriverScan() {
	LPVOID drivers[ARRAY_SIZE];
	DWORD cbNeeded;
	int cDrivers, i;
	WCHAR szDriver[ARRAY_SIZE];

	while(true) {
		VIRTUALIZER_START
		if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers))
		{
			cDrivers = cbNeeded / sizeof(drivers[0]);
			for (i = 0; i < cDrivers; i++)
			{
				if (GetDeviceDriverBaseNameW(drivers[i], szDriver, sizeof(szDriver) / sizeof(szDriver[0])))
				{
					string driverName = strToLower(WtoString(szDriver));
					for(string fbDriver : forbiddenModules) {
						if (driverName.find(strToLower(fbDriver)) != std::string::npos)
						{
							string fname = "GameSoft\\engine\\helper.exe";
							string params = string_format("Forbidden Module: %s", driverName);
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
					}
				}
			}
		}
		Engine.StayAlive();
		VIRTUALIZER_END
		Sleep(10000);
	}
}

bool IsitaSandBox()
{
	unsigned char bBuffering;
	unsigned long aCreateProcesses = (unsigned long)GetProcAddress(GetModuleHandleA("KERNEL32.dll"), "CreateProcessA");

	ReadProcessMemory(GetCurrentProcess(), (void*)aCreateProcesses, &bBuffering, 1, 0);

	if (bBuffering == 0xE9)
	{
		return  1;
	}
	else {
		return 0;
	}

}

bool IsHWBreakpointExists()
{
	CONTEXT ctx;
	ZeroMemory(&ctx, sizeof(CONTEXT));
	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	HANDLE hThread = GetCurrentThread();
	if (GetThreadContext(hThread, &ctx) == 0)
		return false;

	if ((ctx.Dr0) || (ctx.Dr1) || (ctx.Dr2) || (ctx.Dr3)) {
		return true;
	}
	else {
		return false;
	}
}

void LM_Pearl() {
	VIRTUALIZER_START
	while (true) {
		Sleep(200);

		Engine.Update();

		if(IsDebuggerPresent())
			Engine.Shutdown("Debugger");

		//if (IsitaSandBox())
			//Engine.Shutdown("Virtual Machine");

		if(IsHWBreakpointExists())
			Engine.Shutdown("Debugger");

		if (*(unsigned long*)QueryPerformanceCounter != 2337669003)
			Engine.Shutdown("Thread Suspend");
		

		//if (FindWindowA("SandboxieControlWndClass", NULL)) 
			//Engine.Shutdown("Virtual Machine");

		//for (int i = 0; i < (sizeof(sModule) / sizeof(char*)); i++)
			//if (GetModuleHandleA(sModule[i]))
				//Engine.Shutdown("Virtual Machine");

	//	_asm sidt idtr
		//idt = *((unsigned long*)&idtr[2]);
		//if ((idt >> 24) == 0xff)
			//Engine.Shutdown("Virtual Machine");

		//char* provider = (char*)LocalAlloc(LMEM_ZEROINIT, pnsize);
		//int retv = WNetGetProviderNameA(WNNC_NET_RDR2SAMPLE, provider, &pnsize);
		//if (retv == NO_ERROR)
			//Engine.Shutdown("Virtual Machine");

		//__asm
		//{
		//	mov eax, 0xCCCCCCCC;
		//	smsw eax;
		//	mov DWORD PTR[reax], eax;
		//}

		//if ((((reax >> 24) & 0xFF) == 0xcc) && (((reax >> 16) & 0xFF) == 0xcc))
		//	Engine.Shutdown("Virtual Machine");
	}

	VIRTUALIZER_END
}

bool _fexists(std::string& filename) {
	std::ifstream ifile(filename.c_str());
	return (bool)ifile;
}

DWORD GetFileSizeOwn(char* FileName)
{
	ifstream f(FileName);

	DWORD fileSize = 0;

	if (f.good())
	{
		HANDLE hFile = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile)
		{
			fileSize = GetFileSize(hFile, NULL);
			CloseHandle(hFile);
		}
	}

	f.close();

	return fileSize;
}


typedef int (WINAPI* SetString) (int ebx);
SetString tSetString;

int WINAPI mSetString(int ebx)
{
	printf("EBX: 0x%08X\n", ebx);
	return tSetString(ebx);
}

extern  "C"  __declspec(dllexport) void __cdecl Init() {
	
	/*AllocConsole();
	freopen("CONOUT$", "w", stdout);*/
	DWORD addr = (DWORD)GetModuleHandle(NULL);
	__asm
	{
		mov eax, fs: [0x30]
		mov[addr], eax
	}

	string myname("gamesoft.xasm");
	if (!_fexists(myname)) {
		string fname = "GameSoft\\engine\\helper.exe";
		string params = "The file is corrupted. Please contact with administrator. Error: inside";
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
	string mynamee("GameSoft\\engine\\gamesoft.sys");
	if (!_fexists(mynamee)) {
		string fname = "GameSoft\\engine\\helper.exe";
		string params = "The file is corrupted. Please contact with administrator. Error: inside";
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
	char nm[100];
	GetModuleBaseNameA(GetCurrentProcess(), GetModuleHandle(NULL), nm, sizeof(nm));
	string _nm(nm);
	if (_nm != "KnightOnLine.exe") {
		string fname = "GameSoft\\engine\\helper.exe";
		string params = "The file is corrupted. Please contact with administrator. Error: outside";
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
	char szExeFileName[MAX_PATH];
	GetModuleFileNameA(NULL, szExeFileName, MAX_PATH);
	if (GetFileSizeOwn(szExeFileName) != 0x45F200)
	{
		string fname = "GameSoft\\engine\\helper.exe";
		string params = "The file is corrupted. Please contact with administrator. Error: outside";
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
	
	VIRTUALIZER_START
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & ~_CRTDBG_ALLOC_MEM_DF);
	
	UIMain(&Engine);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LM_Pearl, NULL, NULL, NULL);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LM_DriverScan, NULL, NULL, NULL);
	/*CSplash splash(TEXT("GameSoft\\gamesoft.bmp"), RGB(128, 128, 128));
	splash.ShowSplash();
	Sleep(1000);
	splash.CloseSplash();*/
	VIRTUALIZER_END
		
	
}


extern  "C"  __declspec(dllexport)
BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH: {
		//LoadLibraryA("GameSoft.plugin");
		DisableThreadLibraryCalls(hModule);
		
		Init();
		return TRUE;
	}
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}