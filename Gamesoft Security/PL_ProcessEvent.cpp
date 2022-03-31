#include "PL_ProcessEvent.h"
#include <iostream>

#define MAX 10000

string forbiddenFiles[] = { "speedhack-i386.dll","speedhack-x86_64.dll", "d3dhook.dll", "d3dhook64.dll", "allochook-i386.dll", "allochook-x86_64.dll", "dbk32.sys", "dbk64.sys" };
string PLUGINS_X86[] = {"gamesoft_remote.pl"};
char PLUGIN_PATH[MAX_PATH];

string convertToString(char* a, int size);
extern void __stdcall LM_Shutdown(std::string log);
extern string WtoString(WCHAR s[]);

bool fexists(std::string& filename) {
	std::ifstream ifile(filename.c_str());
	return (bool)ifile;
}

BOOL IsWow64(HANDLE process)
{
	BOOL bIsWow64 = FALSE;

	typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(process, &bIsWow64))
		{
			//handle error
		}
	}
	return bIsWow64;
}


WCHAR* GetProcName(DWORD aPid)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);
	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	for (BOOL bok = Process32First(processesSnapshot, &processInfo); bok; bok = Process32Next(processesSnapshot, &processInfo))
	{
		if (aPid == processInfo.th32ProcessID)
		{
			CloseHandle(processesSnapshot);
			return processInfo.szExeFile;
		}

	}
	CloseHandle(processesSnapshot);
}



int PearlDriver::CreateProcessEvent(DWORD PID) {
	if (PID == GetCurrentProcessId()) return 1;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, PID);
	if (hProcess != NULL) {
		char path[MAX_PATH];

		HMODULE hModules[MAX] = { 0 };
		DWORD dwModuleSize = 0;

		EnumProcessModules(hProcess, hModules, sizeof(hModules), &dwModuleSize);
		dwModuleSize /= sizeof(HMODULE);

		for (HMODULE mdl : hModules) {
			GetModuleFileNameExA(hProcess, mdl, path, sizeof(path));
			PathRemoveFileSpecA(path);
			string pathProcess = convertToString(path, strlen(path));
			for (string fb : forbiddenFiles) {
				string filepath = pathProcess + "\\" + fb;
				if (fexists(filepath)) {
					TerminateProcess(hProcess, 0);
					__asm {
						MOV AL, 1
						MOV EBX, 0
						INT 80h
					}
					TerminateProcess(GetCurrentProcess(), 0);
				}
			}
		}
		if (!IsWow64(hProcess)) {
			for (string plugin : PLUGINS_X86) {
				char* thisPath = (char*)(string(PLUGIN_PATH) + "\\GameSoft\\plugins\\" + string(plugin)).c_str();
				LPVOID LoadLibAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
				LPVOID LoadPath = VirtualAllocEx(hProcess, NULL, strlen(thisPath), MEM_COMMIT, PAGE_READWRITE);
				if (LoadPath) {
					WriteProcessMemory(hProcess, (LPVOID)LoadPath, thisPath, strlen(thisPath), NULL);
					HANDLE RemoteThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibAddr, LoadPath, 0, 0);
					WaitForSingleObject(RemoteThread, INFINITE);
					CloseHandle(RemoteThread);
					VirtualFreeEx(hProcess, LoadPath, strlen(thisPath), MEM_RELEASE);
				}
			}
		}
		CloseHandle(hProcess);
	}
	return 1;
}

int PearlDriver::TerminateProcessEvent(DWORD PID) {
	return 1;
}

void PearlDriver::EventThread() {
	vector<DWORD> current;
	while (true) {
		current.clear();
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		unsigned int i;
		if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
			TerminateThread(GetCurrentThread(), 0);

		cProcesses = cbNeeded / sizeof(DWORD);
		for (i = 0; i < cProcesses; i++)
			if (aProcesses[i] != 0)
				current.push_back(aProcesses[i]);
		vector<DWORD> diff = getDiffrence(tmp, current);
		for (DWORD pid : diff) {
			if (current.size() > tmp.size())
				CreateProcessEvent(pid) ;
			else
				TerminateProcessEvent(pid);
		}
		if (diff.size() > 0) {
			tmp = current;
		}
		Sleep(30);
	}
}

string convertToString(char* a, int size)
{
	int i;
	string s = "";
	for (i = 0; i < size; i++) {
		s = s + a[i];
	}
	return s;
}

PearlDriver::PearlDriver() {
	HMODULE me = GetModuleHandle(L"KnightOnLine.exe");
	GetModuleFileNameA(me, PLUGIN_PATH, sizeof(PLUGIN_PATH));
	PathRemoveFileSpecA(PLUGIN_PATH);


	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;
	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	TerminateThread(GetCurrentThread(), 0);

	cProcesses = cbNeeded / sizeof(DWORD);
	for (i = 0; i < cProcesses; i++)
		if (aProcesses[i] != 0)
			tmp.push_back(aProcesses[i]);
	EventThread();
}

vector<DWORD> PearlDriver::getDiffrence(vector<DWORD> a, vector<DWORD> b) {
	vector <DWORD> ret;
	if (a.size() > b.size()) {
		for (DWORD itr : a) {
			if (!(std::find(b.begin(), b.end(), itr) != b.end())) {
				ret.push_back(itr);
			}
		}
	}
	else {
		for (DWORD itr : b) {
			if (!(std::find(a.begin(), a.end(), itr) != a.end())) {
				ret.push_back(itr);
			}
		}
	}
	return ret;
}
