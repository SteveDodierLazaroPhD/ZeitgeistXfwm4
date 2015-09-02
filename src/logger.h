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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <glib.h>
#include <sys/time.h>
#include <time.h>

#include "client.h"

#ifndef INC_LOGGER_H
#define INC_LOGGER_H

#define UCL_STUDY_IDLE_EVENT                     "activity://session-manager/xfsettingsd/PresenceAPI"
#define UCL_STUDY_WINDOW_TITLE_CHANGE_EVENT      "activity://window-manager/xfwm4/WindowTitleChange"
#define UCL_STUDY_WORKSPACE_CHANGE_EVENT         "activity://window-manager/xfwm4/WorkspaceChange"
#define UCL_STUDY_WINDOW_OPEN_EVENT              "activity://window-manager/xfwm4/WindowOpen"
#define UCL_STUDY_WINDOW_CLOSED_EVENT            "activity://window-manager/xfwm4/WindowClosed"
#define UCL_STUDY_APP_CRASHED_EVENT              "activity://window-manager/xfwm4/AppCrashed"
#define UCL_STUDY_ACTIVE_WINDOWS_EVENT           "activity://window-manager/xfwm4/ActiveWindows"
#define UCL_STUDY_MANIFESTATION_WINDOW_MANAGER   "activity://window-manager/xfwm4/WindowManagerManifestation"

void                     logWindowNameChange                    (Client *,
                                                                 const gchar *);

//void LogAppKilled(const pid_t pid, const unsigned long winId);
void                     logClientTerminate                     (Client *);

void                     logClientSetWorkspace                  (Client *c,
                                                                 guint previous_ws,
                                                                 guint new_ws);
                                                             
//void LogOpenWindow(/*const CompWindow * const win, */const unsigned long winId);
void                     logCompAddWindow                       (Window,
                                                                 Client *);

//void LogClosedWindow(/*const CompWindow * const win, */const unsigned long winId);
void                     logClientClose                         (Client *c);

//void LogActiveWindow(const unsigned long winId, const std::string winTitle);
//void LogInactiveWindow(const unsigned long winId);
void                     logSetFocus                            (Client *c,
                                                                 guint32 timestamp);
                                                                 
                                                                 



#endif /* INC_LOGGER_H */
