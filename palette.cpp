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

#include "std.h"
#include "tools/options.h"
#include <tinyxml2.h>
#include "palette.h"

class ePalette : public xOptions::eOptionString
{
public:
	ePalette(const char* _id)
	{
		customizable = false;
		id = new char[strlen(_id) + 1];
		strcpy(const_cast<char*>(id), _id);

		enum { INTENSITY = 200, BRIGHT = 55 };
		for(int c = 0; c < 16; ++c)
		{
			byte i = c&8 ? INTENSITY + BRIGHT : INTENSITY;
			byte b = c&1 ? i : 0;
			byte r = c&2 ? i : 0;
			byte g = c&4 ? i : 0;
			items[c] = (b << 16)|(g << 8)|r;
		}
	}
	virtual ~ePalette() { SAFE_DELETE_ARRAY(id); }
	const dword* Data() const { return items; }
	virtual const char* Name() const { return id; }
protected:
	byte Component(word hex_str) const;
	virtual void OnOption();
protected:
	const char* id;
	dword items[16];
};

byte ePalette::Component(word hex_str) const
{
	byte c = 0;
	for(int i = 0; i < 2; ++i)
	{
		byte v = ((byte*)&hex_str)[1 - i];
		if(v >= '0' && v <= '9') v -= '0';
		if(v >= 'a' && v <= 'f') v -= 'a' - 10;
		if(v >= 'A' && v <= 'F') v -= 'A' - 10;
		c |= v << i * 4;
	}
	return c;
}
void ePalette::OnOption()
{
	if(loading && strlen(value) == 96)
	{
		const word* src = (const word*)value;
		for(int c = 0; c < 16; ++c)
		{
			byte* dst = (byte*)&items[c];
			for(int i = 0; i < 3; ++i)
			{
				*dst++ = Component(*src++);
			}
		}
	}
}

static class ePalettes : public xOptions::eRootOption<xOptions::eOptionInt>
{
public:
	ePalettes() : size(0)
	{
		memset(items, 0, sizeof(items));
		memset(values, 0, sizeof(values));
		ePalette* item = new ePalette("default");
		items[size] = item;
		values[size] = item->Name();
		++size;
	}
	virtual ~ePalettes()
	{
		for(int i = 0; i < COUNT; ++i)
		{
			SAFE_DELETE(items[i]);
		}
	}
	const dword* Palette() const { return items[value]->Data(); }
	virtual void Change(bool next = true)
	{
		eOptionInt::Change(size, next);
	}
	virtual const char* Name() const { return "palette"; }
	virtual int Order() const { return 60; }
protected:
	virtual const char** Values() const { return (const char**)values; }
	virtual void OnOption()
	{
		if(loading && loading_node)
		{
			for(tinyxml2::XMLElement* xe = loading_node->FirstChildElement(); xe; xe = xe->NextSiblingElement())
			{
				if(size >= COUNT)
					break;
				ePalette* item = new ePalette(xe->Value());
				items[size] = item;
				values[size] = item->Name();
				++size;
			}
			Value(loading_node->GetText());
		}
		for(int i = 0; i < COUNT; ++i)
		{
			Option(items[i]);
		}
	}
protected:
	enum { COUNT = 32 };
	ePalette* items[COUNT + 1];
	int size;
	const char* values[COUNT + 1];
} op_palettes;

const dword* Palette() { return op_palettes.Palette(); }
