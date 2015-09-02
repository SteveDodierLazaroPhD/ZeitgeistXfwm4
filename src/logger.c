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


        oroborus - (c) 2001 Ken Lynch
        xfwm4    - (c) 2002-2015 Olivier Fourdan
        xfwm4-zg - (c) 2015 Steve Dodier-Lazaro

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include <glib.h>
#include <sys/time.h>
#include <time.h>
#include <zeitgeist.h>

#include "client.h"
#include "logger.h"
#include "sync_source.h"
#include "zeitgeist_manager.h"

void logWindowNameChange (Client *c, const gchar *old_name)
{
    if(!c) return;
    TRACE ("Client %p changed name to %s\n", c, c->name);

    ZeitgeistManager *manager  = manager_get ();
    char             *actor    = manager_get_actor_name_from_pid (c->pid);
    ZeitgeistEvent   *event    = zeitgeist_event_new ();

    zeitgeist_event_set_interpretation (event, UCL_STUDY_WINDOW_TITLE_CHANGE_EVENT);
    zeitgeist_event_set_manifestation (event, ZEITGEIST_ZG_SCHEDULED_ACTIVITY);
    zeitgeist_event_set_actor (event, actor);
    free (actor);

    ZeitgeistSubject *subject = zeitgeist_subject_new ();
    zeitgeist_subject_set_uri (subject, c->name? c->name : "n/a");
    zeitgeist_subject_set_interpretation (subject, ZEITGEIST_NFO_SOFTWARE);
    zeitgeist_subject_set_origin (subject, old_name);
    zeitgeist_subject_set_manifestation (subject, ZEITGEIST_NFO_SOFTWARE_ITEM);
    zeitgeist_subject_set_mimetype (subject, "text/plain");

    gchar *text = g_strdup_printf ("UCL Study window title change (%s)", c->name);
    zeitgeist_subject_set_text (subject, text);
    free (text);

    zeitgeist_event_add_subject (event, subject);

    ZeitgeistSubject *usubject = manager_get_ucl_subject (c->window, c->pid);
    zeitgeist_event_add_subject (event, usubject);

    GError *error = NULL;
    ZeitgeistLog *log = zeitgeist_log_get_default ();
    zeitgeist_log_insert_events_no_reply (log, event, NULL, &error);

    if (error)
    {
      g_warning ("Impossible to log event for window title change (%s): %s ", c->name, error->message);
      g_error_free (error);
    }
}

void logClientTerminate (Client *c)
{
    if(!c) return;
    TRACE ("Client %p:%s will be terminated\n", c, c->name);

    ZeitgeistManager *manager  = manager_get ();
    char             *actor    = manager_get_actor_name_from_pid (c->pid);
    ZeitgeistEvent   *event    = zeitgeist_event_new ();

    zeitgeist_event_set_interpretation (event, UCL_STUDY_APP_CRASHED_EVENT);
    zeitgeist_event_set_manifestation (event, ZEITGEIST_ZG_SCHEDULED_ACTIVITY);
    zeitgeist_event_set_actor (event, actor);
    free (actor);

    ZeitgeistSubject *subject = zeitgeist_subject_new ();
    zeitgeist_subject_set_uri (subject, c->name ? c->name : "n/a");
    zeitgeist_subject_set_interpretation (subject, ZEITGEIST_NFO_SOFTWARE);
    zeitgeist_subject_set_manifestation (subject, ZEITGEIST_NFO_SOFTWARE_ITEM);
    zeitgeist_subject_set_mimetype (subject, "text/plain");

    gchar *text = g_strdup_printf ("UCL Study app window froze (%s)", c->name);
    zeitgeist_subject_set_text (subject, text);
    free (text);

    zeitgeist_event_add_subject (event, subject);

    ZeitgeistSubject *usubject = manager_get_ucl_subject (c->window, c->pid);
    zeitgeist_event_add_subject (event, usubject);

    GError *error = NULL;
    ZeitgeistLog *log = zeitgeist_log_get_default ();
    zeitgeist_log_insert_events_no_reply (log, event, NULL, &error);

    if (error)
    {
      g_warning ("Impossible to log event for window freezing (%s): %s ", c->name, error->message);
      g_error_free (error);
    }
}

void logClientSetWorkspace (Client *c, guint previous_ws, guint new_ws)
{
    if(!c) return;
    TRACE ("Client %p:%s is moving from workspace %lu to %lu\n", c, c->name, previous_ws, new_ws);

    char             *actor    = manager_get_actor_name_from_pid (c->pid);

    if (g_strcmp0 (actor, "application://xfce4-panel.desktop") == 0 || g_strcmp0 (actor, "application://xfdesktop.desktop") == 0)
    {
        free (actor);
        return;
    }

    ZeitgeistManager *manager  = manager_get ();
    ZeitgeistEvent   *event    = zeitgeist_event_new ();
    ZeitgeistWindow  *zwin     = g_hash_table_lookup (manager->openWins, GULONG_TO_POINTER (c->window));

    if (!zwin)
    {
        fprintf (stderr, "Could not make a zeitgeist window object for window %lu\n", c->window);
        return;
    }

    if (!(previous_ws == zwin->workspace))
        TRACE("Client %p:%s was thought to be on workspace %lu, but was on %lu. It is now moving to %lu.\n", zwin->workspace, previous_ws, new_ws);

    zwin->workspace = new_ws;

    zeitgeist_event_set_interpretation (event, UCL_STUDY_WORKSPACE_CHANGE_EVENT);
    zeitgeist_event_set_manifestation (event, ZEITGEIST_ZG_USER_ACTIVITY);
    zeitgeist_event_set_actor (event, actor);
    free (actor);

    ZeitgeistSubject *subject = zeitgeist_subject_new ();
    zeitgeist_subject_set_uri (subject, zwin->title ? zwin->title : "n/a");
    zeitgeist_subject_set_interpretation (subject, ZEITGEIST_NFO_SOFTWARE);
    zeitgeist_subject_set_manifestation (subject, ZEITGEIST_NFO_SOFTWARE_ITEM);
    zeitgeist_subject_set_mimetype (subject, "text/plain");

    gchar *text = g_strdup_printf ("UCL Study window is moving from workspace %lu to %lu", previous_ws, new_ws);
    zeitgeist_subject_set_text (subject, text);
    free (text);

    zeitgeist_event_add_subject (event, subject);

    ZeitgeistSubject *usubject = manager_get_ucl_subject (c->window, c->pid);
    zeitgeist_event_add_subject (event, usubject);

    GError *error = NULL;
    ZeitgeistLog *log = zeitgeist_log_get_default ();
    zeitgeist_log_insert_events_no_reply (log, event, NULL, &error);

    if (error)
    {
      g_warning ("Impossible to log event for workspace change (%s): %s ", c->name, error->message);
      g_error_free (error);
    }
}

void logCompAddWindow (Window w, Client *c)
{
    if(!c || !w) return;
    TRACE ("Client %p:%s will be added\n", c, c->name);

    ZeitgeistManager *manager  = manager_get ();
    char             *actor    = manager_get_actor_name_from_pid (c->pid);
    ZeitgeistEvent   *event    = zeitgeist_event_new ();
    ZeitgeistWindow  *zwin     = NULL;

    //FIXME check for NULL

    if (!g_hash_table_contains (manager->openWins, GULONG_TO_POINTER (c->window)))
    {
        zwin = zeitgeist_window_new (c->window, c->pid, c->name, c->win_workspace);
        if (zwin)
            g_hash_table_insert (manager->openWins, GULONG_TO_POINTER (zwin->xid), zwin);
    }
    else
    {
        zwin = g_hash_table_lookup (manager->openWins, GULONG_TO_POINTER (c->window));
        TRACE (stderr, "Was asked to add a window that is already open, ignoring... (%lu:%s)\n", c->window, c->name);
    }

    if (!zwin)
    {
        fprintf (stderr, "Could not make a zeitgeist window object for window %lu\n", c->window);
        return;
    }

    zeitgeist_event_set_interpretation (event, UCL_STUDY_WINDOW_OPEN_EVENT);
    zeitgeist_event_set_manifestation (event, ZEITGEIST_ZG_USER_ACTIVITY);
    zeitgeist_event_set_actor (event, actor);
    free (actor);

    ZeitgeistSubject *subject = zeitgeist_subject_new ();
    zeitgeist_subject_set_uri (subject, zwin->title ? zwin->title : "n/a");
    zeitgeist_subject_set_interpretation (subject, ZEITGEIST_NFO_SOFTWARE);
    zeitgeist_subject_set_manifestation (subject, ZEITGEIST_NFO_SOFTWARE_ITEM);
    zeitgeist_subject_set_mimetype (subject, "text/plain");

    gchar *text = g_strdup_printf ("UCL Study window opened (%s, on workspace %u)", zwin->title, zwin->workspace);
    zeitgeist_subject_set_text (subject, text);
    free (text);

    zeitgeist_event_add_subject (event, subject);

    ZeitgeistSubject *usubject = manager_get_ucl_subject (c->window, c->pid);
    zeitgeist_event_add_subject (event, usubject);

    GError *error = NULL;
    ZeitgeistLog *log = zeitgeist_log_get_default ();
    zeitgeist_log_insert_events_no_reply (log, event, NULL, &error);

    if (error)
    {
      g_warning ("Impossible to log event for new open window (%s): %s ", c->name, error->message);
      g_error_free (error);
    }
}

void logClientClose (Client *c)
{
    if(!c) return;
    TRACE ("Client %p:%s will be closed\n", c, c->name);

    ZeitgeistManager *manager  = manager_get ();
    char             *actor    = manager_get_actor_name_from_pid (c->pid);
    ZeitgeistEvent   *event    = zeitgeist_event_new ();

    if (!g_hash_table_contains (manager->openWins, GULONG_TO_POINTER (c->window)))
        return;

    zeitgeist_event_set_interpretation (event, UCL_STUDY_WINDOW_CLOSED_EVENT);
    zeitgeist_event_set_manifestation (event, ZEITGEIST_ZG_USER_ACTIVITY);
    zeitgeist_event_set_actor (event, actor);
    free (actor);

    ZeitgeistSubject *subject = zeitgeist_subject_new ();
    zeitgeist_subject_set_uri (subject, c->name ? c->name : "n/a");
    zeitgeist_subject_set_interpretation (subject, ZEITGEIST_NFO_SOFTWARE);
    zeitgeist_subject_set_manifestation (subject, ZEITGEIST_NFO_SOFTWARE_ITEM);
    zeitgeist_subject_set_mimetype (subject, "text/plain");

    gchar *text = g_strdup_printf ("UCL Study window closed (%s, on desktop %u)", c->name, c->win_workspace);
    zeitgeist_subject_set_text (subject, text);
    free (text);

    zeitgeist_event_add_subject (event, subject);

    ZeitgeistSubject *usubject = manager_get_ucl_subject (c->window, c->pid);
    zeitgeist_event_add_subject (event, usubject);

    GError *error = NULL;
    ZeitgeistLog *log = zeitgeist_log_get_default ();
    zeitgeist_log_insert_events_no_reply (log, event, NULL, &error);

    if (error)
    {
      g_warning ("Impossible to log event for new open window (%s): %s ", c->name, error->message);
      g_error_free (error);
    }

    g_hash_table_remove (manager->openWins, GULONG_TO_POINTER (c->window));
}

void logSetFocus (Client *c, guint32 timestamp)
{
    if(!c) return;
    TRACE ("Client %p:%s is taking focus at %u\n", c, c->name, timestamp);

    ZeitgeistManager *manager  = manager_get ();
    time_t            now      = time (NULL);

    // First notify that the previous window is now inactive
    if (manager->currentWin && manager->currentWin != c->window)
    {
        manager_add_active_duration (manager, manager->currentWin, ((double)(now - manager->currentWinTime)));
        manager->currentWin = 0;
    }

    manager_add_active_window (manager, c->window);

    // In case the same window is said to be active multiple times, to avoid resetting the timer
    if (manager->currentWin != c->window)
    {
        manager->currentWinTime = (double) now;
        manager->currentWin = c->window;
    }
}
