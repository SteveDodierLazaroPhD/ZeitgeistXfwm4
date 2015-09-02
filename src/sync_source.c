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

#include <glib.h>
#include <sys/time.h>
#include <time.h>

#include "sync_source.h"

static gboolean
SyncedTimeoutSourcePrepare (GSource *source,
                            gint    *timeout)
{
    if (!source || !timeout)
    {
        fprintf (stderr, "Error while preparing a synced timeout source (%p:%p)\n", source, timeout);
        return FALSE;
    }

    SyncSource *syncSource = (SyncSource *) source;
    gint64 currentTime;
    gint64 elapsedTime;

    currentTime = g_get_real_time();
    elapsedTime = currentTime - syncSource->startTime;

    if (elapsedTime >= syncSource->microseconds)
    {
        syncSource->startTime += syncSource->microseconds;
        *timeout = syncSource->startTime - elapsedTime;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static gboolean
SyncedTimeoutSourceDispatch (GSource     *source,
                             GSourceFunc  sourceFunc,
                             gpointer     userData)
{
    if (sourceFunc)
        return sourceFunc(userData);

    return FALSE;
}

static GSourceFuncs source_funcs = {
    SyncedTimeoutSourcePrepare,
    NULL,
    SyncedTimeoutSourceDispatch,
    NULL
};

SyncSource *sync_source_new (unsigned int microseconds)
{
    GSource *source_ = NULL;
    source_ = g_source_new (&source_funcs, sizeof(SyncSource));
    if (!source_)
        return NULL;

    g_source_set_name(source_, "SyncSource");

    SyncSource *ssource = (SyncSource *)source_;
    ssource->microseconds = microseconds;

    gint64 realTime = g_get_real_time();
    gint microSecs = realTime % 1000;
    gint milliSecs = (realTime / 1000) % 1000;
    gint secs = (realTime / 1000000) % 60;
    gint mins = (realTime / 60000000) % 60;

    gint alignment = ((60 - secs) * 1000000) + ((mins % 2) ? 0:60000000) - microSecs - (milliSecs * 1000);
    ssource->startTime = realTime + alignment - microseconds;
    //g_source_set_ready_time (source_, ssource->startTime);

    return ssource;
}

unsigned int sync_source_run (SyncSource *source, GSourceFunc callback)
{
    unsigned int source_id;
    if (!source)
      return 0;

    GSource *source_ = (GSource *) source;
    g_source_set_callback (source_, callback, NULL, NULL);
    source_id = g_source_attach (source_, NULL);

    return source_id;
}

