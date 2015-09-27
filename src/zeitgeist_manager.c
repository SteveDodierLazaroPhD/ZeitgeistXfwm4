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

#include "zeitgeist_manager.h"

#include <glib.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>

ZeitgeistSubject *manager_get_ucl_subject (const Window xid, const pid_t pid)
{
    ZeitgeistSubject *subject = zeitgeist_subject_new ();

    gchar *pid_str = pid >= 0? g_strdup_printf ("%d", pid) : g_strdup ("n/a");
    gchar *xid_str = xid ? g_strdup_printf ("%lu", xid) : g_strdup ("n/a");

    gchar *study_uri = g_strdup_printf ("activity://null///pid://%s///winid://%s///", pid_str, xid_str);
    free (pid_str);
    free (xid_str);

    zeitgeist_subject_set_uri (subject, study_uri);
    free (study_uri);

    zeitgeist_subject_set_interpretation (subject, ZEITGEIST_NFO_SOFTWARE);
    zeitgeist_subject_set_manifestation (subject, ZEITGEIST_ZG_WORLD_ACTIVITY);
    zeitgeist_subject_set_mimetype (subject, "application/octet-stream");
    zeitgeist_subject_set_text (subject, "ucl-study-metadata");

    return subject;
}

char *manager_get_actor_name_from_pid (const pid_t pid)
{
    if (pid <= 0)
        return NULL;

    char *link_file = g_strdup_printf ("/proc/%d/exe", pid);
    if (!link_file)
        return NULL;

    char buff[PATH_MAX+1];
    ssize_t len = readlink(link_file, buff, sizeof(buff)-1);
    g_free (link_file);
    if (len < 0)
        return NULL;

    buff[len] = '\0';
    gchar *split = strrchr (buff, '/');

    if (!split)
        return NULL;

    gchar *actor_name = g_strdup_printf ("application://%s.desktop", split+1);
    return actor_name;
}

/* Inspired by xdotool.
Copyright (c) 2007, 2008, 2009: Jordan Sissel.
Copyright (c) 2015: Steve Dodier-Lazaro <sidnioulz@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Jordan Sissel nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY JORDAN SISSEL ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL JORDAN SISSEL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
pid_t manager_get_pid_from_xid (Window xid)
{
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy || !xid)
        return 0;

    static Atom pid_atom = -1;
    if (pid_atom == (Atom)-1)
        pid_atom = XInternAtom(dpy, "_NET_WM_PID", False);

    Atom actual_type;
    int actual_format;
    unsigned long _nitems;
    unsigned long _bytes_after;
    unsigned char *prop;
    int status;

    status = XGetWindowProperty(dpy, xid, pid_atom, 0, (~0L),
        False, AnyPropertyType, &actual_type, &actual_format,
        &_nitems, &_bytes_after, &prop);

    if (status != Success || !prop)
        return 0;
    else
        return prop[1]*256 + prop[0];
}

static ZeitgeistManager *_manager_get (int reset)
{
    static ZeitgeistManager *__manager = NULL;

    if (!__manager && !reset)
    {
        __manager = malloc (sizeof (ZeitgeistManager));

        __manager->openWins = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)zeitgeist_window_free);
        __manager->openWinDurations = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, free);
        __manager->activeWins = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)zeitgeist_window_free);
        __manager->activeWinDurations = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, free);

        __manager->currentWin = 0;
        __manager->currentWinTime = (double) time (NULL);
        __manager->source = NULL;

        manager_set_timer (__manager);
    }
    else if (reset)
    {
        manager_unset_timer (__manager);

        g_hash_table_destroy (__manager->openWins);
        g_hash_table_destroy (__manager->openWinDurations);
        g_hash_table_destroy (__manager->activeWins);
        g_hash_table_destroy (__manager->activeWinDurations);

        free (__manager);
        __manager = NULL;
    }

    return __manager;
}

ZeitgeistManager *manager_get ()
{
    return _manager_get (0);
}

void manager_clear()
{
    _manager_get (1);
}

static gboolean _logTimeWindowCallback (gpointer user_data)
{
    ZeitgeistManager *manager = manager_get ();
    TRACE ("Zeitgeist Time Window Callback called.\n");
    if (manager)
    {
        ZeitgeistEvent *event = zeitgeist_event_new ();
        zeitgeist_event_set_interpretation (event, UCL_STUDY_ACTIVE_WINDOWS_EVENT);
        zeitgeist_event_set_manifestation (event, ZEITGEIST_ZG_HEURISTIC_ACTIVITY);
        zeitgeist_event_set_actor (event, "application://xfwm4.desktop");

        if (manager->currentWin)
        {
            time_t now = time (NULL);
            manager_add_active_duration (manager, manager->currentWin, (double)(now - manager->currentWinTime));
            manager->currentWinTime = (double) now;
        }

        GList *iter = NULL;
        for (iter = g_hash_table_get_keys (manager->activeWins); iter; iter = iter->next)
        {
            ZeitgeistWindow *zwin = g_hash_table_lookup (manager->activeWins, g_list_nth_data (iter, 0));
            if(!zwin)
                continue;

            double activeDuration = g_hash_table_contains (manager->activeWinDurations, GULONG_TO_POINTER (zwin->xid)) ?
                     *((double *) g_hash_table_lookup (manager->activeWinDurations, GULONG_TO_POINTER (zwin->xid)))
                   : -1;

            if (activeDuration >= MIN_FOCUS_DURATION)
            {
                //for all active Wins, add subject
                ZeitgeistSubject *subject = zeitgeist_subject_new ();

                gchar *xidStr = zwin->xid? g_strdup_printf ("%lu", zwin->xid) : g_strdup ("n/a");
                gchar *activeUri = g_strdup_printf("window://%s///active://%f", xidStr, activeDuration);
                free (xidStr);

                zeitgeist_subject_set_uri (subject, activeUri);
                free (activeUri);

                zeitgeist_subject_set_interpretation (subject, ZEITGEIST_NFO_SOFTWARE);
                zeitgeist_subject_set_manifestation (subject, ZEITGEIST_NFO_SOFTWARE_ITEM);
                zeitgeist_subject_set_mimetype (subject, "application/octet-stream");

                char *displayUri = g_strdup_printf ("UCL Study active window ('%s') was active for %f seconds", zwin->title, activeDuration);
                zeitgeist_subject_set_text (subject, displayUri);
                free (displayUri);

                zeitgeist_event_add_subject (event, subject);
            }
        }

        g_hash_table_remove_all (manager->activeWins);
        g_hash_table_remove_all (manager->activeWinDurations);

        if (manager->currentWin)
        {
            manager_add_active_window (manager, manager->currentWin);
        }

        GError *error = NULL;
        ZeitgeistLog *log = zeitgeist_log_get_default ();
        zeitgeist_log_insert_events_no_reply (log, event, NULL, &error);

        if (error)
        {
            g_warning ("UCL: could not log an active windows time window because of an error: %s", error->message);
            g_error_free (error);
            return G_SOURCE_CONTINUE;
        }

        return G_SOURCE_CONTINUE;
    }
    else
    {
        g_warning ("UCL: callback ran, but no manager found, aborting Zeitgeist time window logging");
        return G_SOURCE_REMOVE;
    }
}

void manager_set_timer (ZeitgeistManager *manager)
{
    if (!manager)
        return;

    fprintf (stderr, "Initializing manager timer...\n");
    manager->source = sync_source_new (INTERVAL_DURATION * 1000000);
    if (manager->source)
        sync_source_run (manager->source, _logTimeWindowCallback);
}

void manager_unset_timer(ZeitgeistManager *manager)
{
    if (!manager)
        return;

    if (manager->source)
    {
        g_source_destroy ((GSource *) manager->source);
        manager->source = NULL;
    }
}

ZeitgeistWindow *zeitgeist_window_new (const Window xid, const pid_t pid, const char *title, const int ws)
{
    if (!xid || !title)
        return NULL;

    ZeitgeistWindow *w = malloc (sizeof (ZeitgeistWindow));
    if (!w)
        return NULL;

    w->xid = xid;
    w->pid = pid; //FIXME we disabled this because it crashes on Tcl windows ? pid : manager_get_pid_from_xid (xid);
    w->title = strdup (title);
    w->workspace = ws;

    return w;
}

void zeitgeist_window_free (ZeitgeistWindow *w)
{
    if (!w)
        return;

    if (w->title)
        free (w->title);

    free (w);
}

void manager_add_active_window (ZeitgeistManager *mgr, Window xid)
{
    if (!mgr || !xid)
        return;

    if (!g_hash_table_contains (mgr->activeWins, GULONG_TO_POINTER (xid)))
    {
        ZeitgeistWindow *zwin = g_hash_table_lookup (mgr->openWins, GULONG_TO_POINTER (xid));
        if (zwin)
        {
            ZeitgeistWindow *copy = zeitgeist_window_new (zwin->xid, zwin->pid, zwin->title, zwin->workspace);
            g_hash_table_insert (mgr->activeWins, GULONG_TO_POINTER (xid), copy);
        }
    }
}

void manager_add_active_duration (ZeitgeistManager *mgr, Window xid, const double duration)
{
    if (!mgr || !xid)
        return;

    double *newDuration = malloc (sizeof (double));
    if (!newDuration)
        return;
    *newDuration = duration;

    if (g_hash_table_contains (mgr->activeWinDurations, GULONG_TO_POINTER (xid)))
    {
        double *oldDuration = g_hash_table_lookup (mgr->activeWinDurations, GULONG_TO_POINTER (xid));
        *newDuration += *oldDuration;
        // The old duration should be freed when we do the next insert
    }

    g_hash_table_insert (mgr->activeWinDurations, GULONG_TO_POINTER (xid), newDuration);
}

