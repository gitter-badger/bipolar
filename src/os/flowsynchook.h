/*
    Copyright 2014-2015 Paul Colby

    This file is part of Bipolar.

    Bipolar is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Biplar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Bipolar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __FLOWSYNC_HOOK_H__
#define __FLOWSYNC_HOOK_H__

#include <QDir>

class FlowSyncHook {

public:

    static QDir flowSyncDir();

    static QDir installableHookDir();

    static bool install(const QDir &fromDir, QDir toDir);

    static int getVersion(const QDir &dir);

};

#endif // __FLOWSYNC_HOOK_H__
