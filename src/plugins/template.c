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



#include "HexEditor/plugin.h"
#include <System.h>


/* Template */
/* types */
typedef struct _HexEditorPlugin
{
	HexEditorPluginHelper * helper;
	/* FIXME really implement */
	GtkWidget * widget;
	GtkWidget * view;
} TemplatePlugin;


/* prototypes */
/* plug-in */
static TemplatePlugin * _templateplugin_init(HexEditorPluginHelper * helper);
static void _templateplugin_destroy(TemplatePlugin * template);

static GtkWidget * _templateplugin_get_widget(TemplatePlugin * template);
static void _templateplugin_read(TemplatePlugin * template, off_t offset,
		char const * buffer, size_t size);


/* plug-in */
HexEditorPluginDefinition plugin =
{
	"Template",
	NULL,
	NULL,
	_templateplugin_init,
	_templateplugin_destroy,
	_templateplugin_get_widget,
	_templateplugin_read
};


/* functions */
/* templateplugin_init */
static TemplatePlugin * _templateplugin_init(HexEditorPluginHelper * helper)
{
	TemplatePlugin * template;

	if((template = object_new(sizeof(*template))) == NULL)
		return NULL;
	template->helper = helper;
	/* FIXME really implement */
	template->widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(template->widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	template->view = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(template->widget), template->view);
	gtk_widget_show_all(template->widget);
	return template;
}


/* templateplugin_destroy */
static void _templateplugin_destroy(TemplatePlugin * template)
{
	object_delete(template);
}


/* templateplugin_get_widget */
static GtkWidget * _templateplugin_get_widget(TemplatePlugin * template)
{
	return template->widget;
}


/* templateplugin_read */
static void _templateplugin_read(TemplatePlugin * template, off_t offset,
		char const * buffer, size_t size)
{
	/* FIXME implement */
}
