/**********************************************************************

	--- Qt Architect generated file ---

	File: fileOptions.h

    This file is part of Xgfe: X Windows GUI front end to Gnuplot
    Copyright (C) 1998 David Ishee

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.       

 *********************************************************************/
using namespace std;

#ifndef fileOptions_included
#define fileOptions_included

#include "fileOptionsData.h"
#include "gnuInterface.h"

class fileOptions : public fileOptionsData
{
    Q_OBJECT

public:

    fileOptions
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~fileOptions();

  void setGnuInterface(gnuInterface* gnu);
  void setFormat();

private:
  gnuInterface* gnuInt;
};
#endif // fileOptions_included
