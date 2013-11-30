/* $Id$ */
static char const _copyright[] =
"Copyright © 2013 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop HexEditor */
static char const _license[] =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.";



#include <stdlib.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "hexeditor.h"
#include "window.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)


/* HexEditorWindow */
/* private */
/* types */
struct _HexEditorWindow
{
	HexEditor * hexeditor;

	/* widgets */
	GtkWidget * window;
};


/* prototypes */
/* callbacks */
static void _hexeditorwindow_on_close(gpointer data);
static gboolean _hexeditorwindow_on_closex(gpointer data);
static void _hexeditorwindow_on_contents(gpointer data);
static void _hexeditorwindow_on_open(gpointer data);

#ifndef EMBEDDED
/* menus */
static void _hexeditorwindow_on_file_close(gpointer data);
static void _hexeditorwindow_on_file_open(gpointer data);
static void _hexeditorwindow_on_file_properties(gpointer data);
static void _hexeditorwindow_on_edit_preferences(gpointer data);
static void _hexeditorwindow_on_help_about(gpointer data);
static void _hexeditorwindow_on_help_contents(gpointer data);
#endif


/* constants */
#ifndef EMBEDDED
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};
#endif

#ifdef EMBEDDED
static const DesktopAccel _hexeditorwindow_accel[] =
{
	{ G_CALLBACK(_hexeditorwindow_on_close), GDK_CONTROL_MASK, GDK_KEY_W },
	{ G_CALLBACK(_hexeditorwindow_on_contents), 0, GDK_KEY_F1 },
	{ NULL, 0, 0 }
};
#endif

#ifndef EMBEDDED
/* menus */
static const DesktopMenu _hexeditorwindow_menu_file[] =
{
	{ "_Open", G_CALLBACK(_hexeditorwindow_on_file_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O },
	{ "", NULL, NULL, 0, 0 },
	{ "_Properties", G_CALLBACK(_hexeditorwindow_on_file_properties),
		GTK_STOCK_PROPERTIES, GDK_MOD1_MASK, GDK_KEY_Return },
	{ "", NULL, NULL, 0, 0 },
	{ "_Close", G_CALLBACK(_hexeditorwindow_on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _hexeditorwindow_menu_edit[] =
{
	{ "_Preferences", G_CALLBACK(_hexeditorwindow_on_edit_preferences),
		GTK_STOCK_PREFERENCES, GDK_CONTROL_MASK, GDK_KEY_P },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _hexeditorwindow_menu_help[] =
{
	{ "_Contents", G_CALLBACK(_hexeditorwindow_on_help_contents),
		"help-contents", 0, GDK_KEY_F1 },
	{ "_About", G_CALLBACK(_hexeditorwindow_on_help_about), GTK_STOCK_ABOUT, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _hexeditorwindow_menubar[] =
{
	{ "_File", _hexeditorwindow_menu_file },
	{ "_Edit", _hexeditorwindow_menu_edit },
	{ "_Help", _hexeditorwindow_menu_help },
	{ NULL, NULL }
};
#endif


/* public */
/* functions */
/* hexeditorwindow_new */
HexEditorWindow * hexeditorwindow_new(char const * filename)
{
	HexEditorWindow * hexeditor;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;

	if((hexeditor = object_new(sizeof(*hexeditor))) == NULL)
		return NULL;
	group = gtk_accel_group_new();
	hexeditor->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(hexeditor->window), 640, 480);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(hexeditor->window), "text-editor");
#endif
	gtk_window_set_title(GTK_WINDOW(hexeditor->window),
			_("Hexadecimal editor"));
	g_signal_connect_swapped(hexeditor->window, "delete-event", G_CALLBACK(
				_hexeditorwindow_on_closex), hexeditor);
	hexeditor->hexeditor = NULL;
	if(hexeditor->window != NULL)
	{
		gtk_widget_realize(hexeditor->window);
		hexeditor->hexeditor = hexeditor_new(hexeditor->window, group,
				NULL, filename);
	}
	if(hexeditor->hexeditor == NULL)
	{
		hexeditorwindow_delete(hexeditor);
		return NULL;
	}
	gtk_window_add_accel_group(GTK_WINDOW(hexeditor->window), group);
	g_object_unref(group);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
#ifdef EMBEDDED
	desktop_accel_create(_hexeditorwindow_accel, hexeditor, group);
#endif
#ifndef EMBEDDED
	/* menubar */
	widget = desktop_menubar_create(_hexeditorwindow_menubar, hexeditor,
			group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
#endif
	widget = hexeditor_get_widget(hexeditor->hexeditor);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(hexeditor->window), vbox);
	gtk_widget_show_all(hexeditor->window);
	return hexeditor;
}


/* hexeditor_delete */
void hexeditorwindow_delete(HexEditorWindow * hexeditor)
{
	if(hexeditor->hexeditor != NULL)
		hexeditor_delete(hexeditor->hexeditor);
	if(hexeditor->window != NULL)
		gtk_widget_destroy(hexeditor->window);
	object_delete(hexeditor);
}


/* useful */
/* callbacks */
/* hexeditorwindow_on_close */
static void _hexeditorwindow_on_close(gpointer data)
{
	HexEditorWindow * hexeditor = data;

	gtk_widget_hide(hexeditor->window);
	gtk_main_quit();
}


/* hexeditorwindow_on_closex */
static gboolean _hexeditorwindow_on_closex(gpointer data)
{
	HexEditorWindow * hexeditor = data;

	_hexeditorwindow_on_close(hexeditor);
	return TRUE;
}


/* hexeditorwindow_on_contents */
static void _hexeditorwindow_on_contents(gpointer data)
{
	desktop_help_contents(PACKAGE, "hexeditor");
}


#ifndef EMBEDDED
/* menus */
/* hexeditorwindow_on_file_close */
static void _hexeditorwindow_on_file_close(gpointer data)
{
	HexEditorWindow * hexeditor = data;

	_hexeditorwindow_on_close(hexeditor);
}


/* hexeditorwindow_on_file_open */
static void _hexeditorwindow_on_file_open(gpointer data)
{
	HexEditorWindow * hexeditor = data;

	_hexeditorwindow_on_open(hexeditor);
}


/* hexeditorwindow_on_file_properties */
static void _hexeditorwindow_on_file_properties(gpointer data)
{
	HexEditorWindow * hexeditor = data;

	hexeditor_show_properties(hexeditor->hexeditor, TRUE);
}


/* hexeditorwindow_on_edit_preferences */
static void _hexeditorwindow_on_edit_preferences(gpointer data)
{
	HexEditorWindow * hexeditor = data;

	hexeditor_show_preferences(hexeditor->hexeditor, TRUE);
}


/* hexeditorwindow_on_help_about */
static void _hexeditorwindow_on_help_about(gpointer data)
{
	HexEditorWindow * hexeditor = data;
	GtkWidget * widget;
	char const comments[] = N_("Hexadecimal editor for the DeforaOS"
			" desktop");

	widget = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(
				hexeditor->window));
	desktop_about_dialog_set_authors(widget, _authors);
	desktop_about_dialog_set_comments(widget, _(comments));
	desktop_about_dialog_set_copyright(widget, _copyright);
	desktop_about_dialog_set_logo_icon_name(widget, "text-editor");
	desktop_about_dialog_set_license(widget, _license);
	desktop_about_dialog_set_name(widget, PACKAGE);
	desktop_about_dialog_set_version(widget, VERSION);
	desktop_about_dialog_set_website(widget, "http://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_destroy(widget);
}


/* hexeditorwindow_on_help_contents */
static void _hexeditorwindow_on_help_contents(gpointer data)
{
	HexEditorWindow * hexeditor = data;

	_hexeditorwindow_on_contents(hexeditor);
}
#endif


/* hexeditorwindow_on_open */
static void _hexeditorwindow_on_open(gpointer data)
{
	HexEditorWindow * hexeditor = data;

	hexeditor_open_dialog(hexeditor->hexeditor);
}
