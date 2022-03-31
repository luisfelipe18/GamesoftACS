#pragma once
#include "stdafx.h"
#include <tlhelp32.h>

using namespace std;

class PearlDriver {
public:
	PearlDriver();
	int CreateProcessEvent(DWORD PID);
	int TerminateProcessEvent(DWORD PID);
private:
	vector<DWORD> tmp;
	void EventThread();
	vector<DWORD> getDiffrence(vector<DWORD> a, vector<DWORD> b);
};