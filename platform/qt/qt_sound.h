/*
Portable ZX-Spectrum emulator.
Copyright (C) 2001-2011 SMT, Dexus, Alone Coder, deathsoft, djdron, scor

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

#ifndef QT_SOUND_H
#define QT_SOUND_H

#include "../../std_types.h"

#pragma once

namespace xPlatform
{

class eAudioBuffer
{
public:
	eAudioBuffer() : ready(0) {}
	void	Update(int active_sound_src);
	dword	Ready() const { return ready; }
	const void*	Ptr() const { return buffer; }
	void	Use(dword size);

protected:
	void	Fill(const void* data, dword size);
	enum { BUF_SIZE = 65536 };
	byte	buffer[BUF_SIZE];
	dword	ready;
};

}
//namespace xPlatform

#endif // QT_SOUND_H
