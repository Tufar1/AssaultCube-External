#pragma once
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>

HANDLE gamehandle = nullptr;
DWORD pid = 0;

DWORD GetPid(const char* processName) {
	HANDLE snapshot;
	PROCESSENTRY32 entry;

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return 0;

	entry.dwSize = sizeof(entry);

	if (!Process32First(snapshot, &entry)) {
		CloseHandle(snapshot);
		return 0;
	}

	do {
		if (!_stricmp(entry.szExeFile, processName)) {
			CloseHandle(snapshot);
			return entry.th32ProcessID;
		}
	} while (Process32Next(snapshot, &entry));

	CloseHandle(snapshot);
	return 0;
}

uintptr_t GetModuleBaseAddress(const char* modName) {
	uintptr_t modBaseAddr = NULL;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (hSnap != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry)) {
			do {
				if (!_stricmp(modEntry.szModule, modName)) {
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}

bool GetProcessHandle(const char* processname) {
    pid = GetPid(processname);

    if(pid != 0){
		gamehandle = OpenProcess(PROCESS_ALL_ACCESS, NULL, pid);

	    return true;
    }
    return false;
}

void end() {
	CloseHandle(gamehandle);
}

template<typename T>
T RPM(uintptr_t address) {
	T buffer;
	ReadProcessMemory(gamehandle, (void*)address, &buffer, sizeof(buffer), NULL);
	return buffer;
}

template<typename T>
void WPM(uintptr_t address, T buffer) {
	WriteProcessMemory(gamehandle, (void*)address, &buffer, sizeof(buffer), NULL);
}