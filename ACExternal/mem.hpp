#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <wil/resource.h>
#include <string>

class Memory {
private:
	wil::unique_handle gameHandle {nullptr};
	DWORD pid {0};

	DWORD GetPid(const std::string_view& processName) const {
		wil::unique_tool_help_snapshot snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

		PROCESSENTRY32 entry = {};
		entry.dwSize = sizeof(entry);

		if (Process32First(snapshot.get(), &entry)) {
			do {
				if (!processName.compare(entry.szExeFile)) {
					return entry.th32ProcessID;
				}
			} while (Process32Next(snapshot.get(), &entry));
		}
		return 0;
	}

public:
	explicit Memory(const std::string_view& processName) {
		pid = GetPid(processName);
		if (pid != 0)
			gameHandle.reset(OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));
	}

	uintptr_t GetModuleBaseAddress(std::string_view moduleName) {
		MODULEENTRY32 entry = { };
		entry.dwSize = sizeof(::MODULEENTRY32);

		wil::unique_tool_help_snapshot snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));

		std::uintptr_t result = 0;

		while (Module32Next(snapshot.get(), &entry))
		{
			if (!moduleName.compare(entry.szModule))
			{
				result = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
				break;
			}
		}

		return result;
	}

	template<typename T>
	T RPM(uintptr_t address) {
		T buffer;
		ReadProcessMemory(gameHandle.get(), (void*)address, &buffer, sizeof(buffer), NULL);
		return buffer;
	}

	template<typename T>
	void WPM(uintptr_t address, T buffer) {
		WriteProcessMemory(gameHandle.get(), (void*)address, &buffer, sizeof(buffer), NULL);
	}

	bool IsValid() {
		if (pid == 0 || gameHandle == nullptr) {
			return false;
		}
		return true;
	}
};
