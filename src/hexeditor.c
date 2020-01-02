/* $Id$ */
static char const _copyright[] =
"Copyright © 2013-2018 Pierre Pronchery <khorben@defora.org>";
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



#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "HexEditor/plugin.h"
#include "hexeditor.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) string

/* constants */
#ifndef PROGNAME_HEXEDITOR
# define PROGNAME_HEXEDITOR	"hexeditor"
#endif
#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR			PREFIX "/bin"
#endif


/* HexEditor */
/* private */
/* types */
struct _HexEditor
{
	Config * config;

	char * filename;
	int fd;
	GIOChannel * channel;
	guint source;
	gsize offset;
	gsize size;
	time_t time;

	/* preferences */
	HexEditorPrefs prefs;

	/* widgets */
	GtkWidget * widget;
	GtkWidget * window;
	PangoFontDescription * bold;
#if GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * infobar;
	GtkWidget * infobar_label;
#endif
	GtkWidget * view_addr;
	GtkTextBuffer * view_addr_tbuf;
	GtkTextIter view_addr_iter;
	GtkWidget * view_hex;
	GtkTextBuffer * view_hex_tbuf;
	GtkTextIter view_hex_iter;
	GtkWidget * view_data;
	GtkTextBuffer * view_data_tbuf;
	GtkTextIter view_data_iter;
	/* progress */
	GtkWidget * pg_window;
	GtkWidget * pg_progress;
	/* plug-ins */
	GtkWidget * pl_view;
	GtkListStore * pl_store;
	GtkWidget * pl_combo;
	GtkWidget * pl_box;
	HexEditorPluginHelper pl_helper;
};


/* constants */
typedef enum _HexEditorPluginColumn
{
	HEPC_NAME = 0,
	HEPC_ENABLED,
	HEPC_ICON,
	HEPC_NAME_DISPLAY,
	HEPC_PLUGIN,
	HEPC_HEXEDITORPLUGINDEFINITION,
	HEPC_HEXEDITORPLUGIN,
	HEPC_WIDGET
} HexEditorPluginColumn;
#define HEPC_LAST	HEPC_WIDGET
#define HEPC_COUNT	(HEPC_LAST + 1)


/* prototypes */
/* accessors */
static String * _hexeditor_get_config_filename(char const * filename);
static int _hexeditor_plugin_is_enabled(HexEditor * hexeditor,
		char const * plugin);

/* useful */
static void _hexeditor_close(HexEditor * hexeditor, gboolean plugins);
static int _hexeditor_config_load(HexEditor * hexeditor);
static int _hexeditor_error(HexEditor * hexeditor, char const * message,
		int ret);

/* callbacks */
static void _hexeditor_on_open(gpointer data);
static void _hexeditor_on_plugin_combo_change(gpointer data);
#ifdef EMBEDDED
static void _hexeditor_on_preferences(gpointer data);
#endif
static void _hexeditor_on_progress_cancel(gpointer data);
static gboolean _hexeditor_on_progress_delete(gpointer data);
#ifdef EMBEDDED
static void _hexeditor_on_properties(gpointer data);
#endif


/* variables */
static DesktopToolbar _hexeditor_toolbar[] =
{
	{ N_("Open"), G_CALLBACK(_hexeditor_on_open), GTK_STOCK_OPEN, 0, 0,
		NULL },
#ifdef EMBEDDED
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Properties"), G_CALLBACK(_hexeditor_on_properties),
		GTK_STOCK_PROPERTIES, GDK_MOD1_MASK, GDK_KEY_Return, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Preferences"), G_CALLBACK(_hexeditor_on_preferences),
		GTK_STOCK_PREFERENCES, GDK_CONTROL_MASK, GDK_KEY_P, NULL },
#endif
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* hexeditor_new */
static void _new_plugins(HexEditor * hexeditor);
static void _new_progress(HexEditor * hexeditor);

HexEditor * hexeditor_new(GtkWidget * window, GtkAccelGroup * group,
		HexEditorPrefs * prefs, char const * filename)
{
	HexEditor * hexeditor;
	GtkWidget * vbox;
	GtkWidget * hpaned;
	GtkWidget * hbox;
	GtkWidget * widget;
	GtkAdjustment * adjustment;
	char const * p;

	if((hexeditor = object_new(sizeof(*hexeditor))) == NULL)
		return NULL;
	/* default preferences */
	hexeditor->prefs.uppercase = 0;
	if((hexeditor->config = config_new()) == NULL
			|| _hexeditor_config_load(hexeditor) != 0)
		_hexeditor_error(NULL, _("Error while loading configuration"),
				1);
	hexeditor->filename = NULL;
	hexeditor->fd = -1;
	hexeditor->channel = NULL;
	hexeditor->source = 0;
	hexeditor->offset = 0;
	hexeditor->size = 0;
	hexeditor->time = 0;
	if(prefs != NULL)
		hexeditor->prefs = *prefs;
	hexeditor->bold = pango_font_description_new();
	pango_font_description_set_weight(hexeditor->bold, PANGO_WEIGHT_BOLD);
	hexeditor->window = window;
	/* create the widget */
	hexeditor->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	vbox = hexeditor->widget;
	/* toolbar */
	widget = desktop_toolbar_create(_hexeditor_toolbar, hexeditor, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(2, 18, 0)
	/* infobar */
	hexeditor->infobar = gtk_info_bar_new_with_buttons(GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE, NULL);
	gtk_info_bar_set_message_type(GTK_INFO_BAR(hexeditor->infobar),
			GTK_MESSAGE_ERROR);
	g_signal_connect(hexeditor->infobar, "close", G_CALLBACK(
				gtk_widget_hide), NULL);
	g_signal_connect(hexeditor->infobar, "response", G_CALLBACK(
				gtk_widget_hide), NULL);
	widget = gtk_info_bar_get_content_area(GTK_INFO_BAR(
				hexeditor->infobar));
	hexeditor->infobar_label = gtk_label_new(NULL);
	gtk_widget_show(hexeditor->infobar_label);
	gtk_box_pack_start(GTK_BOX(widget), hexeditor->infobar_label, TRUE,
			TRUE, 0);
	gtk_widget_set_no_show_all(hexeditor->infobar, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), hexeditor->infobar, FALSE, TRUE, 0);
#endif
	/* view */
	hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_position(GTK_PANED(hpaned), 500);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	/* view: address */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(
				widget));
	hexeditor->view_addr = gtk_text_view_new();
	hexeditor->view_addr_tbuf = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(hexeditor->view_addr));
	gtk_text_buffer_get_end_iter(hexeditor->view_addr_tbuf,
			&hexeditor->view_addr_iter);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(hexeditor->view_addr),
			FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(hexeditor->view_addr), FALSE);
	gtk_container_add(GTK_CONTAINER(widget), hexeditor->view_addr);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* view: hexadecimal */
	widget = gtk_scrolled_window_new(NULL, adjustment);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	hexeditor->view_hex = gtk_text_view_new();
	hexeditor->view_hex_tbuf = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(hexeditor->view_hex));
	gtk_text_buffer_get_end_iter(hexeditor->view_hex_tbuf,
			&hexeditor->view_hex_iter);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(hexeditor->view_hex),
			FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(hexeditor->view_hex), FALSE);
	gtk_container_add(GTK_CONTAINER(widget), hexeditor->view_hex);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 4);
	/* view: data */
	widget = gtk_scrolled_window_new(NULL, adjustment);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	hexeditor->view_data = gtk_text_view_new();
	hexeditor->view_data_tbuf = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(hexeditor->view_data));
	gtk_text_buffer_get_end_iter(hexeditor->view_data_tbuf,
			&hexeditor->view_data_iter);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(hexeditor->view_data),
			FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(hexeditor->view_data), FALSE);
	gtk_container_add(GTK_CONTAINER(widget), hexeditor->view_data);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_paned_add1(GTK_PANED(hpaned), hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
	p = (hexeditor->config != NULL)
		? config_get(hexeditor->config, NULL, "font") : NULL;
	hexeditor_set_font(hexeditor, p);
	gtk_widget_set_sensitive(hexeditor->view_addr, FALSE);
	gtk_widget_set_sensitive(hexeditor->view_hex, FALSE);
	gtk_widget_set_sensitive(hexeditor->view_data, FALSE);
	_new_progress(hexeditor);
	_new_plugins(hexeditor);
	gtk_paned_add2(GTK_PANED(hpaned), hexeditor->pl_view);
	if(filename != NULL)
		hexeditor_open(hexeditor, filename);
	gtk_widget_show_all(vbox);
	return hexeditor;
}

static void _new_plugins(HexEditor * hexeditor)
{
	GtkCellRenderer * renderer;
	char const * plugins;
	char * p;
	char * q;
	size_t i;

	hexeditor->pl_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(hexeditor->pl_view), 4);
	gtk_widget_set_no_show_all(hexeditor->pl_view, TRUE);
	hexeditor->pl_store = gtk_list_store_new(HEPC_COUNT, G_TYPE_STRING,
			G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING,
			G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
			G_TYPE_POINTER);
	hexeditor->pl_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(
				hexeditor->pl_store));
	g_signal_connect_swapped(hexeditor->pl_combo, "changed", G_CALLBACK(
				_hexeditor_on_plugin_combo_change), hexeditor);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(hexeditor->pl_combo),
			renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(hexeditor->pl_combo),
			renderer, "pixbuf", HEPC_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(hexeditor->pl_combo),
			renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(hexeditor->pl_combo),
			renderer, "text", HEPC_NAME_DISPLAY, NULL);
	gtk_box_pack_start(GTK_BOX(hexeditor->pl_view), hexeditor->pl_combo,
			FALSE, TRUE, 0);
	hexeditor->pl_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start(GTK_BOX(hexeditor->pl_view), hexeditor->pl_box, TRUE,
			TRUE, 0);
	hexeditor->pl_helper.hexeditor = hexeditor;
	hexeditor->pl_helper.error = _hexeditor_error;
	/* load the plug-ins */
	if((plugins = config_get(hexeditor->config, NULL, "plugins")) == NULL
			|| strlen(plugins) == 0)
		return;
	if((p = strdup(plugins)) == NULL)
		return; /* XXX report error */
	for(q = p, i = 0;;)
	{
		if(q[i] == '\0')
		{
			hexeditor_load(hexeditor, q);
			break;
		}
		if(q[i++] != ',')
			continue;
		q[i - 1] = '\0';
		hexeditor_load(hexeditor, q);
		q += i;
		i = 0;
	}
}

static void _new_progress(HexEditor * hexeditor)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hexeditor->pg_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(hexeditor->pg_window), 16);
	gtk_window_set_decorated(GTK_WINDOW(hexeditor->pg_window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(hexeditor->pg_window), 200, 50);
	gtk_window_set_modal(GTK_WINDOW(hexeditor->pg_window), TRUE);
	gtk_window_set_title(GTK_WINDOW(hexeditor->pg_window), _("Progress"));
	gtk_window_set_transient_for(GTK_WINDOW(hexeditor->pg_window),
			GTK_WINDOW(hexeditor->window));
	gtk_window_set_position(GTK_WINDOW(hexeditor->pg_window),
			GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect_swapped(hexeditor->pg_window, "delete-event",
			G_CALLBACK(_hexeditor_on_progress_delete), hexeditor);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	hexeditor->pg_progress = gtk_progress_bar_new();
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(hexeditor->pg_progress),
			TRUE);
#endif
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(hexeditor->pg_progress), "");
	gtk_box_pack_start(GTK_BOX(hbox), hexeditor->pg_progress, FALSE, TRUE,
			0);
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_hexeditor_on_progress_cancel), hexeditor);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(hexeditor->pg_window), hbox);
}


/* hexeditor_delete */
static void _delete_plugins(HexEditor * hexeditor);

void hexeditor_delete(HexEditor * hexeditor)
{
	_hexeditor_close(hexeditor, FALSE);
	_delete_plugins(hexeditor);
	pango_font_description_free(hexeditor->bold);
	if(hexeditor->config != NULL)
		config_delete(hexeditor->config);
	object_delete(hexeditor);
}

static void _delete_plugins(HexEditor * hexeditor)
{
	GtkTreeModel * model = GTK_TREE_MODEL(hexeditor->pl_store);
	GtkTreeIter iter;
	gboolean valid;
	Plugin * plugin;
	HexEditorPluginDefinition * hepd;
	HexEditorPlugin * hep;

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, HEPC_PLUGIN, &plugin,
				HEPC_HEXEDITORPLUGINDEFINITION, &hepd,
				HEPC_HEXEDITORPLUGIN, &hep, -1);
		if(hepd->destroy != NULL)
			hepd->destroy(hep);
		plugin_delete(plugin);
	}
}


/* accessors */
/* hexeditor_get_widget */
GtkWidget * hexeditor_get_widget(HexEditor * hexeditor)
{
	return hexeditor->widget;
}


/* hexeditor_set_font */
void hexeditor_set_font(HexEditor * hexeditor, char const * font)
{
	PangoFontDescription * desc;

	if(font == NULL)
	{
		desc = pango_font_description_new();
		pango_font_description_set_family(desc, "Monospace");
	}
	else
		desc = pango_font_description_from_string(font);
	gtk_widget_override_font(hexeditor->view_addr, desc);
	gtk_widget_override_font(hexeditor->view_hex, desc);
	gtk_widget_override_font(hexeditor->view_data, desc);
	pango_font_description_free(desc);
}


/* useful */
/* hexeditor_close */
void hexeditor_close(HexEditor * hexeditor)
{
	_hexeditor_close(hexeditor, TRUE);
}


/* hexeditor_load */
int hexeditor_load(HexEditor * hexeditor, char const * plugin)
{
	Plugin * p;
	HexEditorPluginDefinition * hepd;
	HexEditorPlugin * hep;
	GtkWidget * widget;
	GtkTreeIter iter;
	GtkIconTheme * theme;
	GdkPixbuf * icon = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, plugin);
#endif
	if(_hexeditor_plugin_is_enabled(hexeditor, plugin))
		return 0;
	if((p = plugin_new(LIBDIR, PACKAGE, "plugins", plugin)) == NULL)
		return -_hexeditor_error(hexeditor, error_get(NULL), 1);
	if((hepd = plugin_lookup(p, "plugin")) == NULL)
	{
		plugin_delete(p);
		return -_hexeditor_error(hexeditor, error_get(NULL), 1);
	}
	if(hepd->init == NULL || hepd->destroy == NULL
			|| hepd->get_widget == NULL
			|| (hep = hepd->init(&hexeditor->pl_helper)) == NULL)
	{
		plugin_delete(p);
		/* FIXME the error may not be set */
		return -_hexeditor_error(hexeditor, error_get(NULL), 1);
	}
	widget = hepd->get_widget(hep);
	gtk_widget_hide(widget);
	theme = gtk_icon_theme_get_default();
	if(hepd->icon != NULL)
		icon = gtk_icon_theme_load_icon(theme, hepd->icon, 24, 0, NULL);
	if(icon == NULL)
		icon = gtk_icon_theme_load_icon(theme, "gnome-settings", 24, 0,
				NULL);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_list_store_insert_with_values(hexeditor->pl_store, &iter, -1,
#else
	gtk_list_store_append(hexeditor->pl_store, &iter);
	gtk_list_store_set(hexeditor->pl_store, &iter,
#endif
			HEPC_NAME, plugin, HEPC_ICON, icon,
			HEPC_NAME_DISPLAY, hepd->name,
			HEPC_PLUGIN, p, HEPC_HEXEDITORPLUGINDEFINITION, hepd,
			HEPC_HEXEDITORPLUGIN, hep, HEPC_WIDGET, widget, -1);
	if(icon != NULL)
		g_object_unref(icon);
	gtk_box_pack_start(GTK_BOX(hexeditor->pl_box), widget, TRUE, TRUE, 0);
	if(gtk_widget_get_no_show_all(hexeditor->pl_view) == TRUE)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(hexeditor->pl_combo), 0);
		gtk_widget_set_no_show_all(hexeditor->pl_view, FALSE);
		gtk_widget_show_all(hexeditor->pl_view);
	}
	return 0;
}


/* hexeditor_open */
static gboolean _open_on_can_read(GIOChannel * channel, GIOCondition condition,
		gpointer data);
static gboolean _open_on_idle(gpointer data);
static void _open_plugins_read(HexEditor * hexeditor, char const * buf,
		size_t size);
static void _open_progress(HexEditor * hexeditor);
static void _open_read_1(HexEditor * hexeditor, char * buf, gsize pos);
static void _open_read_16(HexEditor * hexeditor, char * buf, gsize pos);

int hexeditor_open(HexEditor * hexeditor, char const * filename)
{
	char buf[256];
	gchar * p;
	struct stat st;

	if(filename == NULL)
		return hexeditor_open_dialog(hexeditor);
	hexeditor_close(hexeditor);
	if((hexeditor->filename = strdup(filename)) == NULL)
		return -_hexeditor_error(hexeditor, strerror(errno), 1);
	if((hexeditor->fd = open(filename, O_RDONLY)) < 0)
	{
		free(hexeditor->filename);
		hexeditor->filename = NULL;
		return -_hexeditor_error(hexeditor, strerror(errno), 1);
	}
	p = g_filename_display_name(filename);
	snprintf(buf, sizeof(buf), "%s - %s", _("Hexadecimal editor"), p);
	g_free(p);
	gtk_window_set_title(GTK_WINDOW(hexeditor->window), buf);
	hexeditor->channel = g_io_channel_unix_new(hexeditor->fd);
	g_io_channel_set_encoding(hexeditor->channel, NULL, NULL);
	hexeditor->source = g_io_add_watch(hexeditor->channel, G_IO_IN,
			_open_on_can_read, hexeditor);
	hexeditor->offset = 0;
	hexeditor->size = 0;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(hexeditor->pg_progress),
			0.0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(hexeditor->pg_progress), "");
	if(fstat(hexeditor->fd, &st) == 0)
		hexeditor->size = st.st_size;
	hexeditor->time = time(NULL);
	gtk_widget_show_all(hexeditor->pg_window);
	return 0;
}

static gboolean _open_on_can_read(GIOChannel * channel, GIOCondition condition,
		gpointer data)
{
	HexEditor * hexeditor = data;
	GIOStatus status;
	char buf[BUFSIZ];
	gsize size = sizeof(buf);
	GError * error = NULL;
	gsize i;

	if(channel != hexeditor->channel || condition != G_IO_IN)
		return FALSE;
	status = g_io_channel_read_chars(channel, buf, size, &size, &error);
	if(status == G_IO_STATUS_AGAIN)
		/* this status can be ignored */
		return TRUE;
	if(status != G_IO_STATUS_NORMAL && status != G_IO_STATUS_EOF)
	{
		hexeditor->source = 0;
		hexeditor_close(hexeditor);
		if(status == G_IO_STATUS_ERROR)
		{
			_hexeditor_error(hexeditor, error->message, 1);
			g_error_free(error);
		}
		gtk_widget_hide(hexeditor->pg_window);
		return FALSE;
	}
	/* read until the end of a line */
	for(i = 0; ((hexeditor->offset + i) % 16) != 0 && i < size; i++)
		_open_read_1(hexeditor, buf, i);
	/* read complete lines */
	for(; i + 15 < size; i += 16)
		_open_read_16(hexeditor, buf, i);
	/* read until the end of the buffer */
	for(; i < size; i++)
		_open_read_1(hexeditor, buf, i);
	/* tell the plug-ins */
	_open_plugins_read(hexeditor, buf, size);
	hexeditor->offset += size;
	if(status == G_IO_STATUS_EOF)
	{
		/* tell the plug-ins if relevant */
		if(size != 0)
			_open_plugins_read(hexeditor, NULL, 0);
		hexeditor->source = 0;
		gtk_widget_set_sensitive(hexeditor->view_addr, TRUE);
		gtk_widget_set_sensitive(hexeditor->view_hex, TRUE);
		gtk_widget_set_sensitive(hexeditor->view_data, TRUE);
		gtk_widget_hide(hexeditor->pg_window);
		return FALSE;
	}
	_open_progress(hexeditor);
	hexeditor->source = g_idle_add(_open_on_idle, hexeditor);
	return FALSE;
}

static gboolean _open_on_idle(gpointer data)
{
	HexEditor * hexeditor = data;

	hexeditor->source = g_io_add_watch(hexeditor->channel, G_IO_IN,
			_open_on_can_read, hexeditor);
	return FALSE;
}

static void _open_plugins_read(HexEditor * hexeditor, char const * buf,
		size_t size)
{
	GtkTreeModel * model = GTK_TREE_MODEL(hexeditor->pl_store);
	GtkTreeIter iter;
	gboolean valid;
	HexEditorPluginDefinition * hepd;
	HexEditorPlugin * hep;

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter,
				HEPC_HEXEDITORPLUGINDEFINITION, &hepd,
				HEPC_HEXEDITORPLUGIN, &hep, -1);
		if(hepd->read != NULL)
			hepd->read(hep, hexeditor->offset, buf, size);
	}
}

static void _open_progress(HexEditor * hexeditor)
{
	time_t t;
	gdouble fraction;
	char buf[16];
	GtkProgressBar * progress = GTK_PROGRESS_BAR(hexeditor->pg_progress);

	/* pulse the progress bar once per second */
	if((t = time(NULL)) <= hexeditor->time)
		return;
	hexeditor->time = t;
	if(hexeditor->size == 0)
	{
		gtk_progress_bar_pulse(progress);
		return;
	}
	fraction = hexeditor->offset;
	fraction = fraction / hexeditor->size;
	gtk_progress_bar_set_fraction(progress, fraction);
	snprintf(buf, sizeof(buf), "%.1f%%", fraction * 100);
	gtk_progress_bar_set_text(progress, buf);
}

static void _open_read_1(HexEditor * hexeditor, char * buf, gsize pos)
{
	char buf2[16];
	GtkTextBuffer * taddr;
	GtkTextBuffer * thex;
	GtkTextBuffer * tdata;
	int c;
	int size;

	taddr = hexeditor->view_addr_tbuf;
	thex = hexeditor->view_hex_tbuf;
	tdata = hexeditor->view_data_tbuf;
	c = (unsigned char)buf[pos];
	if(((hexeditor->offset + pos) % 16) == 0)
	{
		/* address */
		size = snprintf(buf2, sizeof(buf2), hexeditor->prefs.uppercase
				? "%s%08X" : "%s%08x",
				(hexeditor->offset + pos) ? "\n" : "",
				(unsigned int)(hexeditor->offset + pos));
		gtk_text_buffer_insert(taddr, &hexeditor->view_addr_iter,
				buf2, size);
		/* hexadecimal value */
		size = snprintf(buf2, sizeof(buf2), hexeditor->prefs.uppercase
				? "%s%02X" : "%s%02x",
				(hexeditor->offset + pos) ? "\n" : "", c);
		gtk_text_buffer_insert(thex, &hexeditor->view_hex_iter,
				buf2, size);
		if(hexeditor->offset + pos != 0)
			/* character value */
			gtk_text_buffer_insert(tdata,
					&hexeditor->view_data_iter, "\n", 1);
	}
	else
	{
		/* hexadecimal value */
		size = snprintf(buf2, sizeof(buf2), hexeditor->prefs.uppercase
				? " %02X" : " %02x", c);
		gtk_text_buffer_insert(thex, &hexeditor->view_hex_iter,
				buf2, size);
	}
	/* character value */
	size = snprintf(buf2, sizeof(buf2), "%c", (isascii(c) && isprint(c))
			? c : '.');
	gtk_text_buffer_insert(tdata, &hexeditor->view_data_iter, buf2, size);
}

static void _open_read_16(HexEditor * hexeditor, char * buf, gsize pos)
{
	GtkTextBuffer * taddr;
	GtkTextBuffer * thex;
	GtkTextBuffer * tdata;
	unsigned char c[16];
	int i;
	char buf2[64];

	taddr = hexeditor->view_addr_tbuf;
	thex = hexeditor->view_hex_tbuf;
	tdata = hexeditor->view_data_tbuf;
	/* address */
	i = snprintf(buf2, sizeof(buf2), hexeditor->prefs.uppercase
			? "%s%08X" : "%s%08x",
			(hexeditor->offset + pos) ? "\n" : "",
			(unsigned int)(hexeditor->offset + pos));
	gtk_text_buffer_insert(taddr, &hexeditor->view_addr_iter, buf2, i);
	/* hexadecimal values */
	for(i = 0; i < 16; i++)
		c[i] = (unsigned char)buf[pos + i];
	i = snprintf(buf2, sizeof(buf2), hexeditor->prefs.uppercase
			? "%s%02X %02X %02X %02X %02X %02X %02X %02X"
			" %02X %02X %02X %02X %02X %02X %02X %02X"
			: "%s%02x %02x %02x %02x %02x %02x %02x %02x"
			" %02x %02x %02x %02x %02x %02x %02x %02x",
			(hexeditor->offset + pos) ? "\n" : "",
			c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
			c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15]);
	gtk_text_buffer_insert(thex, &hexeditor->view_hex_iter, buf2, i);
	/* character values */
	if(hexeditor->offset + pos != 0)
		gtk_text_buffer_insert(tdata, &hexeditor->view_data_iter,
				"\n", 1);
	for(i = 0; i < 16; i++)
		if(!isascii(c[i]) || !isprint(c[i]))
			c[i] = '.';
	gtk_text_buffer_insert(tdata, &hexeditor->view_data_iter,
			(char const *)c, 16);
}


/* hexeditor_open_dialog */
int hexeditor_open_dialog(HexEditor * hexeditor)
{
	int ret;
	GtkWidget * dialog;
	GtkFileFilter * filter;
	gchar * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Open file..."),
			GTK_WINDOW(hexeditor->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return -1;
	ret = hexeditor_open(hexeditor, filename);
	g_free(filename);
	return ret;
}


/* hexeditor_show_preferences */
void hexeditor_show_preferences(HexEditor * hexeditor, gboolean show)
{
	/* FIXME implement */
}


/* hexeditor_show_properties */
static GtkWidget * _properties_widget(HexEditor * hexeditor,
		GtkSizeGroup * group, char const * label, GtkWidget * value);

void hexeditor_show_properties(HexEditor * hexeditor, gboolean show)
{
	const unsigned int flags = GTK_DIALOG_MODAL
		| GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget * dialog;
	GtkSizeGroup * hgroup;
	GtkSizeGroup * vgroup;
	GtkWidget * vbox;
	GtkWidget * widget;
	String * s;
	gchar * p;
	gchar * q;
	GError * error = NULL;

	if(show == FALSE)
		/* XXX should really hide the window */
		return;
	dialog = gtk_message_dialog_new(GTK_WINDOW(hexeditor->window), flags,
			GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Properties"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"");
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog),
			gtk_image_new_from_icon_name("gtk-properties",
				GTK_ICON_SIZE_DIALOG));
#endif
	if(hexeditor->filename == NULL)
		s = string_new(_("Properties"));
	else
	{
		p = g_path_get_basename(hexeditor->filename);
		s = string_new_format(_("Properties of %s"), p);
		g_free(p);
	}
	if(s != NULL)
		gtk_window_set_title(GTK_WINDOW(dialog), s);
	string_delete(s);
	hgroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	vgroup = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = dialog->vbox;
#endif
	/* filename */
	/* XXX the filename may be relative */
	p = g_strdup((hexeditor->filename != NULL) ? hexeditor->filename : "");
	if((q = g_filename_to_utf8(p, -1, NULL, NULL, &error)) == NULL)
	{
		_hexeditor_error(NULL, error->message, 1);
		g_error_free(error);
		q = p;
	}
	widget = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(widget), FALSE);
	gtk_entry_set_text(GTK_ENTRY(widget), q);
	gtk_size_group_add_widget(vgroup, widget);
	g_free(p);
	widget = _properties_widget(hexeditor, hgroup, _("Filename:"), widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	/* FIXME implement more properties */
	gtk_widget_show_all(vbox);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static GtkWidget * _properties_widget(HexEditor * hexeditor,
		GtkSizeGroup * group, char const * label, GtkWidget * value)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(label);
	gtk_widget_override_font(widget, hexeditor->bold);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), value, TRUE, TRUE, 0);
	return hbox;
}


/* hexeditor_unload */
int hexeditor_unload(HexEditor * hexeditor, char const * plugin)
{
	GtkTreeModel * model = GTK_TREE_MODEL(hexeditor->pl_store);
	GtkTreeIter iter;
	gboolean valid;
	gchar * p;
	Plugin * pp;
	HexEditorPluginDefinition * hepd;
	HexEditorPlugin * hep;
	GtkWidget * widget;

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, HEPC_NAME, &p,
				HEPC_PLUGIN, &pp,
				HEPC_HEXEDITORPLUGINDEFINITION, &hepd,
				HEPC_HEXEDITORPLUGIN, &hep,
				HEPC_WIDGET, &widget, -1);
		if(strcmp(plugin, p) == 0)
			break;
		g_free(p);
	}
	if(valid != TRUE)
		return 0;
	g_free(p);
	gtk_list_store_remove(hexeditor->pl_store, &iter);
	gtk_container_remove(GTK_CONTAINER(hexeditor->pl_box), widget);
	hepd->destroy(hep);
	plugin_delete(pp);
	if(gtk_tree_model_iter_n_children(model, NULL) == 0)
	{
		gtk_widget_set_no_show_all(hexeditor->pl_view, TRUE);
		gtk_widget_hide(hexeditor->pl_view);
	}
	else if(gtk_combo_box_get_active(GTK_COMBO_BOX(hexeditor->pl_combo))
			< 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(hexeditor->pl_combo), 0);
	return 0;
}


/* private */
/* functions */
/* accessors */
/* hexeditor_get_config_filename */
static String * _hexeditor_get_config_filename(char const * filename)
{
	char const * homedir;

	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	return string_new_append(homedir, "/", filename, NULL);
}


/* hexeditor_plugin_is_enabled */
static int _hexeditor_plugin_is_enabled(HexEditor * hexeditor,
		char const * plugin)
{
	GtkTreeModel * model = GTK_TREE_MODEL(hexeditor->pl_store);
	GtkTreeIter iter;
	gchar * p;
	gboolean valid;
	int res;

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, HEPC_NAME, &p, -1);
		res = strcmp(p, plugin);
		g_free(p);
		if(res == 0)
			return TRUE;
	}
	return FALSE;
}


/* useful */
/* hexeditor_close */
static void _close_reset(HexEditor * hexeditor);

static void _hexeditor_close(HexEditor * hexeditor, gboolean plugins)
{
	if(hexeditor->source != 0)
		g_source_remove(hexeditor->source);
	hexeditor->source = 0;
	hexeditor->offset = 0;
	hexeditor->size = 0;
	hexeditor->time = 0;
	gtk_text_buffer_set_text(hexeditor->view_addr_tbuf, "", 0);
	gtk_text_buffer_get_end_iter(hexeditor->view_addr_tbuf,
			&hexeditor->view_addr_iter);
	gtk_text_buffer_set_text(hexeditor->view_hex_tbuf, "", 0);
	gtk_text_buffer_get_end_iter(hexeditor->view_hex_tbuf,
			&hexeditor->view_hex_iter);
	gtk_text_buffer_set_text(hexeditor->view_data_tbuf, "", 0);
	gtk_text_buffer_get_end_iter(hexeditor->view_data_tbuf,
			&hexeditor->view_data_iter);
	if(hexeditor->channel != NULL)
	{
		g_io_channel_shutdown(hexeditor->channel, TRUE, NULL);
		g_io_channel_unref(hexeditor->channel);
		hexeditor->channel = NULL;
		hexeditor->fd = -1;
	}
	if(hexeditor->fd >= 0 && close(hexeditor->fd) != 0)
		_hexeditor_error(hexeditor, strerror(errno), 1);
	hexeditor->fd = -1;
	free(hexeditor->filename);
	hexeditor->filename = NULL;
	gtk_widget_hide(hexeditor->pg_window);
	gtk_widget_set_sensitive(hexeditor->view_addr, FALSE);
	gtk_widget_set_sensitive(hexeditor->view_hex, FALSE);
	gtk_widget_set_sensitive(hexeditor->view_data, FALSE);
	gtk_window_set_title(GTK_WINDOW(hexeditor->window),
			_("Hexadecimal editor"));
	if(plugins == TRUE)
		_close_reset(hexeditor);
}

static void _close_reset(HexEditor * hexeditor)
{
	GtkTreeModel * model = GTK_TREE_MODEL(hexeditor->pl_store);
	GtkTreeIter iter;
	gboolean valid;
	Plugin * plugin;
	HexEditorPluginDefinition * hepd;
	HexEditorPlugin * hep;
	GtkWidget * widget;

	/* reset every plug-in */
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;)
	{
		gtk_tree_model_get(model, &iter, HEPC_PLUGIN, &plugin,
				HEPC_HEXEDITORPLUGINDEFINITION, &hepd,
				HEPC_HEXEDITORPLUGIN, &hep, -1);
		hepd->destroy(hep);
		if((hep = hepd->init(&hexeditor->pl_helper)) == NULL)
		{
			gtk_list_store_remove(hexeditor->pl_store, &iter);
			continue;
		}
		widget = hepd->get_widget(hep);
		gtk_list_store_set(hexeditor->pl_store, &iter,
				HEPC_HEXEDITORPLUGIN, hep,
				HEPC_WIDGET, widget, -1);
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}


/* hexeditor_config_load */
static int _hexeditor_config_load(HexEditor * hexeditor)
{
	int ret;
	String * filename;
	String const * p;

	if(hexeditor->config == NULL)
		return -1; /* XXX report error */
	if((filename = _hexeditor_get_config_filename(HEXEDITOR_CONFIG_FILE))
			== NULL)
		return -1;
	if((ret = config_load(hexeditor->config, filename)) != 0)
		ret = -_hexeditor_error(NULL, error_get(NULL), 1);
	free(filename);
	/* uppercase */
	if((p = config_get(hexeditor->config, NULL, "uppercase")) != NULL)
		hexeditor->prefs.uppercase = (strtol(p, NULL, 10) > 0) ? 1 : 0;
	/* FIXME also import the font and plug-in values from here */
	return ret;
}


/* hexeditor_error */
static int _error_text(char const * message, int ret);

static int _hexeditor_error(HexEditor * hexeditor, char const * message,
		int ret)
{
#if !GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * dialog;
#endif

	if(hexeditor == NULL)
		return _error_text(message, ret);
#if GTK_CHECK_VERSION(2, 18, 0)
	gtk_label_set_text(GTK_LABEL(hexeditor->infobar_label), message);
	gtk_widget_show(hexeditor->infobar);
#else
	dialog = gtk_message_dialog_new(GTK_WINDOW(hexeditor->window),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
# if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
# endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
#endif
	return ret;
}

static int _error_text(char const * message, int ret)
{
	fprintf(stderr, "%s: %s\n", PROGNAME_HEXEDITOR, message);
	return ret;
}


/* callbacks */
/* hexeditor_on_open */
static void _hexeditor_on_open(gpointer data)
{
	HexEditor * hexeditor = data;

	hexeditor_open_dialog(hexeditor);
}


/* hexeditor_on_plugin_combo_change */
static void _hexeditor_on_plugin_combo_change(gpointer data)
{
	/* FIXME implement */
}


#ifdef EMBEDDED
/* hexeditor_on_preferences */
static void _hexeditor_on_preferences(gpointer data)
{
	HexEditor * hexeditor = data;

	hexeditor_show_preferences(hexeditor, TRUE);
}
#endif


/* hexeditor_on_progress_cancel */
static void _hexeditor_on_progress_cancel(gpointer data)
{
	HexEditor * hexeditor = data;

	hexeditor_close(hexeditor);
}


/* hexeditor_on_progress_delete */
static gboolean _hexeditor_on_progress_delete(gpointer data)
{
	HexEditor * hexeditor = data;

	gtk_widget_show(hexeditor->pg_window);
	return TRUE;
}


#ifdef EMBEDDED
/* hexeditor_on_properties */
static void _hexeditor_on_properties(gpointer data)
{
	HexEditor * hexeditor = data;

	hexeditor_show_properties(hexeditor, TRUE);
}
#endif
