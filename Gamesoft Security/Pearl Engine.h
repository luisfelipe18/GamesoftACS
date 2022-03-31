#pragma once
#include "stdafx.h"
#include "PL_ProcessEvent.h"

class PearlEngine {
public:
	PearlEngine();
	virtual ~PearlEngine();
	User Player;
	void Disconnect();
	void Send(Packet* pkt);
	void Update();
	void StayAlive();
	void SendProcess(uint16 toWHO);
	void __stdcall Shutdown(std::string log);
private:
	void InitPlayer();
};

