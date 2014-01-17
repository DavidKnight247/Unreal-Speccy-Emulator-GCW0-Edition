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

#ifndef __OPTION_H__
#define __OPTION_H__

#include "../std.h"
#include "list.h"

#pragma once

#define DECLARE_OPTION_ACCESSOR_EX(cls, p, id) xOptions::cls* _opt_##id = p
#define DECLARE_OPTION_ACCESSOR_NULL(cls, id) DECLARE_OPTION_ACCESSOR_EX(cls, NULL, id)
#define DECLARE_OPTION_ACCESSOR(cls, obj) DECLARE_OPTION_ACCESSOR_EX(cls, &obj, obj)

#define OPTION_USING(cls, id) extern xOptions::cls* _opt_##id
#define OPTION_GET(id) _opt_##id

namespace tinyxml2
{
class XMLDocument;
class XMLElement;
}

namespace xOptions
{

class eOptionB;

class eRootOptionB : public eList<eRootOptionB>
{
public:
	virtual int Order() const { return 0; }
	virtual eOptionB* OptionB() { return NULL; }
protected:
	eRootOptionB() {}
};

template<class T> struct eRootOption : public eRootOptionB, public T
{
	virtual eOptionB* OptionB() { return this; }
};

class eOptionB
{
public:
	eOptionB();
	virtual ~eOptionB() {}

	eOptionB* Next() const { return next; }
	eOptionB* SubOptions() const { return sub_options; }

	bool Customizable() const { return customizable; }

	virtual const char* Name() const = 0;
	virtual const char*	Value() const { return NULL; }
	virtual void Value(const char* v) { Changing(); }
	virtual void Change(bool next = true) { Changing(); }
	virtual bool Apply(tinyxml2::XMLElement* owner = NULL);
	eOptionB* Find(const char* name) const;
	void Store(tinyxml2::XMLElement* owner, tinyxml2::XMLDocument* doc);
protected:
	virtual const char** Values() const { return NULL; }
	void Changing();
	bool Option(eOptionB& o);
	bool Option(eOptionB* o) { return o ? Option(*o) : false; }
	virtual void OnOption() {}
protected:
	eOptionB* next;
	eOptionB* sub_options;
	bool customizable;
	bool storeable;
	bool changed;
	tinyxml2::XMLElement* loading_node;
	static bool loading;
	static bool applied;
};

template<class T> class eOption : public eOptionB
{
public:
	operator const T&() const { return value; }
	virtual void Set(const T& v) { value = v; Changing(); }
protected:
	T value;
};

class eOptionInt : public eOption<int>
{
public:
	eOptionInt() { Set(0); }
	virtual const char*	Value() const;
	virtual void Value(const char* v);
	void Change(int last, bool next = true);
};

class eOptionBool : public eOption<bool>
{
public:
	eOptionBool() { Set(false); }
	virtual const char*	Value() const;
	virtual void Value(const char* v);
	virtual void Change(bool next = true) { Set(!value); }
protected:
	virtual const char** Values() const;
};

struct eOptionString : public eOption<const char*>
{
	typedef eOption<const char*> eInherited;
	eOptionString() : alloc_size(32) { storeable = false; value = new char[alloc_size]; Value(""); }
	virtual ~eOptionString() { SAFE_DELETE_ARRAY(value); }
	virtual const char*	Value() const { return value; }
	virtual void Value(const char* v) { Set(v); }
	virtual void Set(const char*& v);
	int alloc_size;
};

void Init();
void Done();
void Apply();

eOptionB* Find(const char* name);
template<class T> T* Find(const char* name) { return static_cast<T*>(Find(name)); }

}
//namespace xOptions

#endif//__OPTION_H__
