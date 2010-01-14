#include "std.h"

#include "device.h"

eDevices devices;

//=============================================================================
//	eDevices::eDevices
//-----------------------------------------------------------------------------
eDevices::eDevices() : last(-1)
{
	items[0].dev = NULL;
	memcpy(items + 1, items, AMOUNT - 1);
}
//=============================================================================
//	eDevices::~eDevices
//-----------------------------------------------------------------------------
eDevices::~eDevices()
{
	for(int i = 0; i < sizeof(items); ++i)
	{
		delete items[i].dev;
	}
}
//=============================================================================
//	eDevices::Init
//-----------------------------------------------------------------------------
void eDevices::Init()
{
	for(int i = 0; i <= last; ++i)
	{
		items[i].dev->Init();
	}
}
//=============================================================================
//	eDevices::Reset
//-----------------------------------------------------------------------------
void eDevices::Reset()
{
	for(int i = 0; i <= last; ++i)
	{
		items[i].dev->Reset();
	}
}
//=============================================================================
//	eDevices::Register
//-----------------------------------------------------------------------------
void eDevices::Add(eDevice* d, int id)
{
	++last;
	assert(last < AMOUNT);
	items[last].id	= id;
	items[last].dev	= d;
}
//=============================================================================
//	eDevices::Item
//-----------------------------------------------------------------------------
eDevice* eDevices::Item(int id)
{
	for(int i = 0; i <= last; ++i)
	{
		if(items[i].id == id)
			return items[i].dev;
	}
	return NULL;
}
//=============================================================================
//	eDevices::IoRead
//-----------------------------------------------------------------------------
byte eDevices::IoRead(dword port) const
{
	byte v = 0xff;
	for(int i = 0; i <= last; ++i)
	{
		items[i].dev->IoRead(port, &v);
	}
	return v;
}
//=============================================================================
//	eDevices::IoWrite
//-----------------------------------------------------------------------------
void eDevices::IoWrite(dword port, byte v)
{
	for(int i = 0; i <= last; ++i)
	{
		items[i].dev->IoWrite(port, v);
	}
}
