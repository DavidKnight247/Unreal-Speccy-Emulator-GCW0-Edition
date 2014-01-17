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

#include "../std.h"
#include "../platform/io.h"
#include "memory.h"

#ifdef USE_EMBEDDED_RESOURCES
#include "res/rom/sos128_0.h"
#include "res/rom/sos128_1.h"
#include "res/rom/sos48.h"
#include "res/rom/service.h"
#include "res/rom/dos513f.h"
#endif//USE_EMBEDDED_RESOURCES

#ifdef USE_EXTERN_RESOURCES
extern byte sos128_0[];
extern byte sos128_1[];
extern byte sos48[];
extern byte service[];
extern byte dos513f[];
#endif//USE_EXTERN_RESOURCES

static struct eOptionResetToServiceRom : public xOptions::eOptionBool
{
	virtual const char* Name() const { return "reset to service rom"; }
} op_reset_to_service_rom;
DECLARE_OPTION_ACCESSOR(eOptionBool, op_reset_to_service_rom);

//=============================================================================
//	eMemory::eMemory
//-----------------------------------------------------------------------------
eMemory::eMemory() : rom_page_selected(0), mode_48k(false)
{
	memory = new byte[SIZE];
	memset(memory, 0, SIZE);
}
//=============================================================================
//	eMemory::~eMemory
//-----------------------------------------------------------------------------
eMemory::~eMemory()
{
	delete[] memory;
}
//=============================================================================
//	eMemory::Init
//-----------------------------------------------------------------------------
void eMemory::Init()
{
#if defined(USE_EMBEDDED_RESOURCES) || defined(USE_EXTERN_RESOURCES)
	memcpy(memory->Get(P_ROM_128_0),	sos128_0,	eMemory::PAGE_SIZE);
	memcpy(memory->Get(P_ROM_128_1),	sos128_1,	eMemory::PAGE_SIZE);
	memcpy(memory->Get(P_ROM_48),		sos48,		eMemory::PAGE_SIZE);
	memcpy(memory->Get(P_ROM_SYS),		service,	eMemory::PAGE_SIZE);
	memcpy(memory->Get(P_ROM_DOS),		dos513f,	eMemory::PAGE_SIZE);
#else//USE_EMBEDDED_RESOURCES
	LoadRom(P_ROM_128_0,	xIo::ResourcePath("res/rom/sos128_0.rom"));
	LoadRom(P_ROM_128_1,	xIo::ResourcePath("res/rom/sos128_1.rom"));
	LoadRom(P_ROM_48,		xIo::ResourcePath("res/rom/sos48.rom"));
	LoadRom(P_ROM_SYS,		xIo::ResourcePath("res/rom/service.rom"));
	LoadRom(P_ROM_DOS,		xIo::ResourcePath("res/rom/dos513f.rom"));
#endif//USE_EMBEDDED_RESOURCES
}
//=============================================================================
//	eMemory::Reset
//-----------------------------------------------------------------------------
void eMemory::Reset()
{
	mode_48k = false;
	SetRomPage(op_reset_to_service_rom ? P_ROM_SYS : P_ROM_128_1);
	SetPage(1, P_RAM5);
	SetPage(2, P_RAM2);
	SetPage(3, P_RAM0);
}
//=============================================================================
//	eMemory::SetPage
//-----------------------------------------------------------------------------
void eMemory::SetPage(int idx, int page)
{
	byte* addr = Get(page);
	bank_read[idx] = addr;
	bank_write[idx] = idx ? addr : NULL;
}
//=============================================================================
//	eMemory::Page
//-----------------------------------------------------------------------------
int	eMemory::Page(int idx)
{
	byte* addr = bank_read[idx];
	for(int p = 0; p < P_AMOUNT; ++p)
	{
		if(Get(p) == addr)
			return p;
	}
	assert(false);
	return -1;
}
//=============================================================================
//	eMemory::::LoadRom
//-----------------------------------------------------------------------------
void eMemory::LoadRom(int page, const char* rom)
{
	FILE* f = fopen(rom, "rb");
	assert(f);
	size_t s = fread(Get(page), 1, PAGE_SIZE, f);
	assert(s == PAGE_SIZE);
	fclose(f);
}
void eMemory::Mode48k(bool on)
{
	mode_48k = on;
	if(on)
	{
		SetRomPage(ROM_SOS());
		SetPage(3, P_RAM0);
	}
}
//=============================================================================
//	eMemory::IoWrite
//-----------------------------------------------------------------------------
bool eMemory::IoWrite(word port) const
{
	return !(port & 2) && !(port & 0x8000); // zx128 port
}
//=============================================================================
//	eMemory::IoWrite
//-----------------------------------------------------------------------------
void eMemory::IoWrite(word port, byte v, int tact)
{
	if(mode_48k)
		return;
	if(v & 0x20)
	{
		Mode48k(true);
		return;
	}
	SetRomPage((rom_page_selected & ~1) + ((v >> 4) & 1));
	int page = P_RAM0 + (v & 7);
	SetPage(3, page);
}
