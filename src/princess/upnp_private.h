/*
	princess/private.h
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

#include <stdio.h>   /* for sprintf() */
#include <string.h>

#ifdef WIN32
#define STRICT
  #define WIN32_LEAN_AND_MEAN  /* Exclude rarely-used stuff from Windows headers */
  #include <windows.h>
  #define WIN32_WINSOCK2
#else
  #include <strings.h>
  #define stricmp strcasecmp
  #define strnicmp strncasecmp
#endif

#define UPNP_MAX_URL  256

/*--- Dynamic buffer management -----------------------------------------------------------------*/

extern struct buffer
{
	struct buffer *next;
	int refcount;
#if LOGLEVEL > 0
	const char *debug_info;
#endif

	char *addr;
	size_t size, len, grow_by;
} *buffer_list;

extern struct _GMutex *buffer_mutex;

struct buffer *alloc_buffer( size_t initial_size, size_t grow_by, const char *debug_info, GError **error );
struct buffer *new_buffer( char *addr, size_t size, size_t len, size_t grow_by, const char *debug_info );
void reset_buffer( struct buffer *buffer );
gboolean enlarge_buffer( struct buffer *buffer, GError **error );
gboolean resize_buffer( struct buffer *buffer, size_t new_size, GError **error );
void add_buffer_ref( struct buffer *buffer );
void release_buffer( struct buffer *buffer );

gboolean append_char_to_buffer( struct buffer *buffer, char c );
gboolean append_string_to_buffer( struct buffer *buffer, const char *string );
/*gboolean append_string_n_to_buffer( struct buffer *buffer, const char *string, int n );*/

/*--- SOAP --------------------------------------------------------------------------------------*/

#define get_action_buffer( outv ) ((struct buffer*)outv->value)

/*--- XML ---------------------------------------------------------------------------------------*/

void xml_wrap_delimiters_to_buffer( struct buffer *dest, const char *src );
void xml_wrap_delimiters_to_string( struct _GString *dest, const char *src );
char *xml_unwrap_delimiters_in_place( char *buf );

/* Test if the given character is a space character (2.3) */
/*#define xml_is_space( c ) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')*/
#define xml_is_space( c ) ((c) != '\0' && ((unsigned char)(c)) <= ' ')

/*--- Debug stuff -------------------------------------------------------------------------------*/

#if LOGLEVEL > 0

  #if LOGLEVEL > 2
    #define upnp_debug( a )    g_debug    a
  #else
    #define upnp_debug( a )
  #endif
  #if LOGLEVEL > 1
    #define upnp_message( a )  g_message  a
  #else
    #define upnp_message( a )
  #endif
  #define upnp_warning( a )    g_warning  a
  #define upnp_assert( a )     g_assert( a )

#else

  #define upnp_debug( a )
  #define upnp_message( a )
  #define upnp_warning( a )
  #define upnp_assert( a )

#endif

#define upnp_critical( a )     g_critical a
#define upnp_error( a )        g_error    a

#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN UPNP_LOG_DOMAIN
#include <glib.h>
#endif

/*--- EOF ---------------------------------------------------------------------------------------*/
