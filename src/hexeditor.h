/* $Id$ */
/* Copyright (c) 2013 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop HexEditor */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#ifndef HEXEDITOR_HEXEDITOR_H
# define HEXEDITOR_HEXEDITOR_H

# include "HexEditor.h"


/* HexEditor */
/* protected */
/* types */
typedef struct _HexEditorPrefs
{
	int uppercase;
} HexEditorPrefs;


/* constants */
# define HEXEDITOR_CONFIG_FILE ".hexeditor"


/* functions */
HexEditor * hexeditor_new(GtkWidget * window, GtkAccelGroup * group,
		HexEditorPrefs * prefs, char const * device);
void hexeditor_delete(HexEditor * hexeditor);

/* accessors */
GtkWidget * hexeditor_get_widget(HexEditor * hexeditor);

void hexeditor_set_font(HexEditor * hexeditor, char const * font);

/* useful */
void hexeditor_close(HexEditor * hexeditor);
int hexeditor_open(HexEditor * hexeditor, char const * filename);
int hexeditor_open_dialog(HexEditor * hexeditor);

/* plug-ins */
int hexeditor_load(HexEditor * hexeditor, char const * plugin);
int hexeditor_unload(HexEditor * hexeditor, char const * plugin);

void hexeditor_show_preferences(HexEditor * hexeditor, gboolean show);
void hexeditor_show_properties(HexEditor * hexeditor, gboolean show);

#endif /* !HEXEDITOR_HEXEDITOR_H */
