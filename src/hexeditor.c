/* $Id$ */
static char const _copyright[] =
"Copyright (c) 2013 Pierre Pronchery <khorben@defora.org>";
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



#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "hexeditor.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif


/* HexEditor */
/* private */
/* types */
struct _HexEditor
{
	int fd;
	GIOChannel * channel;
	guint source;
	gsize offset;

	/* widgets */
	GtkWidget * widget;
	GtkWidget * window;
	PangoFontDescription * bold;
#if GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * infobar;
	GtkWidget * infobar_label;
#endif
	GtkWidget * view_addr;
	GtkWidget * view_hex;
	GtkWidget * view_data;
	/* progress */
	GtkWidget * pg_window;
	GtkWidget * pg_progress;
};


/* prototypes */
static int _hexeditor_error(HexEditor * hexeditor, char const * message,
		int ret);

/* callbacks */
static void _hexeditor_on_open(gpointer data);
#ifdef EMBEDDED
static void _hexeditor_on_preferences(gpointer data);
#endif
static gboolean _hexeditor_on_progress_delete(gpointer data);
#ifdef EMBEDDED
static void _hexeditor_on_properties(gpointer data);
#endif


/* variables */
static DesktopToolbar _hexeditor_toolbar[] =
{
	{ "Open", G_CALLBACK(_hexeditor_on_open), GTK_STOCK_OPEN, 0, 0, NULL },
#ifdef EMBEDDED
	{ "", NULL, NULL, 0, 0, NULL },
	{ "Properties", G_CALLBACK(_hexeditor_on_properties),
		GTK_STOCK_PROPERTIES, GDK_MOD1_MASK, GDK_KEY_Return, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ "Preferences", G_CALLBACK(_hexeditor_on_preferences),
		GTK_STOCK_PREFERENCES, GDK_CONTROL_MASK, GDK_KEY_P, NULL },
#endif
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* hexeditor_new */
HexEditor * hexeditor_new(GtkWidget * window, GtkAccelGroup * group,
		char const * filename)
{
	HexEditor * hexeditor;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	GtkAdjustment * adjustment;

	if((hexeditor = object_new(sizeof(*hexeditor))) == NULL)
		return NULL;
	hexeditor->fd = -1;
	hexeditor->channel = NULL;
	hexeditor->source = 0;
	hexeditor->offset = 0;
	hexeditor->window = window;
	/* create the widget */
	hexeditor->bold = pango_font_description_new();
	pango_font_description_set_weight(hexeditor->bold, PANGO_WEIGHT_BOLD);
#if GTK_CHECK_VERSION(3, 0, 0)
	hexeditor->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	hexeditor->widget = gtk_vbox_new(FALSE, 0);
#endif
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
	hbox = gtk_hbox_new(FALSE, 0);
	/* view: address */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(
				widget));
	hexeditor->view_addr = gtk_text_view_new();
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
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(hexeditor->view_data),
			FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(hexeditor->view_data), FALSE);
	gtk_container_add(GTK_CONTAINER(widget), hexeditor->view_data);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	hexeditor_set_font(hexeditor, NULL);
	/* progress */
	hexeditor->pg_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(hexeditor->pg_window), 16);
	gtk_window_set_decorated(GTK_WINDOW(hexeditor->pg_window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(hexeditor->pg_window), 200, 50);
	gtk_window_set_modal(GTK_WINDOW(hexeditor->pg_window), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(hexeditor->pg_window),
			GTK_WINDOW(hexeditor->window));
	gtk_window_set_position(GTK_WINDOW(hexeditor->pg_window),
			GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect_swapped(hexeditor->pg_window, "delete-event",
			G_CALLBACK(_hexeditor_on_progress_delete), hexeditor);
	hbox = gtk_hbox_new(FALSE, 4);
	hexeditor->pg_progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(hbox), hexeditor->pg_progress, FALSE, TRUE,
			0);
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(hexeditor_close),
			hexeditor);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(hexeditor->pg_window), hbox);
	if(filename != NULL)
		hexeditor_open(hexeditor, filename);
	gtk_widget_show_all(vbox);
	return hexeditor;
}


/* hexeditor_delete */
void hexeditor_delete(HexEditor * hexeditor)
{
	hexeditor_close(hexeditor);
	pango_font_description_free(hexeditor->bold);
	object_delete(hexeditor);
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
	gtk_widget_modify_font(hexeditor->view_addr, desc);
	gtk_widget_modify_font(hexeditor->view_hex, desc);
	gtk_widget_modify_font(hexeditor->view_data, desc);
	pango_font_description_free(desc);
}


/* useful */
/* hexeditor_close */
void hexeditor_close(HexEditor * hexeditor)
{
	GtkTextBuffer * tbuf;

	if(hexeditor->source != 0)
		g_source_remove(hexeditor->source);
	hexeditor->source = 0;
	hexeditor->offset = 0;
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_addr));
	gtk_text_buffer_set_text(tbuf, "", 0);
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_hex));
	gtk_text_buffer_set_text(tbuf, "", 0);
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_data));
	gtk_text_buffer_set_text(tbuf, "", 0);
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
	gtk_widget_hide(hexeditor->pg_window);
}


/* hexeditor_open */
static gboolean _open_on_can_read(GIOChannel * channel, GIOCondition condition,
		gpointer data);
static gboolean _open_on_idle(gpointer data);
static void _open_read_1(HexEditor * hexeditor, char * buf, gsize pos);
static void _open_read_16(HexEditor * hexeditor, char * buf, gsize pos);

int hexeditor_open(HexEditor * hexeditor, char const * filename)
{
	hexeditor_close(hexeditor);
	if((hexeditor->fd = open(filename, O_RDONLY)) < 0)
		return -_hexeditor_error(hexeditor, strerror(errno), 1);
	hexeditor->channel = g_io_channel_unix_new(hexeditor->fd);
	g_io_channel_set_encoding(hexeditor->channel, NULL, NULL);
	hexeditor->source = g_io_add_watch(hexeditor->channel, G_IO_IN,
			_open_on_can_read, hexeditor);
	hexeditor->offset = 0;
	gtk_widget_show_all(hexeditor->pg_window);
	return 0;
}

static gboolean _open_on_can_read(GIOChannel * channel, GIOCondition condition,
		gpointer data)
{
	HexEditor * hexeditor = data;
	GIOStatus status;
	char buf[BUFSIZ];
	gsize size;
	GError * error = NULL;
	gsize i;

	if(channel != hexeditor->channel || condition != G_IO_IN)
		return FALSE;
	status = g_io_channel_read_chars(channel, buf, sizeof(buf), &size,
			&error);
	if(status == G_IO_STATUS_AGAIN)
		/* this status can be ignored */
		return TRUE;
	if(status == G_IO_STATUS_ERROR)
	{
		hexeditor_close(hexeditor);
		_hexeditor_error(hexeditor, error->message, 1);
		g_error_free(error);
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
	hexeditor->offset += i;
	if(status != G_IO_STATUS_NORMAL)
	{
		hexeditor->source = 0;
		gtk_widget_hide(hexeditor->pg_window);
		return FALSE;
	}
	/* FIXME do not pulse so often */
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(hexeditor->pg_progress));
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

static void _open_read_1(HexEditor * hexeditor, char * buf, gsize pos)
{
	char buf2[16];
	GtkTextBuffer * taddr;
	GtkTextBuffer * thex;
	GtkTextBuffer * tdata;
	GtkTextIter iter;
	int c;

	taddr = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_addr));
	thex = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_hex));
	tdata = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_data));
	c = (unsigned char)buf[pos];
	if(((hexeditor->offset + pos) % 16) == 0)
	{
		/* address */
		snprintf(buf2, sizeof(buf2), "%s%08x",
				(hexeditor->offset + pos) ? "\n" : "",
				(unsigned int)(hexeditor->offset + pos));
		gtk_text_buffer_get_end_iter(taddr, &iter);
		gtk_text_buffer_insert(taddr, &iter, buf2, -1);
		/* hexadecimal value */
		snprintf(buf2, sizeof(buf2), "%s%02x",
				(hexeditor->offset + pos) ? "\n" : "", c);
		gtk_text_buffer_get_end_iter(thex, &iter);
		gtk_text_buffer_insert(thex, &iter, buf2, -1);
		if(hexeditor->offset + pos != 0)
		{
			/* character value */
			gtk_text_buffer_get_end_iter(tdata, &iter);
			gtk_text_buffer_insert(tdata, &iter, "\n", 1);
		}
	}
	else
	{
		/* hexadecimal value */
		snprintf(buf2, sizeof(buf2), " %02x", c);
		gtk_text_buffer_get_end_iter(thex, &iter);
		gtk_text_buffer_insert(thex, &iter, buf2, -1);
	}
	/* character value */
	snprintf(buf2, sizeof(buf2), "%c", isascii(c) && isprint(c) ? c : '.');
	gtk_text_buffer_get_end_iter(tdata, &iter);
	gtk_text_buffer_insert(tdata, &iter, buf2, -1);
}

static void _open_read_16(HexEditor * hexeditor, char * buf, gsize pos)
{
	GtkTextBuffer * taddr;
	GtkTextBuffer * thex;
	GtkTextBuffer * tdata;
	GtkTextIter iter;
	unsigned char c[16];
	int i;
	char buf2[64];

	taddr = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_addr));
	thex = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_hex));
	tdata = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexeditor->view_data));
	/* address */
	gtk_text_buffer_get_end_iter(taddr, &iter);
	snprintf(buf2, sizeof(buf2), "%s%08x",
			(hexeditor->offset + pos) ? "\n" : "",
			(unsigned int)(hexeditor->offset + pos));
	gtk_text_buffer_insert(taddr, &iter, buf2, -1);
	/* hexadecimal values */
	for(i = 0; i < 16; i++)
		c[i] = (unsigned char)buf[pos + i];
	gtk_text_buffer_get_end_iter(thex, &iter);
	snprintf(buf2, sizeof(buf2), "%s%02x %02x %02x %02x %02x %02x %02x %02x"
			" %02x %02x %02x %02x %02x %02x %02x %02x",
			(hexeditor->offset + pos) ? "\n" : "",
			c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
			c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15]);
	gtk_text_buffer_insert(thex, &iter, buf2, -1);
	/* character values */
	if(hexeditor->offset + pos != 0)
	{
		gtk_text_buffer_get_end_iter(tdata, &iter);
		gtk_text_buffer_insert(tdata, &iter, "\n", 1);
	}
	gtk_text_buffer_get_end_iter(tdata, &iter);
	for(i = 0; i < 16; i++)
		if(!isascii(c[i]) || !isprint(c[i]))
			c[i] = '.';
	gtk_text_buffer_insert(tdata, &iter, (char const *)c, 16);
}


/* hexeditor_show_preferences */
void hexeditor_show_preferences(HexEditor * hexeditor, gboolean show)
{
	/* FIXME implement */
}


/* hexeditor_show_properties */
void hexeditor_show_properties(HexEditor * hexeditor, gboolean show)
{
	GtkWidget * dialog;
	GtkWidget * vbox;

	if(show == FALSE)
		/* XXX should really hide the window */
		return;
	dialog = gtk_dialog_new_with_buttons(_("Properties"),
			GTK_WINDOW(hexeditor->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 200);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = dialog->vbox;
#endif
	/* FIXME really implement */
	gtk_widget_show_all(vbox);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* private */
/* functions */
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
	fprintf(stderr, "%s: %s\n", PACKAGE, message);
	return ret;
}


/* callbacks */
/* hexeditor_on_open */
static void _hexeditor_on_open(gpointer data)
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
