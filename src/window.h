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



#ifndef HEXEDITOR_WINDOW_H
# define HEXEDITOR_WINDOW_H


/* public */
/* types */
typedef struct _HexEditorWindow HexEditorWindow;


/* functions */
HexEditorWindow * hexeditorwindow_new(char const * filename);
void hexeditorwindow_delete(HexEditorWindow * hexeditor);

#endif /* !HEXEDITOR_WINDOW_H */
