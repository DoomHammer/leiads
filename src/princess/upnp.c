/*
	princess/upnp.c
	Copyright (C) 2008-2011 Axel Sommerfeldt (axel.sommerfeldt@f-m.fm)

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

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  #define WSA_VERSION MAKEWORD( 2, 2 )
  typedef int socklen_t;
  #define IS_VALID_SOCKET(s) ((s) != INVALID_SOCKET)
#else
  #include <sys/types.h>
/*#include <sys/socket.h>*/
  #include <sys/ioctl.h>   /* for ioctl() */
  #include <sys/select.h>  /* for select() */
  #include <sys/unistd.h>  /* for close() */
  #include <netinet/tcp.h> /* for TCP_NODELAY */
  #include <arpa/inet.h>   /* for inet_addr() */
  #include <netdb.h>       /* for gethostbyname() */
  #include <signal.h>      /* for sigaction() */
  #include <errno.h>       /* for errno */
  typedef int SOCKET;
  #define INVALID_SOCKET -1
  #define IS_VALID_SOCKET(s) ((s) >= 0)
  #define closesocket close
  #define ioctlsocket ioctl
  #define WSAGetLastError() errno
#endif

#ifndef SD_BOTH
/*
 * WinSock 2 extension -- manifest constants for shutdown()
 */
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02
#endif

#define UPNP_RECV_TIMEOUT_1ST    30000  /* ms */
#define UPNP_RECV_TIMEOUT_NEXT   20000  /* ms */
#define UPNP_RECV_SEND_TIMEOUT   10000  /* ms, 2.5s are not sufficient here */

/* Note: The value of UPNP_RECV_SEND_TIMEOUT is a little bit critical.
   Tests have shown that 2.5s are not sufficient, but some (older) Bute
   firmware (e.g. SneakyMusicDs_1.1-0.4) hangs until a timeout occurs
   so this value shouldn't be too large either.
   Having a timeout of 10s seems to be a good compromise. */

#define HTTP_VERSION             "1.1"  /* used by http_get(), SOAP & GENA */
#define HTTP_SERVER              "Princess Leia/0.6"
#define HTTP_MAX_HEADER_SIZE     1024
#define HTTP_MAX_CONNECTIONS     9

#define SSDP_BUFFER_SIZE          (8*1024)
#define DESCRIPTION_BUFFER_SIZE  (64*1024)
#define HTTP_BUFFER_SIZE         (20*1024)
#define SOAP_BUFFER_SIZE         (64*1024)
#define GENA_BUFFER_SIZE          (8*1024)

#define SOCKET_ERROR_VALUE       (-999 << 16)

/*--- static helpers ----------------------------------------------------------------------------*/

/* Copies a string until '\0', '\r' or '\n' */
static char *upnp_strcpy( const char *what, char *dest, int sizeof_dest, const char *src )
{
	int destlen = 0;

	upnp_assert( src != NULL && dest != NULL && sizeof_dest > 0 );
	sizeof_dest--;

	for (;;)
	{
		char ch = *src++;
		if ( ch == '\0' || ch == '\r' || ch == '\n' ) break;

		if ( destlen >= sizeof_dest )
		{
			upnp_warning(( "upnp_strcpy( \"%s\" ): %d >= %d\n", what, destlen, sizeof_dest ));
			upnp_assert( destlen < sizeof_dest );
			break;
		}

		dest[destlen++] = ch;
	}
	dest[destlen] = '\0';
	return dest;
}


/*** Sockets ***/

/* Allocate a new socket */
static SOCKET new_socket( int type )
{
	SOCKET s = socket( AF_INET, type, 0 );
	upnp_assert( IS_VALID_SOCKET( s ) );
	if ( type == SOCK_STREAM )
	{
		int flag = 1;
		setsockopt( s, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag) );
	}

	return s;
}

/* Initialize a sockaddr_in structure */
static void init_sockaddr( struct sockaddr_in *addr )
{
	memset( addr, 0, sizeof( struct sockaddr_in ) );
	addr->sin_family = AF_INET;
	/*addr->sin_port = htons( 0 );*/
	/*addr->sin_addr.s_addr = htonl( INADDR_ANY );*/
}

/* Set a sockaddr_in structure to a specific address */
/*static void set_sockaddr( struct sockaddr_in *addr, u_long sin_addr, u_short sin_port )
{
	init_sockaddr( addr );
	addr->sin_port = htons( sin_port );
	addr->sin_addr.s_addr = htonl( sin_addr );
}*/
static int set_sockaddr( struct sockaddr_in *addr, const char *sin_addr, u_short sin_port )
{
	init_sockaddr( addr );
	addr->sin_port = htons( sin_port );
	addr->sin_addr.s_addr = inet_addr( sin_addr );
	if ( addr->sin_addr.s_addr == 0xFFFFFFFFUL )
	{
		struct hostent *hp;

		upnp_message(( "Resolving %s...\n", sin_addr ));

		hp = gethostbyname( sin_addr );
		if ( hp != NULL )
		{
			u_long hl_addr;

			addr->sin_family = hp->h_addrtype;
			addr->sin_addr = *(struct in_addr*)hp->h_addr_list[0];
			/* host_name = hp->h_name; */

			hl_addr = ntohl( addr->sin_addr.s_addr );
			upnp_message(( "%s = %d.%d.%d.%d\n", hp->h_name,
				(int)(hl_addr >> 24) & 0xFF, (int)(hl_addr >> 16) & 0xFF,
				(int)(hl_addr >> 8) & 0xFF, (int)hl_addr & 0xFF ));
		}
		else
		{
			upnp_warning(( "%s: Unknown host.\n", sin_addr ));
			return -1;
		}
	}

	return 0;
}

/* Get a sockaddr_in structure as string */
static char *get_sockaddr( char dest[/*UPNP_MAX_HOST_ADDRESS*/], const struct sockaddr_in *addr )
{
	u_long hl_addr = ntohl( addr->sin_addr.s_addr );
	sprintf( dest, "%d.%d.%d.%d",
		(int)(hl_addr >> 24) & 0xFF, (int)(hl_addr >> 16) & 0xFF,
		(int)(hl_addr >> 8) & 0xFF, (int)hl_addr & 0xFF );
	return dest;
}

/* Wait for incoming data (with timeout in ms) */
static int socket_select_read( SOCKET s, long tv_msec )
{
	struct timeval to;
	fd_set rread;
	int err;

	FD_ZERO( &rread );
	FD_SET( s, &rread );

	to.tv_sec = tv_msec / 1000;
	to.tv_usec = (tv_msec % 1000) * 1000;

	err = select( s + 1, &rread, 0, 0, &to );
	/*upnp_debug(( "select() returned %d\n", err ));*/
	if ( err > 0 ) upnp_assert( FD_ISSET( s, &rread ) );
	return err;
}

static gboolean shutdown_socket( SOCKET *socket, int how )
{
	SOCKET s = *socket;
	*socket = INVALID_SOCKET;
	upnp_assert( !IS_VALID_SOCKET( *socket ) );

	upnp_message(( "shutdown_socket(): socket = %d\n", (int)s ));
	if ( IS_VALID_SOCKET( s ) )
	{
		upnp_message(( "shutdown_socket(): closesocket( %d )\n", (int)s ));
		shutdown( s, how );
		closesocket( s );
		return TRUE;
	}

	return FALSE;
}

static gboolean close_socket( SOCKET *socket )
{
	SOCKET s = *socket;
	*socket = INVALID_SOCKET;
	upnp_assert( !IS_VALID_SOCKET( *socket ) );

	upnp_message(( "close_socket(): socket = %d\n", (int)s ));
	if ( IS_VALID_SOCKET( s ) )
	{
		upnp_message(( "close_socket(): closesocket( %d )\n", (int)s ));
		closesocket( s );
		return TRUE;
	}

	return FALSE;
}

static int socket_error( void )
{
	gint code = (SOCKET_ERROR_VALUE|(WSAGetLastError() & 0xFFFF));
	upnp_assert( code < 0 );
	return code;
}

static int set_socket_error( GError **err, const gchar *format, const struct sockaddr_in *addr )
{
	gint code = socket_error();

	if ( err != NULL && addr != NULL )
	{
		char buf[UPNP_MAX_HOST_ADDRESS];
		gchar *message;

		message = g_strdup_printf( format, get_sockaddr( buf, addr ) );
		upnp_message(( "*** %d:%s\n", code, message ));
		g_set_error( err, UPNP_ERROR, code, message );
		g_free( message );
	}
	else
	{
		upnp_message(( "*** %d:%s\n", code, format ));
		g_set_error( err, UPNP_ERROR, code, format );
	}

	return code;
}


/*** HTTP (Part 1: attributes) ***/

#define INVALID_HTTP_RESULT 999

static gint set_upnp_error( GError **err, gint code, const gchar *message )
{
	register gint _code = code;
	if ( (_code & ~0xFFFF) == SOCKET_ERROR_VALUE ) _code &= 0xFFFF;

	upnp_message(( "*** %d:%s\n", _code, message ));
	g_set_error( err, UPNP_ERROR, _code, message );

	return code;
}

/* Returns the HTTP result code (or 999 if not valid) */
static int get_http_result( const struct buffer *buffer, GError **error )
{
	char text[64], *s;
	int result = 0;

	upnp_assert( buffer != NULL && buffer->addr != NULL );

	if ( strncmp( buffer->addr, "HTTP/1.", 7 ) == 0 && buffer->addr[7] != '\0' && xml_is_space( buffer->addr[8] ) )
	{
		result = (int)strtoul( buffer->addr + 9, &s, 10 );
		while ( xml_is_space( *s ) ) s++;
		upnp_strcpy( "HTTP result", text, sizeof( text ), s );
	}
	else
	{
		strcpy( text, "Invalid HTTP format" );
	}

	upnp_debug(( "HTTP result = %d \"%s\"\n", result, text ));

	if ( result >= 200 && result < 300 )
	{
		result = 0;
	}
	else
	{
		if ( result == 0 ) result = INVALID_HTTP_RESULT;  /* Invalid HTTP result */
		set_upnp_error( error, result, text );
	}

	return result;
}

/* Get the value of a HTTP header field (or NULL if not found) */
static char *get_http_header_field( const struct buffer *buffer, const char *item )
{
	upnp_assert( item != NULL );

	if ( buffer != NULL && buffer->addr != NULL && buffer->len > 0 )
	{
		size_t itemlen = strlen( item );
		char *s = buffer->addr;

		for (;;)
		{
			/* Goto next header line */
			for (;;)
			{
				char ch = *s++;

				if ( ch == '\r' || ch == '\0' )  /* End of line */
				{
					if ( *s == '\n' ) s++;
					break;
				}

				if ( ch == '\n' ) break;  /* see RFC 2616: 19.3 Tolerant Applications */
			}

			upnp_assert( s < buffer->addr + buffer->len );
			if ( *s == '\r' || *s == '\n' ) break;  /* End of HTTP header */
			upnp_assert( *s != '\0' );

			if ( strnicmp( s, item, itemlen ) == 0 && s[itemlen] == ':' )
			{
				char *result;

				/* Item found */
				s += itemlen + 1;
				while ( *s == ' ' || *s == '\t' ) s++;
				result = s;
				while ( *s != '\0' && *s != '\r' && *s != '\n' ) s++;
				*s = '\0';

				upnp_debug(( "%s = \"%s\"\n", item, result ));
				return result;
			}
		}
	}

	upnp_debug(( "%s = <NULL>\n", item ));
	return NULL;
}

/* Returns the pointer to the (received) HTTP content (or NULL if none) */
static char *get_http_content( const struct buffer *buffer )
{
	return (buffer != NULL && buffer->addr != NULL && buffer->len > 0)
		? buffer->addr + buffer->len : NULL;
}

/*** Sending & Receiving ***/

/* Must be called prior upnp_send() */
static void prepare_for_upnp_send( struct buffer *buffer )
{
	/* We don't use a separate buffer here but simply reserve
	   MAX_HTTP_HEADER bytes for the HTTP header */
	upnp_assert( buffer->addr != NULL && buffer->size > HTTP_MAX_HEADER_SIZE );
	buffer->addr[buffer->len = HTTP_MAX_HEADER_SIZE] = '\0';
}

/* Return the length of the HTTP content */
static unsigned long upnp_content_length( struct buffer *buffer )
{
	upnp_assert( buffer != NULL && buffer->len >= HTTP_MAX_HEADER_SIZE );
	upnp_assert( strlen( buffer->addr + HTTP_MAX_HEADER_SIZE ) == buffer->len - HTTP_MAX_HEADER_SIZE );
	return buffer->len - HTTP_MAX_HEADER_SIZE;
}

/* Send bytes */
static int _upnp_send( SOCKET s, const char *buf, size_t len )
{
	while ( len > 0 )
	{
		int err = send( s, buf, len, 0 );
		if ( err == 0 )
		{
			upnp_warning(( "_upnp_send(): send(%lu) = %d\n", (unsigned long)len, err ));
			return UPNP_ERR_SOCKET_CLOSED;
		}
		if ( err < 0 )
		{
			err = socket_error();
			upnp_warning(( "_upnp_send(): send(%lu) = %d\n", (unsigned long)len, err ));
			return err;
		}

		buf += err;
		len -= err;
	}

	return 0;
}

/* Send HTTP header + content (TCP) */
static int upnp_send( SOCKET s, struct buffer *buffer )
{
	upnp_assert( buffer != NULL && buffer->addr != NULL );

#if LOGLEVEL >= 5
	upnp_debug(( "upnp_send():\n%s", buffer->addr ));
#endif

	/* append content to header */
	upnp_assert( strlen( buffer->addr ) < HTTP_MAX_HEADER_SIZE );
	upnp_assert( buffer->len >= HTTP_MAX_HEADER_SIZE );

#if LOGLEVEL >= 6
	upnp_debug(( "%s\n", buffer->addr + HTTP_MAX_HEADER_SIZE ));
#endif
	strcat( buffer->addr, buffer->addr + HTTP_MAX_HEADER_SIZE );

	return _upnp_send( s, buffer->addr, strlen( buffer->addr ) );
}

/* Send HTTP header (UDP) */
static int upnp_sendto( SOCKET s, struct buffer *buffer, const struct sockaddr_in *to )
{
	const char *buf;
	size_t len;

	upnp_assert( buffer != NULL && buffer->addr != NULL );

	buf = buffer->addr;
	len = strlen( buf );
#if LOGLEVEL >= 5
	upnp_debug(( "upnp_sendto():\n%s\n", buf ));
#endif

	while ( len > 0 )
	{
		int err = sendto( s, buf, len, 0, (const struct sockaddr*)to, sizeof( struct sockaddr_in ) );
		if ( err == 0 ) return UPNP_ERR_SOCKET_CLOSED;
		if ( err < 0 )
		{
			err = socket_error();
			upnp_warning(( "upnp_sendto(): sendto(%lu) = %d\n", (unsigned long)len, err ));
			return err;
		}

		buf += err;
		len -= err;
	}

	return 0;
}

/* Receive bytes until the socket gets closed (TCP) (Timeout = UPNP_RECV_TIMEOUT) */
static int _upnp_recv( SOCKET s, char *buf, int size, long tv_msec )
{
	int err;

	if ( tv_msec > 0 )
	{
		err = socket_select_read( s, tv_msec );  /* Error (<0) or Timeout(==0) */
		if ( err == 0 ) return UPNP_ERR_TIMEOUT;
		if ( err < 0 )
		{
			err = socket_error();
			upnp_warning(( "_upnp_recv(): socket_select_read() = %d\n", err ));
			return err;
		}
	}

	upnp_assert( buf != NULL && size > 1 );

	err = recv( s, buf, size - 1, 0 );
	if ( err == 0 ) return UPNP_ERR_SOCKET_CLOSED;
	if ( err < 0 )
	{
		err = socket_error();
		upnp_warning(( "_upnp_recv(): recv(%d) = %d\n", size-1, err ));
		return err;
	}

	upnp_assert( err < size );
	buf[err] = '\0';
	return err;
}

/* Receive HTTP header */
static int upnp_recv_header( SOCKET s, struct buffer *buffer )
{
	long tv_msec = UPNP_RECV_TIMEOUT_1ST;
	const char *content;
	size_t offset = 0;

	upnp_assert( buffer != NULL && buffer->addr != NULL && buffer->size > 0 );

	buffer->len = 0;  /* we have no valid HTTP header so far */

	for (;;)
	{
		int err = _upnp_recv( s, buffer->addr + offset, buffer->size - offset, tv_msec );
		if ( err < 0 )
		{
			upnp_warning(( "upnp_recv_header(): _upnp_recv() = %d\n", err ));
			return err;
		}

		tv_msec = UPNP_RECV_TIMEOUT_NEXT;

		upnp_assert( err > 0 );
		offset += err;

		content = strstr( buffer->addr, "\r\n\r\n" );
		if ( content != NULL )
		{
			content += 4;
		}
		else
		{
			content = strstr( buffer->addr, "\n\n" );  /* see RFC 2616: 19.3 Tolerant Applications */
			if ( content != NULL )
			{
				content += 2;
			}
		}

		if ( content != NULL )
		{
			/* Set buffer->len to the len of the HTTP header */
			/* (This value will be used by get_http_header_field() and get_http_content()) */
			buffer->len = content - buffer->addr;

		#if LOGLEVEL >= 5
			upnp_debug(( "upnp_recv_header(): HTTP header:\n%s\n", buffer->addr ));
		#endif

			return (int)offset;
		}
	}
}

/* Defragment "chunked" HTTP/1.1 data */
static char *upnp_recv_defragment( char *src, int len, int *state, long *chunk_size )
{
	char *dest = src;  /* defragment in-place */

	while ( --len >= 0 )
	{
		char ch = *src++;

		switch ( *state )
		{
		case 1:  /* chunk start */
			*chunk_size = 0;
			*state = 2;
			/* pass through */

		case 2:  /* reading chunk len */
			if ( ch >= 'a' )
			{
				upnp_assert( ch <= 'f' );
				*chunk_size = *chunk_size * 0x10 + (ch - ('a' - '\x0a'));
			}
			else if ( ch >= 'A' )
			{
				upnp_assert( ch <= 'F' );
				*chunk_size = *chunk_size * 0x10 + (ch - ('A' - '\x0A'));
			}
			else if ( ch >= '0' )
			{
				upnp_assert( ch <= '9' );
				*chunk_size = *chunk_size * 0x10 + (ch - '0');
			}
			else
			{
				*state = 3;
			}
			break;

		case 3:  /* waiting for end of chunk len */
			if ( ch == '\n' ) *state = 4;
			break;

		case 4:  /* chunk data start */
			if ( *chunk_size == 0 )
			{
				*dest = '\0';
				return NULL;  /* empty chunk => finished */
			}
			*state = 5;
			/* pass through */

		case 5:  /* chunk data */
			*dest++ = ch;
			if ( --(*chunk_size) == 0 ) *state = 6;
			break;

		case 6:  /* waiting for end of chunk data */
			upnp_assert( ch == '\r' || ch == '\n' );
			if ( ch == '\n' ) *state = 1;
			break;
		}
	}

	return dest;
}

/* HTTP receive */
static int upnp_recv( SOCKET s, struct buffer *buffer, GError **error )
{
	size_t offset = 0;
	int err;

	char *content;
	long current_length, content_length;
	const char *value;
	int chunked;

	upnp_assert( buffer != NULL && buffer->addr != NULL );

	err = upnp_recv_header( s, buffer );
	if ( err < 0 )
	{
		upnp_warning(( "upnp_recv(): upnp_recv_header() = %d\n", err ));
		set_upnp_error( error, err, "Can't retrieve header from server" );
		return err;
	}

	content = get_http_content( buffer );
	offset = (size_t)err;

	current_length = (buffer->addr + offset) - content;
	upnp_assert( current_length >= 0 );

	value = get_http_header_field( buffer, "Content-Length" );
	if ( value != NULL )
	{
		content_length = strtol( value, NULL, 10 );
	}
	else
	{
		content_length = ( get_http_header_field( buffer, "Content-Type" ) != NULL )
			? -1 : 0;
	}

	if ( content_length != 0 )
	{
		value = get_http_header_field( buffer, "Transfer-Encoding" );
		chunked = ( value != NULL && stricmp( value, "chunked" ) == 0 );
	}
	else
	{
		chunked = FALSE;
	}

	upnp_debug(( "upnp_recv(): Current-Length = %ld, Content-Length = %ld, chunked = %s\n",
		current_length, content_length, chunked ? "TRUE" : "FALSE" ));

	if ( chunked )
	{
		upnp_assert( content_length < 0 );  /* we can't handle that (yet)! */
		content_length = -1;

		chunked = 1;  /* chunk start */
		content = upnp_recv_defragment( content, (int)current_length, &chunked, &current_length );
		if ( content == NULL )
			content_length = current_length;  /* finished */
		else
			offset = content - buffer->addr;
	}
	else if ( content_length > 0 )
	{
		if ( !resize_buffer( buffer, (content - buffer->addr) + content_length + 1, error ) )
		{
			upnp_warning(( "upnp_recv(): resize_buffer(%ld) failed\n", (content - buffer->addr) + content_length + 1 ));
			return -1;
		}
	}

	while ( content_length < 0 || current_length < content_length )
	{
		upnp_assert( offset < buffer->size );
		if ( offset == buffer->size - 1 )
		{
			if ( !enlarge_buffer( buffer, error ) )
			{
				upnp_warning(( "upnp_recv(): enlarge_buffer(0) failed\n" ));
				return -1;
			}
		}

		err = _upnp_recv( s, buffer->addr + offset, buffer->size - offset, UPNP_RECV_TIMEOUT_NEXT );
		if ( err == UPNP_ERR_SOCKET_CLOSED && content_length < 0 && !chunked )
			break;  /* We have to assume we get everything here */

		if ( err < 0 )
		{
			upnp_warning(( "upnp_recv(): current_length = %ld < content_length = %ld\n", current_length, content_length ));
			set_upnp_error( error, err, "Couldn't retrieve all data from server" );
			return err;  /* we did not get all bytes! */
		}

		upnp_assert( err > 0 );

		if ( chunked )
		{
			content = upnp_recv_defragment( buffer->addr + offset, err, &chunked, &current_length );
			if ( content == NULL )
				content_length = current_length;  /* finished */
			else
				offset = content - buffer->addr;
		}
		else
		{
			offset += err;
			current_length += err;
		}
	}

#if LOGLEVEL >= 6
	upnp_debug(( "upnp_recv(): HTTP body:\n%s\n", buffer->addr + buffer->len ));
#endif
	return get_http_result( buffer, error );
}

/* Receive bytes (UDP) (Timeout = tv_sec) */
static int upnp_recvfrom( SOCKET s, struct buffer *buffer, long tv_msec, GError **error )
{
	static const char error_message[] = "Can't retrieve data from server";

	struct sockaddr_in from;
	socklen_t fromlen = sizeof( from );
	int err;

	err = socket_select_read( s, tv_msec );
	if ( err == 0 )
	{
		err = set_upnp_error( error, UPNP_ERR_TIMEOUT, error_message );
		return err;
	}
	if ( err < 0 )
	{
		err = set_socket_error( error, error_message, NULL );
		return err;
	}

	upnp_assert( buffer != NULL && buffer->addr != NULL && buffer->size > 1 );

	err = recvfrom( s, buffer->addr, buffer->size - 1, 0, (struct sockaddr*)&from, &fromlen );
	if ( err == 0 )
	{
		err = set_upnp_error( error, UPNP_ERR_SOCKET_CLOSED, error_message );
		return err;
	}
	if ( err < 0 )
	{
		err = set_socket_error( error, error_message, NULL );
		return err;
	}

	upnp_assert( (unsigned)err < buffer->size - 1 );
	buffer->addr[buffer->len = err] = '\0';

	upnp_debug(( "recvfrom() = %d\n", err ));
#if LOGLEVEL >= 5
	upnp_debug(( "upnp_recvfrom():\n%s\n", buffer->addr ));
#endif

	return get_http_result( buffer, error );
}

/* A combination of upnp_send(), upnp_recv(), and shutdown() */
static int upnp_send_recv( SOCKET s, struct buffer *buffer, GError **error )
{
	int err = upnp_send( s, buffer );
	upnp_debug(( "upnp_send() = %d\n", err ));
	if ( err >= 0 )
	{
		err = upnp_recv( s, buffer, error );
		upnp_debug(( "upnp_recv() = %d\n", err ));
	}
	else
	{
		set_upnp_error( error, err, "Can't send data to server" );
	}

	shutdown( s, 1 /*=SD_SEND*/ );
	return err;
}

/* A combination of connect() and upnp_send_recv() */
static int upnp_connect_send_recv( struct sockaddr_in *addr, struct buffer *buffer, GError **error )
{
	SOCKET s = new_socket( SOCK_STREAM );

	int err = connect( s, (struct sockaddr*)addr, sizeof( struct sockaddr_in ) );
	if ( err >= 0 )
	{
		upnp_debug(( "connect() = %d\n", err ));
		err = upnp_send_recv( s, buffer, error );
	}
	else
	{
		err = set_socket_error( error, "Can't connect to %s", addr );
	}

	closesocket( s );
	return err;
}

/*--- HTTP (Client) -----------------------------------------------------------------------------*/

static const char *http_user_agent;

void http_set_user_agent( const char *user_agent )
{
	http_user_agent = user_agent;
}

/* Splits the given URL to address, port, and path */
static const char *http_url( const char *url, char host[/*UPNP_MAX_HOST_ADDRESS*/], struct sockaddr_in *addr, GError **error )
{
	const char *path;
	u_short port;
	char *s;

	upnp_assert( url != NULL && host != NULL && addr != NULL );
	if ( url != NULL && host != NULL && addr != NULL )
	{
		if ( strnicmp( url, "http://", 7 ) == 0 )
		{
			url += 7;

			strncpy( host, url, UPNP_MAX_HOST_ADDRESS-1 )[UPNP_MAX_HOST_ADDRESS-1] = '\0';
			s = strchr( host, ':' );
			if ( s == NULL ) s = strchr( host, '/' );
			if ( s != NULL ) *s = '\0';

			url += strlen( host );

			if ( *url == ':' )
			{
				port = (unsigned short)atol( url + 1 );
				path = strchr( url, '/' );
			}
			else
			{
				port = 80;
				path = (char*)url;
			}

			if ( path == NULL || *path == '\0' ) path = "/";

			if ( set_sockaddr( addr, host, port ) != 0 )
			{
				set_upnp_error( error, UPNP_ERR_UNKNOWN_HOST, "Unknown host" );
				path = NULL;
			}
		}
		else
		{
			set_upnp_error( error, UPNP_ERR_INVALID_URL, "Invalid URL" );
			path = NULL;
		}
	}
	else
	{
		set_upnp_error( error, UPNP_ERR_INVALID_PARAMS, "Invalid parameter" );
		path = NULL;
	}

	return path;
}

/* Builds a URL */
static char *build_url( const upnp_device *device, const char *serviceURL, char url[/*UPNP_MAX_URL*/] )
{
	const char *URLBase = device->URLBase;
	char *s;

	if ( device == NULL )
	{
		upnp_warning(( "build_url(): device not available\n" ));
		return NULL;
	}
	if ( URLBase == NULL || URLBase[0] == '\0' )
	{
		upnp_warning(( "build_url(): URLBase not available\n" ));
		return NULL;
	}
	if ( serviceURL == NULL || serviceURL[0] == '\0' )
	{
		upnp_warning(( "build_url(): serviceURL not available\n" ));
		return NULL;
	}

	upnp_debug(( "URLBase = %s\nserviceURL = %s\n", URLBase, serviceURL ));

	/* URL = URLBase + serviceURL */
	upnp_assert( url != NULL );
	strcpy( url, URLBase );
	if ( serviceURL[0] == '/' )
	{
		s = strchr( url + 7, '/' );    /* first '/' */
	}
	else
	{
		s = strrchr( url, '/' );       /* last '/' */
		if ( s != NULL ) s++;
	}
	if ( s == NULL )   /* invalid URLBase format */
	{
		upnp_warning(( "build_url(): invalid URLBase format \"%s\"\n", URLBase ));
		return NULL;
	}

	strcpy( s, serviceURL );
	upnp_assert( strlen( url ) < UPNP_MAX_URL );

	upnp_debug(( "build_url() = \"%s\"\n", url ));
	return url;
}

static void set_http_content( struct buffer *buffer, char **type, char **content, size_t *length )
{
	upnp_assert( content != NULL );
	*content = get_http_content( buffer );
	if ( *content != NULL )
	{
		if ( type != NULL )
		{
			*type = get_http_header_field( buffer, "Content-Type" );
		}

		if ( length != NULL )
		{
			const char *content_length = get_http_header_field( buffer, "Content-Length" );
			if ( content_length != NULL ) *length = strtoul( content_length, NULL, 10 );
			else *length = 0L;  /* unknown */
		}
	}
	else
	{
		if ( type != NULL ) *type = NULL;
		if ( length != NULL ) *length = 0L;

		release_buffer( buffer );
	}
}

static void http_error( const char *url, GError **error )
{
	if ( error != NULL && *error != NULL )
	{
		GString *message = g_string_new( (*error)->message );
		size_t url_len;

		g_string_append( message, "\n\nURL = " );

		if ( url == NULL ) url = "<NULL>";
		url_len = strlen( url );
		if ( url_len > 50 )
		{
			g_string_append_len( message, url, 29 );
			g_string_append( message, "..." );
			g_string_append( message, url + url_len - 19 );
		}
		else
		{
			g_string_append( message, url );
		}

		g_free( (*error)->message );
		(*error)->message = g_string_free( message, FALSE );

		upnp_warning(( "HTTP error %d \"%s\"\n", (*error)->code, (*error)->message ));
	}
}

#define MAX_REDIRECTIONS 3

/* HTTP GET <url> */
gboolean http_get( const char *url, size_t initial_size, char **type, char **content, size_t *length, GError **error )
{
	gboolean result = FALSE;
	const char *_url = url;
	struct buffer *buffer;

	upnp_assert( url != NULL );
	set_http_content( NULL, type, content, length );

	buffer = alloc_buffer( initial_size, 0, "http_get()", error );
	if ( buffer != NULL )
	{
		int i = 0;

		for (;;)
		{
			char host[UPNP_MAX_HOST_ADDRESS];
			struct sockaddr_in addr;
			const char *path;
			int err;

			upnp_debug(( "http_get( \"%s\" )...\n", url ));
			path = http_url( _url, host, &addr, error );
			if ( path != NULL )
			{
				prepare_for_upnp_send( buffer );

				/* Header */
				sprintf( buffer->addr,
					"GET %s HTTP/" HTTP_VERSION "\r\n"
					"HOST: %s\r\n",
					path, host /*,addr->sin_port?*/ );
				if ( http_user_agent != NULL )
					strcat( strcat( strcat( buffer->addr, "USER-AGENT: " ), http_user_agent ), "\r\n" );
				strcat( buffer->addr, "\r\n" );
				upnp_debug(( "%s", buffer->addr ));

				err = upnp_connect_send_recv( &addr, buffer, error );
			}
			else
			{
				upnp_warning(( "http_get(): http_url() failed\n" ));
				err = UPNP_ERR_INVALID_URL;
			}

			if ( err == 0 )
			{
				result = TRUE;
				break;
			}

			if ( err < 300 || err >= 400 ) break;

			_url = get_http_header_field( buffer, "Location" );
			if ( _url == NULL ) break;

			if ( ++i > MAX_REDIRECTIONS )
			{
				*content = (char*)_url;
				if ( type != NULL ) *type = "LOCATION";
				if ( length != NULL ) *length = 0;

				buffer = NULL;
				break;
			}

			g_clear_error( error );
		}

		if ( buffer != NULL ) set_http_content( buffer, type, content, length );
	}
	else
	{
		upnp_warning(( "http_get(): alloc_buffer() failed\n" ));
	}

	http_error( url, error );
	return result;
}

/* HTTP POST <url> */
gboolean http_post( const char *url, size_t initial_size, char **type, char **content, size_t *length, GError **error )
{
	gboolean result = FALSE;
	struct buffer *buffer;

	char host[UPNP_MAX_HOST_ADDRESS];
	struct sockaddr_in addr;
	const char *path;

	const char *post_type, *post_content;

	upnp_assert( url != NULL );
	upnp_assert( type != NULL && *type != NULL );
	upnp_assert( content != NULL && *content != NULL );

	post_type = *type;
	post_content = *content;
	set_http_content( NULL, type, content, length );

	buffer = alloc_buffer( initial_size, 0, "http_post()", error );
	if ( buffer != NULL )
	{
		upnp_debug(( "http_post( \"%s\" )...\n", url ));
		path = http_url( url, host, &addr, error );
		if ( path != NULL )
		{
			int err;

			prepare_for_upnp_send( buffer );

			/* Content */
			append_string_to_buffer( buffer, post_content );

			/* Header */
			sprintf( buffer->addr,
				"POST %s HTTP/" HTTP_VERSION "\r\n"
				"HOST: %s\r\n"
				"CONTENT-LENGTH: %lu\r\n"
				"CONTENT-TYPE: %s\r\n",
				path, host,
				upnp_content_length( buffer ), post_type );
			if ( http_user_agent != NULL )
				strcat( strcat( strcat( buffer->addr, "USER-AGENT: " ), http_user_agent ), "\r\n" );
			strcat( buffer->addr, "\r\n" );
			upnp_debug(( "%s", buffer->addr ));

			err = upnp_connect_send_recv( &addr, buffer, error );
			if ( err == 0 ) result = TRUE;

			set_http_content( buffer, type, content, length );
		}
		else
		{
			upnp_warning(( "http_post(): http_url() failed\n" ));
		}
	}
	else
	{
		upnp_warning(( "http_post(): alloc_buffer() failed\n" ));
	}

	http_error( url, error );
	return result;
}

/*--- HTTP (Server) -----------------------------------------------------------------------------*/

enum { HTTP_SERVER_MASK = 1, GENA_SERVER_MASK = 2 };

int http_server_started;
SOCKET http_server_socket;
unsigned short http_server_port;
GThread *http_server_thread;

gint http_connections;

struct string_list
{
	struct string_list *next;
	gchar *str;
} *http_url_list;

static void http_server_free_url_list( struct string_list *url_list )
{
	struct string_list *strl;

	for ( strl = url_list; strl != NULL; )
	{
		struct string_list *next = strl->next;

		g_free( strl->str );
		g_free( strl );

		strl = next;
	}
}

void http_server_add_to_url_list( gchar *str )
{
	upnp_message(( "http_server_addto_url_list( \"%s\" )\n", (str != NULL) ? str : "<NULL>" ));
	upnp_assert( str != NULL );

	if ( str != NULL )
	{
		struct string_list *strl = g_new0( struct string_list, 1 );
		strl->str = g_strdup( str );
		list_append( &http_url_list, strl );
	}
}

gboolean http_server_remove_from_url_list( const gchar *str )
{
	struct string_list *strl;

	upnp_assert( str != NULL );

	for ( strl = http_url_list; strl != NULL; strl = strl->next )
	{
		if ( str != NULL && strcmp( strl->str, str ) == 0 )
		{
			struct string_list *old_http_url_list = http_url_list;
			http_url_list = strl->next;
			strl->next = NULL;
			http_server_free_url_list( old_http_url_list );

			upnp_message(( "http_server_remove_from_url_list( \"%s\" ) = TRUE\n", (str != NULL) ? str : "<NULL>" ));
			return TRUE;
		}
	}

	upnp_warning(( "http_server_remove_from_url_list( \"%s\" ) = FALSE\n", (str != NULL) ? str : "<NULL>" ));
	return FALSE;
}

void http_server_remove_all_from_url_list( void )
{
	struct string_list *strl = http_url_list;
	http_url_list = NULL;
	http_server_free_url_list( strl );
}

gchar *http_get_server_address( const upnp_device *device )
{
	if ( IS_VALID_SOCKET( http_server_socket ) && http_server_port != 0 && http_server_thread != NULL )
	{
		if ( device != NULL )
		{
			const upnp_subscription *subscription;
			for ( subscription = subscriptionList; subscription != NULL; subscription = subscription->next )
			{
				if ( subscription->device == device )
				{
					return g_strdup_printf( "%s:%u", subscription->sock_addr, http_server_port );
				}
			}

			;;; /* TODO: Connect to device to determine own IP address... */
		}
		else
		{
			return g_strdup_printf( "localhost:%u", http_server_port );
		}
	}

	return NULL;
}

FILE *fopen_utf8( const char *filename, const char *mode )
{
	gchar *locale_filename = g_locale_from_utf8( filename, -1, NULL, NULL, NULL );
	if ( locale_filename != NULL )
	{
		FILE *fp = fopen( locale_filename, mode );
		g_free( locale_filename );
		return fp;
	}

	return NULL;
}

static const char *http_get_file( char *filename, SOCKET dest, struct buffer *buffer )
{
	const char *content_type = "application/octet-stream";
	const char *http_result = NULL;
	char *range = NULL, *s;
	FILE *fp;

	upnp_message(( "http_get_file( %s )...\n", filename ));

	for ( s = strchr( filename, '?' ); s != NULL; s = strchr( s, '&' ) )
	{
		*s++ = '\0';

		if ( strncmp( s, "bytes=", 6 ) == 0 )
		{
			range = s;
		}
		else if ( strncmp( s, "content-type=", 13 ) == 0 )
		{
			content_type = s + 13;
		}
	}

	upnp_message(( "...http_get_file( %s )...\n", filename ));

	fp = fopen_utf8( filename, "rb" );
	if ( fp != NULL )
	{
		long from = 0, to = 0;

		if ( range == NULL ) range = get_http_header_field( buffer, "Range" );
		if ( range == NULL || *range == '\0' ) range = "bytes=0-";
		upnp_message(( "=> Range = \"%s\"\n", range ));

		if ( strncmp( range, "bytes=", 6 ) == 0 )
		{
			from = strtol( range + 6, &range, 10 );
			if ( from >= 0 && *range == '-' )
			{
				if ( *++range == '\0' )
				{
					fseek( fp, 0, SEEK_END );
					to = ftell( fp );
					rewind( fp );
				}
				else
				{
					to = strtol( range, &range, 10 ) + 1;
				}
			}
		}

		if ( *range == '\0' )
		{
			gchar *answer;
			int err;

			answer = g_strdup_printf(
				"HTTP/"HTTP_VERSION" 200 OK\r\n"
				"SERVER: "HTTP_SERVER"\r\n"
				"ACCEPT-RANGES: bytes\r\n"
				"CONNECTION: keep-alive\r\n"
				"CONTENT-LENGTH: %ld\r\n"
				"CONTENT-TYPE: %s\r\n" /*audio/mpeg\r\n"*/
				"\r\n", to - from, content_type );

			err = _upnp_send( dest, answer, strlen( answer ) );
			upnp_message(( "http_server(): _upnp_send( %s ) = %d\n", answer, err ));
			g_free( answer );

			if ( err == 0 && from != 0 )
			{
				err = fseek( fp, from, SEEK_CUR );
				upnp_message(( "http_get_file(): fseek( %ld ) = %d\n", from, err ));
			}

			while ( err == 0 && from < to )
			{
				size_t to_send = to - from;
				size_t part = (to_send < buffer->size) ? to_send : buffer->size;

				size_t read = fread( buffer->addr, sizeof(unsigned char), part, fp );
				upnp_message(( "http_server(): fread() = %lu\n", (unsigned long)read ));
				if ( read <= 0 ) break;

				err = _upnp_send( dest, buffer->addr, read );
				upnp_message(( "http_server(): _upnp_send() = %d\n", err ));

				from += read;
			}
		}
		else
		{
			upnp_warning(( "http_server(): invalid Range value \"%s\"\n", range ));
			http_result = "400 Bad Request";
		}

		fclose( fp );
	}
	else
	{
		upnp_warning(( "http_server(): file \"%s\" not found\n", filename ));
		http_result = "404 File Not Found";
	}

	upnp_message(( "http_get_file( %s ) = \"%s\"\n", filename, http_result ));
	return http_result;
}

static const char *http_get_http( char *url, SOCKET dest, struct buffer *buffer )
{
	char host[UPNP_MAX_HOST_ADDRESS], *s;
	const char *http_result = NULL;
	struct sockaddr_in addr;
	const char *path;
	int err, i = 0;

	do
	{
		upnp_message(( "http_get_http( %s )...\n", url ));

		path = http_url( url, host, &addr, NULL );
		if ( path != NULL )
		{
			SOCKET src = new_socket( SOCK_STREAM );

			err = connect( src, (struct sockaddr*)&addr, sizeof( struct sockaddr_in ) );
			upnp_message(( "connect() = %d\n", err ));
			if ( err >= 0 )
			{
				GString *header = g_string_new( "" );

				g_string_printf( header,
					"GET %s HTTP/" HTTP_VERSION "\r\n"
					"HOST: %s\r\n",
					path, host /*,addr->sin_port?*/ );
				if ( http_user_agent != NULL )
					g_string_append_printf( header, "USER-AGENT: %s\r\n", http_user_agent );
				g_string_append( header, "\r\n" );
				err = _upnp_send( src, header->str, strlen( header->str ) );
				upnp_message(( "http_get_http(): _upnp_send( <header> ) = %d\n", err ));
				g_string_free( header, TRUE );

				if ( err >= 0 )
				{
					err = upnp_recv_header( src, buffer );
					if ( err >= 0 )
					{
						int result;

						upnp_message(( "http_get_http(): HTTP header:\n%s\n", buffer->addr ));

						result = get_http_result( buffer, NULL );
						upnp_message(( "http_get_http(): HTTP result = %d\n", result ));

						if ( result >= 300 && result < 400 && (s = get_http_header_field( buffer, "Location" )) != NULL )
						{
							strcpy( url, s );
						}
						else
						{
							for (;;)
							{
								err = _upnp_send( dest, buffer->addr, err );
								if ( err < 0 ) break;

								err = _upnp_recv( src, buffer->addr, buffer->size, 0 );
								if ( err < 0 ) break;
							}
						}
					}
					else
					{
						upnp_warning(( "http_get_http(): upnp_recv_header() failed\n" ));
						http_result = "504 Gateway Time-out";
					}
				}
				else
				{
					upnp_warning(( "http_get_http(): _upnp_send() failed\n" ));
					http_result = "504 Gateway Time-out";
				}

				shutdown( src, 1 /*=SD_SEND*/ );
			}
			else
			{
				upnp_warning(( "http_get_http(): connect() failed\n" ));
				http_result = "503 Service Unavailable";
			}

			closesocket( src );
		}
		else
		{
			upnp_warning(( "http_get_http(): http_url() failed\n" ));
			http_result = "400 Bad Request";
			err = UPNP_ERR_INVALID_URL;
		}

		if ( ++i > MAX_REDIRECTIONS ) break;
	}
	while ( err >= 0 );

	return NULL;
}

static const char *http_get_func( SOCKET s, struct buffer *buffer )
{
	const char *http_result;

	char *arg = buffer->addr + 4;
	while ( xml_is_space( *arg ) ) arg++;

	if ( strncmp( arg, "/file//", 7 ) == 0 )
	{
		gchar *filename = url_decoded( arg + 7 );
		if ( filename != NULL )
		{
			http_result = http_get_file( filename, s, buffer );
			g_free( filename );
		}
		else
		{
			http_result = "403 Forbidden";
		}
	}
	else if ( strncmp( arg, "/http//", 7 ) == 0 )
	{
		char *x;

		strncpy( arg, "http://", 7 );
		for ( x = arg; !xml_is_space( *x ); x++ ) ;
		*x++ = '\0';

		if ( http_server_remove_from_url_list( arg ) )
		{
			upnp_message(( "http_server_func(): Access to URL %s is allowed\n", arg ));
			http_result = http_get_http( arg, s, buffer );
		}
		else
		{
			upnp_warning(( "http_server_func(): Access to URL %s is forbidden\n", arg ));
			http_result = "403 Forbidden";
		}
	}
	else
	{
		http_result = "404 Not Found";
	}

	return http_result;
}

static const char *gena_notify_func( SOCKET s, struct buffer *buffer );

gpointer http_server_func( gpointer user_data )
{
	SOCKET s = (SOCKET)GPOINTER_TO_UINT( user_data );
	struct buffer *buffer;
	int err;

	buffer = alloc_buffer( HTTP_BUFFER_SIZE, 0, "http_server_func()", NULL );
	if ( buffer == NULL )
	{
		long tv_msec = UPNP_RECV_TIMEOUT_1ST;
		char buf[16];

		upnp_warning(( "http_server_func(): alloc_buffer() failed\n" ));
		err = UPNP_ERR_OUT_OF_MEMORY;

		while ( _upnp_recv( s, buf, sizeof( buf ), tv_msec ) > 0 ) tv_msec = UPNP_RECV_TIMEOUT_NEXT;
	}
	else
	{
		err = upnp_recv( s, buffer, NULL );
	#if LOGLEVEL >= 4 && LOGLEVEL < 5  /* otherwise upnp_recv() will do this */
		upnp_debug(( "\n%s\n", buffer->addr ));
	#endif
	}

	upnp_message(( "http_server_func(): err = %d (should be %d)\n", err, INVALID_HTTP_RESULT ));
	if ( err != INVALID_HTTP_RESULT )
	{
		release_buffer( buffer );
		buffer = NULL;
	}

	if ( buffer != NULL )
	{
		const char *http_result;

		if ( (http_server_started & GENA_SERVER_MASK) != 0 && strncmp( buffer->addr, "NOTIFY ", 7 ) == 0 )
		{
			http_result = gena_notify_func( s, buffer );
		}

		else if ( (http_server_started & HTTP_SERVER_MASK) != 0 && strncmp( buffer->addr, "GET ", 4 ) == 0 )
		{
			g_atomic_int_add( &http_connections, 1 );
			http_result = http_get_func( s, buffer );
			g_atomic_int_add( &http_connections, -1 );
		}
		else
		{
			http_result = "405 Method Not Allowed";
		}

		release_buffer( buffer );

		if ( http_result != NULL )
		{
			char buf[256];
			int n;

			upnp_message(( "http_server_func(): %s\n", http_result ));
			n = sprintf( buf, "HTTP/"HTTP_VERSION" %s\r\nSERVER: "HTTP_SERVER"\r\nConnection: close\r\n\r\n", http_result );
			/* TODO: "Date: Tue, 15 Dec 2009 13:35:54 GMT\r\n" */
			upnp_assert( n < sizeof( buf ) );
			_upnp_send( s, buf, n );

			err = _upnp_recv( s, buf, sizeof(buf), UPNP_RECV_SEND_TIMEOUT );
			upnp_message(( "http_server_func(): _upnp_recv() = %d\n", err ));
		}
	}

	err = shutdown( s, 1 /*=SD_SEND*/ );
	upnp_message(( "http_server_func(): shutdown() = %d\n", err ));
	err = closesocket( s );
	upnp_message(( "http_server_func(): closesocket() = %d\n", err ));

	return NULL;
}

gpointer http_server( gpointer user_data )
{
	socklen_t len = sizeof( struct sockaddr_in );
	struct sockaddr_in addr;

	upnp_message(( "=> http_server()\n" ));

	while ( IS_VALID_SOCKET( http_server_socket ) )
	{
		SOCKET s = accept( http_server_socket, (struct sockaddr*)&addr, &len );
		upnp_message(( "http_server(): accept() = %d, %s, %s\n", (int)s,
			IS_VALID_SOCKET( s ) ? "TRUE" : "FALSE",
			IS_VALID_SOCKET( http_server_socket ) ? "TRUE" : "FALSE" ));
		if ( !IS_VALID_SOCKET( http_server_socket ) || !IS_VALID_SOCKET( s ) ) break;

		g_thread_create( http_server_func, GUINT_TO_POINTER( (guint)s ), FALSE, NULL );
	}

	upnp_message(( "<= http_server()\n" ));
	return NULL;
}

/* Start the HTTP server */
static gboolean _http_start_server( int bitmask, GError **error )
{
	gboolean result = FALSE;
	struct sockaddr_in addr;
	int err;

	if ( IS_VALID_SOCKET( http_server_socket ) )
	{
		http_server_started |= bitmask;
		return TRUE;
	}

	http_end_server();

	http_server_socket = new_socket( SOCK_STREAM );
	err = 1;
	setsockopt( http_server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&err, sizeof(err) );

	init_sockaddr( &addr );
	err = bind( http_server_socket, (struct sockaddr*)&addr, sizeof( struct sockaddr_in ) );
	if ( err >= 0 )
	{
		socklen_t addrlen = sizeof( addr );

		upnp_message(( "http_start_server(): bind() = %d\n", err ));

		err = getsockname( http_server_socket, (struct sockaddr*)&addr, &addrlen );
		if ( err >= 0 )
		{
			upnp_message(( "http_start_server(): getsockname() = %d\n", err ));

			http_server_port = ntohs( addr.sin_port );
			upnp_message(( "http_start_server(): http_server_port = %u\n", http_server_port ));

			err = listen( http_server_socket, HTTP_MAX_CONNECTIONS /*SOMAXCONN*/ );
			if ( err >= 0 )
			{
				upnp_message(( "http_start_server(): listen() = %d\n", err ));
				upnp_assert( g_thread_supported() );

				http_server_thread = g_thread_create( http_server, NULL, TRUE, error );
				if ( http_server_thread != NULL )
				{
					http_server_started |= bitmask;
					result = TRUE;
				}
				else
				{
					upnp_warning(( "http_start_server(): g_thread_create() failed\n" ));
				}
			}
			else
			{
				set_socket_error( error, "Can't get network socket name", NULL );
			}
		}
		else
		{
			set_socket_error( error, "Can't get network socket name", NULL );
		}
	}
	else
	{
		set_socket_error( error, "Can't bind network socket", NULL );
	}

	if ( !result ) close_socket( &http_server_socket );

	upnp_message(( "http_start_server(): http_socket = %s\n", (result) ? "TRUE" : "FALSE" ));
	return result;
}

gboolean http_start_server( GError **error )
{
	return _http_start_server( HTTP_SERVER_MASK, error );
}

/* Stop the HTTP server */
static void _http_stop_server( int bitmask )
{
	http_server_started &= ~bitmask;

	/*if ( http_server_started == 0 && close_socket( &http_server_socket ) )*/
	if ( http_server_started == 0 && shutdown_socket( &http_server_socket, SD_BOTH ) )
	{
		unsigned short port = http_server_port;
		http_server_port = 0;

		if ( port != 0 )
		{
			struct sockaddr_in addr;

			if ( set_sockaddr( &addr, "127.0.0.1", port ) == 0 )
			{
				SOCKET s = new_socket( SOCK_STREAM );
				int err;

				upnp_message(( "http_stop_server(): end server thread...\n" ));
				err = connect( s, (struct sockaddr*)&addr, sizeof( struct sockaddr_in ) );
				upnp_message(( "http_stop_server(): connect() = %d\n", err ));

				closesocket( s );
			}
		}
	}
}

void http_stop_server( void )
{
	http_server_remove_all_from_url_list();
	_http_stop_server( HTTP_SERVER_MASK );
}

/* End the HTTP server thread */
static void _http_end_server( int bitmask )
{
	_http_stop_server( bitmask );

	if ( http_server_started == 0 && http_server_thread != NULL )
	{
		upnp_message(( "http_end_server(): wait for end of HTTP thread...\n" ));

		g_thread_join( http_server_thread );
		http_server_thread = NULL;

		upnp_message(( "http_end_server(): wait for end of HTTP thread...ok\n" ));
	}
}

void http_end_server( void )
{
	http_server_remove_all_from_url_list();
	_http_end_server( HTTP_SERVER_MASK );
}

/*--- HTTP (URL) --------------------------------------------------------------------------------*/

static const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

gboolean url_is_unreserved_char( char ch )
{
	/* see RFC 3986 "Uniform Resource Identifier (URI): Generic Syntax" */
	return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
	       (ch >= '0' && ch <= '9') || ch == '-' || ch == '.' || ch == '_' || ch == '~';
}

gchar *url_encoded( const char *arg )
{
	gchar *result, *dest;
	size_t len, i;

	upnp_assert( arg != NULL );
	if ( arg == NULL ) return NULL;

	result = g_new( gchar, (len = strlen( arg )) * 3 + 1 );
	dest = result;
	for ( i = 0; i < len; i++ )
	{
		gchar ch = *arg++;
		if ( url_is_unreserved_char( ch ) )
		{
			*dest++ = ch;
		}
		else
		{
			*dest++ = '%';
			*dest++ = hex[(unsigned char)ch >> 4];
			*dest++ = hex[(unsigned char)ch & 0xF];
		}
	}
	*dest = '\0';

	return result;
}

gchar *url_encoded_path( const char *arg )
{
	gchar *result, *dest;
	size_t len, i;

	upnp_assert( arg != NULL );
	if ( arg == NULL ) return NULL;

	result = g_new( gchar, (len = strlen( arg )) * 3 + 1 );
	dest = result;
	for ( i = 0; i < len; i++ )
	{
		gchar ch = *arg++;
		if ( url_is_unreserved_char( ch ) || ch == ':' || ch == '/' )
		{
			*dest++ = ch;
		}
		else
		{
			*dest++ = '%';
			*dest++ = hex[(unsigned char)ch >> 4];
			*dest++ = hex[(unsigned char)ch & 0xF];
		}
	}
	*dest = '\0';

	return result;
}

static int _url_decoded( char ch )
{
	if ( ch >= '0' && ch <= '9' )
		return ch - '0';
	if ( ch >= 'A' && ch <= 'F' )
		return (ch - 'A') + 10;
	if ( ch >= 'a' && ch <= 'f' )
		return (ch - 'a') + 10;
	return -1;
}

gchar *url_decoded( const char *url )
{
	gchar *result, *dest;

	upnp_assert( url != NULL );
	if ( url == NULL ) return NULL;

	result = g_new( gchar, strlen( url ) + 1 );
	dest = result;

	for (;;)
	{
		char ch = *url++;
		if ( xml_is_space( ch ) ) break;

		if ( ch == '%' )
		{
			int i = _url_decoded( *url++ );
			int j = _url_decoded( *url++ );
			if ( i >= 0 && j >= 0 )
			{
				ch = (char)(i * 16 + j);
			}
			else
			{
				url -= 2;
			}
		}

		*dest++ = ch;
	}
	*dest = '\0';

	return result;
}


/*--- SSDP --------------------------------------------------------------------------------------*/

int SSDP_TTL = 4, SSDP_LOOP = 0, SSDP_MX = 2;
double SSDP_2ND_ATTEMPT = 1.0/*s*/, SSDP_TIMEOUT = 30.0/*s*/, SSDP_GET_DESCRIPTION_TIMEOUT = 30.0/*s*/;

static upnp_device *deviceList;

static int m_search( const char *stv[], int stc, int mx,
		SOCKET s, struct buffer *buffer, struct sockaddr_in *addr )
{
	int i, err = 0;

	upnp_assert( stv != NULL && mx > 0 );

	for ( i = 0; i < stc; i++ )
	{
		sprintf( buffer->addr,
			"M-SEARCH * HTTP/1.1\r\n"
			"HOST: 239.255.255.250:1900\r\n"
			"MAN: \"ssdp:discover\"\r\n"
			"MX: %d\r\n"
			"ST: %s\r\n"
			"\r\n", mx, stv[i] );

		err = upnp_sendto( s, buffer, addr );
		if ( err < 0 ) break;
	}

	upnp_message(( "m_search( mx = %d ) = %d\n", mx, err ));
	return err;
}

static char *upnp_stpcpy( char **dest, const char *src )
{
	char *result;

	if ( src != NULL )
	{
		result = *dest;
		*dest = g_stpcpy( *dest, src ) + 1;
	}
	else
		result = NULL;

	return result;
}

static char *get_service_description( upnp_service *service, char *content, char *buffer )
{
	char *name, *s;

	while ( (s = xml_unbox( &content, &name, NULL )) != NULL )
	{
		upnp_debug(( "    %s = \"%s\"\n", name, s ));

		if ( strcmp( name, "serviceType" ) == 0 )
			service->serviceType = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "serviceId" ) == 0 )
		{
			service->serviceId = upnp_stpcpy( &buffer, s );

			if ( service->serviceId != NULL )
			{
				/*
					Workaround for bug in MediaTomb and gmediarender:
					Replace "schemas-upnp-org" by "upnp-org"
				*/
				s = strstr( service->serviceId, ":schemas-upnp-org:" );
				if ( s != NULL )
				{
					upnp_warning(( "incorrect serviceId = %s\n", service->serviceId ));
					memmove( s + 1, s + 9, strlen( s + 9 ) + 1 );
					buffer -= 8;
					upnp_warning(( "corrected serviceId = %s\n", service->serviceId ));
				}

				/*
					Workaround for bug in gmediarender:
					Replace "service" by "serviceId"
				*/
				s = strstr( service->serviceId, ":service:" );
				if ( s != NULL )
				{
					upnp_warning(( "incorrect serviceId = %s\n", service->serviceId ));
					buffer += 2;
					memmove( s + 10, s + 8, strlen( s + 8 ) + 1 );
					s[8] = 'I'; s[9] = 'd';
					upnp_warning(( "corrected serviceId = %s\n", service->serviceId ));
				}
			}
		}
		else if ( strcmp( name, "SCPDURL" ) == 0 )
			service->SCPDURL = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "controlURL" ) == 0 )
			service->controlURL = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "eventSubURL" ) == 0 )
			service->eventSubURL = upnp_stpcpy( &buffer, s );
		else
			upnp_warning(( "get_upnp_description(): Unsupported service element \"%s\"\n", name ));
	}

	return buffer;
}

static char *get_device_description( upnp_device *device, char *content, char *buffer )
{
	char *name, *s;

	device->friendlyName = "??";

	while ( (s = xml_unbox( &content, &name, NULL )) != NULL )
	{
		upnp_debug(( "  %s = \"%s\"\n", name, s ));

		if ( strcmp( name, "deviceType" ) == 0 )
			device->deviceType = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "friendlyName" ) == 0 )
			device->friendlyName = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "manufacturer" ) == 0 )
			device->manufacturer = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "manufacturerURL" ) == 0 )
			device->manufacturerURL = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "modelDescription" ) == 0 )
			device->modelDescription = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "modelName" ) == 0 )
			device->modelName = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "modelNumber" ) == 0 )
			device->modelNumber = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "modelURL" ) == 0 )
			device->modelURL = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "presentationURL" ) == 0 )
			device->presentationURL = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "serialNumber" ) == 0 )
			device->serialNumber = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "UDN" ) == 0 )
			device->UDN = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "UPC" ) == 0 )
			device->UPC = upnp_stpcpy( &buffer, s );
		else if ( strcmp( name, "serviceList" ) == 0 )
		{
			char *xcontent = s;
			while ( (s = xml_unbox( &xcontent, &name, NULL )) != NULL )
			{
				if ( strcmp( name, "service" ) == 0 )
				{
					upnp_service *service;

					while ( ((unsigned long)buffer & 7) != 0 ) buffer++;
					service = (upnp_service *)buffer;
					buffer += sizeof( upnp_service );

					memset( service, 0, sizeof( upnp_service ) );
					service->device = device;
					buffer = get_service_description( service, s, buffer );

					upnp_assert( offsetof( upnp_service, next ) == 0 );
					list_append( &device->serviceList, service );
				}
				else
					upnp_warning(( "get_upnp_description(): Unsupported serviceList element \"%s\"\n", name ));
			}
		}
		else if ( strcmp( name, "deviceList" ) == 0 )
		{
			char *xcontent = s;
			while ( (s = xml_unbox( &xcontent, &name, NULL )) != NULL )
			{
				if ( strcmp( name, "device" ) == 0 )
				{
					upnp_device *subdevice;

					while ( ((unsigned long)buffer & 7) != 0 ) buffer++;
					subdevice = (upnp_device *)buffer;
					buffer += sizeof( upnp_device );

					memset( subdevice, 0, sizeof( upnp_device ) );
					subdevice->parent = device;
					subdevice->URLBase = device->URLBase;
					buffer = get_device_description( subdevice, s, buffer );

					upnp_assert( offsetof( upnp_device, next ) == 0 );
					list_append( &device->deviceList, subdevice );
				}
				else
					upnp_warning(( "get_upnp_description(): Unsupported deviceList element \"%s\"\n", name ));
			}
		}
		else
		{
			upnp_warning(( "get_upnp_description(): Unsupported device element \"%s\"\n", name ));
		}
	}

	return buffer;
}

static gpointer get_upnp_description( gpointer data )
{
	upnp_device *device = (upnp_device *)data;
	char *content = NULL;
	gboolean success;

	upnp_message(( "upnp_get_description( %s )...\n", device->LOCATION ));

	success = http_get( device->LOCATION, DESCRIPTION_BUFFER_SIZE, NULL, &content, NULL, NULL );
	upnp_message(( "upnp_get_description(): http_get( %s ) = %s, %p\n", device->LOCATION, (success) ? "TRUE" : "FALSE", content ));

	if ( success && content != NULL && (!g_thread_supported() || device->_timer != NULL /* no timeout */) )
	{
		char *buffer, *name, *s = content;

	#if 0
		char filename[128];
		static int x;
		FILE *fp;

		sprintf( filename, "device_%d.xml", ++x );
		if ( (fp = fopen( filename, "wb" )) != NULL )
		{
			fputs( content, fp );
			fclose( fp );
		}
	#endif

		device->_buffer = g_new( char, strlen( content ) );
		upnp_assert( device->_buffer != NULL );
		buffer = device->_buffer;

		s = xml_unbox( &s, &name, NULL );
		if ( s != NULL && strcmp( name, "root" ) == 0 )
		{
			char *t = s;

			while ( (s = xml_unbox( &t, &name, NULL )) != NULL )
			{
				if ( strcmp( name, "specVersion" ) == 0 )
				{
					upnp_debug(( "%s = \"%s\"\n", name, s ));
					;  /* TODO: Check if "<major>1</major><minor>0</minor>" */
				}
				else if ( strcmp( name, "URLBase" ) == 0 )
				{
					upnp_debug(( "%s = \"%s\"\n", name, s ));
					device->URLBase = upnp_stpcpy( &buffer, s );
				}
				else if ( strcmp( name, "device" ) == 0 )
				{
					if ( device->URLBase == NULL )
					{
						strcpy( buffer, device->LOCATION );
						device->URLBase = buffer;
						buffer = strrchr( device->URLBase, '/' );       /* last '/' */
						upnp_assert( buffer != NULL );
						if ( buffer == NULL ) break;
						buffer++;  /* skip '/' */
						*buffer++ = '\0';

						upnp_debug(( "device->URLBase = \"%s\"\n", device->URLBase ));
					}

					buffer = get_device_description( device, s, buffer );
				}
				else
				{
					upnp_warning(( "get_upnp_description(): Unknown root element \"%s\"\n", name ));
				}
			}

			/* TODO: g_realloc( device->_buffer, buffer - device->_buffer ); */

			if ( device->deviceType != NULL )
				upnp_message(( "upnp_get_description( %s ) ...ok, deviceType = %s\n", device->LOCATION, device->deviceType ));
			else
				upnp_warning(( "upnp_get_description( %s ) ...failed, deviceType = <NULL>\n", device->LOCATION ));
		}
		else
		{
			upnp_warning(( "get_upnp_description(): No root element found\n" ));
			g_free( device->_buffer );
			device->_buffer = NULL;
		}

		device->_got_description = TRUE;
	}
	else
	{
		upnp_warning(( "upnp_get_description( %s ) failed (success = %s)\n", device->LOCATION, success ? "TRUE" : "FALSE" ));
	}

	upnp_free_string( content );
	return NULL;
}

/* Do an UPnP search */
gboolean upnp_search_devices( const char *stv[], int stc,
	gboolean (*callback)( struct _GTimer *timer, void *user_data ), void *user_data,
	GError **error )
{
	gboolean result = TRUE;
	struct sockaddr_in addr;
	struct buffer *buffer;
	SOCKET s;
	int err;

	upnp_device *device;
	GTimer *timer, *mx_timer;
	int do_2nd_attempt;

	buffer = alloc_buffer( SSDP_BUFFER_SIZE, 0, "upnp_search_devices()", error );
	if ( buffer == NULL )
	{
		upnp_warning(( "get_upnp_description(): alloc_buffer() failed\n" ));
		return FALSE;
	}

	upnp_message(( "upnp_search_devices()...\n" ));

	s = new_socket( SOCK_DGRAM );
	init_sockaddr( &addr );
	err = bind( s, (struct sockaddr*)&addr, sizeof( struct sockaddr_in ) );
	if ( err == 0 )
	{
/*		unsigned long if_addr = htonl( INADDR_ANY ); */
		int mx;

		upnp_debug(( "bind() = %d\n", err ));

/*		err = setsockopt( s, IPPROTO_IP, IP_MULTICAST_IF, (char*)&if_addr, sizeof( if_addr ) ); */
/*		upnp_debug(( "setsockopt( IPPROTO_IP, IP_MULTICAST_IF ) = %d\n", err )); */

		err = setsockopt( s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&SSDP_TTL, sizeof( SSDP_TTL ) );
		upnp_debug(( "setsockopt( IPPROTO_IP, IP_MULTICAST_TTL ) = %d\n", err ));

		err = setsockopt( s, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&SSDP_LOOP, sizeof( SSDP_LOOP ) );
		upnp_debug(( "setsockopt( IPPROTO_IP, IP_MULTICAST_LOOP ) = %d\n", err ));

		set_sockaddr( &addr, "239.255.255.250", 1900 );

		timer = g_timer_new();
		mx = SSDP_MX;

		do
		{
			err = m_search( stv, stc, mx, s, buffer, &addr );
			if ( err < 0 )
			{
				set_upnp_error( error, err, "Sending data over network failed" );
				result = FALSE;
				break;
			}

			mx_timer = g_timer_new();
			do_2nd_attempt = (SSDP_2ND_ATTEMPT != 0.0);

			for (;;)
			{
				err = upnp_recvfrom( s, buffer, 250 /* ms */, NULL );
				if ( err == UPNP_ERR_TIMEOUT )
				{
					gdouble elapsed = g_timer_elapsed( mx_timer, NULL );
					upnp_message(( "UPNP_ERR_TIMEOUT, elapsed = %f\n", elapsed ));

					if ( elapsed > (mx + 1) + SSDP_2ND_ATTEMPT )
					{
						/* Are still threads running? */
						for ( device = deviceList; device != NULL; device = device->next )
						{
							/* Still inside get_upnp_description()? */
							if ( device->_timer != NULL && !device->_got_description )
							{
								upnp_device *d;

								upnp_message(( "upnp_search_devices(): Still trying to get %s...\n", device->LOCATION ));

								/* If yes: Check if we already have the device description
								   from a different IP address */
								for ( d = deviceList; d != NULL; d = d->next )
								{
									if ( d != device && strcmp( d->USN, device->USN ) == 0 && d->_got_description )
									{
										upnp_message(( "upnp_search_devices(): ...but we use %s instead\n", d->LOCATION ));
										break;
									}
								}
								if ( d == NULL )
								{
									/* No, so we check if SSDP_GET_DESCRIPTION_TIMEOUT is already elapsed */
									elapsed = g_timer_elapsed( device->_timer, NULL );
									upnp_warning(( "upnp_search_devices(): \"%s\" = %fs\n", device->LOCATION, elapsed ));

									if ( elapsed < SSDP_GET_DESCRIPTION_TIMEOUT ) break;  /* So we are still waiting... */
								}
							}
						}

						if ( device == NULL ) break;  /* All threads are either finished or exhausted */
					}
				}
				else if ( err == 0 )
				{
					const char *ST, *USN, *LOCATION;

				#if LOGLEVEL >= 4 && LOGLEVEL < 5  /* otherwise upnp_recvfrom() will do this */
					upnp_debug(( "\n%s\n", buffer->addr ));
				/*
					HTTP/1.1 200 OK
					EXT: 
					SERVER: RTEMS/4.7.99, UPnP/1.0, Linn UPnP Stack/2.0
					CACHE-CONTROL: max-age=1800
					LOCATION: http://192.168.0.10:55178/Ds/device.xml
					ST: urn:linn-co-uk:service:Media:1
					USN: uuid:4c494e4e-0050-c221-75a9-10000003013f::urn:linn-co-uk:service:Media:1
				*/
				#endif

					ST       = get_http_header_field( buffer, "ST" );
					USN      = get_http_header_field( buffer, "USN" );
					LOCATION = get_http_header_field( buffer, "LOCATION" );

				#if LOGLEVEL > 1 && LOGLEVEL < 3
					upnp_message(( "ST = \"%s\"\n", (ST != NULL) ? ST : "<NULL>" ));
					upnp_message(( "USN = \"%s\"\n", (USN != NULL) ? USN : "<NULL>" ));
					upnp_message(( "LOCATION = \"%s\"\n", (LOCATION != NULL) ? LOCATION : "<NULL>" ));
				#endif

					/* TODO: Evaluate ST for making sure that this is a response to our search */

					if ( ST != NULL && USN != NULL && LOCATION != NULL &&
					     strstr( LOCATION, "127.0.0.1" ) == NULL /* Workaround for Twonky5 bug */ )
					{
						upnp_add_device( USN, LOCATION, error );
					}
					else
					{
						upnp_warning(( "upnp_search_devices(): invalid answer received\n" ));
					}
				}
				else
				{
					upnp_warning(( "upnp_search_devices(): upnp_recvfrom() = %d\n", err ));
					break;
				}

				if ( do_2nd_attempt )  /* no 2nd UDP transmission done yet */
				{
					gdouble elapsed = g_timer_elapsed( mx_timer, NULL );
					upnp_message(( "elapsed = %f\n", elapsed ));

					if ( elapsed >= SSDP_2ND_ATTEMPT )
					{
						/* second UCP transmission... */
						int _err = m_search( stv, stc, mx, s, buffer, &addr );
						if ( _err < 0 ) { err = _err; break; }
						do_2nd_attempt = FALSE;  /* ...done */

						upnp_message(( "reset timer...\n" ));
						g_timer_reset( mx_timer );
					}
				}
			}

			g_timer_destroy( mx_timer );

			if ( callback == NULL || (*callback)( timer, user_data ) ) break;

			mx++;
		}
		while ( SSDP_TIMEOUT == 0.0 || g_timer_elapsed( timer, NULL ) < SSDP_TIMEOUT );

		g_timer_destroy( timer );

		for ( device = deviceList; device != NULL; device = device->next )
		{
			if ( device->_timer != NULL )
			{
				g_timer_destroy( device->_timer );
				device->_timer = NULL;
			}
		}
	}
	else
	{
		set_socket_error( error, "Associating a local address with a socket failed", NULL );
		result = FALSE;
	}

	closesocket( s );
	release_buffer( buffer );

	upnp_message(( "upnp_search_devices() = %s\n", (result) ? "TRUE" : "FALSE" ));
	return result;
}

/* Add device to list */
upnp_device *upnp_add_device( const char *USN, const char *LOCATION, GError **error )
{
	upnp_device *device;

	upnp_assert( USN != NULL && LOCATION != NULL );
	if ( USN == NULL || LOCATION == NULL )
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_INVALID_PARAMS, "Invalid USN or LOCATION" );
		return NULL;
	}

	for ( device = deviceList; device != NULL; device = device->next )
	{
		if ( strcmp( device->LOCATION, LOCATION ) == 0 ) break;
	}
	if ( device == NULL )
	{
		char *s;

		upnp_message(( "upnp_add_device(): get_description( \"%s\" )...\n", LOCATION ));

		device = (upnp_device *)g_malloc0( sizeof( upnp_device ) + strlen( USN ) + strlen( LOCATION ) + 2 );
		upnp_assert( device != NULL );

		s = (char *)(device + 1);
		device->USN      = upnp_stpcpy( &s, USN );
		device->LOCATION = upnp_stpcpy( &s, LOCATION );

		upnp_assert( offsetof( upnp_device, next ) == 0 );
		list_prepend( &deviceList, device );

		if ( g_thread_supported() )
		{
			device->_timer = g_timer_new();
			g_thread_create( get_upnp_description, device, FALSE, NULL );
		}
		else
		{
			get_upnp_description( device );
		}
	}

	return device;
}

/* Remove device from list */
void upnp_remove_device( const char *USN )
{
	upnp_device *device;

	upnp_assert( USN != NULL );
	if ( USN == NULL ) return;

	for ( device = upnp_get_first_device( NULL ); device != NULL; device = upnp_get_next_device( device, NULL ) )
	{
		if ( strcmp( device->USN, USN ) == 0 )
		{
			upnp_device_mark_invalid( device );
		}
	}
}

/* Free all devices */
static void free_device_data( upnp_device *device )
{
	upnp_assert( device != NULL );

	if ( device->free_user_data != NULL )
	{
		(*device->free_user_data)( device->user_data );
		device->user_data = NULL;
	}

	g_free( device->_buffer );
	device->_buffer = NULL;
}

void upnp_free_all_devices( void )
{
	upnp_device *device = deviceList;
	deviceList = NULL;

	while ( device != NULL )
	{
		upnp_device *next = device->next;

		free_device_data( device );
		g_free( device );

		device = next;
	}
}

gboolean upnp_device_is_valid( const upnp_device *device )
{
	return (device != NULL && device->_got_description && device->deviceType != NULL );
}

void upnp_device_mark_invalid( upnp_device *device )
{
	upnp_assert( upnp_device_is_valid( device ) );

	if ( upnp_device_is_valid( device ) )
	{
		device->deviceType = NULL;
		device->deviceList = NULL;
		device->serviceList = NULL;

		free_device_data( device );
	}
}

/* Test if the given device is the desired UPnP device or contains the desired service */
gboolean upnp_device_is( const upnp_device *device, const char *type_or_id )
{
	upnp_assert( type_or_id != NULL );

	if ( device != NULL )
	{
		if ( strstr( type_or_id, ":device:" ) != NULL )
		{
			if ( device->deviceType != NULL && strcmp( device->deviceType, type_or_id ) == 0 )
				return TRUE;
		}
		else if ( strstr( type_or_id, ":service:" ) != NULL )
		{
			const upnp_service *service;

			for ( service = device->serviceList; service != NULL; service = service->next )
			{
				if ( service->serviceType != NULL && strcmp( service->serviceType, type_or_id ) == 0 )
					return TRUE;
			}
		}
		else if ( strstr( type_or_id, ":serviceId:" ) != NULL )
		{
			const upnp_service *service;

			for ( service = device->serviceList; service != NULL; service = service->next )
			{
				if ( service->serviceId != NULL && strcmp( service->serviceId, type_or_id ) == 0 )
					return TRUE;
			}
		}
		else
		{
			upnp_error(( "upnp_device_is(): unknown type \"%s\"\n", type_or_id ));
		}
	}

	return FALSE;
}

/* Test if the given service is the desired one */
gboolean upnp_service_is( const upnp_service *service, const char *type_or_id )
{
	upnp_assert( type_or_id != NULL );

	if ( service != NULL )
	{
		if ( strstr( type_or_id, ":service:" ) != NULL )
		{
			if ( service->serviceType != NULL && strcmp( service->serviceType, type_or_id ) == 0 )
				return TRUE;
		}
		else if ( strstr( type_or_id, ":serviceId:" ) != NULL )
		{
			if ( service->serviceId != NULL && strcmp( service->serviceId, type_or_id ) == 0 )
				return TRUE;
		}
		else
		{
			upnp_error(( "upnp_service_is(): unknown type \"%s\"\n", type_or_id ));
		}
	}

	return FALSE;
}

upnp_service *upnp_get_service( const upnp_device *device, const char *service_type_or_id )
{
	upnp_service *service;

	upnp_assert( device != NULL );
	upnp_assert( service_type_or_id != NULL );

	if ( device == NULL )
	{
		upnp_warning(( "upnp_get_service(): device == NULL\n" ));
		return NULL;
	}

	for ( service = device->serviceList; service != NULL; service = service->next )
	{
		if ( upnp_service_is( service, service_type_or_id ) ) break;
	}

	if ( service == NULL )
	{
		upnp_warning(( "upnp_get_service(): service \"%s\" not found\n", service_type_or_id ));
	}

	return service;
}

char *upnp_get_allowedValueRange( const upnp_device *device, const char *service_type_or_id, const char *state_variable_name, GError **error )
{
	const upnp_service *service = upnp_get_service( device, service_type_or_id );
	char url[UPNP_MAX_URL];

	upnp_debug(( "upnp_get_allowedValueRange( %p, \"%s\", \"%s\" )\n", (void*)device, service_type_or_id, state_variable_name ));

	if ( service != NULL && build_url( device, service->SCPDURL, url ) != NULL )
	{
		char *content = NULL;

		if ( http_get( url, 0, NULL, &content, NULL, error ) )
		{
			char *s = content;
			char *name;

			s = xml_unbox( &s, &name, NULL );
			upnp_assert( s != NULL && strcmp( name, "scpd" ) == 0 );
			if ( s != NULL && strcmp( name, "scpd" ) == 0 )
			{
				xml_iter iter;

				for ( s = xml_first( s, &iter ); s != NULL; s = xml_next( &iter ) )
				{
					s = xml_unbox( &s, &name, NULL );
					if ( s == NULL ) break;

					if ( strcmp( name, "serviceStateTable" ) == 0 )
					{
						xml_iter iter;

						for ( s = xml_first( s, &iter ); s != NULL; s = xml_next( &iter ) )
						{
							s = xml_unbox( &s, &name, NULL );
							if ( s == NULL ) break;

							if ( strcmp( name, "stateVariable" ) == 0 )
							{
								xml_iter iter;

								for ( s = xml_first( s, &iter ); s != NULL; s = xml_next( &iter ) )
								{
									s = xml_unbox( &s, &name, NULL );
									if ( s == NULL ) break;

									if ( strcmp( name, "name" ) == 0 && strcmp( s, state_variable_name ) != 0 ) break;

									if ( strcmp( name, "allowedValueRange" ) == 0 )
									{
										upnp_debug(( "upnp_get_allowedValueRange() = \"%s\"\n", s ));
										return s;
									}
								}
							}
						}
					}
				}
			}

			g_set_error( error, UPNP_ERROR, UPNP_ERR_NO_VALID_XML, "Invalid answer from UPnP service %s", service_type_or_id );
		}

		upnp_free_string( content );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, "UPnP service %s not found", service_type_or_id );
	}

	upnp_debug(( "upnp_get_allowedValueRange() = NULL\n" ));
	return NULL;
}

static upnp_device *get_first_or_next_device( const upnp_device *device, const char *type_or_id, int next )
{
	for (;;)
	{
		if ( next )
		{
			upnp_assert( device != NULL );

			/* Search sub-devices first */
			if ( device->deviceList != NULL )
			{
				upnp_device *subdevice = get_first_or_next_device( device->deviceList, type_or_id, FALSE );
				if ( subdevice != NULL ) return subdevice;
			}

			if ( device->next == NULL && device->parent != NULL )
				device = device->parent;

			/* Try next device afterwards */
			device = device->next;
		}

		if ( device == NULL ) break;

		if ( upnp_device_is_valid( device ) )
		{
			if ( type_or_id == NULL || upnp_device_is( device, type_or_id ) )
				return (upnp_device*)device;
		}

		next = TRUE;
	}

	return NULL;
}

upnp_device *upnp_get_first_device( const char *type_or_id )
{
	return get_first_or_next_device( deviceList, type_or_id, FALSE );
}

upnp_device *upnp_get_next_device( const upnp_device *device, const char *type_or_id )
{
	return get_first_or_next_device( device, type_or_id, TRUE );
}

int upnp_num_devices( const char *type_or_id )
{
	const upnp_device *device;
	int result = 0;

	for ( device = upnp_get_first_device( type_or_id ); device != NULL; device = upnp_get_next_device( device, type_or_id ) )
	{
		result++;
	}
	return result;
}

char *_upnp_get_ip_address( const char *url, char addr[UPNP_MAX_HOST_ADDRESS], unsigned *port )
{
	struct sockaddr_in tmp;

	if ( http_url( url, addr, &tmp, NULL ) != NULL )
	{
		if ( port != NULL ) *port = tmp.sin_port;
		return get_sockaddr( addr, &tmp );
	}

	return NULL;
}

/* Return the IP address or the given search result (or NULL if not available) */
/*char *_upnp_get_ip_address( const upnp_device *device, char *value, size_t sizeof_value )
{
	char addr[UPNP_MAX_HOST_ADDRESS];

	if ( device != NULL && _upnp_get_ip_address( device->URLBase, addr, NULL ) != NULL )
	{
		return upnp_strcpy( "ip_address", value, sizeof_value, addr );
	}

	return NULL;
}*/
gchar *upnp_get_ip_address( const upnp_device *device )
{
	char addr[UPNP_MAX_HOST_ADDRESS];

	if ( device != NULL && _upnp_get_ip_address( device->URLBase, addr, NULL ) != NULL )
	{
		return g_strdup( addr );
	}

	return NULL;
}

unsigned upnp_get_ip_port( const upnp_device *device )
{
	char tmp[UPNP_MAX_HOST_ADDRESS];
	unsigned port;

	if ( device != NULL && _upnp_get_ip_address( device->URLBase, tmp, &port ) != NULL )
	{
		return port;
	}

	return 0;
}

/*--- SOAP --------------------------------------------------------------------------------------*/

static void soap_error( const char *serviceTypeOrId, const char *action, const struct buffer *buffer, GError **error )
{
	if ( error != NULL && *error != NULL )
	{
		gchar *faultcode = NULL;
		gchar *faultstring = NULL;
		gchar *errorCode = NULL;
		gchar *errorDescription = NULL;
		GString *message;

		char *content = get_http_content( buffer );
		if ( content != NULL )
		{
			char *s = strstr( content, "<s:Fault" );
			if ( s == NULL ) s = strstr( content, "<SOAP-ENV:Fault" );
			if ( s != NULL && (s = xml_unbox( &s, NULL, NULL )) != NULL )
			{
				faultcode = xml_get_named_str( s, "faultcode" );
				faultstring = xml_get_named_str( s, "faultstring" );
				errorCode = xml_get_named_str( s, "errorCode" );
				errorDescription = xml_get_named_str( s, "errorDescription" );
			}
		}

		if ( errorCode != NULL )
		{
			(*error)->code = atoi( errorCode );
			g_free( errorCode );
		}

		message = g_string_new( ( errorDescription != NULL ) ? errorDescription : (*error)->message );
		g_free( errorDescription );

		upnp_assert( serviceTypeOrId != NULL && action != NULL );
		g_string_append_printf( message, "\n\nservice = %s\naction = %s", serviceTypeOrId, action );

		if ( faultcode != NULL )
		{
			g_string_append_printf( message, "\nfaultcode = %s", faultcode );
			g_free( faultcode );
		}

		if ( faultstring != NULL )
		{
			g_string_append_printf( message, "\nfaultstring = %s", faultstring );
			g_free( faultstring );
		}

		g_free( (*error)->message );
		(*error)->message = g_string_free( message, FALSE );

		upnp_warning(( "SOAP error %d \"%s\"\n", (*error)->code, (*error)->message ));
	}
}

/* Do a UPnP action */
gboolean upnp_action( const upnp_device *device, const char *serviceTypeOrId,
                      const upnp_arg inv[], upnp_arg outv[], int *outc,
                      GError **error )
{
	char url[UPNP_MAX_URL], host[UPNP_MAX_HOST_ADDRESS];
	const upnp_service *service;
	struct sockaddr_in addr;
	struct buffer *buffer;
	const char *path;
	int i;

	upnp_assert( serviceTypeOrId != NULL && inv != NULL );
	upnp_debug(( "upnp_action( %s, \"%s\" )...\n", serviceTypeOrId, inv->name ));

	if ( outv != NULL ) outv->value = NULL;

	if ( device == NULL )
	{
		upnp_warning(( "upnp_action( \"%s\"): device == NULL\n", inv->name ));
		set_upnp_error( error, UPNP_ERR_DEVICE_NOT_FOUND, "UPnP device not found" );
		soap_error( serviceTypeOrId, inv->name, NULL, error );
		return FALSE;
	}

	service = upnp_get_service( device, serviceTypeOrId );
	if ( service == NULL )
	{
		upnp_warning(( "upnp_action( \"%s\" ): service \"%s\" not found\n", inv->name, serviceTypeOrId ));
		set_upnp_error( error, UPNP_ERR_SERVICE_NOT_FOUND, "UPnP service not found" );
		soap_error( serviceTypeOrId, inv->name, NULL, error );
		return FALSE;
	}

	path = http_url( build_url( device, service->controlURL, url ), host, &addr, error );
	if ( path == NULL )
	{
		upnp_warning(( "upnp_action( \"%s::%s\" ): controlURL not available\n", serviceTypeOrId, inv->name ));
		soap_error( serviceTypeOrId, inv->name, NULL, error );
		return FALSE;
	}

	upnp_debug(( "upnp_action(): path = %s\n", path ));
	upnp_debug(( "upnp_action(): serviceType = %s\n", service->serviceType ));

	buffer = alloc_buffer( SOAP_BUFFER_SIZE, 0, inv->name, error );
	if ( buffer == NULL )
	{
		upnp_warning(( "upnp_action(): alloc_buffer() failed\n" ));
		soap_error( serviceTypeOrId, inv->name, NULL, error );
		return FALSE;
	}

	prepare_for_upnp_send( buffer );

	/* Content */
	append_string_to_buffer( buffer,
	/*	"<?xml version=\"1.0\"?>" */
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
		" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
			"<s:Body>" "<u:" );
	append_string_to_buffer( buffer, inv->name );
	append_string_to_buffer( buffer, " xmlns:u=\"" );
	append_string_to_buffer( buffer, service->serviceType );
	append_string_to_buffer( buffer, "\">" );

	for ( i = 1; i < inv->u.argc; i++ )
	{
	#if LOGLEVEL >= 4
		upnp_debug(( "in[%d]: %s = \"%s\"\n", i, inv[i].name, inv[i].value ));
	#endif

		append_char_to_buffer( buffer, '<' );
		append_string_to_buffer( buffer, inv[i].name );
		append_char_to_buffer( buffer, '>' );

		xml_wrap_delimiters_to_buffer( buffer, inv[i].value );

		append_char_to_buffer( buffer, '<' );
		append_char_to_buffer( buffer, '/' );
		append_string_to_buffer( buffer, inv[i].name );
		append_char_to_buffer( buffer, '>' );
	}

	append_string_to_buffer( buffer, "</u:" );
	append_string_to_buffer( buffer, inv->name );
	append_string_to_buffer( buffer, ">" "</s:Body>" "</s:Envelope>" );

	/* Header */
	sprintf( buffer->addr,
		"POST %s HTTP/" HTTP_VERSION "\r\n"
		"HOST: %s\r\n"
		"CONTENT-LENGTH: %lu\r\n"
		"CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
		"SOAPACTION: \"%s#%s\"\r\n"
		"\r\n",
		path, host, upnp_content_length( buffer ),
		service->serviceType, inv->name );

	/* HTTP */
	if ( upnp_connect_send_recv( &addr, buffer, error ) != 0 )
	{
		upnp_warning(( "upnp_action( \"%s::%s\" ): upnp_connect_send_recv() failed\n", serviceTypeOrId, inv->name ));
		soap_error( serviceTypeOrId, inv->name, buffer, error );
		release_buffer( buffer );
		return FALSE;
	}

	if ( outv != NULL && outc != NULL && *outc > 0 )
	{
		const char *content = get_http_content( buffer );
		char *s;

		memset( outv, 0, sizeof( upnp_arg ) * *outc );

		outv->name = inv->name;  /* for debugging purposes */
		outv->value = (char*)buffer;
		outv->u.argc = 1;
		outv++;

		/* Get response, could be coded as <u:...> or <m:...> or <ns0:...> or ... */
		upnp_assert( content != NULL );
		sprintf( buffer->addr, ":%sResponse", inv->name );
		s = strstr( content, buffer->addr );
		if ( s != NULL )
		{
			while ( *--s != '<' ) ;

			s = xml_unbox( &s, NULL, NULL );  /* get content of "<u:XxxResponse>" */
			if ( s != NULL )
			{
				for ( i = 1; i < *outc; i++ )
				{
					if ( (outv->value = xml_unbox( &s, (char**)&outv->name, NULL )) == NULL )
					{
						upnp_warning(( "upnp_action(): %d out values expected, but only %d found.\n",
							*outc, i ));
						break;
					}

				#if LOGLEVEL >= 4
					upnp_debug(( "out[%d]: %s = \"%s\"\n", i, outv->name, outv->value ));
				#endif

					outv++;
				}

				*outc = i;
			}
			else
			{
				upnp_warning(( "upnp_action(): xml_unbox(\"%s\") failed\n", buffer->addr + 1 ));
				set_upnp_error( error, UPNP_ERR_NO_VALID_XML, "Invalid UPnP Response" );
				soap_error( serviceTypeOrId, inv->name, NULL, error );
				release_buffer( buffer );
				return FALSE;
			}
		}
		else
		{
			upnp_warning(( "upnp_action(): \"%s\" not found\n", buffer->addr + 1 ));
			set_upnp_error( error, UPNP_ERR_NO_VALID_XML, "No UPnP Response" );
			soap_error( serviceTypeOrId, inv->name, NULL, error );
			release_buffer( buffer );
			return FALSE;
		}
	}
	else
	{
		upnp_debug(( "upnp_release_action( \"%s\" )\n", inv->name ));
		release_buffer( buffer );
	}

	upnp_debug(( "upnp_action( \"%s\" ) = TRUE [%d]\n", inv->name, (outc != NULL) ? *outc : 0 ));
	return TRUE;
}

/* Free the buffer which contains the action out-args */
void upnp_release_action( upnp_arg outv[] )
{
	upnp_debug(( "upnp_release_action( \"%s\" )\n", (outv != NULL) ? outv->name : "<null>" ));

	if ( outv != NULL && outv->value != NULL )
	{
		release_buffer( (struct buffer*)outv->value );
		outv->value = NULL;
	}
}

/* Sets the UPnP action name */
void upnp_set_action_name( upnp_arg inv[], const char *name )
{
	upnp_assert( inv != NULL && name != NULL );
	/*upnp_debug(( "upnp_set_action_name( \"%s\" )\n", name ));*/

	inv->name = name;
	inv->u.argc = 1;
}

void upnp_add_boolean_arg( upnp_arg inv[], const char *name, upnp_boolean value )
{
	upnp_add_string_arg( inv, name, (value) ? "1" : "0" );
}

void upnp_add_i2_arg( upnp_arg inv[], const char *name, upnp_i2 value )
{
	upnp_add_i4_arg( inv, name, value );
}

void upnp_add_ui2_arg( upnp_arg inv[], const char *name, upnp_ui2 value )
{
	upnp_add_ui4_arg( inv, name, value );
}

void upnp_add_i4_arg( upnp_arg inv[], const char *name, upnp_i4 value )
{
	upnp_arg *in = inv + inv->u.argc++;

	upnp_assert( inv != NULL && name != NULL );

	in->name = name;
	/*in->value = ltoa( value, in->value_buf, 10 );*/
	sprintf( in->u.value_buf, "%d", value );
	in->value = in->u.value_buf;
}

void upnp_add_ui4_arg( upnp_arg inv[], const char *name, upnp_ui4 value )
{
	upnp_arg *in = inv + inv->u.argc++;

	upnp_assert( inv != NULL && name != NULL );

	in->name = name;
	/*in->value = ultoa( value, in->value_buf, 10 );*/
	sprintf( in->u.value_buf, "%u", value );
	in->value = in->u.value_buf;
}

void upnp_add_string_arg( upnp_arg inv[], const char *name, const char *value )
{
	upnp_arg *in = inv + inv->u.argc++;

	upnp_assert( inv != NULL && name != NULL );
	upnp_assert( value != NULL );

	in->name = name;
	in->value = (value != NULL) ? value : "";
}

upnp_boolean upnp_get_boolean_value( upnp_arg outv[], int index )
{
	/* UPnP-arch-DeviceArchitecture-v1 1-20081015.pdf:
	  "boolean: "0" for false or "1" for true.
	   The values "true", "yes", "false", or "no" are deprecated
	   and MUST NOT be sent but MUST be accepted when received.
	   When received, the values "true" and "yes" MUST be interpreted as true
	   and the values "false" and "no" MUST be interpreted as false."
	*/

	/*upnp_arg *out = outv + outv->u.argc++;*/
	upnp_arg *out = outv + index;

	upnp_assert( outv != NULL && out->value != NULL );
	upnp_assert( out->value[0] != '\0' && strchr( "01tyfn", out->value[0] ) != NULL );
	return out->value[0] == '1' || out->value[0] == 't' || out->value[0] == 'y';
}

upnp_i2 upnp_get_i2_value( upnp_arg outv[], int index )
{
	return (upnp_i2)upnp_get_i4_value( outv, index );
}

upnp_ui2 upnp_get_ui2_value( upnp_arg outv[], int index )
{
	return (upnp_ui2)upnp_get_ui4_value( outv, index );
}

upnp_i4 upnp_get_i4_value( upnp_arg outv[], int index )
{
	/*upnp_arg *out = outv + outv->u.argc++;*/
	upnp_arg *out = outv + index;

	upnp_assert( outv != NULL && out->value != NULL );
	upnp_assert( out->value[0] != '\0' && strchr( "-0123456789", out->value[0] ) != NULL );
	return (upnp_i4)strtol( out->value, NULL, 10 );
}

upnp_ui4 upnp_get_ui4_value( upnp_arg outv[], int index )
{
	/*upnp_arg *out = outv + outv->u.argc++;*/
	upnp_arg *out = outv + index;

	upnp_assert( outv != NULL && out->value != NULL );
	upnp_assert( out->value[0] != '\0' && strchr( "0123456789", out->value[0] ) != NULL );
	return (upnp_ui4)strtoul( out->value, NULL, 10 );
}

upnp_string upnp_get_string_value( upnp_arg outv[], int index )
{
	/*upnp_arg *out = outv + outv->u.argc++;*/
	upnp_arg *out = outv + index;

	upnp_assert( outv != NULL && out->value != NULL );
	add_buffer_ref( get_action_buffer( outv ) );
	return (char*)out->value;
}

static struct buffer *get_upnp_buffer( upnp_string str )
{
	struct buffer *buf;

	g_mutex_lock( buffer_mutex );

	for ( buf = buffer_list; buf != NULL; buf = buf->next )
	{
		if ( buf->addr < str && str < buf->addr + buf->size )
		{
			break;
		}
	}

	g_mutex_unlock( buffer_mutex );

	return buf;
}

upnp_string upnp_shrink_to_fit( upnp_string str )
{
	struct buffer *buf = get_upnp_buffer( str );

	if ( buf != NULL )
	{
		if ( buf->refcount == 1 )
		{
			size_t offset = str - buf->addr;

			size_t new_size = offset + strlen( str ) + 1;
			upnp_assert( new_size < buf->size );

			resize_buffer( buf, new_size, NULL );

			str = buf->addr + offset;
		}
		else
		{
			upnp_warning(( "upnp_shrink_to_fit( %p ): refcount = %d\n", (void *)str, buf->refcount ));
		}
	}
	else
	{
		upnp_critical(( "upnp_shrink_to_fit( %p ): buffer not found\n", (void *)str ));
	}

	return str;
}

/* Free the buffer which contains the string 'str' is pointing to */
void upnp_free_string( upnp_string str )
{
	if ( str != NULL )
	{
		struct buffer *buf = get_upnp_buffer( str );

		if ( buf != NULL )
		{
			release_buffer( buf );
		}
		else
		{
			upnp_critical(( "upnp_free_string( %p ): buffer not found\n", (void*)str ));
		}
	}
}

/*--- GENA --------------------------------------------------------------------------------------*/

upnp_subscription *subscriptionList;
GMutex *gena_mutex;

static gboolean start_gena_server( GError **error );
static void stop_gena_server( void );
static void end_gena_server( void );

static const char *gena_get_path( upnp_subscription *subscription, char url[/*UPNP_MAX_URL*/], char host[/*UPNP_MAX_HOST_ADDRESS*/], struct sockaddr_in *addr, GError **error )
{
	const upnp_service *service;
	const char *path;

	upnp_assert( subscription != NULL );

	if ( subscription->device == NULL )
	{
		upnp_warning(( "gena_get_path(): device == NULL\n" ));
		g_set_error( error, UPNP_ERROR, UPNP_ERR_DEVICE_NOT_FOUND, "UPnP device %s not found", subscription->serviceTypeOrId );
		return NULL;
	}

	service = upnp_get_service( subscription->device, subscription->serviceTypeOrId );
	if ( service == NULL )
	{
		upnp_warning(( "gena_get_path(): service \"%s\" not found\n", subscription->serviceTypeOrId ));
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, "UPnP service %s not found", subscription->serviceTypeOrId );
		return NULL;
	}

	path = http_url( build_url( subscription->device, service->eventSubURL, url ), host, addr, error );
	if ( path == NULL )
	{
		upnp_warning(( "gena_get_path( \"%s\" ): eventSubURL not available\n", subscription->serviceTypeOrId ));
		return NULL;
	}

	return path;
}

static void gena_error( upnp_subscription *subscription, const char *method, GError **error )
{
	if ( error != NULL && *error != NULL )
	{
		gchar *message = g_strdup_printf( "%s\ndevice = %s\nservice = %s\nmethod = %s",
			(*error)->message,
			subscription->device->friendlyName, subscription->serviceTypeOrId,
			method );
		g_free( (*error)->message );
		(*error)->message = message;

		upnp_warning(( "GENA error %d \"%s\"\n", (*error)->code, (*error)->message ));
	}
}

/* Renew a subscription (This will be done automatically by timer) */
static gboolean gena_renew_subscription( upnp_subscription *subscription, GError **error )
{
	char url[UPNP_MAX_URL], host[UPNP_MAX_HOST_ADDRESS];
	struct sockaddr_in addr;
	struct buffer *buffer;
	const char *path;
	int err;

	upnp_assert( subscription != NULL );
	upnp_debug(( "gena_renew_subscription( %s, %s )...\n",
		(subscription->device != NULL) ? subscription->device->friendlyName : "<NULL>",
		(subscription->serviceTypeOrId) ? subscription->serviceTypeOrId : "<NULL>" ));

	path = gena_get_path( subscription, url, host, &addr, error );
	if ( path == NULL ) return FALSE;
	upnp_debug(( "gena_renew_subscription(): path = %s\n", path ));

	buffer = alloc_buffer( GENA_BUFFER_SIZE, 0, subscription->serviceTypeOrId, error );
	if ( buffer == NULL )
	{
		upnp_warning(( "gena_renew_subscription(): alloc_buffer() failed\n" ));
		return FALSE;
	}

	prepare_for_upnp_send( buffer );

	/* Header */
	err = sprintf( buffer->addr,
		"SUBSCRIBE %s HTTP/" HTTP_VERSION "\r\n"
		"HOST: %s\r\n"
		"SID: %s\r\n",
		path, host, subscription->SID );

	if ( subscription->timeout > 0 )
		err += sprintf( buffer->addr + err, "TIMEOUT: Second-%d\r\n", subscription->timeout );
	else if ( subscription->timeout == 0 )
		err += sprintf( buffer->addr + err, "TIMEOUT: infinite\r\n" );

	strcpy( buffer->addr + err, "\r\n" );

	err = upnp_connect_send_recv( &addr, buffer, error );
#if LOGLEVEL >= 4 && LOGLEVEL < 5  /* otherwise upnp_recv() will do this */
	upnp_debug(( "\n%s\n", buffer->addr ));
#endif

	release_buffer( buffer );

	upnp_debug(( "gena_renew_subscription() = %d\n", err ));
	gena_error( subscription, "subscribe", error );
	return err == 0;
}

/* The timer function which do the resubscribing */
static gboolean gena_timer( gpointer data )
{
	upnp_subscription *subscription;
	gboolean result = FALSE;  /* default: end timer */

	g_mutex_lock( gena_mutex );

	for ( subscription = subscriptionList; subscription != NULL; subscription = subscription->next )
	{
		if ( subscription == (upnp_subscription*)data && subscription->timeout_id != 0 )
		{
			result = gena_renew_subscription( subscription, NULL );
			break;
		}
	}

	g_mutex_unlock( gena_mutex );

	return result;
}

/* Subscribe to a UPnP service (internal) */
static gboolean gena_subscribe( upnp_subscription *subscription, struct buffer *buffer, gboolean lock_mutex, GError **error )
{
	gboolean result = FALSE;
	char url[UPNP_MAX_URL], host[UPNP_MAX_HOST_ADDRESS];
	struct sockaddr_in addr;
	const char *path;
	SOCKET s;
	int err;

	upnp_assert( subscription != NULL && buffer != NULL );
	upnp_debug(( "gena_subscribe( %s, %s )...\n",
		(subscription->device != NULL) ? subscription->device->friendlyName : "<NULL>",
		(subscription->serviceTypeOrId) ? subscription->serviceTypeOrId : "<NULL>" ));

	path = gena_get_path( subscription, url, host, &addr, error );
	if ( path == NULL ) return FALSE;
	upnp_debug(( "gena_subscribe(): path = %s\n", path ));

	prepare_for_upnp_send( buffer );

	s = new_socket( SOCK_STREAM );
	err = connect( s, (struct sockaddr*)&addr, sizeof( struct sockaddr_in ) );
	if ( err >= 0 )
	{
		struct sockaddr_in addr = {0};
		socklen_t addrlen = sizeof( addr );

		upnp_debug(( "connect() = %d\n", err ));

		err = getsockname( s, (struct sockaddr*)&addr, &addrlen );
		upnp_debug(( "gena_subscribe(): getsockname() = %d, addr = %s\n", err, inet_ntoa( addr.sin_addr ) ));
		get_sockaddr( subscription->sock_addr, &addr );

		/* Header */
		err = sprintf( buffer->addr,
			"SUBSCRIBE %s HTTP/" HTTP_VERSION "\r\n"
			"HOST: %s\r\n"
			"CALLBACK: <http://%s:%u/>\r\n"
			"NT: upnp:event\r\n",
			path, host, subscription->sock_addr, http_server_port );

		if ( subscription->timeout > 0 )
			err += sprintf( buffer->addr + err, "TIMEOUT: Second-%d\r\n", subscription->timeout );
		else if ( subscription->timeout == 0 )
			err += sprintf( buffer->addr + err, "TIMEOUT: infinite\r\n" );

		strcpy( buffer->addr + err, "\r\n" );

		if ( lock_mutex )
		{
			upnp_assert( gena_mutex != NULL );
			g_mutex_lock( gena_mutex );
		}

		err = upnp_send_recv( s, buffer, error );
		if ( err == 0 )
		{
			const char *SID, *TIMEOUT;

			subscription->SEQ = 0;

			SID = get_http_header_field( buffer, "SID" );
			upnp_message(( "gena_subscribe(): SID = \"%s\"\n", (SID != NULL) ? SID : "<NULL>" ));
			upnp_strcpy( "SID", subscription->SID, sizeof( subscription->SID ), (SID != NULL) ? SID : "<NULL>" );

			subscription->timeout_id = 0;
			TIMEOUT = get_http_header_field( buffer, "TIMEOUT" );
			upnp_message(( "gena_subscribe(): TIMEOUT = \"%s\"\n", (TIMEOUT != NULL) ? TIMEOUT : "<NULL>" ));
			if ( TIMEOUT != NULL && strnicmp( TIMEOUT, "Second-", 7 ) == 0 )
			{
				unsigned interval = atoi( TIMEOUT + 7 ) * 1000;  /* s => ms */
				if ( interval > 0 )
				{
					interval -= interval / 10;  /* Sicherheitsabstand */
					subscription->timeout_id = g_timeout_add_full( G_PRIORITY_LOW, interval, gena_timer, subscription, NULL );
					upnp_message(( "gena_subscribe(): timeout_id = %u\n", subscription->timeout_id ));
				}
			}

			result = TRUE;
		}
		else if ( lock_mutex )
		{

			upnp_assert( gena_mutex != NULL );
			g_mutex_unlock( gena_mutex );
		}

	#if LOGLEVEL >= 4 && LOGLEVEL < 5  /* otherwise upnp_recv() will do this */
		upnp_debug(( "\n%s\n", buffer->addr ));
	#endif
	}
	else
	{
		set_socket_error( error, "Can't connect to %s", &addr );
	}

	closesocket( s );

	upnp_debug(( "gena_subscribe() = %d\n", err ));
	gena_error( subscription, "subscribe", error );
	return result;
}

/* Unsubscribe from a UPnP service (internal) */
static gboolean gena_unsubscribe( upnp_subscription *subscription, struct buffer *buffer, GError **error )
{
	gboolean result = FALSE;
	guint timeout_id;

	upnp_assert( subscription != NULL );
	upnp_debug(( "gena_unsubscribe( %s, %s )...\n",
		(subscription->device != NULL) ? subscription->device->friendlyName : "<NULL>",
		(subscription->serviceTypeOrId) ? subscription->serviceTypeOrId : "<NULL>" ));

	timeout_id = subscription->timeout_id;
	subscription->timeout_id = 0;
	if ( timeout_id != 0 ) g_source_remove( timeout_id );

	if ( buffer != NULL )
	{
		char url[UPNP_MAX_URL], host[UPNP_MAX_HOST_ADDRESS];
		struct sockaddr_in addr;
		const char *path;
		int err;

		path = gena_get_path( subscription, url, host, &addr, error );
		if ( path == NULL ) return FALSE;
		upnp_debug(( "gena_unsubscribe(): path = %s\n", path ));

		prepare_for_upnp_send( buffer );

		/* Header */
		sprintf( buffer->addr,
			"UNSUBSCRIBE %s HTTP/" HTTP_VERSION "\r\n"
			"HOST: %s\r\n"
			"SID: %s\r\n"
			"\r\n",
			path, host, subscription->SID );

		err = upnp_connect_send_recv( &addr, buffer, error );
	#if LOGLEVEL >= 4 && LOGLEVEL < 5  /* otherwise upnp_recv() will do this */
		upnp_debug(( "\n%s\n", buffer->addr ));
	#endif
		if ( err == 0 ) result = TRUE;

		gena_error( subscription, "unsubscribe", error );
	}

	upnp_debug(( "gena_unsubscribe() = %s\n", (result) ? "TRUE" : "FALSE" ));
	return result;
}

/* Start the GENA server */
static gboolean start_gena_server( GError **error )
{
	if ( gena_mutex == NULL ) gena_mutex = g_mutex_new();
	return _http_start_server( GENA_SERVER_MASK, error );
}

/* Stop the GENA server */
static void stop_gena_server( void )
{
	_http_stop_server( GENA_SERVER_MASK );
}

/* End the GENA server thread */
static void end_gena_server( void )
{
	_http_end_server( GENA_SERVER_MASK );
}

/* The GENA server which handles the UPnP eventing */
/* Handles an incoming GENA server connection */
static const char *gena_notify_func( SOCKET s, struct buffer *buffer )
{
	const char *NT, *NTS, *SID, *SEQ;
	const char *http_result;

	if ( strstr( buffer->addr, "HTTP/1." ) != NULL &&
	     ( NT = get_http_header_field( buffer, "NT"  )) != NULL &&
	     (NTS = get_http_header_field( buffer, "NTS" )) != NULL &&
	     (SID = get_http_header_field( buffer, "SID" )) != NULL &&
	     (SEQ = get_http_header_field( buffer, "SEQ" )) != NULL )
	{
		http_result = "412 Precondition Failed";

		if ( strcmp( NT, "upnp:event" ) == 0 && strcmp( NTS, "upnp:propchange" ) == 0 )
		{
			upnp_subscription *subscription;

			g_mutex_lock( gena_mutex );

			for ( subscription = subscriptionList; subscription != NULL; subscription = subscription->next )
			{
				if ( strcmp( subscription->SID, SID ) == 0 )
				{
					gboolean success;

					g_mutex_lock( subscription->mutex );

					if ( strtoul( SEQ, NULL, 10 ) == subscription->SEQ )
					{
						char *s = get_http_content( buffer );
						upnp_assert( s != NULL );

						if ( s != NULL && (s = strstr( s, "<e:propertyset" )) != NULL &&
							(s = xml_unbox( &s, NULL, NULL )) != NULL )
						{
							success = (*subscription->callback)( subscription, s );
							if ( success ) add_buffer_ref( buffer );  /* do not free buffer */
						}
						else
						{
							success = FALSE;
						}

						if ( ++subscription->SEQ == 0 ) subscription->SEQ++;  /* handle overflow */
					}
					else
					{
						upnp_warning(( "gena_server(): Subscription to %s out of sync, resubscribing...\n", subscription->serviceTypeOrId ));

						success = gena_unsubscribe( subscription, buffer, NULL );
						if ( success ) success = gena_subscribe( subscription, buffer, FALSE, NULL );
					}

					g_mutex_unlock( subscription->mutex );

					http_result = ( success ) ? "200 OK" : "500 Internal Server Error";
					break;
				}
			}

			g_mutex_unlock( gena_mutex );
		}
	}
	else
	{
		http_result = "400 Bad Request";
	}

	return http_result;
}

static void free_subscription( upnp_subscription *subscription )
{
	upnp_assert( subscription != NULL );

	if ( subscription != NULL )
	{
		g_mutex_free( subscription->mutex );
		g_free( subscription );
	}
}

/* Subscribe to a UPnP service */
upnp_subscription *upnp_subscribe( const upnp_device *device, const char* serviceTypeOrId, int timeout,
	gboolean (*callback)( upnp_subscription *subscription, char *content ), void *user_data,
	GError **error )
{
	upnp_subscription *subscription;
	struct buffer *buffer;

	upnp_message(( "upnp_subscribe( %s, %s )...\n", (device != NULL) ? device->friendlyName : "<NULL>", (serviceTypeOrId != NULL) ? serviceTypeOrId : "<NULL>" ));
	upnp_assert( serviceTypeOrId != NULL );

	if ( !start_gena_server( error ) ) return NULL;

	buffer = alloc_buffer( GENA_BUFFER_SIZE, 0, serviceTypeOrId, error );
	if ( buffer == NULL )
	{
		upnp_warning(( "upnp_subscribe(): alloc_buffer() failed\n" ));
		return NULL;
	}

	subscription = g_new0( upnp_subscription, 1 );
	upnp_assert( subscription != NULL );

	subscription->device = device;
	subscription->serviceTypeOrId = serviceTypeOrId;
	subscription->timeout = timeout;
	subscription->callback = callback;
	subscription->user_data = user_data;
	subscription->mutex = g_mutex_new();

	if ( gena_subscribe( subscription, buffer, TRUE, error ) )
	{
		upnp_assert( offsetof( upnp_subscription, next ) == 0 );
		list_prepend( &subscriptionList, subscription );

		g_mutex_unlock( gena_mutex );
	}
	else
	{
		free_subscription( subscription );
		subscription = NULL;
	}

	release_buffer( buffer );

	upnp_message(( "upnp_subscribe() = %p\n", (gpointer)subscription ));
	return subscription;
}

/* Unsubscribe from a UPnP service */
gboolean upnp_unsubscribe( const upnp_device *device, const char *serviceTypeOrId, GError **error )
{
	upnp_subscription *subscription, *previous;

	upnp_message(( "upnp_unsubscribe( %s, %s )...\n", (device != NULL) ? device->friendlyName : "<NULL>", (serviceTypeOrId != NULL) ? serviceTypeOrId : "<NULL>" ));
	upnp_assert( serviceTypeOrId != NULL );

	upnp_assert( gena_mutex != NULL || subscriptionList == NULL );
	if ( gena_mutex != NULL ) g_mutex_lock( gena_mutex );

	upnp_assert( offsetof( upnp_subscription, next ) == 0 );
	previous = (upnp_subscription*)&subscriptionList;
	for ( subscription = subscriptionList; subscription != NULL; subscription = subscription->next )
	{
		if ( subscription->device == device && strcmp( subscription->serviceTypeOrId, serviceTypeOrId ) == 0 )
		{
			struct buffer *buffer;
			gboolean result;

			previous->next = subscription->next;  /* remove from list */

			upnp_assert( gena_mutex != NULL );
			g_mutex_unlock( gena_mutex );

			buffer = alloc_buffer( GENA_BUFFER_SIZE, 0, subscription->serviceTypeOrId, error );
			if ( buffer == NULL )
			{
				upnp_warning(( "upnp_unsubscribe(): alloc_buffer() failed\n" ));
				return FALSE;
			}

			result = gena_unsubscribe( subscription, buffer, error );

			release_buffer( buffer );

			free_subscription( subscription );

			if ( subscriptionList == NULL ) stop_gena_server();

			upnp_message(( "upnp_unsubscribe() = %s\n", (result) ? "TRUE" : "FALSE" ));
			return result;
		}

		previous = subscription;
	}

	if ( gena_mutex != NULL ) g_mutex_unlock( gena_mutex );

	upnp_message(( "upnp_unsubscribe() = TRUE (not subscribed)\n" ));
	return TRUE;
}

const upnp_subscription *upnp_get_subscription( const upnp_device *device, const char *serviceTypeOrId )
{
	const upnp_subscription *subscription;

	upnp_assert( serviceTypeOrId != NULL );
	upnp_debug(( "upnp_get_subscription( %s )...\n", serviceTypeOrId ));

	upnp_assert( gena_mutex != NULL || subscriptionList == NULL );
	if ( gena_mutex != NULL ) g_mutex_lock( gena_mutex );

	for ( subscription = subscriptionList; subscription != NULL; subscription = subscription->next )
	{
		if ( (device == NULL || subscription->device == device) && strcmp( subscription->serviceTypeOrId, serviceTypeOrId ) == 0 )
		{
			break;  /* found */
		}
	}

	if ( gena_mutex != NULL ) g_mutex_unlock( gena_mutex );

	return subscription;
}

/* Unsubscribe from all subscribed UPnP services */
void upnp_unsubscribe_all( gboolean connected )
{
	upnp_subscription *subscription;
	struct buffer *buffer;

	upnp_message(( "upnp_unsubscribe_all()...\n" ));

	subscription = subscriptionList;
	subscriptionList = NULL;

	if ( subscription != NULL )
	{
		buffer = (connected) ? alloc_buffer( GENA_BUFFER_SIZE, 0, "upnp_unsubscribe_all()", NULL ) : NULL;
		do
		{
			upnp_subscription *next = subscription->next;

			upnp_message(( "upnp_unsubscribe_all(): unsubscribing %s...\n", subscription->serviceTypeOrId ));
			gena_unsubscribe( subscription, buffer, NULL );
			free_subscription( subscription );

			subscription = next;
		}
		while ( subscription != NULL );

		release_buffer( buffer );
	}

	upnp_message(( "upnp_unsubscribe_all(): stop GENA server\n" ));
	stop_gena_server();

	upnp_message(( "upnp_unsubscribe_all()...ok\n" ));
}

/*--- Initialization & Cleanup ------------------------------------------------------------------*/

gboolean upnp_startup( GError **error )
{
	upnp_message(( "upnp_startup()...\n" ));

#ifdef WIN32
{
	WSADATA wsa_data = { 0 };
	int err;

	err = WSAStartup( WSA_VERSION, &wsa_data );
	upnp_message(( "WSAStartup() = %d, %d.%d (%d.%d)\n", err,
		LOBYTE( wsa_data.wVersion ), HIBYTE( wsa_data.wVersion ),
		LOBYTE( wsa_data.wHighVersion ), HIBYTE( wsa_data.wHighVersion ) ));
	if ( err != 0 )
	{
		set_upnp_error( error, err, "Initialization of Windows Sockets failed" );
		return FALSE;
	}
}
#else
{
	struct sigaction sa;
	int err;

	sa.sa_handler = SIG_IGN;
	sigemptyset( &sa.sa_mask );
	sa.sa_flags = 0;

	err = sigaction( SIGPIPE, &sa, NULL );
	upnp_message(( "sigaction( SIGPIPE, SIG_IGN ) = %d\n", err ));
	if ( err != 0 )
	{
		set_upnp_error( error, err, "Initialization of BSD Sockets failed" );
		return FALSE;
	}
}
#endif

	/* Memory */
	buffer_mutex = ( g_thread_supported() ) ? g_mutex_new() : NULL;
	buffer_list = NULL;

	/* HTTP */
	http_server_socket = INVALID_SOCKET;
	http_server_port = 0;
	http_server_thread = NULL;

	/* SSDP */
	deviceList = NULL;

	/* GENA */
	gena_mutex = NULL;

	upnp_message(( "upnp_startup()...ok\n" ));
	return TRUE;
}

void upnp_cleanup( gboolean connected )
{
	upnp_message(( "upnp_cleanup()...\n" ));

	/* GENA */
	upnp_message(( "upnp_cleanup(): GENA\n" ));
	upnp_unsubscribe_all( connected );
	end_gena_server();
	if ( gena_mutex != NULL )
	{
		g_mutex_free( gena_mutex );
		gena_mutex = NULL;
	}

	/* SSDP */
	upnp_message(( "upnp_cleanup(): SSDP\n" ));
	upnp_free_all_devices();

	/* HTTP */
	upnp_message(( "upnp_cleanup(): HTTP\n" ));
	http_end_server();

	/* Memory */
	upnp_message(( "upnp_cleanup(): Memory\n" ));
#if 0
	while ( buffer_list != NULL )
	{
		upnp_warning(( "upnp_cleanup(): %p,%d,\"%s\"\n",
			(void*)buffer_list->addr, buffer_list->refcount, buffer_list->debug_info ));

		if ( buffer_list->refcount > 0 )
		{
			buffer_list->refcount = 1;  /* free memory */
			release_buffer( buffer_list );
		}
	}
#endif
	if ( buffer_mutex != NULL )
	{
		g_mutex_free( buffer_mutex );
		buffer_mutex = NULL;
	}

#ifdef WIN32
	WSACleanup();
#endif

	upnp_message(( "upnp_cleanup()...ok\n" ));
}

/*-----------------------------------------------------------------------------------------------*/

GQuark upnp_error_quark( void )
{
	return g_quark_from_static_string( "princess-context-error-quark" );
}

/*-----------------------------------------------------------------------------------------------*/

/*
	Online Resources regarding BSD sockets:

	http://www.cs.vu.nl/~jms/socket-info.html
	http://www.lowtek.com/sockets/
	http://www.faqs.org/faqs/unix-faq/socket/
	http://www.linuxjournal.com/article/2333
*/
