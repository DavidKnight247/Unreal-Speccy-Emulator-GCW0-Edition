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

#include "options.h"
#include "../platform/platform.h"

#ifdef USE_CONFIG
#include <tinyxml2.h>
#include "../platform/io.h"
#endif//USE_CONFIG

namespace xOptions
{

static char buf[256];
static const char* OptNameToXmlName(const char* name)
{
	strcpy(buf, name);
	for(char* b = buf; *b; ++b)
	{
		if(*b == ' ')
			*b = '_';
	}
	return buf;
}
static const char* XmlNameToOptName(const char* name)
{
	strcpy(buf, name);
	for(char* b = buf; *b; ++b)
	{
		if(*b == '_')
			*b = ' ';
	}
	return buf;
}

bool eOptionB::loading = false;
bool eOptionB::applied = true;

using namespace tinyxml2;

eOptionB::eOptionB() : next(NULL), sub_options(NULL), customizable(true), storeable(true)
	, changed(false), loading_node(NULL)
{
}
void eOptionB::Changing()
{
	changed = true;
	applied = false;
}
bool eOptionB::Apply(XMLElement* owner)
{
	loading_node = owner;
	if(loading && loading_node)
	{
		const char* v = loading_node->GetText();
		Value(v ? v : "");
	}
	sub_options = NULL;
	OnOption();
	loading_node = NULL;
	bool res = changed;
	changed = false;
	return res;
}
bool eOptionB::Option(eOptionB& o)
{
	eOptionB* last = sub_options;
	for(eOptionB* p = last; p; p = p->Next())
	{
		last = p;
	}
	if(last)
		last->next = &o;
	else
		sub_options = &o;
	o.next = NULL;
	XMLElement* xe = NULL;
	if(loading && loading_node)
	{
		const char* id = OptNameToXmlName(o.Name());
		xe = loading_node->FirstChildElement(id);
	}
	return o.Apply(xe);
}
eOptionB* eOptionB::Find(const char* name) const
{
	const char* sub_name = strchr(name, '/');
	int size = sub_name ? sub_name - name : strlen(name);
	for(eOptionB* o = sub_options; o; o = o->Next())
	{
		if(!strncmp(name, o->Name(), size))
			return sub_name ? o->Find(sub_name + 1) : o;
	}
	return NULL;
}
void eOptionB::Store(XMLElement* owner, XMLDocument* doc)
{
	if(!(storeable && Value()) && !sub_options)
		return;
	XMLElement* xe = doc->NewElement(OptNameToXmlName(Name()));
	owner->LinkEndChild(xe);
	if(storeable && Value())
	{
		xe->LinkEndChild(doc->NewText(Value()));
	}
	for(eOptionB* o = sub_options; o; o = o->Next())
	{
		o->Store(xe, doc);
	}
}

struct eOA : public eRootOptionB // access to protected members hack
{
	static void SortByOrder()
	{
		bool swapped;
		do
		{
			swapped = false;
			for(eRootOptionB* a = First(), *pa = NULL; a; pa = a, a = a->Next())
			{
				for(eRootOptionB* b = a->Next(), *pb = a; b; pb = b, b = b->Next())
				{
					if(b->Order() < a->Order())
					{
						Swap((eOA*)pa, (eOA*)a, (eOA*)pb, (eOA*)b);
						swapped = true;
						break;
					}
				}
				if(swapped)
					break;
			}
		} while(swapped);
	}
	static void Swap(eOA* pa, eOA* a, eOA* pb, eOA* b)
	{
		eRootOptionB* n = a->next;
		a->next = b->next;
		if(a != pb)
			b->next = n;
		else
			b->next = a;
		if(pa)
			pa->next = b;
		else
			_First() = b;
		if(a != pb)
			pb->next = a;
	}
};

void eOptionInt::Change(int last, bool next)
{
	if(next)
		Set(value < last - 1 ? value + 1 : 0);
	else
		Set(value > 0 ? value - 1 : last - 1);
}
const char*	eOptionInt::Value() const
{
	const char** vals = Values();
	if(!vals)
		return NULL;
	return vals[value];
}
void eOptionInt::Value(const char* v)
{
	const char** vals = Values();
	if(!vals)
		return;
	int i = -1;
	for(; *vals; ++vals)
	{
		++i;
		if(!strcmp(*vals, v))
		{
			Set(i);
			break;
		}
	}
}

const char*	eOptionBool::Value() const
{
	const char** vals = Values();
	if(!vals)
		return NULL;
	return vals[value ? 1 : 0];
}
void eOptionBool::Value(const char* v)
{
	const char** vals = Values();
	if(!vals)
		return;
	if(!strcmp(v, vals[0]))
		Set(false);
	else if(!strcmp(v, vals[1]))
		Set(true);
}
const char** eOptionBool::Values() const
{
	static const char* values[] = { "off", "on", NULL };
	return values;
}

void eOptionString::Set(const char*& v)
{
	int s = strlen(v) + 1;
	if(!value || alloc_size < s)
	{
		SAFE_DELETE_ARRAY(value);
		value = new char[s];
		alloc_size = s;
	}
	strcpy(const_cast<char*>(value), v);
	storeable = s > 1;
	eInherited::Set(value);
}

static class eRoot : public eOptionB
{
	typedef eOptionB eInherited;
public:
	eRoot() { customizable = false; }
	virtual const char* Name() const { return "options"; }
	void Apply()
	{
		if(applied)
			return;
		eInherited::Apply(NULL);
		applied = true;
	}
	void Load(XMLElement* owner)
	{
		loading = true;
		eInherited::Apply(owner);
		loading = false;
	}
protected:
	virtual void OnOption()
	{
		for(eRootOptionB* r = eRootOptionB::First(); r; r = r->Next())
		{
			Option(r->OptionB());
		}
	}
} root;

eOptionB* Find(const char* name) { return root.Find(name); }
void Apply() { root.Apply(); }

#ifdef USE_CONFIG
static const char* FileName() { return xIo::ProfilePath("unreal_speccy_portable.xml"); }
void Init()
{
	eOA::SortByOrder();
	XMLDocument doc(true, COLLAPSE_WHITESPACE);
	if(doc.LoadFile(FileName()) == XML_SUCCESS)
	{
		XMLElement* xe = doc.RootElement();
		if(xe)
		{
			root.Load(xe->FirstChildElement());
		}
	}
}
void Done()
{
	XMLDocument doc(true, COLLAPSE_WHITESPACE);
	XMLDeclaration* decl = doc.NewDeclaration();
	doc.LinkEndChild(decl);
	XMLElement* xe = doc.NewElement("UnrealSpeccyPortable");
	doc.LinkEndChild(xe);
	root.Store(xe, &doc);
	doc.SaveFile(FileName());
}

#else//USE_CONFIG

void Init()
{
	eOA::SortByOrder();
	Apply();
}
void Done() {}

#endif//USE_CONFIG

}
//namespace xOptions
