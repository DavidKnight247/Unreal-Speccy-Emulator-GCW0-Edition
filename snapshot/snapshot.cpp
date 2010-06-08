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
#include "../z80/z80.h"
#include "../devices/memory.h"
#include "../devices/ula.h"
#include "../speccy.h"

#include "snapshot.h"

namespace xSnapshot
{

#pragma pack(push, 1)
struct eSnapshot_SNA
{
	byte i;
	word alt_hl, alt_de, alt_bc, alt_af;
	word hl, de, bc, iy, ix;
	byte iff1; // 00 - reset, FF - set
	byte r;
	word af, sp;
	byte im, pFE;
	byte page5[eMemory::PAGE_SIZE];	// 4000-7FFF
	byte page2[eMemory::PAGE_SIZE];	// 8000-BFFF
	byte page[eMemory::PAGE_SIZE];	// C000-FFFF
	// 128k extension
	word pc;
	byte p7FFD;
	byte trdos;
	byte pages[6 * eMemory::PAGE_SIZE]; // all other pages (can contain duplicated page 5 or 2)
	enum { S_48 = 49179, S_128_5 = 131103, S_128_6 = 147487 };
};

struct eSnapshot_Z80
{
	byte a,f;
	word bc,hl,pc,sp;
	byte i,r,flags;
	word de,bc1,de1,hl1;
	byte a1,f1;
	word iy,ix;
	byte iff1, iff2, im;
	/* 2.01 extension */
	word len, newpc;
	byte model, p7FFD;
	byte r1,r2, p7FFD_1;
	byte AY[16];
	/* 3.0 extension */
};
#pragma pack(pop)


struct eZ80Accessor : public xZ80::eZ80
{
	bool SetState(const eSnapshot_SNA* s, size_t buf_size);
	size_t StoreState(eSnapshot_SNA* s);
	bool SetState(const eSnapshot_Z80* s, size_t buf_size);
	void UnpackPage(byte* dst, int dstlen, byte* src, int srclen);
};
bool eZ80Accessor::SetState(const eSnapshot_SNA* s, size_t buf_size)
{
	bool sna48 = (buf_size == eSnapshot_SNA::S_48);
	bool sna128 = (buf_size == eSnapshot_SNA::S_128_5) || (buf_size == eSnapshot_SNA::S_128_6);
	if(!sna48 && !sna128)
		return false;

	alt.af = s->alt_af;
	alt.bc = s->alt_bc;
	alt.de = s->alt_de;
	alt.hl = s->alt_hl;
	af = s->af;
	bc = s->bc;
	de = s->de;
	hl = s->hl;
	ix = s->ix;
	iy = s->iy;
	sp = s->sp;
	pc = s->pc;
	i = s->i;
	r_low = s->r;
	r_hi = s->r & 0x80;
	im = s->im;
	iff1 = s->iff1 ? 1 : 0;

	devices->IoWrite(0xfe, s->pFE, t);
	int p_size = eMemory::PAGE_SIZE;
	memcpy(memory->Get(eMemory::P_RAM5), s->page5, p_size);
	memcpy(memory->Get(eMemory::P_RAM2), s->page2, p_size);
	int p = sna48 ? 0 : (s->p7FFD & 7);
	memcpy(memory->Get(eMemory::P_RAM0 + p), s->page, p_size);

	if(sna48)
	{
		pc = memory->Read(sp) + 0x100 * memory->Read(sp+1);
		sp += 2;
		memory->SetPage(0, eMemory::P_ROM1);
		return true;
	}
	devices->IoWrite(0x7ffd, s->p7FFD, t);
	memory->SetPage(0, s->trdos ? eMemory::P_ROM3 : eMemory::P_ROM1);
	const byte* page = s->pages;
	byte mapped = 0x24 | (1 << (s->p7FFD & 7));
	for(int i = 0; i < 8; ++i)
	{
		if(!(mapped & (1 << i)))
		{
			memcpy(memory->Get(eMemory::P_RAM0 + i), page, p_size);
			page += p_size;
		}
	}
	return true;
}
size_t eZ80Accessor::StoreState(eSnapshot_SNA* s)
{
	s->trdos = devices->Get<eRom>()->DosSelected();
	s->alt_af = alt.af; s->alt_bc = alt.bc;
	s->alt_de = alt.de; s->alt_hl = alt.hl;
	s->af = af; s->bc = bc; s->de = de; s->hl = hl;
	s->ix = ix; s->iy = iy; s->sp = sp; s->pc = pc;
	s->i = i; s->r = (r_low & 0x7F)+r_hi; s->im = im;
	s->iff1 = iff1 ? 0xFF : 0;
	byte p7FFD = memory->Page(3) - eMemory::P_RAM0;
	if(!devices->Get<eUla>()->FirstScreen())
		p7FFD |= 0x08;
	byte pFE = devices->Get<eUla>()->BorderColor();
	s->p7FFD = p7FFD;
	s->pFE = pFE;
	byte mapped = 0x24 | (1 << (p7FFD & 7));
	if(devices->Get<eRam>()->Mode48k())
	{
		mapped = 0xff;
		s->sp -= 2;
		memory->Write(s->sp, pc_l);
		memory->Write(s->sp + 1, pc_h);
	}
	memcpy(s->page5, memory->Get(eMemory::P_RAM5), eMemory::PAGE_SIZE);
	memcpy(s->page2, memory->Get(eMemory::P_RAM2), eMemory::PAGE_SIZE);
	memcpy(s->page,  memory->Get(eMemory::P_RAM0 + (p7FFD & 7)), eMemory::PAGE_SIZE);
	byte* page = s->pages;
	int stored_128_pages = 0;
	for(byte i = 0; i < 8; i++)
	{
		if(!(mapped & (1 << i)))
		{
			memcpy(page, memory->Get(eMemory::P_RAM0 + i), eMemory::PAGE_SIZE);
			page += eMemory::PAGE_SIZE;
			++stored_128_pages;
		}
	}
	switch(stored_128_pages)
	{
	case 0:
		return eSnapshot_SNA::S_48;
	case 6:
		return eSnapshot_SNA::S_128_6;
	}
	return eSnapshot_SNA::S_128_5;
}
bool eZ80Accessor::SetState(const eSnapshot_Z80* s, size_t buf_size)
{
	bool model48k = (s->model < 3);
	byte flags = s->flags;
	if(flags == 0xFF)
		flags = 1;
	byte* ptr = (byte*)s + 30;
	word reg_pc = s->pc;
	if(reg_pc == 0)
	{ // 2.01
		ptr += 2 + s->len;
		reg_pc = s->newpc;
		while(ptr < (byte*)s + buf_size)
		{
			byte* p48[] =
			{
				0, 0, 0, 0,
				memory->Get(eMemory::P_RAM2), memory->Get(eMemory::P_RAM0), 0, 0,
				memory->Get(eMemory::P_RAM5), 0, 0, 0
			};
			byte* p128[] =
			{
				0, 0, 0, memory->Get(eMemory::P_RAM0),
				memory->Get(eMemory::P_RAM1), memory->Get(eMemory::P_RAM2), memory->Get(eMemory::P_RAM3), memory->Get(eMemory::P_RAM4),
				memory->Get(eMemory::P_RAM5), memory->Get(eMemory::P_RAM6), memory->Get(eMemory::P_RAM7), 0
			};
			word len = ptr[0] | word(ptr[1]) << 8;
			if(ptr[2] > 11)
				return false;
			byte* dstpage = model48k ? p48[ptr[2]] : p128[ptr[2]];
			if(!dstpage)
				return false;
			ptr += 3;
			if(len == 0xFFFF)
				memcpy(dstpage, ptr, len = eMemory::PAGE_SIZE);
			else
				UnpackPage(dstpage, eMemory::PAGE_SIZE, ptr, len);
			ptr += len;
		}
	}
	else
	{
		int len = buf_size - 30;
		byte* mem48 = ptr;
		if(flags&0x20)
		{
			//data in packed format
			mem48 = new byte[3*eMemory::PAGE_SIZE];
			UnpackPage(mem48, 3*eMemory::PAGE_SIZE, ptr, len);
		}
		memcpy(memory->Get(eMemory::P_RAM5), mem48, eMemory::PAGE_SIZE);
		memcpy(memory->Get(eMemory::P_RAM2), mem48 + eMemory::PAGE_SIZE, eMemory::PAGE_SIZE);
		memcpy(memory->Get(eMemory::P_RAM0), mem48 + 2*eMemory::PAGE_SIZE, eMemory::PAGE_SIZE);
		if(flags&0x20)
			delete[] mem48;
		model48k = true;
	}
	a = s->a, f = s->f;
	bc = s->bc, de = s->de, hl = s->hl;
	alt.bc = s->bc1, alt.de = s->de1, alt.hl = s->hl1;
	alt.a = s->a1, alt.f = s->f1;
	pc = reg_pc, sp = s->sp; ix = s->ix, iy = s->iy;
	i = s->i, r_low = s->r & 0x7F;
	r_hi = ((flags & 1) << 7);
	byte pFE = (flags >> 1) & 7;
	devices->IoWrite(0xfe, pFE, t);
	iff1 = s->iff1, iff2 = s->iff2; im = s->im & 3;
	devices->IoWrite(0x7ffd, model48k ? 0x30 : s->p7FFD, t);
	memory->SetPage(0, (model48k || (s->p7FFD & 0x10)) ? eMemory::P_ROM1 : eMemory::P_ROM0);
	return true;
}
void eZ80Accessor::UnpackPage(byte* dst, int dstlen, byte* src, int srclen)
{
	memset(dst, 0, dstlen);
	while(srclen > 0 && dstlen > 0)
	{
		if(srclen >= 4 && src[0] == 0xED && src[1] == 0xED)
		{
			for(byte i = src[2]; i; i--)
				*dst++ = src[3], dstlen--;
			srclen -= 4;
			src += 4;
		}
		else
		{
			*dst++ = *src++;
			--dstlen;
			--srclen;
		}
	}
}

bool Load(eSpeccy* speccy, const char* type, const void* data, size_t data_size)
{
	speccy->Reset();
	eZ80Accessor* z80 = (eZ80Accessor*)speccy->CPU();
	if(!strcmp(type, "sna"))
		return z80->SetState((const eSnapshot_SNA*)data, data_size);
	else if(!strcmp(type, "z80"))
		return z80->SetState((const eSnapshot_Z80*)data, data_size);
	return false;
}

bool Store(eSpeccy* speccy, const char* file)
{
	FILE* f = fopen(file, "wb");
	if(!f)
		return false;
	eSnapshot_SNA* s = new eSnapshot_SNA;
	eZ80Accessor* z80 = (eZ80Accessor*)speccy->CPU();
	size_t size = z80->StoreState(s);
	bool ok = false;
	if(size)
		ok = fwrite(s, 1, size, f) == size;
	delete[] s;
	fclose(f);
	return ok;
}

}
//namespace xSnapshot
