/*
Portable ZX-Spectrum emulator.
Copyright (C) 2001-2010 SMT, Dexus, Alone Coder, deathsoft, djdron, scor

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef	__MEMORY_H__
#define	__MEMORY_H__

#include "device.h"

#pragma once

#undef PAGE_SIZE

//*****************************************************************************
//	eMemory
//-----------------------------------------------------------------------------
class eMemory : public eDevice
{
public:
	eMemory();
	virtual ~eMemory();
	virtual void Init();
	virtual void Reset();
	void ReadRom(word addr)
	{
		byte pc_h = addr >> 8;
		if(rom_page_selected == ROM_SOS() && (pc_h == 0x3d))
			SetRomPage(P_ROM_DOS);
		else if(DosSelected() && (pc_h & 0xc0)) // pc > 0x3fff closes tr-dos
			SetRomPage(ROM_SOS());
	}
	byte Read(word addr) const
	{
		byte* a = bank_read[(addr >> 14) & 3] + (addr & (PAGE_SIZE - 1));
		return *a;
	}
	void Write(word addr, byte v)
	{
		byte* a = bank_write[(addr >> 14) & 3];
		if(!a) //rom write prevent
			return;
		a += (addr & (PAGE_SIZE - 1));
		*a = v;
	}
	byte* Get(int page) { return memory + page * PAGE_SIZE; }
	bool DosSelected() const { return rom_page_selected == P_ROM_DOS; }
	enum ePage
	{
		P_ROM_128_1 = 0, P_ROM_128_0, P_ROM_SYS, P_ROM_DOS, P_ROM_48,
		P_RAM0, P_RAM1, P_RAM2, P_RAM3,
		P_RAM4, P_RAM5, P_RAM6, P_RAM7,
		P_AMOUNT
	};
	void SetRomPage(int page) { rom_page_selected = page; SetPage(0, rom_page_selected); }
	int	Page(int idx);
	int ROM_SOS() const { return mode_48k ? P_ROM_48 : P_ROM_128_0; }
	bool Mode48k() const { return mode_48k; }
	void Mode48k(bool on);

	virtual bool IoWrite(word port) const;
	virtual void IoWrite(word port, byte v, int tact);
	static eDeviceId Id() { return D_MEMORY; }
	virtual dword IoNeed() const { return ION_WRITE; }
	virtual const char* Name() const { return "memory"; }
	enum { BANKS_AMOUNT = 4, PAGE_SIZE = 0x4000, SIZE = P_AMOUNT * PAGE_SIZE };
protected:
	void LoadRom(int page, const char* rom);
	void SetPage(int idx, int page);
protected:
	byte* bank_read[BANKS_AMOUNT];
	byte* bank_write[BANKS_AMOUNT];
	byte* memory;
	int rom_page_selected;
	bool mode_48k;
};

#endif//__MEMORY_H__
