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



#ifndef DESKTOP_HEXEDITOR_PLUGIN_H
# define DESKTOP_HEXEDITOR_PLUGIN_H


/* HexEditor */
/* public */
/* types */
typedef struct _HexEditor HexEditor;

typedef struct _HexEditorPlugin HexEditorPlugin;

typedef struct _HexEditorPluginHelper
{
	HexEditor * hexeditor;
	int (*error)(HexEditor * hexeditor, char const * message, int ret);
} HexEditorPluginHelper;

typedef const struct _HexEditorPluginDefinition
{
	char const * name;
	char const * icon;
	char const * description;
	HexEditorPlugin * (*init)(HexEditorPluginHelper * helper);
	void (*destroy)(HexEditorPlugin * plugin);
	GtkWidget * (*get_widget)(HexEditorPlugin * plugin);
} HexEditorPluginDefinition;

#endif /* DESKTOP_HEXEDITOR_PLUGIN_H */
