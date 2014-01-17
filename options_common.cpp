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

#include "platform/platform.h"
#include "platform/io.h"
#include "ui/ui.h"
#include "options_common.h"

#ifdef USE_UI
OPTION_USING(eOptionBool, op_open_file);
#endif//USE_UI

OPTION_USING(eOptionInt, op_palettes);

OPTION_USING(eOptionInt, op_drive);
OPTION_USING(eOptionBool, op_devices);
OPTION_USING(eOptionInt, op_sound_chip);
OPTION_USING(eOptionInt, op_ay_stereo);

namespace xPlatform
{

static struct eOptionLastFile : public xOptions::eOptionString
{
	eOptionLastFile() { customizable = false; }
	const char* Folder()
	{
		static char lf[xIo::MAX_PATH_LEN];
		strcpy(lf, value);
		char* n = lf;
		char* n_end = n + strlen(n);
		while(n_end > n && *n_end != '\\' && *n_end != '/')
			--n_end;
		if(*n_end == '\\' || *n_end == '/')
		{
			++n_end;
			*n_end = '\0';
		}
		return lf;
	}
	virtual const char* Name() const { return "last file"; }
} op_last_file;
DECLARE_OPTION_ACCESSOR(eOptionString, op_last_file);

const char* OpLastFolder() { return op_last_file.Folder(); }

struct eOptionState : public xOptions::eOptionBool
{
	eOptionState() { storeable = false; }
	virtual const char*	Value() const { return NULL; }
	const char* SnapshotName() const
	{
		static char name[xIo::MAX_PATH_LEN];
		strcpy(name, op_last_file);
		int l = strlen(name);
		if(!l || name[l - 1] == '/' || name[l - 1] == '\\')
			return NULL;
		char* e = name + l;
		while(e > name && *e != '.' && *e != '\\' && *e != '/')
			--e;
		if(*e != '.')
			return NULL;
		*e = '\0';
		strcat(name, ".sna");
		return name;
	}
};

static struct eOptionSaveState : public eOptionState
{
	virtual const char* Name() const { return "save state"; }
	virtual void Change(bool next = true)
	{
		const char* name = SnapshotName();
		if(name)
			Set(Handler()->OnSaveFile(name));
		else
			Set(false);
	}
} op_save_state;
DECLARE_OPTION_ACCESSOR(eOptionBool, op_save_state);

static struct eOptionLoadState : public eOptionState
{
	virtual const char* Name() const { return "load state"; }
	virtual void Change(bool next = true)
	{
		const char* name = SnapshotName();
		if(name)
			Set(Handler()->OnOpenFile(name));
		else
			Set(false);
	}
} op_load_state;
DECLARE_OPTION_ACCESSOR(eOptionBool, op_load_state);

static struct eOptionAutoPlayImage : public xOptions::eOptionBool
{
	eOptionAutoPlayImage() { Set(true); }
	virtual const char* Name() const { return "auto play image"; }
} op_auto_play_image;
DECLARE_OPTION_ACCESSOR(eOptionBool, op_auto_play_image);

static struct eOptionQuit : public xOptions::eOptionBool
{
	eOptionQuit() { storeable = false; }
	virtual const char* Name() const { return "quit"; }
	virtual const char** Values() const { return NULL; }
} op_quit;
DECLARE_OPTION_ACCESSOR(eOptionBool, op_quit);

static struct eOptionFile : public xOptions::eRootOption<xOptions::eOptionB>
{
	virtual const char* Name() const { return "file"; }
	virtual int Order() const { return 0; }
protected:
	virtual void OnOption()
	{
#ifdef USE_UI
		Option(OPTION_GET(op_open_file));
#endif//USE_UI
		Option(op_load_state);
		Option(op_save_state);
		Option(OPTION_GET(op_auto_play_image));
		Option(op_quit);
	}
} op_file;


#ifdef USE_WXWIDGETS
OPTION_USING(eOptionInt, op_window_size);
OPTION_USING(eOptionBool, op_full_screen);
#endif//USE_WXWIDGETS

static struct eOptionView : public xOptions::eRootOption<xOptions::eOptionB>
{
	virtual const char* Name() const { return "view"; }
	virtual int Order() const { return 1; }
protected:
	virtual void OnOption()
	{
		Option(OPTION_GET(op_palettes));
		Option(OPTION_GET(op_zoom));
		Option(OPTION_GET(op_filtering));
#ifdef USE_WXWIDGETS
		Option(OPTION_GET(op_window_size));
		Option(OPTION_GET(op_full_screen));
#endif//USE_WXWIDGETS
	}
} op_view;


static struct eOptionReset : public xOptions::eOptionB
{
	eOptionReset() { storeable = false; }
	virtual const char* Name() const { return "reset"; }
	virtual void Change(bool next = true) { Handler()->OnAction(A_RESET); }
} op_reset;

static struct eOptionPause : public xOptions::eOptionBool
{
	eOptionPause() { storeable = false; }
	virtual const char* Name() const { return "pause"; }
	virtual void Change(bool next = true)
	{
		eOptionBool::Change();
		Handler()->VideoPaused(self);
	}
} op_pause;

static struct eOptionTapeFast : public xOptions::eOptionBool
{
	eOptionTapeFast() { Set(true); }
	virtual const char* Name() const { return "fast"; }
} op_tape_fast;
DECLARE_OPTION_ACCESSOR(eOptionBool, op_tape_fast);

static struct eOptionTape : public xOptions::eOptionInt
{
	eOptionTape() { storeable = false; }
	virtual const char* Name() const { return "tape"; }
protected:
	virtual const char** Values() const
	{
		static const char* values[] = { "n/a", "stop", "start", NULL };
		return values;
	}
	virtual void OnOption()
	{
		if(changed)
		{
			switch(Handler()->OnAction(A_TAPE_TOGGLE))
			{
			case AR_TAPE_NOT_INSERTED:	Set(0);	break;
			case AR_TAPE_STOPPED:		Set(1);	break;
			case AR_TAPE_STARTED:		Set(2);	break;
			default: break;
			}
		}
		Option(OPTION_GET(op_tape_fast));
	}
} op_tape;

static struct eOptionJoy : public xOptions::eOptionInt
{
	eOptionJoy() { Set(J_KEMPSTON); }
	dword KeyFlags()
	{
		switch(value)
		{
		case J_KEMPSTON:	return KF_KEMPSTON;
		case J_CURSOR:		return KF_CURSOR;
		case J_QAOP:		return KF_QAOP;
		case J_SINCLAIR2:	return KF_SINCLAIR2;
		}
		return KF_QAOP;
	}
	virtual const char* Name() const { return "joystick"; }
	virtual const char** Values() const
	{
		static const char* values[] = { "kempston", "cursor", "qaop", "sinclair2", NULL };
		return values;
	}
	virtual void Change(bool next = true)
	{
		eOptionInt::Change(J_LAST, next);
	}
} op_joy;
DECLARE_OPTION_ACCESSOR(eOptionInt, op_joy);

dword OpJoyKeyFlags() { return op_joy.KeyFlags(); }

#ifdef USE_OAL
OPTION_USING(eOptionBool, op_true_speed);
#endif//USE_OAL
static struct eOptionDevice : public xOptions::eRootOption<xOptions::eOptionB>
{
	virtual const char* Name() const { return "device"; }
	virtual int Order() const { return 2; }
protected:
	virtual void OnOption()
	{
		Option(op_reset);
		Option(op_pause);
		Option(op_tape);
		Option(op_joy);
#ifdef USE_OAL
		Option(OPTION_GET(op_true_speed));
#endif//USE_OAL
		Option(OPTION_GET(op_reset_to_service_rom));
		Option(OPTION_GET(op_drive));
		Option(OPTION_GET(op_sound_chip));
		Option(OPTION_GET(op_ay_stereo));
		Option(OPTION_GET(op_devices));
	}
} op_device;


static struct eOptionSoundSource : public xOptions::eOptionInt
{
	eOptionSoundSource() { Set(S_AY); }
	virtual const char* Name() const { return "source"; }
	virtual const char** Values() const
	{
		static const char* values[] = { "beeper", "ay", "tape", NULL };
		return values;
	}
	virtual void Change(bool next = true)
	{
		eOptionInt::Change(S_LAST, next);
	}
} op_sound_source;
DECLARE_OPTION_ACCESSOR(eOptionInt, op_sound_source);

static struct eOptionVolume : public xOptions::eOptionInt
{
	eOptionVolume() { Set(V_50); }
	virtual const char* Name() const { return "volume"; }
	virtual const char** Values() const
	{
		static const char* values[] = { "mute", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", NULL };
		return values;
	}
	virtual void Change(bool next = true)
	{
		eOptionInt::Change(V_LAST, next);
	}
} op_volume;
DECLARE_OPTION_ACCESSOR(eOptionInt, op_volume);

static struct eOptionSound : public xOptions::eRootOption<xOptions::eOptionB>
{
	virtual const char* Name() const { return "sound"; }
	virtual int Order() const { return 3; }
protected:
	virtual void OnOption()
	{
		Option(op_sound_source);
		Option(op_volume);
	}
} op_sound;

}
//namespace xPlatform
