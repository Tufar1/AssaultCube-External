#pragma once

namespace offset {
	uintptr_t LocalPlayer = 0x18AC00;
	uintptr_t EntityList = 0x18AC04;
	uintptr_t PlayerCount = 0x18AC0C;
	uintptr_t viewMatrix = 0x17DFD0;

	uintptr_t X = 0x4;
	uintptr_t Y = 0x8;
	uintptr_t ZHead = 0xC;
	uintptr_t Zfoot = 0x30;

	uintptr_t viewAngleX = 0x34;
	uintptr_t viewAngleY = 0x38;

	uintptr_t Health = 0xEC;
	uintptr_t Name = 0x205;
	uintptr_t Team = 0x30C;
}