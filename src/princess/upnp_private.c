/*
	princess/upnp_private.c
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

#include "upnp.h"
#include "upnp_private.h"

/*=== Dynamic buffer management =================================================================*/

struct buffer *buffer_list;
struct _GMutex *buffer_mutex;

/* Initialize a dynamic buffer */
struct buffer *alloc_buffer( size_t initial_size, size_t grow_by, const char *debug_info, GError **error )
{
	struct buffer *buffer;
	char *addr;

	if ( grow_by > 0 )
	{
		if ( initial_size != 0 )
		{
			size_t m = initial_size % grow_by;
			if ( m != 0 ) initial_size += (grow_by - initial_size);
		}
		else
			initial_size = grow_by;
	}
	else if ( initial_size == 0 )
	{
		initial_size = 64 * 1024;  /* 64k */
	}

	addr = g_try_malloc( initial_size );
	upnp_debug(( "alloc_buffer( \"%s\" ): malloc( %lu ) = %p\n", debug_info, (unsigned long)initial_size, addr ));

	if ( addr == NULL )
	{
		upnp_warning(( "alloc_buffer( \"%s\" ): malloc( %lu ) failed\n", debug_info, (unsigned long)initial_size ));
		g_set_error( error, UPNP_ERROR, UPNP_ERR_OUT_OF_MEMORY, "Out of memory" );
		return NULL;
	}

	buffer = new_buffer( addr, initial_size, 0, grow_by, debug_info );
	upnp_assert( buffer != NULL );
	reset_buffer( buffer );

	return buffer;
}

struct buffer *new_buffer( char *addr, size_t size, size_t len, size_t grow_by, const char *debug_info )
{
	struct buffer *buffer = g_malloc( sizeof( struct buffer ) );
	upnp_assert( buffer != NULL );

	upnp_assert( addr != NULL );
	buffer->addr = addr;
	buffer->size = size;
	buffer->len = len;
	buffer->grow_by = grow_by;

	buffer->refcount = 1;
#if LOGLEVEL
	buffer->debug_info = debug_info;
#endif

	g_mutex_lock( buffer_mutex );
	buffer->next = buffer_list;
	buffer_list = buffer;
	g_mutex_unlock( buffer_mutex );

	return buffer;
}

/* Reset contents of a dynamic buffer */
void reset_buffer( struct buffer *buffer )
{
	upnp_assert( buffer != NULL && buffer->addr != NULL );

	buffer->addr[buffer->len = 0] = '\0';
}

/* Enlarge a dynamic buffer */
gboolean enlarge_buffer( struct buffer *buffer, GError **error )
{
	size_t new_size;

	upnp_assert( buffer != NULL && buffer->addr != NULL );

	new_size = buffer->size;
	if ( buffer->grow_by > 0 )
		new_size += buffer->grow_by;
	else
		new_size *= 2;

	return resize_buffer( buffer, new_size, error );
}

/* Resize a dynamic buffer */
gboolean resize_buffer( struct buffer *buffer, size_t new_size, GError **error )
{
	char *new_addr;

	upnp_assert( buffer != NULL && buffer->addr != NULL );
	upnp_assert( new_size > 0 );

	new_addr = g_try_realloc( buffer->addr, new_size );
	upnp_message(( "resize_buffer( \"%s\" ): g_try_realloc( %p, %lu -> %lu ) = %p\n",
			buffer->debug_info, buffer->addr, (unsigned long)buffer->size, (unsigned long)new_size, new_addr ));

	if ( new_addr == NULL )
	{
		upnp_warning(( "resize_buffer( \"%s\" ): g_try_realloc( %p, %lu -> %lu ) failed\n",
			buffer->debug_info, buffer->addr, (unsigned long)buffer->size, (unsigned long)new_size ));
		g_set_error( error, UPNP_ERROR, UPNP_ERR_OUT_OF_MEMORY, "Out of memory" );
		return FALSE;
	}

	buffer->addr = new_addr;
	buffer->size = new_size;
	return TRUE;
}

/* Increment the reference counter */
void add_buffer_ref( struct buffer *buffer )
{
	upnp_assert( buffer != NULL );
	upnp_message(( "add_buffer_ref( \"%s\", %d )\n", buffer->debug_info, buffer->refcount ));
	upnp_assert( buffer->refcount > 0 );

	g_atomic_int_inc( &buffer->refcount );
}

/* Free a dynamic buffer */
void release_buffer( struct buffer *buffer )
{
	if ( buffer == NULL ) return;

	if ( buffer_mutex == NULL )
	{
		upnp_warning(( "******* release_buffer(): buffer_mutex == NULL\n" ));
		return;
	}

	upnp_debug(( "release_buffer( \"%s\", %d )\n", buffer->debug_info, buffer->refcount ));

	upnp_assert( buffer->refcount > 0 );
	if ( buffer->refcount <= 0 ) return;

	if ( g_atomic_int_dec_and_test( &buffer->refcount ) )
	{
		g_mutex_lock( buffer_mutex );
		if ( buffer_list == buffer )
		{
			buffer_list = buffer->next;
		}
		else
		{
			struct buffer *buf;

			for ( buf = buffer_list; buf != NULL; buf = buf->next )
			{
				if ( buf->next == buffer )
				{
					buf->next = buffer->next;
					break;
				}
			}
			upnp_assert( buf != NULL );
		}
		g_mutex_unlock( buffer_mutex );

		upnp_debug(( "release_buffer( \"%s\" ): free( %p )\n", buffer->debug_info, buffer->addr ));
		g_free( buffer->addr );
		buffer->addr = NULL;

		g_free( buffer );
	}
}

/* Append a character to a dynamic buffer */
gboolean append_char_to_buffer( struct buffer *buffer, char c )
{
	upnp_assert( buffer != NULL && buffer->addr != NULL );
	upnp_assert( buffer->len < buffer->size );

	if ( buffer->len == buffer->size - 1 )
	{
		if ( !enlarge_buffer( buffer, NULL /*error*/ ) )
			return FALSE;
	}

	buffer->addr[buffer->len++] = c;
	buffer->addr[buffer->len] = '\0';
	return TRUE;
}

/* Append a string to a dynamic buffer */
gboolean append_string_to_buffer( struct buffer *buffer, const char *string )
{
	upnp_assert( buffer != NULL && buffer->addr != NULL );

	while ( *string != '\0' )
	{
		if ( !append_char_to_buffer( buffer, *string++ ) )
			return FALSE;
	}

	return TRUE;
}

#if 0  /* not used yet */
gboolean append_string_n_to_buffer( struct buffer *buffer, const char *string, int n )
{
	upnp_assert( buffer != NULL && buffer->addr != NULL );

	while ( *string != '\0' && --n >= 0 )
	{
		if ( !append_char_to_buffer( buffer, *string++ ) )
			return FALSE;
	}
	return TRUE;
}
#endif

/*===============================================================================================*/
