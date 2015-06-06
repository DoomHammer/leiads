/*
	princess/upnp_xml.c
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

/*
	These XML functions are not fully XML 1.0 complaint, but are (hopefully)
	sufficient for UPnP A/V. These are the things which are not implemented (yet):

	- CDATA Sections (2.7)
	- Prolog and Document Type Declaration (2.8)
	- Standalone Document Declaration (2.9)
	- White Space Handling - "xml:space" (2.10)
	- Language Identification (2.12)
	- Element Type Declarations (3.2)
	- Attribute-List Declarations (3.3)
	- Conditional Sections (3.4)
	- Other entities than the five predeclared ones (4)
*/

/*-----------------------------------------------------------------------------------------------*/

#if LOGLEVEL

static const char *opening( const char *str )
{
	static char buf[48];
	strcpy( strncpy( buf, str, 40 ) + 40, "..." );  /* Not thread safe! */
	return buf;
}

#endif

/*-----------------------------------------------------------------------------------------------*/

/* Copies a XML content, and unwraps the delimiters if necessary */
char *xml_strcpy( char *dest, size_t sizeof_dest, const char *src, const char *src_end )
{
	size_t len = src_end - src;
	upnp_assert( dest != NULL && src != NULL && src_end != NULL );

	if ( sizeof_dest > 0 )
	{
		if ( len >= sizeof_dest ) len = sizeof_dest - 1;
		strncpy( dest, src, len )[len] = '\0';
		if ( *dest != '<' ) xml_unwrap_delimiters_in_place( dest );
	}

	return dest;
}

gchar *xml_gstrcpy( const char *src, const char *src_end )
{
	gchar *dest;

	gssize len = src_end - src;
	upnp_assert( src != NULL && src_end != NULL );

	dest = g_new( gchar, len + 1 );
	if ( dest != NULL )
	{
		strncpy( dest, src, len )[len] = '\0';
		if ( *dest != '<' ) xml_unwrap_delimiters_in_place( dest );
	}

	return dest;
}

static struct _GString *xml_gstring_copy( const char *src, const char *src_end )
{
	struct _GString *dest;

	gssize len = src_end - src;
	upnp_assert( src != NULL && src_end != NULL );

	dest = g_string_sized_new( len + 1 );
	if ( dest != NULL )
	{
		g_string_append_len( dest, src, len );
		if ( *dest->str != '<' )
		{
			xml_unwrap_delimiters_in_place( dest->str );
			g_string_truncate( dest, strlen( dest->str ) );
		}
	}

	return dest;
}

/*-----------------------------------------------------------------------------------------------*/

/* Get information (name, attributes, content, next element) about the given element */
enum xml_stop { COMPLETE = 0, STOP_AFTER_NAME, STOP_AFTER_ATTRIBUTES, STOP_AFTER_CONTENT };
static gboolean xml_get_element_info( const char *xml, struct xml_info *info, enum xml_stop stop )
{
	upnp_assert( /*xml != NULL &&*/ info != NULL );

	if ( xml == NULL || *xml == '\0' ) return FALSE;

	/* No idea what this is, but since the QNAP version of Twonky 5.1 uses it we skip it anyway */
	while ( *xml == '#' )
	{
		upnp_warning(( "xml_get_element_info(): Garbage detected (Twonky5!?): %s\n", opening( xml ) ));

		xml = strchr( ++xml, '#' );
		if ( xml == NULL )
		{
			upnp_warning(( "xml_get_element_info(): Missing # delimiter\n" ));
			return FALSE;
		}

		xml++;
		while ( xml_is_space( *xml ) ) xml++;
	}

	for (;;)
	{
		if ( *xml++ != '<' )
		{
			upnp_warning(( "xml_get_element_info(): STag expected: %s\n", opening( --xml ) ));
			return FALSE;
		}
		if ( *xml == '/' )
		{
			upnp_warning(( "xml_get_element_info(): ETag detected: %s\n", opening( --xml ) ));
			return FALSE;
		}

		/* Comments (2.5) */
		if ( xml[0] == '!' && xml[1] == '-' && xml[2] == '-' )
		{
			/* Skip comment */
			xml = strstr( xml + 3, "-->" );
			if ( xml == NULL )
			{
				upnp_warning(( "xml_get_element_info(): Incomplete Comment\n" ));
				return FALSE;
			}

			xml += 3;
		}

		/* Processing Instructions (2.6) */
		else if ( xml[0] == '?' )
		{
			xml = strstr( xml + 1, "?>" );
			if ( xml == NULL )
			{
				upnp_warning(( "xml_get_element_info(): Incomplete PI\n" ));
				return FALSE;
			}

			xml += 2;
		}

		else break;

		while ( xml_is_space( *xml ) ) xml++;
	}

	/* Parse name */
	info->name = (char*)xml;
	while ( !xml_is_space( *xml ) && *xml != '/' && *xml != '>' )
	{
		if ( *xml == '\0' )
		{
			upnp_warning(( "xml_get_element_info(): Invalid Name: %s\n", opening( info->name ) ));
			return FALSE;  /* no valid XML */
		}

		xml++;
	}
	info->end_of_name = (char*)xml;
	if ( info->end_of_name == info->name )
	{
		upnp_warning(( "xml_get_element_info(): Invalid Name: %s\n", opening( info->name ) ));
		return FALSE;
	}

	if ( stop == STOP_AFTER_NAME ) return TRUE;

	while ( xml_is_space( *xml ) ) xml++;

	/* Parse attributes (if any) */
	info->attributes = (char*)xml;
	if ( *xml != '/' && *xml != '>' )
	{
		do
		{
			char ch = *xml++;
			if ( ch == '\0' )
			{
				upnp_warning(( "xml_get_element_info(): Incomplete Attributes: %s\n", opening( info->attributes ) ));
				return FALSE;
			}

			if ( ch == '\"' || ch == '\'' )
			{
				xml = strchr( xml, ch );
				if ( xml == NULL )
				{
					upnp_warning(( "xml_get_element_info(): Incomplete Attributes: %s\n", opening( info->attributes ) ));
					return FALSE;
				}

				xml++;
			}
		}
		while ( *xml != '/' && *xml != '>' );
	}
	info->end_of_attributes = (char*)xml;

	if ( stop == STOP_AFTER_ATTRIBUTES ) return TRUE;

	if ( *xml++ == '/' )  /* empty element */
	{
		info->end_of_content = info->content = (char*)xml;

		if ( *xml != '>' )
		{
			upnp_warning(( "xml_get_element_info(): Incomplete EmptyElemTag: %s\n", opening( --xml ) ));
			return FALSE;
		}
	}
	else  /* == '>' */  /* Parse content */
	{
		size_t name_len = info->end_of_name - info->name;

		/* Discard white space at the start of the content */
		while ( xml_is_space( *xml ) ) xml++;
		info->content = (char*)xml;

		/* Search corresponding end-tag */
		for (;;)
		{
			xml = strchr( xml, '<' );
			if ( xml == NULL )
			{
				upnp_warning(( "xml_get_element_info(): ETag expected\n" ));
				return FALSE;
			}

			if ( xml[1] == '/' && strncmp( xml + 2, info->name, name_len ) == 0 )
			{
				const char *s = xml + 2 + name_len;
				if ( xml_is_space( *s ) || *s == '>' )  /* end-tag found */
				{
					info->end_of_content = (char*)xml;
					xml = s;
					break;
				}
			}
			else if ( strncmp( xml + 1, info->name, name_len ) == 0 )
			{
				const char *s = xml + 1 + name_len;
				if ( xml_is_space( *s ) || *s == '/' || *s == '>' )  /* start-tag found */
				{
					struct xml_info info;
					if ( !xml_get_element_info( xml, &info, COMPLETE ) )
					{
						upnp_warning(( "xml_get_element_info(): Not Well-Formed Content: %s\n", opening( xml ) ));
						return FALSE;
					}

					xml = info.next;
					continue;
				}
			}

			xml++;
		}

		/* Discard white space at the end of the content */
		do info->end_of_content--;
		while ( info->end_of_content >= info->content && xml_is_space( *info->end_of_content ) );
		info->end_of_content++;
	}

	if ( stop == STOP_AFTER_CONTENT ) return TRUE;

	while ( xml_is_space( *xml ) ) xml++;

	if ( *xml++ != '>' )
	{
		upnp_warning(( "xml_get_element_info(): Incomplete ETag: %s\n", opening( --xml ) ));
		return FALSE;
	}

	while ( xml_is_space( *xml ) ) xml++;

	info->next = (char*)xml;
	return TRUE;
}

/* Get information (name, attributes, content, next element) about the given element */
gboolean xml_get_info( const char *element, struct xml_info *info )
{
	return xml_get_element_info( element, info, COMPLETE );
}

/* Retrieve the name of the given element */
char *xml_get_name( const char *element, char *name, size_t sizeof_name )
{
	struct xml_info info;
	if ( xml_get_element_info( element, &info, STOP_AFTER_NAME ) )
		return xml_strcpy( name, sizeof_name, info.name, info.end_of_name );

	return NULL;
}

gchar *xml_get_name_str( const char *element )
{
	struct xml_info info;
	if ( xml_get_element_info( element, &info, STOP_AFTER_NAME ) )
		return xml_gstrcpy( info.name, info.end_of_name );
	
	return NULL;
}

struct _GString *xml_get_name_string( const char *element )
{
	struct xml_info info;
	if ( xml_get_element_info( element, &info, STOP_AFTER_NAME ) )
		return xml_gstring_copy( info.name, info.end_of_name );

	return NULL;
}

#if 0  /* not needed yet */
/* Retrieve the attributes of the given element */
char *_xml_get_attributes( const char *element, char *attributes, size_t sizeof_attributes )
{
	struct xml_info info;
	if ( xml_get_element_info( element, &info, STOP_AFTER_ATTRIBUTES ) )
		return xml_strcpy( attributes, sizeof_attributes, info.attributes, info.end_of_attributes );
	
	return NULL;
}
/*struct _GString *xml_get_attributes( const char *element );*/
#endif

#if 0  /* not needed yet */
/* Retrieve the content of the given element */
char *xml_get_content( const char *element, char* content, size_t sizeof_content )
{
	struct xml_info info;
	if ( xml_get_element_info( element, &info, STOP_AFTER_CONTENT ) )
		return xml_strcpy( content, sizeof_content, info.content, info.end_of_content );

	return NULL;
}
/*gchar *xml_get_content_str( const char *element );*/
/*struct _GString *xml_get_content_string( const char *element );*/
#endif

gchar *xml_box( const char *name, const char *content, const char *attribute_name_1, ... )
{
	va_list args;
	gchar *result;

	va_start( args, attribute_name_1 );  /* Initialize variable arguments. */
	result = xml_box_v( name, content, attribute_name_1, args );
	va_end( args );                      /* Reset variable arguments.      */
	return result;
}

gchar *xml_box_v( const char *name, const char *content, const char *attribute_name_1, va_list args )
{
	struct _GString *dest = g_string_new( "<" );
	const char *s;

	g_string_append( dest, name );

	if ( attribute_name_1 != NULL )
	{
		do
		{
			const char *attribute_value_1 = va_arg( args, const char * );
			if ( attribute_value_1 != NULL )
			{
				g_string_append_c( dest, ' ' );
				g_string_append( dest, attribute_name_1 );
				g_string_append( dest, "=\"" );
				xml_wrap_delimiters_to_string( dest, attribute_value_1 );
				g_string_append( dest, "\"" );
			}

			attribute_name_1 = va_arg( args, const char * );
		}
		while ( attribute_name_1 != NULL );
	}

	if ( content != NULL )
	{
		g_string_append_c( dest, '>' );

		xml_wrap_delimiters_to_string( dest, content );

		g_string_append_c( dest, '<' );
		g_string_append_c( dest, '/' );
		if ( (s = strchr( name, ' ' )) != NULL )
			g_string_append_len( dest, name, s - name );
		else
			g_string_append( dest, name );
	}
	else
	{
		g_string_append_c( dest, '/' );
	}

	g_string_append_c( dest, '>' );

	return g_string_free( dest, FALSE );
}

void xml_string_append_boxed( GString *str, const char *name, const char *content, const char *attribute_name_1, ... )
{
	upnp_assert( name != NULL );

	if ( content != NULL )
	{
		va_list args;
		gchar *s;

		va_start( args, attribute_name_1 );
		s = xml_box_v( name, content, attribute_name_1, args );
		va_end( args );

		g_string_append( str, s );

		g_free( s );
	}
}

/* Extracts name, attributes, and content of a given element
   (`element' will point to the next element afterwards) */
char *xml_unbox( char **element, char **name /*= NULL*/, char **attributes /*= NULL*/ )
{
	struct xml_info info;

	if ( xml_get_element_info( *element, &info, COMPLETE ) )
	{
		*element = info.next;

		if ( name != NULL )
		{
			*info.end_of_name = '\0';
			*name = info.name;
		}
		if ( attributes != NULL )
		{
			*info.end_of_attributes = '\0';
			*attributes = info.attributes;
		}

		*info.end_of_content = '\0';

		if ( info.content[0] != '<' ) xml_unwrap_delimiters_in_place( info.content );
		return info.content;
	}

	return NULL;
}

/*-----------------------------------------------------------------------------------------------*/

/* Get information (name, value, next attribute) about the given attribute */
static gboolean xml_get_attribute_info( const char *xml, struct xml_info *info )
{
	if ( xml == NULL || *xml == '\0' || *xml == '/' || *xml == '>' ) return FALSE;

	/* Parse name */
	info->name = (char*)xml;
	while ( !xml_is_space( *xml ) && *xml != '=' && *xml != '>' && *xml != '\0' )
		xml++;
	info->end_of_name = (char*)xml;
	if ( info->end_of_name == info->name )
	{
		upnp_warning(( "xml_get_attribute_info(): Invalid Name: %s\n", opening( info->name ) ));
		return FALSE;
	}

	while ( xml_is_space( *xml ) ) xml++;

	if ( *xml == '=' )
	{
		char quote;

		do xml++;
		while ( xml_is_space( *xml ) );

		/* Parse value */
		quote = *xml++;
		if ( quote != '\"' && quote != '\'' )
		{
			upnp_warning(( "xml_get_attribute_info(): Not Well-Formed AttValue: %s\n", opening( --xml ) ));
			return FALSE;
		}

		info->content = (char*)xml;
		xml = strchr( xml, quote );
		if ( xml == NULL )
		{
			upnp_warning(( "xml_get_attribute_info(): Not Well-Formed AttValue: %c%s\n", quote, opening( info->content ) ));
			return FALSE;
		}

		info->end_of_content = (char*)xml++;

		while ( xml_is_space( *xml ) ) xml++;
	}
	else
	{
		info->end_of_content = info->content = (char*)xml;
	}

	info->next = (char*)xml;
	return TRUE;
}

/* Extracts name and value of a given attribute
   (`attribute' will point to the next attribute afterwards) */
char *xml_unbox_attribute( char **attribute, char **name )
{
	struct xml_info info;
	if ( xml_get_attribute_info( *attribute, &info ) )
	{
		*attribute = info.next;

		if ( name != NULL )
		{
			*info.end_of_name = '\0';
			*name = info.name;
		}

		*info.end_of_content = '\0';

		if ( info.content[0] != '<' ) xml_unwrap_delimiters_in_place( info.content );
		return info.content;
	}

	return NULL;
}

/*-----------------------------------------------------------------------------------------------*/

/* XML interation: Get the first element of content
  (NOTE: When the iteration aborts before the end of the content,
   xml_abort_iteration() have to be called to restore the content) */
char *xml_first( char *content, xml_iter *iter )
{
	struct xml_info info;

	if ( xml_get_info( content, &info ) )
	{
		upnp_assert( iter != NULL );
		iter->next = info.next;
		iter->x = *info.next;
		*info.next = '\0';

		return content;
	}

	return NULL;

}

/* XML interation: Get the next element of content
  (NOTE: When the iteration aborts before the end of the content,
   xml_abort_iteration() have to be called to restore the content) */
char *xml_next( xml_iter *iter )
{
	upnp_assert( iter != NULL );

	xml_abort_iteration( iter );           /* restore content first */
	return xml_first( iter->next, iter );  /* get next element */
}

/* XML interation: Restore the content */
void xml_abort_iteration( xml_iter *iter )
{
	upnp_assert( iter != NULL );
	upnp_assert( iter->next != NULL );
	upnp_assert( iter->x == '<' || iter->x == '\0' );

	*iter->next = iter->x;
}

/*-----------------------------------------------------------------------------------------------*/

/* Find a named element */
char *xml_find_named( const char *xml, const char *name, struct xml_info *info )
{
	size_t name_len;

	upnp_assert( name != NULL );
	if ( xml == NULL || name == NULL ) return NULL;

	name_len = strlen( name );
	for (;;)
	{
		xml = strchr( xml, '<' );
		if ( xml == NULL ) return NULL;

		if ( strncmp( xml + 1, name, name_len ) == 0 )
		{
			char ch = xml[1+name_len];

			if ( xml_is_space( ch ) || ch == '/' || ch == '>' )
			{
				/* found */
				return xml_get_info( xml, info ) ? (char*)xml : NULL;
			}

			xml += name_len;
		}

		xml++;
	}
}

/* Retrieve a named element content */
char *xml_get_named( const char *xml, const char *name, char *content, size_t sizeof_content )
{
	return xml_get_named_with_attributes( xml, name, content, sizeof_content, NULL, 0 );
}

gchar *xml_get_named_str( const char *xml, const char *name )
{
	return xml_get_named_with_attributes_str( xml, name, NULL );
}

struct _GString *xml_get_named_string( const char *xml, const char *name )
{
	return xml_get_named_with_attributes_string( xml, name, NULL );
}

/* Retrieve a named element content and attributes */
char *xml_get_named_with_attributes( const char *xml, const char *name, char *content, size_t sizeof_content, char *attributes, size_t sizeof_attributes )
{
	struct xml_info info;

	xml = xml_find_named( xml, name, &info );
	if ( xml == NULL ) return NULL;

	if ( attributes != NULL ) xml_strcpy( attributes, sizeof_attributes, info.attributes, info.end_of_attributes );
	return xml_strcpy( content, sizeof_content, info.content, info.end_of_content );
}

gchar *xml_get_named_with_attributes_str( const char *xml, const char *name, gchar **attributes )
{
	struct xml_info info;

	xml = xml_find_named( xml, name, &info );
	if ( xml == NULL ) return NULL;

	if ( attributes != NULL ) *attributes = xml_gstrcpy( info.attributes, info.end_of_attributes );
	return xml_gstrcpy( info.content, info.end_of_content );
}

struct _GString *xml_get_named_with_attributes_string( const char *xml, const char *name, struct _GString **attributes )
{
	struct xml_info info;

	xml = xml_find_named( xml, name, &info );
	if ( xml == NULL ) return NULL;

	if ( attributes != NULL ) *attributes = xml_gstring_copy( info.attributes, info.end_of_attributes );
	return xml_gstring_copy( info.content, info.end_of_content );
}

/*-----------------------------------------------------------------------------------------------*/

/* Find a named attribute */
static gboolean xml_find_named_attribute( const char *element, const char *name, struct xml_info *info )
{
	upnp_assert( name != NULL );

	if ( xml_get_element_info( element, info, STOP_AFTER_ATTRIBUTES ) )
	{
		size_t name_len = strlen( name );

		while ( xml_get_attribute_info( info->attributes, info ) )
		{
			if ( (size_t)(info->end_of_name - info->name) == name_len && strncmp( info->name, name, name_len  ) == 0 )
			{
				/* found */
				return TRUE;
			}

			info->attributes = info->next;
		}
	}

	return FALSE;
}

/* Retrieve a named attribute value */
char *xml_get_named_attribute( const char *element, const char *name, char *value, size_t sizeof_value )
{
	struct xml_info info;

	if ( xml_find_named_attribute( element, name, &info ) )
		return xml_strcpy( value, sizeof_value, info.content, info.end_of_content );

	return NULL;
}

gchar *xml_get_named_attribute_str( const char *element, const char *name )
{
	struct xml_info info;

	if ( xml_find_named_attribute( element, name, &info ) )
		return xml_gstrcpy( info.content, info.end_of_content );

	return NULL;
}

struct _GString *xml_get_named_attribute_string( const char *element, const char *name )
{
	struct xml_info info;

	if ( xml_find_named_attribute( element, name, &info ) )
		return xml_gstring_copy( info.content, info.end_of_content );

	return NULL;
}

/*-----------------------------------------------------------------------------------------------*/

static const char _amp[]  = "&amp;";
static const char _lt[]   = "&lt;";
static const char _gt[]   = "&gt;";
static const char _apos[] = "&apos;";
static const char _quot[] = "&quot;";

/* Escapes the XML delimiters &<>'" */
gchar *xml_wrap_delimiters( const char *src )
{
	if ( src != NULL )
	{
		GString *dest = g_string_new( "" );
		xml_wrap_delimiters_to_string( dest, src );
		return g_string_free( dest, FALSE );
	}

	return NULL;
}

void xml_wrap_delimiters_to_buffer( struct buffer *dest, const char *src )
{
	upnp_assert( src != NULL );
	if ( src == NULL ) return;

	for (;;)
	{
		char c = *src++;
		if ( c == '\0' ) break;

		if ( c == '&' )
			append_string_to_buffer( dest, _amp );
		else if ( c == '<' )
			append_string_to_buffer( dest, _lt );
		else if ( c == '>' )
			append_string_to_buffer( dest, _gt );
		else if ( c == '\'' )
			append_string_to_buffer( dest, _apos );
		else if ( c == '\"' )
			append_string_to_buffer( dest, _quot );
		else
			append_char_to_buffer( dest, c );
	}
}

void xml_wrap_delimiters_to_string( GString *dest, const char *src )
{
	for (;;)
	{
		char c = *src++;
		if ( c == '\0' ) break;

		if ( c == '&' )
			g_string_append( dest, _amp );
		else if ( c == '<' )
			g_string_append( dest, _lt );
		else if ( c == '>' )
			g_string_append( dest, _gt );
		else if ( c == '\'' )
			g_string_append( dest, _apos );
		else if ( c == '\"' )
			g_string_append( dest, _quot );
		else
			g_string_append_c( dest, c );
	}
}

/* Unwraps the XML delimiters &<>'" (in place) */
gchar *xml_unwrap_delimiters( const char *src )
{
	return ( src != NULL ) ? xml_unwrap_delimiters_in_place( g_strdup( src ) ) : NULL;
}

char *xml_unwrap_delimiters_in_place( char *buf )
{
	char *src = buf, *dest = buf;

	for (;;)
	{
		char c = *src++;
		if ( c == '&' )
		{
			if ( strncmp( src, _amp + 1, 4 ) == 0 )
			{
				src += 4;
				c = '&';
			}
			else if ( strncmp( src, _lt + 1, 3 ) == 0 )
			{
				src += 3;
				c = '<';
			}
			else if ( strncmp( src, _gt + 1, 3 ) == 0 )
			{
				src += 3;
				c = '>';
			}
			else if ( strncmp( src, _apos + 1, 5 ) == 0 )
			{
				src += 5;
				c = '\'';
			}
			else if ( strncmp( src, _quot + 1, 5 ) == 0 )
			{
				src += 5;
				c = '\"';
			}
			else
			{
				upnp_warning(( "xml_unwrap_delimiters(): unexpected &%s\n", opening( src ) ));
			}
		}

		*dest++ = c;
		if ( c == '\0' ) break;
	}

	return buf;
}

/*-----------------------------------------------------------------------------------------------*/

static int _num_xml_contents;

struct xml_content *new_xml_content( char *str, int n, int free )
{
	struct xml_content *result;

	if ( str != NULL )
	{
		result = g_new( struct xml_content, 1 );

		upnp_assert( n >= 0 );

		result->str = str;
		result->n = n;
		result->ref_count = 1;
		result->free = free;

		g_atomic_int_add( &_num_xml_contents, 1 );
	}
	else
	{
		upnp_warning(( "new_xml_content( str = <NULL> )\n" ));
		result = NULL;
	}

	return result;
}

struct xml_content *xml_content_from_string( GString *string, int n )
{
	return new_xml_content( g_string_free( string, FALSE ), n, G_FREE );
}

struct xml_content *copy_xml_content( struct xml_content *content )
{
	if ( content != NULL )
	{
		upnp_assert( content->ref_count > 0 );
		if ( content->ref_count > 0 )
			g_atomic_int_inc( &content->ref_count );
		else
			content = NULL;
	}

	return content;
}

struct xml_content *append_xml_content( struct xml_content *dest, struct xml_content *src, GError **error )
{
	if ( dest == NULL || dest->str == NULL || dest->n == 0 )
	{
		free_xml_content( dest );
		dest = (src != NULL) ? copy_xml_content( src ) : new_xml_content( NULL, 0, UPNP_FREE );
	}
	else if ( src == NULL || src->str == NULL || src->n == 0 )
	{
		;  /* nothing to do */
	}
	else
	{
		gsize n_bytes = strlen( dest->str ) + strlen( src->str ) + 1;
		char *content;

		if ( dest->ref_count > 1 )
		{
			content = g_try_malloc( n_bytes );
			if ( content != NULL )
			{
				int n = dest->n;
				strcpy( content, dest->str );
				free_xml_content( dest );
				dest = new_xml_content( content, n, G_FREE );
			}
		}
		else if ( dest->free == UPNP_FREE )
		{
			content = g_try_malloc( n_bytes );
			if ( content != NULL )
			{
				strcpy( content, dest->str );
				upnp_free_string( dest->str );
			}
		}
		else if ( dest->free == G_FREE )
		{
			content = g_try_realloc( dest->str, n_bytes );
		}
		else
		{
			upnp_assert( FALSE );  /* Should never happen */
			content = NULL;
			g_set_error( error, 0, 0, "Internal error in append_xml_content()" );
		}

		if ( content != NULL )
		{
			dest->str = strcat( content, src->str );
			dest->n += src->n;
		}
		else
		{
			free_xml_content( dest );
			dest = NULL;

			if ( error != NULL && *error == NULL )
				g_set_error( error, 0, 0, "Out of memory" );
		}
	}

	return dest;
}

struct xml_content *append_to_xml_content( struct xml_content *dest, const char *src, GError **error )
{
	/* TODO */
	return NULL;
}

struct xml_content *prepend_to_xml_content( struct xml_content *dest, const char *src, GError **error )
{
	/* TODO */
	return NULL;
}

void free_xml_content( struct xml_content *content )
{
	if ( content != NULL )
	{
		upnp_assert( content->ref_count > 0 );
		if ( content->ref_count > 0 && g_atomic_int_dec_and_test( &content->ref_count ) )
		{
			switch ( content->free )
			{
			case UPNP_FREE:
				upnp_free_string( content->str );
				break;
			case G_FREE:
				g_free( content->str );
				break;
			default:
				upnp_assert( FALSE );
			}

			g_free( content );
			g_atomic_int_add( &_num_xml_contents, -1 );
		}
	}
}

int num_xml_contents()
{
	return _num_xml_contents;
}

/*-----------------------------------------------------------------------------------------------*/
