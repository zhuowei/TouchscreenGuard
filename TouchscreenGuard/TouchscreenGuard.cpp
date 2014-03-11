// TouchscreenGuard.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

static HWINEVENTHOOK shellHook;

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
}


int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hinstprev, LPSTR cmdline, int cmdShow) {
	MSG msg;
	shellHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, ShellHookProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
	if (shellHook == NULL) {
		MessageBox(NULL, L"Failed to initialize hook", L"TouchscreenGuard", MB_ICONERROR);
	}
	//HWND window = CreateWindowEx(0, TEXT("Scratch"), TEXT("Scratch"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hinstance, 0);
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UnhookWinEvent(shellHook);
	return 0;
}
