/*      $Id$

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2, or (at your option)
        any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., Inc., 51 Franklin Street, Fifth Floor, Boston,
        MA 02110-1301, USA.

        xfwm4-zg - (c) 2015 Steve Dodier-Lazaro
 */

#ifndef UNITY_ZEITGEIST_MANAGER_H
#define UNITY_ZEITGEIST_MANAGER_H

#include <glib.h>
#include <sys/types.h>
#include <unistd.h>
#include <zeitgeist.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include "logger.h"
#include "sync_source.h"

#define INTERVAL_DURATION 120
#define MIN_FOCUS_DURATION 5

#ifndef GULONG_TO_POINTER
#define GULONG_TO_POINTER(u) ((gpointer) (u))
#endif

#ifndef GPOINTER_TO_ULONG
#define GPOINTER_TO_ULONG(p) ((gulong) (p))
#endif

typedef struct ZeitgeistWindow
{
    Window xid;
    pid_t pid;
    char *title;
    guint workspace;
} ZeitgeistWindow;

typedef struct ZeitgeistManager
{
    GHashTable *openWins;
    GHashTable *openWinDurations;
    GHashTable *activeWins;
    GHashTable *activeWinDurations;

    Window currentWin;
    double currentWinTime;

    SyncSource *source;
} ZeitgeistManager;


ZeitgeistSubject *manager_get_ucl_subject (const Window xid, const pid_t pid);
char *manager_get_actor_name_from_pid (const pid_t pid);
pid_t manager_get_pid_from_xid (Window xid);
ZeitgeistManager *manager_get ();
void manager_clear();

void manager_set_timer(ZeitgeistManager *mgr);
void manager_unset_timer(ZeitgeistManager *mgr);
ZeitgeistWindow *zeitgeist_window_new (const Window xid, const pid_t pid, const char *title, const int ws);
void zeitgeist_window_free (ZeitgeistWindow *w);
void manager_add_active_window (ZeitgeistManager *mgr, Window xid);
void manager_add_active_duration (ZeitgeistManager *mgr, Window xid, const double duration);
void manager_reset_durations (ZeitgeistManager *mgr);

#endif //UNITY_ZEITGEIST_MANAGER_H
