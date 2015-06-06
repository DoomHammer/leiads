/*
	debug.c
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

#if LOGLEVEL > 0

#ifdef WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN  /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#endif

GMutex *log_mutex;
gchar *log_filename;
FILE *log_file;

static void print( const gchar *prefix, const gchar *string )
{
	g_mutex_lock( log_mutex );

#ifdef WIN32
	OutputDebugString( prefix );
	OutputDebugString( string );
#else
	fputs( prefix, stdout );
	fputs( string, stdout );
	/*fflush( stdout );*/
#endif

	if ( log_file != NULL )
	{
		fputs( prefix, log_file );
		fputs( string, log_file );
		fflush( log_file );
	}

	g_mutex_unlock( log_mutex );
}

static void print_handler( const gchar *string )
{
	print( "", string );
}

static void printwarn_handler( const gchar *string )
{
	print( "*** ", string );
}

static void printerr_handler( const gchar *string )
{
	print( "******* ", string );
}

static void log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data )
{
	if ( (log_level & G_LOG_FLAG_FATAL) != 0 )
		printerr_handler( "FATAL:\n" );

	if ( (log_level & (G_LOG_LEVEL_ERROR|G_LOG_LEVEL_CRITICAL)) != 0 )
		printerr_handler( message );
	else if ( (log_level & G_LOG_LEVEL_WARNING) != 0 )
		printwarn_handler( message );
	else
		print_handler( message );
}

gchar *build_logfilename( const char *basename, const char *extension )
{
	const gchar *my_docs_dir;

#if defined( WIN32 )
	my_docs_dir = g_get_user_data_dir();
#elif defined( MAEMO )
	my_docs_dir = g_getenv( "MYDOCSDIR" );
#else
	my_docs_dir = g_get_home_dir();
#endif

	if ( my_docs_dir != NULL )
	{
		char filename[64 /*sizeof("leiads-radiotime_2009-09-16-15-09.logx")+1*/];
		time_t timer = time( NULL );
		size_t i;

		i = strlen( strcpy( filename, basename ) );
		strftime( filename + i, sizeof( filename ) - i - strlen( extension ),
			"_%Y-%m-%d-%H-%M.", localtime( &timer ) );
		strcat( filename, extension );

		return g_build_filename( my_docs_dir, filename, NULL );
	}

	return NULL;
}

static void open_logfile( void )
{
#ifndef NOLOGFILE

	log_filename = build_logfilename( "leiads", "log" );
	if ( log_filename != NULL )
	{
		log_file = fopen_utf8( log_filename, "w" );
		if ( log_file != NULL )
		{
#ifdef WIN32
			OutputDebugString( NAME " log file: " );
			OutputDebugString( log_filename );
			OutputDebugString( "\n" );
#else
			printf( NAME " log file: %s\n", log_filename );
#endif
		}
	}

#endif
}

static void close_logfile( void )
{
#ifndef NOLOGFILE

	if ( log_file != NULL )
	{
		fclose( log_file );
		log_file = NULL;
	}
	if ( log_filename != NULL )
	{
		/*g_remove( log_filename );*/
		g_free( log_filename );
		log_filename = NULL;
	}

#endif
}

/*-----------------------------------------------------------------------------------------------*/

void debug_init()
{
	log_mutex = g_mutex_new();

	g_set_print_handler( print_handler );
	g_set_printerr_handler( printerr_handler );

	g_log_set_handler( G_LOG_DOMAIN, (1 << G_LOG_LEVEL_USER_SHIFT)-1, log_handler, NULL );
	g_log_set_handler( UPNP_LOG_DOMAIN, (1 << G_LOG_LEVEL_USER_SHIFT)-1, log_handler, NULL );
	g_log_set_handler( NULL, (1 << G_LOG_LEVEL_USER_SHIFT)-1, log_handler, NULL );

	open_logfile();
}

void debug_exit()
{
	close_logfile();

	g_mutex_free( log_mutex );
	log_mutex = NULL;
}

/*-----------------------------------------------------------------------------------------------*/
#endif
