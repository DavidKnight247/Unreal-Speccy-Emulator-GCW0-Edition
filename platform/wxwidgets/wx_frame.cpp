/*
Portable ZX-Spectrum emulator.
Copyright (C) 2001-2013 SMT, Dexus, Alone Coder, deathsoft, djdron, scor

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

#include "../platform.h"

#ifdef USE_WXWIDGETS

#include "../../tools/options.h"
#include "../../options_common.h"
#include "wx_cmdline.h"

#include <wx/wx.h>
#include <wx/dnd.h>
#include <wx/aboutdlg.h>

OPTION_USING(eOptionInt, op_drive);

namespace xPlatform
{

void InitSound();
void DoneSound();

wxWindow* CreateGLCanvas(wxWindow* parent);

OPTION_USING(eOptionBool, op_true_speed);

static struct eOptionWindowSize : public xOptions::eOptionInt
{
	eOptionWindowSize() { Set(1); }
	virtual const char* Name() const { return "window size"; }
	virtual const char** Values() const
	{
		static const char* values[] = { "100%", "200%", "300%", NULL };
		return values;
	}
} op_window_size;
DECLARE_OPTION_ACCESSOR(eOptionInt, op_window_size);

static struct eOptionFullScreen : public xOptions::eOptionBool
{
	virtual const char* Name() const { return "full screen"; }
} op_full_screen;
DECLARE_OPTION_ACCESSOR(eOptionBool, op_full_screen);

static struct eOptionAbout : public xOptions::eOptionBool
{
	virtual const char* Name() const { return "about"; }
	virtual const char** Values() const { return NULL; }
} op_about;

static struct eOptionHelp : public xOptions::eRootOption<xOptions::eOptionB>
{
	virtual const char* Name() const { return "help"; }
	virtual int Order() const { return 10; }
protected:
	virtual void OnOption()
	{
		Option(op_about);
	}
} op_help;

extern const wxEventType evtMouseCapture;
extern const wxEventType evtSetStatusText;
extern const wxEventType evtExitFullScreen;

#ifndef _MAC
struct DropFilesTarget : public wxFileDropTarget
{
	virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
	{
		if(filenames.empty())
			return false;
		return Handler()->OnOpenFile(wxConvertWX2MB(filenames[0].c_str()));
	}
};
#endif//_MAC


//=============================================================================
//	Frame
//-----------------------------------------------------------------------------
class Frame : public wxFrame
{
public:
	Frame(const wxString& title, const wxPoint& pos, const eCmdLine& cmdline);
	void ShowFullScreen(bool on);

private:
	void CreateOption(int& id, xOptions::eOptionB* o, wxMenu* m);
	void OnChange(wxCommandEvent& event);
	void OnQuit()	{ Close(true); };
	void OnAbout();
	void OnOpenFile();
	void OnSaveFile();
	void SetFullScreen(bool on);
	void OnExitFullScreen(wxCommandEvent& event);
	void OnFullScreenToggle();
	void OnResize();
	void OnMouseCapture(wxCommandEvent& event);
	void OnSetStatusText(wxCommandEvent& event);

	enum { ID_OnChange };

private:
	DECLARE_EVENT_TABLE()

	wxWindow*	gl_canvas;
	const wxSize org_size;
	enum { MENU_ITEMS_COUNT = 256 };
	typedef void (Frame::*eHandler)();
	struct eMenuItem
	{
		xOptions::eOptionB* option;
		int value;
		eHandler handler;
	};
	eMenuItem menu_items[MENU_ITEMS_COUNT];
};

//=============================================================================
//	EVENT_TABLE
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(Frame, wxFrame)
	EVT_COMMAND(wxID_ANY, evtMouseCapture, Frame::OnMouseCapture)
	EVT_COMMAND(wxID_ANY, evtSetStatusText, Frame::OnSetStatusText)
	EVT_COMMAND(wxID_ANY, evtExitFullScreen, Frame::OnExitFullScreen)
END_EVENT_TABLE()


//=============================================================================
//	Frame::Frame
//-----------------------------------------------------------------------------
Frame::Frame(const wxString& title, const wxPoint& pos, const eCmdLine& cmdline)
	: wxFrame((wxFrame*)NULL, -1, title, pos)
	, org_size(320, 240)
{
	memset(menu_items, 0, sizeof(menu_items));
#ifdef _WINDOWS
	SetIcon(wxICON(unreal_speccy_portable));
#endif//_WINDOWS
#ifdef _LINUX
	SetIcon(wxIcon(wxT("unreal_speccy_portable.xpm")));
#endif//_LINUX
	wxMenuBar* mb = new wxMenuBar;
	using namespace xOptions;
	int id = 0;
	for(eRootOptionB* o = eRootOptionB::First(); o; o = o->Next())
	{
		wxMenu* m = new wxMenu;
		for(xOptions::eOptionB* so = o->OptionB()->SubOptions(); so; so = so->Next())
		{
			CreateOption(id, so, m);
		}
		mb->Append(m, _(o->OptionB()->Name()).Capitalize());
	}
	Connect(ID_OnChange, ID_OnChange + id, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Frame::OnChange));

	struct eItem
	{
		xOptions::eOptionB* option;
		eHandler handler;
	};
	eItem items[] =
	{
		{ OPTION_GET(op_open_file)	, &Frame::OnOpenFile },
		{ &op_window_size			, &Frame::OnResize },
		{ &op_full_screen			, &Frame::OnFullScreenToggle },
		{ &op_about					, &Frame::OnAbout }
	};
	int count = sizeof(items) / sizeof(eItem);
	for(int i = 0; i < id; ++i)
	{
		eMenuItem& mi = menu_items[i];
		for(int j = 0; j < count; ++j)
		{
			if(mi.option == items[j].option)
			{
				mi.handler = items[j].handler;
			}
		}
	}

#ifdef _MAC
//	menuFile->Append(wxID_ABOUT, _("About ") + title);
#else//_MAC
	SetDropTarget(new DropFilesTarget);
#endif//_MAC

	SetMenuBar(mb);

	CreateStatusBar();
	SetStatusText(_("Ready..."));

	SetClientSize(org_size);
	SetMinSize(GetSize());

	if(cmdline.size_percent >= 0)
	{
		op_full_screen.Set(false);
		SetClientSize(org_size*cmdline.size_percent/100);
	}
	else
	{
		SetClientSize(org_size*(op_window_size + 1));
	}

	gl_canvas = CreateGLCanvas(this);
	gl_canvas->SetFocus();

	if(!cmdline.file_to_open.empty())
		Handler()->OnOpenFile(wxConvertWX2MB(cmdline.file_to_open));
}
void Frame::CreateOption(int& id, xOptions::eOptionB* o, wxMenu* m)
{
	using namespace xOptions;
	const char* value = NULL;
	if(o->Customizable())
	{
		value = o->Value();
	}
	else if(!o->SubOptions())
		return;
	wxString name = _(o->Name()).Capitalize();
	wxMenu* sm = NULL;
	eOptionBool* ob = dynamic_cast<eOptionBool*>(o);
	if(!o->SubOptions() && (!o->Values() || ob))
	{
		m->Append(ID_OnChange + id, name, "", o->Values() ? wxITEM_CHECK : wxITEM_NORMAL);
		if(o->Values())
			m->Check(ID_OnChange + id, *ob);
		menu_items[id++].option = o;
	}
	else
	{
		sm = new wxMenu;
		if(o->Values())
		{
			int begin_id = id;
			for(const char** v = o->Values(); v && *v; ++v)
			{
				sm->Append(ID_OnChange + id, _(*v).Capitalize(), "", wxITEM_RADIO);
				menu_items[id].option = o;
				menu_items[id++].value = id - begin_id;
			}
			sm->Check(ID_OnChange + begin_id + *static_cast<eOptionInt*>(o), true);
		}
		if(o->Values() && o->SubOptions())
			sm->Append(ID_OnChange + id++, "", "", wxITEM_SEPARATOR);
		m->Append(ID_OnChange + id++, name, sm);
	}
	for(xOptions::eOptionB* so = o->SubOptions(); so; so = so->Next())
	{
		CreateOption(id, so, sm);
	}
}
void Frame::OnChange(wxCommandEvent& event)
{
	int id = event.GetId();
	xOptions::eOptionB* o = menu_items[id].option;
	xOptions::eOptionInt* oi = dynamic_cast<xOptions::eOptionInt*>(o);
	if(oi)
		oi->Set(menu_items[id].value);
	else
		o->Change();
	xOptions::Apply();
	if(menu_items[id].handler)
		(this->*menu_items[id].handler)();
}
//=============================================================================
//	Frame::ShowFullScreen
//-----------------------------------------------------------------------------
void Frame::ShowFullScreen(bool on)
{
#ifdef _MAC
	if(on)
	{
		GetStatusBar()->Hide();
	}
	else
	{
		GetStatusBar()->Show();
	}
#endif//_MAC
	wxFrame::ShowFullScreen(on, wxFULLSCREEN_ALL);
}
//=============================================================================
//	Frame::OnAbout
//-----------------------------------------------------------------------------
void Frame::OnAbout()
{
	op_about.Set(false);
	wxAboutDialogInfo info;
	info.SetName(GetTitle());
	info.SetDescription(_("Portable ZX Spectrum emulator."));
	info.SetCopyright(_("Copyright (C) 2001-2013 SMT, Dexus, Alone Coder, deathsoft, djdron, scor."));
#ifndef _MAC
	info.SetVersion(_("0.0.56"));
	info.SetWebSite(_("http://code.google.com/p/unrealspeccyp/"));
	info.SetLicense(_(
"This program is free software: you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation, either version 3 of the License, or\n\
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
			));
#endif//_MAC
	wxAboutBox(info);
}
//=============================================================================
//	Frame::OnOpenFile
//-----------------------------------------------------------------------------
void Frame::OnOpenFile()
{
	OPTION_GET(op_open_file)->Set(false);
	wxFileDialog fd(this, wxFileSelectorPromptStr, wxConvertMB2WX(OpLastFolder()));
	fd.SetWildcard(
			L"Supported files|*.sna;*.z80;*.szx;*.rzx;*.trd;*.scl;*.fdi;*.tap;*.csw;*.tzx;*.zip;"
							L"*.SNA;*.Z80;*.SZX;*.RZX;*.TRD;*.SCL;*.FDI;*.TAP;*.CSW;*.TZX;*.ZIP|"
			L"All files|*.*|"
			L"Snapshot files (*.sna;*.z80;*.szx)|*.sna;*.z80;*.szx;*.SNA;*.Z80;*.SZX|"
			L"Replay files (*.rzx)|*.rzx;*.RZX|"
			L"Disk images (*.trd;*.scl;*.fdi)|*.trd;*.scl;*.fdi;*.TRD;*.SCL;*.FDI|"
			L"Tape files (*.tap;*.csw;*.tzx)|*.tap;*.csw;*.tzx;*.TAP;*.CSW;*.TZX|"
			L"ZIP archives (*.zip)|*.zip;*.ZIP"
		);
	if(fd.ShowModal() == wxID_OK)
	{
		if(Handler()->OnOpenFile(wxConvertWX2MB(fd.GetPath().c_str())))
		{
			SetStatusText(_("File open OK"));
//			menu_quick_save->Enable(true);
		}
		else
			SetStatusText(_("File open FAILED"));
	}
}
//=============================================================================
//	Frame::OnSaveFile
//-----------------------------------------------------------------------------
void Frame::OnSaveFile()
{
	Handler()->VideoPaused(true);
	wxFileDialog fd(this, wxFileSelectorPromptStr, wxConvertMB2WX(OpLastFolder()), wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
	fd.SetWildcard(
			L"Snapshot files (*.sna)|*.sna;*.SNA|"
			L"Screenshot files (*.png)|*.png;*.PNG"
		);
	if(fd.ShowModal() == wxID_OK)
	{
		wxString path = fd.GetPath();
		int fi = fd.GetFilterIndex();
		size_t p = path.length() - 4;
		if(path.length() < 4 || (
				path.rfind(L".sna") != p && path.rfind(L".SNA") != p &&
				path.rfind(L".png") != p && path.rfind(L".PNG") != p))
			path += fi ? L".png" : L".sna";
		if(Handler()->OnSaveFile(wxConvertWX2MB(path.c_str())))
			SetStatusText(_("File save OK"));
		else
			SetStatusText(_("File save FAILED"));
	}
	Handler()->VideoPaused(false);
}
//=============================================================================
//	Frame::SetFullScreen
//-----------------------------------------------------------------------------
void Frame::SetFullScreen(bool on)
{
	op_full_screen.Set(on);
	if(IsFullScreen() != op_full_screen)
	{
		ShowFullScreen(op_full_screen);
	}
}
//=============================================================================
//	Frame::OnExitFullScreen
//-----------------------------------------------------------------------------
void Frame::OnExitFullScreen(wxCommandEvent& event)
{
	SetFullScreen(false);
}
//=============================================================================
//	Frame::OnToggleFullScreen
//-----------------------------------------------------------------------------
void Frame::OnFullScreenToggle()
{
	SetFullScreen(op_full_screen);
}
//=============================================================================
//	Frame::OnResize
//-----------------------------------------------------------------------------
void Frame::OnResize()
{
	if(IsFullScreen())
	{
		ShowFullScreen(false);
		op_full_screen.Set(false);
	}
	if(IsMaximized())
		Maximize(false);
	SetClientSize(org_size*(op_window_size + 1));
}
//=============================================================================
//	Frame::OnMouseCapture
//-----------------------------------------------------------------------------
void Frame::OnMouseCapture(wxCommandEvent& event)
{
	SetStatusText(event.GetId() ? _("Mouse captured, press ESC to cancel") : _("Mouse released"));
}
//=============================================================================
//	Frame::OnSetStatusText
//-----------------------------------------------------------------------------
void Frame::OnSetStatusText(wxCommandEvent& event)
{
	if(event.GetString() == L"rzx_finished")
		SetStatusText(_("RZX playback finished"));
	else if(event.GetString() == L"rzx_sync_lost")
		SetStatusText(_("RZX error - sync lost"));
	else if(event.GetString() == L"rzx_invalid")
		SetStatusText(_("RZX error - invalid data"));
	else if(event.GetString() == L"rzx_unsupported")
		SetStatusText(_("RZX error - unsupported format"));
}

//=============================================================================
//	CreateFrame
//-----------------------------------------------------------------------------
wxWindow* CreateFrame(const wxString& title, const wxPoint& pos, const eCmdLine& cmdline)
{
	Frame* frame = new Frame(title, pos, cmdline);
	frame->Show(true);
	if(op_full_screen)
		frame->ShowFullScreen(true);
	return frame;
}

}
//namespace xPlatform

#endif//USE_WXWIDGETS
