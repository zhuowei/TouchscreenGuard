// TouchscreenGuard.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define REG_KEY_CONFIG TEXT("Software\\Zhuowei Zhang\\TouchscreenGuard")
#define MAX_VALUE_NAME 16383

static HWINEVENTHOOK shellHook;

static std::vector<std::wstring> disableApps;

static bool touchScreenStatus = true;

static HDEVINFO deviceInfoSet;
static DEVINST touchscreenDevInst;

static void setTouchscreenMode(bool enable, bool force) {
	if (touchScreenStatus == enable && !force) return;
	/* use SetupApi to enable the touchscreen device */
	SP_PROPCHANGE_PARAMS myParams;
	myParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
	myParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
	myParams.StateChange = enable ? DICS_ENABLE : DICS_DISABLE;
	myParams.Scope = DICS_FLAG_GLOBAL;
	myParams.HwProfile = 0;
	SP_DEVINFO_DATA devdata;
	devdata.cbSize = sizeof(devdata);
	BOOL success = SetupDiEnumDeviceInfo(deviceInfoSet, 0, &devdata);
	if (!success) {
		abort();
	}
	success = SetupDiSetClassInstallParams(deviceInfoSet, &devdata, &myParams.ClassInstallHeader, sizeof(myParams));
	if (!success) {
		DWORD lasterror = GetLastError();
		abort();
	}
	success = SetupDiChangeState(deviceInfoSet, &devdata);
	if (!success) {
		DWORD lasterror = GetLastError();
		abort();
	}
	if (enable) {
		OutputDebugString(L"enabling!\n");
	}
	else {
		OutputDebugString(L"disabling!\n");
	}
	touchScreenStatus = enable;
}

void CALLBACK ShellHookProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idobject, LONG idchild, DWORD dwEventThread, DWORD dwEventTime) {
	OutputDebugString(L"Window: ");
	if (hwnd == NULL) {
		OutputDebugString(L"No window!\n");
		return; //No focused window
	}
	DWORD processId;
	DWORD threadId = GetWindowThreadProcessId(hwnd, &processId);
	if (processId == 0) {
		OutputDebugString(L"FAIL\n");
		return;
	}
	HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, processId);
	if (processHandle == NULL) {
		OutputDebugString(L"PROCESS FAIL\n");
		return;
	}
	wchar_t programNameBuffer[MAX_PATH + 2];
	DWORD nameLength = MAX_PATH;
	QueryFullProcessImageName(processHandle, 0, programNameBuffer, &nameLength);
	OutputDebugString(programNameBuffer);
	OutputDebugString(L"\n");
	bool isInDisable = std::binary_search(disableApps.begin(), disableApps.end(), std::wstring(programNameBuffer));
	if (isInDisable) {
		OutputDebugString(L"Should disable here!\n");
	}
	setTouchscreenMode(!isInDisable, false);
}

void addAllValues(PTSTR pszz)
{
	while (*pszz)
	{
		disableApps.push_back(std::wstring(pszz));
		pszz = pszz + _tcslen(pszz) + 1;
	}
}

void readDisableAppsList() {
	HKEY disableListKey;
	TCHAR valueBuffer[MAX_VALUE_NAME];
	LONG success = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_CONFIG, 0, KEY_READ, &disableListKey);
	if (success != ERROR_SUCCESS) {
		abort();
	}
	DWORD myLen = MAX_VALUE_NAME;
	success = RegGetValue(disableListKey, NULL, L"DisableList", RRF_RT_REG_MULTI_SZ, NULL, valueBuffer, &myLen);
	if (success != ERROR_SUCCESS) {
		abort();
	}
	addAllValues(valueBuffer);
	RegCloseKey(disableListKey);
}


int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hinstprev, LPSTR cmdline, int cmdShow) {
	MSG msg;
	readDisableAppsList();
	std::sort(disableApps.begin(), disableApps.end());
	/* Grab the touchscreen: TODO automagically do this */
	deviceInfoSet = SetupDiGetClassDevs(NULL, L"HID\\VID_1B96&PID_0F03&COL03\\6&12988C71&0&0002", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
	if (deviceInfoSet == INVALID_HANDLE_VALUE) {
		DWORD lasterror = GetLastError();
		abort();
	}
	shellHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, ShellHookProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
	if (shellHook == NULL) {
		MessageBox(NULL, L"Failed to initialize hook", L"TouchscreenGuard", MB_ICONERROR);
	}
	setTouchscreenMode(true, true);
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UnhookWinEvent(shellHook);
	return 0;
}
