/*
	about.c
	Copyright (C) 2008-2010 Axel Sommerfeldt (axel.sommerfeldt@f-m.fm)

	--------------------------------------------------------------------

	This file is part of the Leia application.

	Leia is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Leia is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Leia.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "leia-ds.h"

/*-----------------------------------------------------------------------------------------------*/

const gchar copyright[] = COPYRIGHT;
const gchar website[] = WEBSITE;

#if 0
const gchar *authors[] =
{
	"Axel Sommerfeldt <axel.sommerfeldt@f-m.fm>",
	NULL
};
#else
const gchar **authors = NULL;
#endif
const gchar **artists = NULL;
const gchar **documentors = NULL;

/*-----------------------------------------------------------------------------------------------*/

void About( GdkPixbuf *logo )
{
	GtkAboutDialog* dialog = GTK_ABOUT_DIALOG(gtk_about_dialog_new());

	gtk_about_dialog_set_logo( dialog, logo );
#if GTK_MAJOR_VERSION > 2 || GTK_MINOR_VERSION >= 12
	gtk_about_dialog_set_program_name( dialog, NAME );
#else
	gtk_about_dialog_set_name( dialog, NAME );
#endif
#if LOGLEVEL > 0
{
	gchar *version = g_strdup_printf( VERSION " " CODENAME " [loglevel=%d]", LOGLEVEL );
	gtk_about_dialog_set_version( dialog, version );
	g_free( version );
}
#else
	gtk_about_dialog_set_version( dialog, VERSION " " CODENAME );
#endif
	gtk_about_dialog_set_comments( dialog, Text(COMMENTS) );
	gtk_about_dialog_set_copyright( dialog, copyright );
	gtk_about_dialog_set_website( dialog, website );
	/*gtk_about_dialog_set_website_label( dialog, NAME " Homepage" );*/

#if !defined( LASTFM ) && !defined( RADIOTIME )
	gtk_about_dialog_set_license( dialog, Text(LICENSE) );
	gtk_about_dialog_set_wrap_license( dialog, TRUE );
#endif

	gtk_about_dialog_set_authors( dialog, authors );
	gtk_about_dialog_set_artists( dialog, artists );
	gtk_about_dialog_set_documenters( dialog, documentors );
	gtk_about_dialog_set_translator_credits( dialog, Text(TRANSLATOR_CREDITS) );

	gtk_dialog_run( GTK_DIALOG(dialog) );
	gtk_widget_destroy( GTK_WIDGET(dialog) );
}

/*-----------------------------------------------------------------------------------------------*/
