#pragma once
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tchar.h>
#include <initguid.h>
#include <wmistr.h>
#include <array>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <stdio.h>
#include <shlwapi.h>
#include <accctrl.h>
#include <aclapi.h>
#include <shlobj_core.h>
#include <tlhelp32.h>

DEFINE_GUID(WmiMonitorID_GUID, 0x671a8285, 0x4edb, 0x4cae, 0x99, 0xfe, 0x69, 0xa1, 0x5c, 0x48, 0xc0, 0xbc);
typedef struct WmiMonitorID {
	USHORT ProductCodeID[16];
	USHORT SerialNumberID[16];
	USHORT ManufacturerName[16];
	UCHAR WeekOfManufacture;
	USHORT YearOfManufacture;
	USHORT UserFriendlyNameLength;
	USHORT UserFriendlyName[1];
} WmiMonitorID, * PWmiMonitorID;
#define OFFSET_TO_PTR(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))

typedef HRESULT(WINAPI* WOB) (IN LPGUID lpGUID, IN DWORD nAccess, OUT LONG*);
WOB WmiOpenBlock;
typedef HRESULT(WINAPI* WQAD) (IN LONG hWMIHandle, ULONG* nBufferSize, OUT UCHAR* pBuffer);
WQAD WmiQueryAllData;
typedef HRESULT(WINAPI* WCB) (IN LONG);
WCB WmiCloseBlock;

class Serial
{
public:
	void showSerials();
private:
	void killWinmgt();
	void executeAndDisplay(const std::string& title, const std::string& command);
	void executeCommand(const std::string& command, std::stringstream& output);
	void DisplayTitle(const std::string& title);
	void retrieveMonitorInformation();
	const int Width = 80;
	const char LINE_CHAR = '*';
	const std::string ANSI_COL1 = "\x1b[36m";
	const std::string ANSI_COL2 = "\x1b[37m";
	const std::string ANSI_RESET = "\x1b[0m";
};

