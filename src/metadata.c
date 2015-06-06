/*
	metadata.c
	Copyright (C) 2011 Axel Sommerfeldt (axel.sommerfeldt@f-m.fm)

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

#ifndef MAEMO
/*-----------------------------------------------------------------------------------------------*/

#define LITTLE_WORD( b )  ((unsigned)(b)[1] << 8 | (unsigned)(b)[0])
#define LITTLE_DWORD( b ) ((unsigned)(b)[3] << 24 | (unsigned)(b)[2] << 16 | (unsigned)(b)[1] << 8 | (unsigned)(b)[0])
#define BIG_DWORD( b )    ((unsigned)(b)[0] << 24 | (unsigned)(b)[1] << 16 | (unsigned)(b)[2] << 8 | (unsigned)(b)[3])
#define ID3_DWORD( b )    ((unsigned)(b)[0] << 21 | (unsigned)(b)[1] << 14 | (unsigned)(b)[2] << 7 | (unsigned)(b)[3])
#define ID3_2_DWORD( b )  ((unsigned)(b)[0] << 16 | (unsigned)(b)[1] << 8  | (unsigned)(b)[2])

/*-----------------------------------------------------------------------------------------------*/

/* http://flac.sourceforge.net/format.html */

gboolean get_vorbis_comments( GString *item, FILE *fp, unsigned block_length )
{
	unsigned char *buf = g_try_new( unsigned char, block_length ), *s;
	unsigned length, i, n;

	if ( buf == NULL )
	{
		TRACE(( "*** get_vorbis_comments(): unable to allocate %u bytes, abort\n", block_length ));
		return FALSE;
	}
	if ( fread( buf, 1, block_length, fp ) != block_length )
	{
		TRACE(( "*** get_vorbis_comments(): illegal block length %u, abort\n", block_length ));
		return FALSE;
	}

	s = buf;
	length = LITTLE_DWORD( s );  /* vendor_length */
	s += 4 + length;             /* vendor_string */

	n = LITTLE_DWORD( s );
	s += 4;

	for ( i = 0; i < n; i++ )
	{
		unsigned j;

		length = LITTLE_DWORD( s );
		if ( 4 + length > (unsigned)((buf + block_length) - s) )
		{
			TRACE(( "*** get_vorbis_comments(): illegal length %u, abort\n", length ));
			break;
		}

		for ( j = 0; j < length; j++ ) s[j] = s[4+j];
		s[j] = '\0';

		TRACE(( "comment[%u] = \"%s\"\n", i, s ));

		if ( strnicmp( (char *)s, "album=", 6 ) == 0 )
		{
			if ( s[6] != '\0' )
				xml_string_append_boxed( item, "upnp:album", (char *)s + 6, NULL );
		}
		else if ( strnicmp( (char *)s, "albumartist=", 12 ) == 0 )
		{
			;  /* TODO */
		}
		else if ( strnicmp( (char *)s, "artist=", 7 ) == 0 )
		{
			if ( s[7] != '\0' )
				xml_string_append_boxed( item, "upnp:artist", (char *)s + 7, NULL );
		}
		else if ( strnicmp( (char *)s, "composer=", 9 ) == 0 )
		{
			if ( s[7] != '\0' )
			{
				xml_string_append_boxed( item, "upnp:author", (char *)s + 9, "role", "Composer", NULL );
				/*xml_string_append_boxed( item, "upnp:artist", (char *)s + 9, "role", "Composer", NULL );*/
			}
		}
		else if ( strnicmp( (char *)s, "conductor=", 10 ) == 0 )
		{
			;  /* TODO */
		}
		else if ( strnicmp( (char *)s, "date=", 5 ) == 0 )
		{
			if ( s[5] != '\0' )
				xml_string_append_boxed( item, "dc:date", (char *)s + 5, NULL );
		}
		else if ( strnicmp( (char *)s, "discnumber=", 11 ) == 0 )
		{
			;  /* TODO */
		}
		else if ( strnicmp( (char *)s, "ensemble=", 9 ) == 0 )
		{
			;  /* TODO */
		}
		else if ( strnicmp( (char *)s, "genre=", 6 ) == 0 )
		{
			if ( s[6] != '\0' )
				xml_string_append_boxed( item, "upnp:genre", (char *)s + 6, NULL );
		}
		else if ( strnicmp( (char *)s, "title=", 6 ) == 0 )
		{
			if ( s[6] != '\0' )
				xml_string_append_boxed( item, "dc:title", (char *)s + 6, NULL );
		}
		else if ( strnicmp( (char *)s, "totaldiscs=", 11 ) == 0 )
		{
			;  /* TODO */
		}
		else if ( strnicmp( (char *)s, "totaltracks=", 12 ) == 0 )
		{
			;  /* TODO */
		}
		else if ( strnicmp( (char *)s, "tracknumber=", 12 ) == 0 )
		{
			int value = atoi( (char *)s + 12 );
			if ( value )
			{
				sprintf( (char *)s, "%d", value );
				xml_string_append_boxed( item, "upnp:originalTrackNumber", (char *)s, NULL );
			}
		}

		s += 4 + length;
	}

	g_free( buf );
	return TRUE;
}

gboolean get_flac_metadata( GString *item, FILE *fp, const char *location, gboolean album_art )
{
	gboolean vorbis_comments = FALSE;
	unsigned char buffer[64];

	for (;;)
	{
		unsigned last_metadata_block, block_type, block_length;

		if ( fread( buffer, 1, 4, fp ) != 4 ) break;
		last_metadata_block = buffer[0] & 0x80;
		block_type = buffer[0] & 0x7f;
		buffer[0] = 0;
		block_length = BIG_DWORD( buffer );

		TRACE(( "last_metadata_block = %u\n", last_metadata_block ));
		TRACE(( "block_type          = %u\n", block_type ));
		TRACE(( "block_length        = %u\n", block_length ));

		if ( block_type == 4 /* VORBIS_COMMENT */ )
		{
			if ( !get_vorbis_comments( item, fp, block_length ) ) break;

			if ( album_art ) return TRUE;
			vorbis_comments = TRUE;
		}
		else if ( block_type == 6 /* PICTURE */ && !album_art )
		{
			unsigned picture_type;

			if ( fread( buffer, 1, 4, fp ) != 4 ) break;
			picture_type = BIG_DWORD( buffer );
			TRACE(( "Picture type = %u\n", picture_type ));

			if ( picture_type == 3 /* Cover (front) */ )
			{
				gchar *albumArtURI, mime_type[256];
				unsigned length;

				/* MIME type string */
				if ( fread( buffer, 1, 4, fp ) != 4 ) break;
				length = BIG_DWORD( buffer );
				if ( fread( mime_type, 1, length, fp ) != length ) break;
				mime_type[length] = 0;

				/* Description */
				if ( fread( buffer, 1, 4, fp ) != 4 ) break;
				length = BIG_DWORD( buffer );
				fseek( fp, length, SEEK_CUR );   

				fseek( fp, 4 * 4, SEEK_CUR ); /* width, height, color depth, number of colors used */

				/* Picture data */
				if ( fread( buffer, 1, 4, fp ) != 4 ) break;
				length = BIG_DWORD( buffer );
				if ( strncmp( mime_type, "-->", 3 ) == 0 )
				{
					albumArtURI = g_new( char, length + 1 );
					if ( fread( albumArtURI, 1, length, fp ) != length ) break;
					albumArtURI[length] = 0;
				}
				else if ( length > 0 )
				{
					long from = ftell( fp );

					albumArtURI = g_strdup_printf( "%s?bytes=%ld-%ld&content-type=%s", location, from, from + (long)length - 1, mime_type );

					fseek( fp, length, SEEK_CUR );
				}

				xml_string_append_boxed( item, "upnp:albumArtURI", albumArtURI, NULL );
				g_free( albumArtURI );

				if ( vorbis_comments ) return TRUE;
				album_art = TRUE;
			}
			else
			{
				fseek( fp, block_length - 4, SEEK_CUR );
			}
		}
		else
		{
			fseek( fp, block_length, SEEK_CUR );
		}

		if ( last_metadata_block ) return TRUE;
	}

	return FALSE;
}

/*-----------------------------------------------------------------------------------------------*/

/* http://www.xiph.org/ogg/doc/rfc3533.txt */

gboolean get_ogg_metadata( GString *item, FILE *fp )
{
	unsigned char buffer[64];

	for (;;)
	{
		unsigned version, header_type;
		unsigned granule_position1, granule_position2;
		unsigned bitstream_serial_number;
		unsigned page_sequence_number;
		unsigned CRC_checksum;
		unsigned page_segments, page_data_size;
		unsigned i;

		if ( fread( buffer, 1, 27-4, fp ) != 27-4 ) break;

		version = buffer[0];
		header_type = buffer[1];
		granule_position1 = LITTLE_DWORD( buffer + 2 );
		granule_position2 = LITTLE_DWORD( buffer + 6 );
		bitstream_serial_number = LITTLE_DWORD( buffer + 10 );
		page_sequence_number = LITTLE_DWORD( buffer + 14 );
		CRC_checksum = LITTLE_DWORD( buffer + 18 );
		page_segments = buffer[22];

		TRACE(( "version                 = %u\n", version ));
		TRACE(( "header_type             = %u\n", header_type ));
		TRACE(( "granule_position[0]     = %u\n", granule_position1 ));
		TRACE(( "granule_position[1]     = %u\n", granule_position2 ));
		TRACE(( "bitstream_serial_number = %u\n", bitstream_serial_number ));
		TRACE(( "page_sequence_number    = %u\n", page_sequence_number ));
		TRACE(( "CRC_checksum            = %#x\n", CRC_checksum ));
		TRACE(( "page_segments           = %u\n", page_segments ));

		if ( page_segments > 0 )
		{
			if ( page_segments > sizeof( buffer ) ) break;
			if ( fread( buffer, 1, page_segments, fp ) != page_segments ) break;
		}

		page_data_size = 0;
		for ( i = 0; i < page_segments; i++ )
			page_data_size += buffer[i];

		if ( page_data_size > 0 )
		{
			if ( fread( buffer, 1, 7, fp ) != 7 ) break;

			if ( strncmp( (char *)buffer + 1, "vorbis", 6 ) == 0 )
			{
				TRACE(( "buffer[0] = %u\n", buffer[0] ));
				if ( buffer[0] == 3 )
				{
					return get_vorbis_comments( item, fp, page_data_size - 7 );
				}
				else
				{
					fseek( fp, page_data_size - 7, SEEK_CUR );
				}
			}
			else
			{
				TRACE(( "*** no Ogg vorbis stream, abort\n" ));
				break;
			}
		}

		if ( (header_type & 0x04) != 0 ) break;  /* eos page */

		fread( buffer, 1, 4, fp );
		if ( buffer[0] == 'O' && buffer[1] == 'g' && buffer[2] == 'g' && buffer[3] == 'S' )
		{
			;
		}
		else
		{
			TRACE(( "*** no Ogg page, abort\n" ));
			break;
		}
	}

	return FALSE;
}

/*-----------------------------------------------------------------------------------------------*/

/* http://www.id3.org/ID3v1 */
/* http://en.wikipedia.org/wiki/ID3 */
/* http://de.wikipedia.org/wiki/Liste_der_ID3v1-Genres */

static const char *id3_genre[] =
{
	/*   0 */ "Blues",
	/*   1 */ "Classic Rock",
	/*   2 */ "Country",
	/*   3 */ "Dance",
	/*   4 */ "Disco",
	/*   5 */ "Funk",
	/*   6 */ "Grunge",
	/*   7 */ "Hip-Hop",
	/*   8 */ "Jazz",
	/*   9 */ "Metal",
	/*  10 */ "New Age",
	/*  11 */ "Oldies",
	/*  12 */ "Other",
	/*  13 */ "Pop",
	/*  14 */ "R&B",
	/*  15 */ "Rap",
	/*  16 */ "Reggae",
	/*  17 */ "Rock",
	/*  18 */ "Techno",
	/*  19 */ "Industrial",
	/*  20 */ "Alternative",
	/*  21 */ "Ska",
	/*  22 */ "Death Metal",
	/*  23 */ "Pranks",
	/*  24 */ "Soundtrack",
	/*  25 */ "Euro-Techno",
	/*  26 */ "Ambient",
	/*  27 */ "Trip-Hop",
	/*  28 */ "Vocal",
	/*  29 */ "Jazz+Funk",
	/*  30 */ "Fusion",
	/*  31 */ "Trance",
	/*  32 */ "Classical",
	/*  33 */ "Instrumental",
	/*  34 */ "Acid",
	/*  35 */ "House",
	/*  36 */ "Game",
	/*  37 */ "Sound Clip",
	/*  38 */ "Gospel",
	/*  39 */ "Noise",
	/*  40 */ "Alternative Rock",  /* "AlternRock" */
	/*  41 */ "Bass",
	/*  42 */ "Soul",
	/*  43 */ "Punk",
	/*  44 */ "Space",
	/*  45 */ "Meditative",
	/*  46 */ "Instrumental Pop",
	/*  47 */ "Instrumental Rock",
	/*  48 */ "Ethnic",
	/*  49 */ "Gothic",
	/*  50 */ "Darkwave",
	/*  51 */ "Techno-Industrial",
	/*  52 */ "Electronic",
	/*  53 */ "Pop-Folk",
	/*  54 */ "Eurodance",
	/*  55 */ "Dream",
	/*  56 */ "Southern Rock",
	/*  57 */ "Comedy",
	/*  58 */ "Cult",
	/*  59 */ "Gangsta",
	/*  60 */ "Top 40",
	/*  61 */ "Christian Rap",
	/*  62 */ "Pop/Funk",
	/*  63 */ "Jungle",
	/*  64 */ "Native American",
	/*  65 */ "Cabaret",
	/*  66 */ "New Wave",
	/*  67 */ "Psychedelic",
	/*  68 */ "Rave",
	/*  69 */ "Showtunes",
	/*  70 */ "Trailer",
	/*  71 */ "Lo-Fi",
	/*  72 */ "Tribal",
	/*  73 */ "Acid Punk",
	/*  74 */ "Acid Jazz",
	/*  75 */ "Polka",
	/*  76 */ "Retro",
	/*  77 */ "Musical",
	/*  78 */ "Rock & Roll",
	/*  79 */ "Hard Rock",
	/*  80 */ "Folk",
	/*  81 */ "Folk-Rock",
	/*  82 */ "National Folk",
	/*  83 */ "Swing",
	/*  84 */ "Fast Fusion",
	/*  85 */ "Bebob",
	/*  86 */ "Latin",
	/*  87 */ "Revival",
	/*  88 */ "Celtic",
	/*  89 */ "Bluegrass",
	/*  90 */ "Avantgarde",
	/*  91 */ "Gothic Rock",
	/*  92 */ "Progressive Rock",
	/*  93 */ "Psychedelic Rock",
	/*  94 */ "Symphonic Rock",
	/*  95 */ "Slow Rock",
	/*  96 */ "Big Band",
	/*  97 */ "Chorus",
	/*  98 */ "Easy Listening",
	/*  99 */ "Acoustic",
	/* 100 */ "Humour",
	/* 101 */ "Speech",
	/* 102 */ "Chanson",
	/* 103 */ "Opera",
	/* 104 */ "Chamber Music",
	/* 105 */ "Sonata",
	/* 106 */ "Symphony",
	/* 107 */ "Booty Bass",
	/* 108 */ "Primus",
	/* 109 */ "Porn Groove",
	/* 110 */ "Satire",
	/* 111 */ "Slow Jam",
	/* 112 */ "Club",
	/* 113 */ "Tango",
	/* 114 */ "Samba",
	/* 115 */ "Folklore",
	/* 116 */ "Ballad",
	/* 117 */ "Power Ballad",
	/* 118 */ "Rhythmic Soul",
	/* 119 */ "Freestyle",
	/* 120 */ "Duet",
	/* 121 */ "Punk Rock",
	/* 122 */ "Drum Solo",
	/* 123 */ "Acapella",
	/* 124 */ "Euro-House",
	/* 125 */ "Dance Hall",
	/* 126 */ "Goa",
	/* 127 */ "Drum & Bass",
	/* 128 */ "Club-House",
	/* 129 */ "Hardcore",
	/* 130 */ "Terror",
	/* 131 */ "Indie",
	/* 132 */ "BritPop",
	/* 133 */ "Negerpunk",
	/* 134 */ "Polsk Punk",
	/* 135 */ "Beat",
	/* 136 */ "Christian Gangsta Rap",
	/* 137 */ "Heavy Metal",
	/* 138 */ "Black Metal",
	/* 139 */ "Crossover",
	/* 140 */ "Contemporary Christian",
	/* 141 */ "Christian Rock",
	/* 142 */ "Merengue",
	/* 143 */ "Salsa",
	/* 144 */ "Thrash Metal",
	/* 145 */ "Anime",
	/* 146 */ "Jpop",
	/* 147 */ "Synthpop",
};

static void xml_string_append_id3v1( GString *str, const char *name, const char *value, const char *enhanced_value )
{
	char content[96], *s;

	memcpy( content, value, 30 );
	if ( enhanced_value != NULL )
	{
		memcpy( content + 30, enhanced_value, 60 );
		content[90] = '\0';
	}
	else
	{
		content[30] = '\0';
	}

	/* Strings are either space- or zero-padded. */
	s = strrchr( content, '\0' );
	while ( s != content && *--s == ' ' ) *s = '\0';

	/* Unset string entries are filled using an empty string. */
	if ( content[0] == '\0' ) return;

	/* Der Zeichensatz für die Textfelder ist nicht spezifiziert. Ueblich sind ASCII, ISO 8859-1 und Unicode, enkodiert als UTF-8. */
	if ( g_utf8_validate( content, -1, NULL ) )
	{
		TRACE(( "  = \"%s\"\n", content ));
		xml_string_append_boxed( str, name, content, NULL );
	}
	else
	{
		gchar *utf8_content = g_convert( content, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL );
		if ( utf8_content != NULL )
		{
			TRACE(( "  = \"%s\"\n", utf8_content ));
			xml_string_append_boxed( str, name, utf8_content, NULL );
			g_free( utf8_content );
		}
		else
		{
			TRACE(( "*** xml_string_append_id3v2(): g_convert( \"UTF-8\", \"ISO-8859-1\" ) failed\n" ));
		}
	}
}

#define ID3v2_FRAME_HEADER_SIZE    10
#define ID3v2_2_FRAME_HEADER_SIZE   6
#define MIN_PICTURE_DATA_SIZE       6

static void xml_string_append_id3v2_PIC( GString *str, long offset, const char *location, unsigned char *content, unsigned size )
{
	const char *image_format, *description;
	int text_encoding, picture_type;
	unsigned char *s = content;

	if ( size < 4 + MIN_PICTURE_DATA_SIZE )
	{
		TRACE(( "*** xml_string_append_id3v2_APIC(): Illegal frame size %d\n", size ));
		return;
	}

	text_encoding = *s++;
	image_format = (char *)s; s += 3;
	picture_type = *s++;
	description = (char *)s;
	if ( text_encoding == 0 || text_encoding == 3 )
	{
		for (;;)
		{
			unsigned char ch = *s++;
			if ( ch == '\0' ) break;
		}
	}
	else
	{
		for (;;)
		{
			unsigned char ch = s[0] | s[1];
			s += 2;
			if ( ch == '\0' ) break;
		}
	}

	TRACE(( "  text encoding = %d\n", text_encoding ));
	TRACE(( "  image format  = \"%c%c%c\"\n", image_format[0], image_format[1], image_format[2] ));
	TRACE(( "  picture type  = %d\n", picture_type ));

	if ( picture_type == 3 )
	{
		gchar *albumArtURI;

		if ( size < (unsigned)(s - content) + MIN_PICTURE_DATA_SIZE )
		{
			TRACE(( "*** xml_string_append_id3v2_PIC(): Illegal frame size %d\n", size ));
			return;
		}

		offset += (s - content);
		size -= (s - content);

		if ( image_format[0] == '-' && image_format[1] == '-' && image_format[2] == '>' /* "-->" */ )
		{
			albumArtURI = g_new( char, size + 1 );
			memcpy( albumArtURI, s, size );
			albumArtURI[size] = 0;
		}
		else
		{
			const char *mime_type;

			if ( image_format[0] == 'G' && image_format[1] == 'I' && image_format[2] == 'F' )
				mime_type = "image/gif";
			else if ( image_format[0] == 'P' && image_format[1] == 'N' && image_format[2] == 'G' )
				mime_type = "image/png";
			else if ( image_format[0] == 'J' && image_format[1] == 'P' && image_format[2] == 'G' )
				mime_type = "image/jpeg";
			else
			{
				TRACE(( "*** xml_string_append_id3v2_PIC(): Unknown image format \"%c%c%c\"\n", image_format[0], image_format[1], image_format[2] ));
				return;
			}

			albumArtURI = g_strdup_printf( "%s?bytes=%ld-%ld&content-type=%s",
				location, offset, offset + (long)size - 1, mime_type );
		}

		xml_string_append_boxed( str, "upnp:albumArtURI", albumArtURI, NULL );
		g_free( albumArtURI );
	}
}

static void xml_string_append_id3v2_APIC( GString *str, long offset, const char *location, unsigned char *content, unsigned size )
{
	const char *mime_type, *description;
	int text_encoding, picture_type;
	unsigned char *s = content;

	if ( size < 4 + MIN_PICTURE_DATA_SIZE )
	{
		TRACE(( "*** xml_string_append_id3v2_APIC(): Illegal frame size %d\n", size ));
		return;
	}

	text_encoding = *s++;
	mime_type = (char *)s;
	while ( *s++ != '\0' ) ;
	picture_type = *s++;
	description = (char *)s;
	if ( text_encoding == 0 || text_encoding == 3 )
	{
		for (;;)
		{
			unsigned char ch = *s++;
			if ( ch == '\0' ) break;
		}
	}
	else
	{
		for (;;)
		{
			unsigned char ch = s[0] | s[1];
			s += 2;
			if ( ch == '\0' ) break;
		}
	}

	TRACE(( "  text encoding = %d\n", text_encoding ));
	TRACE(( "  mime type     = \"%s\"\n", mime_type ));
	TRACE(( "  picture type  = %d\n", picture_type ));

	if ( picture_type == 3 )
	{
		gchar *albumArtURI;

		if ( size < (unsigned)(s - content) + MIN_PICTURE_DATA_SIZE )
		{
			TRACE(( "*** xml_string_append_id3v2_APIC(): Illegal frame size %d\n", size ));
			return;
		}

		offset += (s - content);
		size -= (s - content);

		if ( strcmp( mime_type, "-->" ) == 0 )
		{
			albumArtURI = g_new( char, size + 1 );
			memcpy( albumArtURI, s, size );
			albumArtURI[size] = 0;
		}
		else
		{
			/* Correct invalid mime type "jpg" */
			if ( strstr( mime_type, "jpg" ) != NULL ) mime_type = "jpeg";

			albumArtURI = g_strdup_printf( "%s?bytes=%ld-%ld&content-type=%s%s",
				location, offset, offset + (long)size - 1,
				( strchr( mime_type, '/' ) != NULL ) ? "" : "image/", mime_type );
		}

		xml_string_append_boxed( str, "upnp:albumArtURI", albumArtURI, NULL );
		g_free( albumArtURI );
	}
}

static void xml_string_append_id3v2_TEXT( GString *str, const char *name, unsigned char *content, unsigned size, gboolean replace_null_bytes )
{
	const char *from_codeset;
	gchar *utf8_content;
	int text_encoding;

	if ( size < 2 )
	{
		TRACE(( "*** xml_string_append_id3v2_TEXT(): Illegal frame size %d\n", size ));
		return;
	}

	text_encoding = content[0];
	if ( text_encoding == 0 ) from_codeset = "ISO-8859-1";
	else if ( text_encoding == 1 ) from_codeset = "UTF-16";
	else if ( text_encoding == 2 ) from_codeset = "UTF-16BE";
	else if ( text_encoding == 3 ) from_codeset = "UTF-8";
	else
	{
		TRACE(( "*** xml_string_append_id3v2(): Illegal text encoding %d\n", text_encoding ));
		return;
	}

	if ( replace_null_bytes )
	{
		/* ID3v2.4 uses a null byte to separate multiple values, so the character "/" can appear in text data again */
		if ( text_encoding == 0 || text_encoding == 3 )
		{
			unsigned i;

			while ( size > 1 && content[size-1] == '\0' ) size--;
			for ( i = 1; i < size; i++ ) if ( content[i] == '\0' ) content[i] = '/';
		}
		else
		{
			;  /* TODO */
		}
	}

	utf8_content = g_convert( (char *)++content, --size, "UTF-8", from_codeset, NULL, NULL, NULL );
	if ( utf8_content != NULL )
	{
		TRACE(( "  = \"%s\"\n", utf8_content ));

		if ( *utf8_content )
		{
			xml_string_append_boxed( str, name, utf8_content, NULL );
		}

		g_free( utf8_content );
	}
	else
	{
		TRACE(( "*** xml_string_append_id3v2(): g_convert( \"UTF-8\", \"%s\" ) failed\n", from_codeset ));
	}
}

static void xml_string_append_id3v2_INT( GString *str, const char *name, unsigned char *content, unsigned size )
{
	const char *from_codeset;
	gchar *utf8_content;
	int text_encoding;

	if ( size < 2 )
	{
		TRACE(( "*** xml_string_append_id3v2_INT(): Illegal frame size %d\n", size ));
		return;
	}

	text_encoding = content[0];
	if ( text_encoding == 0 ) from_codeset = "ISO-8859-1";
	else if ( text_encoding == 1 ) from_codeset = "UTF-16";
	else if ( text_encoding == 2 ) from_codeset = "UTF-16BE";
	else if ( text_encoding == 3 ) from_codeset = "UTF-8";
	else
	{
		TRACE(( "*** xml_string_append_id3v2(): Illegal text encoding %d\n", text_encoding ));
		return;
	}

	utf8_content = g_convert( (char *)++content, --size, "UTF-8", from_codeset, NULL, NULL, NULL );
	if ( utf8_content != NULL )
	{
		int value;

		TRACE(( "  = \"%s\"\n", utf8_content ));

		value = atoi( utf8_content );
		if ( value )
		{
			char buf[12];
			sprintf( buf, "%d", value );
			xml_string_append_boxed( str, name, buf, NULL );
		}

		g_free( utf8_content );
	}
	else
	{
		TRACE(( "*** xml_string_append_id3v2(): g_convert( \"UTF-8\", \"%s\" ) failed\n", from_codeset ));
	}
}

static void xml_string_append_id3v2( GString *str, gsize item_len, long offset, const char *location, gboolean album_art, unsigned char *content, unsigned size, gboolean replace_null_bytes )
{
	char *id = (char *)content - ID3v2_FRAME_HEADER_SIZE;

	if ( id[0] == 'A' && id[1] == 'P' && id[2] == 'I' && id[3] == 'C' )
	{
		if ( !album_art )
			xml_string_append_id3v2_APIC( str, offset, location, content, size );
	}
	else if ( id[0] == 'T' )
	{
		if ( id[1] == 'A' && id[2] == 'L' && id[3] == 'B' )
		{
			xml_string_append_id3v2_TEXT( str, "upnp:album", content, size, replace_null_bytes );
		}
		else if ( id[1] == 'C' && id[2] == 'O' && id[3] == 'M' )
		{
			xml_string_append_id3v2_TEXT( str, "upnp:author role=\"Composer\"", content, size, replace_null_bytes );
		}
		else if ( id[1] == 'C' && id[2] == 'O' && id[3] == 'N' )
		{
			xml_string_append_id3v2_TEXT( str, "upnp:genre", content, size, replace_null_bytes );
		}
		else if ( id[1] == 'D' && id[2] == 'A' && id[3] == 'T' )  /* ID3v2.3 */
		{
			if ( strstr( str->str + item_len, "<dc:date>" ) == NULL )
				xml_string_append_id3v2_TEXT( str, "dc:date", content, size, replace_null_bytes );
		}
		else if ( id[1] == 'D' && id[2] == 'R' && id[3] == 'L' )  /* ID3v2.4 */
		{
			if ( strstr( str->str + item_len, "<dc:date>" ) == NULL )
				xml_string_append_id3v2_TEXT( str, "dc:date", content, size, replace_null_bytes );
		}
		else if ( id[1] == 'I' && id[2] == 'T' && id[3] == '2' )
		{
			xml_string_append_id3v2_TEXT( str, "dc:title", content, size, replace_null_bytes );
		}
		else if ( id[1] == 'P' && id[2] == 'E' )
		{
			if ( strstr( str->str + item_len, "<upnp:artist" ) == NULL )
				xml_string_append_id3v2_TEXT( str, "upnp:artist", content, size, replace_null_bytes );
		}
		else if ( id[1] == 'R' && id[2] == 'C' && id[3] == 'K' )
		{
			xml_string_append_id3v2_INT( str, "upnp:originalTrackNumber", content, size );
		}
		else if ( id[1] == 'Y' && id[2] == 'E' && id[3] == 'R' )  /* ID3v2.3 */
		{
			if ( strstr( str->str + item_len, "<dc:date>" ) == NULL )
				xml_string_append_id3v2_TEXT( str, "dc:date", content, size, replace_null_bytes );
		}
	}
}

gboolean get_id3v1_metadata( GString *item, const char tag[128], const char enhanced_tag[227] )
{
	char value[8];
	int genre;

	xml_string_append_id3v1( item, "dc:title", tag + 3, enhanced_tag + 4 );
	xml_string_append_id3v1( item, "upnp:artist", tag + 33, enhanced_tag + 64 );
	xml_string_append_id3v1( item, "upnp:album", tag + 63, enhanced_tag + 124 );

	strncpy( value, tag + 93, 4 )[4] = '\0';
	if ( value[0] != '\0' ) xml_string_append_boxed( item, "dc:date", value, NULL );

	/*xml_string_append_id3v1( item, "leiads:comment", tag + 97, NULL );*/

	if ( tag[125] == '\0' && tag[126] != '\0' )
	{
		sprintf( value, "%u", (unsigned char)tag[126] );
		xml_string_append_boxed( item, "upnp:originalTrackNumber", value, NULL );
	}

	genre = (unsigned char)tag[127];
	if ( genre <= 147 )
	{
		xml_string_append_boxed( item, "upnp:genre", id3_genre[genre], NULL );
	}
	else if ( genre != 0xFF )
	{
		xml_string_append_boxed( item, "upnp:genre", "Unknown", NULL );
	}
	else if ( enhanced_tag[185] != '\0' )
	{
		xml_string_append_id3v1( item, "upnp:genre", enhanced_tag + 185, NULL );
	}

	return TRUE;
}

gboolean get_id3v2_2_metadata( GString *item, FILE *fp, const char *location, gboolean album_art )
{
	unsigned char header[6], *buf, *s;
	gsize item_len = item->len;
	unsigned total_size, size;
	long offset;

	if ( fread( header, 1, 6, fp ) != 6 )
	{
		TRACE(( "*** get_id3v2_2_metadata(): Unable to read ID3v2 header, aborting\n" ));
		return FALSE;
	}

	if ( (header[1] & 0x80) != 0 )
	{
		TRACE(( "*** get_id3v2_2_metadata(): We don't support Unsynchronisation yet, aborting\n" ));
		return FALSE;
	}
	if ( (header[1] & 0x40) != 0 )
	{
		/* "Since no compression scheme has been decided yet, the ID3 decoder should just ignore
		    the entire tag if the compression is set." */
		TRACE(( "*** get_id3v2_2_metadata(): Unable to support Compression, aborting\n" ));
		return FALSE;
	}

	total_size = ID3_DWORD( header + 2 );
	TRACE(( "get_id3v2_2_metadata(): total size = %u\n", total_size ));
	if ( total_size <= ID3v2_FRAME_HEADER_SIZE ) return TRUE;

	buf = g_try_new( unsigned char, total_size );
	if ( buf == NULL )
	{
		TRACE(( "*** get_id3v2_2_metadata(): unable to allocate %u bytes, abort\n", total_size ));
		return FALSE;
	}
	if ( fread( buf, 1, total_size, fp ) != total_size )
	{
		TRACE(( "*** get_id3v2_2_metadata(): illegal total size %u, abort\n", total_size ));
		return FALSE;
	}

	s = buf;
	offset = ID3v2_FRAME_HEADER_SIZE;

	for (;;)
	{
		unsigned char *id = s;

		if ( (buf + total_size) - s < ID3v2_2_FRAME_HEADER_SIZE ) break;  /* EOF */

		if ( id[0] < 'A' || id[0] > 'Z' )
		{
			TRACE(( "*** get_id3v2_2_metadata(): illegal frame header, abort\n" ));
			break;
		}

		s += ID3v2_2_FRAME_HEADER_SIZE;
		offset += ID3v2_2_FRAME_HEADER_SIZE;

		size = ID3_2_DWORD( id + 3 );
		TRACE(( "get_id3v2_2_metadata(): %c%c%c, frame size = %u\n", id[0], id[1], id[2], size ));
		if ( (unsigned)((buf + total_size) - s) < size )
		{
			TRACE(( "*** get_id3v2_2_metadata(): illegal frame size %u, abort\n", size ));
			break;
		}

		if ( id[0] == 'P' && id[1] == 'I' && id[2] == 'C' )
		{
			if ( !album_art )
				xml_string_append_id3v2_PIC( item, offset, location, s, size );
		}
		else if ( id[0] == 'T' )
		{
			if ( id[1] == 'A' && id[2] == 'L' )  /* Album/Movie/Show title */
			{
				xml_string_append_id3v2_TEXT( item, "upnp:album", s, size, FALSE );
			}
			else if ( id[1] == 'C' && id[2] == 'M' )  /* Composer */
			{
				xml_string_append_id3v2_TEXT( item, "upnp:author role=\"Composer\"", s, size, FALSE );
			}
			else if ( id[1] == 'C' && id[2] == 'O' )  /* Content type */
			{
				xml_string_append_id3v2_TEXT( item, "upnp:genre", s, size, FALSE );
			}
			else if ( id[1] == 'D' && id[2] == 'A' )  /* Date */
			{
				if ( strstr( item->str + item_len, "<dc:date>" ) == NULL )
					xml_string_append_id3v2_TEXT( item, "dc:date", s, size, FALSE );
			}
			else if ( id[1] == 'P' )                  /* Band/Orchestra/Accompaniment */
			{
				if ( strstr( item->str + item_len, "<upnp:artist" ) == NULL )
					xml_string_append_id3v2_TEXT( item, "upnp:artist", s, size, FALSE );
			}
			else if ( id[1] == 'R' && id[2] == 'K' )  /* Track number/Position in set */
			{
				xml_string_append_id3v2_INT( item, "upnp:originalTrackNumber", s, size );
			}
			else if ( id[1] == 'R' && id[2] == 'D' )  /* Recording dates */
			{
				if ( strstr( item->str + item_len, "<dc:date>" ) == NULL )
					xml_string_append_id3v2_TEXT( item, "dc:date", s, size, FALSE );
			}
			else if ( id[1] == 'T' && id[2] == '2' )  /* Title/Songname/Content description */
			{
				xml_string_append_id3v2_TEXT( item, "dc:title", s, size, FALSE );
			}
			else if ( id[1] == 'Y' && id[2] == 'E' )  /* Year */
			{
				if ( strstr( item->str + item_len, "<dc:date>" ) == NULL )
					xml_string_append_id3v2_TEXT( item, "dc:date", s, size, FALSE );
			}
		}

		s += size;
		offset += size;
	}

	g_free( buf );
	return TRUE;
}

gboolean get_id3v2_3_metadata( GString *item, FILE *fp, const char *location, gboolean album_art )
{
	unsigned char header[6], *buf, *s;
	gsize item_len = item->len;
	unsigned total_size, size;
	long offset;

	if ( fread( header, 1, 6, fp ) != 6 )
	{
		TRACE(( "*** get_id3v2_3_metadata(): Unable to read ID3v2 header, aborting\n" ));
		return FALSE;
	}

	if ( (header[1] & 0x80) != 0 )
	{
		TRACE(( "*** get_id3v2_3_metadata(): We don't support Unsynchronisation yet, aborting\n" ));
		return FALSE;
	}

	total_size = ID3_DWORD( header + 2 );
	TRACE(( "get_id3v2_3_metadata(): total size = %u\n", total_size ));
	if ( total_size <= ID3v2_FRAME_HEADER_SIZE ) return TRUE;

	buf = g_try_new( unsigned char, total_size );
	if ( buf == NULL )
	{
		TRACE(( "*** get_id3v2_3_metadata(): unable to allocate %u bytes, abort\n", total_size ));
		return FALSE;
	}
	if ( fread( buf, 1, total_size, fp ) != total_size )
	{
		TRACE(( "*** get_id3v2_3_metadata(): illegal total size %u, abort\n", total_size ));
		return FALSE;
	}

	s = buf;
	offset = ID3v2_FRAME_HEADER_SIZE;

	if ( (header[1] & 0x40) != 0 )
	{
		/* Skip extended header */
		unsigned size = BIG_DWORD( s );
		TRACE(( "get_id3v2_3_metadata(): extended header size = %u\n", size ));
		size += 4;   /* The 'Extended header size' excludes itself */

		s += size;
		offset += size;
	}

	for (;;)
	{
		unsigned char *header = s;

		if ( (buf + total_size) - s < ID3v2_FRAME_HEADER_SIZE ) break;  /* EOF */

		if ( header[0] < 'A' || header[0] > 'Z' )
		{
			TRACE(( "*** get_id3v2_3_metadata(): illegal frame header, abort\n" ));
			break;
		}

		s += ID3v2_FRAME_HEADER_SIZE;
		offset += ID3v2_FRAME_HEADER_SIZE;

		size = BIG_DWORD( header + 4 );
		TRACE(( "get_id3v2_3_metadata(): %c%c%c%c, frame size = %u\n", header[0], header[1], header[2], header[3], size ));
		if ( (unsigned)((buf + total_size) - s) < size )
		{
			TRACE(( "*** get_id3v2_3_metadata(): illegal frame size %u, abort\n", size ));
			break;
		}

		if ( (header[9] & 0x08) != 0 )
		{
			TRACE(( "*** get_id3v2_3_metadata(): We don't support Compression, continuing\n" ));
			continue;
		}
		if ( (header[9] & 0x04) != 0 )
		{
			TRACE(( "*** get_id3v2_3_metadata(): We don't support Encryption, continuing\n" ));
			continue;
		}
		if ( (header[9] & 0x02) != 0 )
		{
			/* Grouping identity */
			s++; offset++; size--;  /* Skip Group byte */
		}

		xml_string_append_id3v2( item, item_len, offset, location, album_art, s, size, FALSE );

		s += size;
		offset += size;
	}

	g_free( buf );
	return TRUE;
}

gboolean get_id3v2_4_metadata( GString *item, FILE *fp, const char *location, gboolean album_art )
{
	unsigned char header[6], *buf, *s;
	gsize item_len = item->len;
	unsigned total_size, size;
	long offset;

	if ( fread( header, 1, 6, fp ) != 6 )
	{
		TRACE(( "*** get_id3v2_4_metadata(): Unable to read ID3v2 header, aborting\n" ));
		return FALSE;
	}

	if ( (header[1] & 0x80) != 0 )
	{
		TRACE(( "*** get_id3v2_4_metadata(): We don't support Unsynchronisation yet, aborting\n" ));
		return FALSE;
	}

	total_size = ID3_DWORD( header + 2 );
	TRACE(( "get_id3v2_4_metadata(): total size = %u\n", total_size ));
	if ( total_size <= ID3v2_FRAME_HEADER_SIZE ) return TRUE;

	buf = g_try_new( unsigned char, total_size );
	if ( buf == NULL )
	{
		TRACE(( "*** get_id3v2_4_metadata(): unable to allocate %u bytes, abort\n", total_size ));
		return FALSE;
	}
	if ( fread( buf, 1, total_size, fp ) != total_size )
	{
		TRACE(( "*** get_id3v2_4_metadata(): illegal total size %u, abort\n", total_size ));
		return FALSE;
	}

	s = buf;
	offset = ID3v2_FRAME_HEADER_SIZE;


	if ( (header[1] & 0x40) != 0 )
	{
		/* Skip extended header */
		unsigned size = ID3_DWORD( s );
		TRACE(( "get_id3v2_4_metadata(): extended header size = %u\n", size ));
		if ( size < 6 )  /* An extended header can never have a size fewer than six bytes */
		{
			TRACE(( "*** get_id3v2_4_metadata(): illegal extended header size %u, abort\n", size ));
			return FALSE;
		}

		s += size;
		offset += size;
	}

	for (;;)
	{
		unsigned char *header = s;

		if ( (buf + total_size) - s < ID3v2_FRAME_HEADER_SIZE ) break;  /* EOF */

		if ( header[0] < 'A' || header[0] > 'Z' )
		{
			TRACE(( "*** get_id3v2_4_metadata(): illegal frame header, abort\n" ));
			break;
		}

		s += ID3v2_FRAME_HEADER_SIZE;
		offset += ID3v2_FRAME_HEADER_SIZE;

		size = ID3_DWORD( s + 4 );
		TRACE(( "get_id3v2_4_metadata(): %c%c%c%c, frame size = %u\n", header[0], header[1], header[2], header[3], size ));
		if ( (unsigned)((buf + total_size) - s) < size )
		{
			TRACE(( "*** get_id3v2_4_metadata(): illegal frame size %u, abort\n", size ));
			break;
		}

		if ( (header[9] & 0x08) != 0 )
		{
			TRACE(( "*** get_id3v2_4_metadata(): We don't support Compression, continuing\n" ));
			continue;
		}
		if ( (header[9] & 0x04) != 0 )
		{
			TRACE(( "*** get_id3v2_4_metadata(): We don't support Encryption, continuing\n" ));
			continue;
		}
		if ( (header[9] & 0x02) != 0 )
		{
			TRACE(( "*** get_id3v2_4_metadata(): We don't support Unsynchronisation yet, continuing\n" ));
			continue;
		}

		xml_string_append_id3v2( item, item_len, offset, location, album_art, s, size, TRUE );

		s += size;
		offset += size;
	}

	g_free( buf );
	return TRUE;
}

/*-----------------------------------------------------------------------------------------------*/

void start_get_metadata( const char *filename, GString *item, const char *server_address, gchar **location, const char *protocol_info )
{
	gchar *encoded_filename;
	char buffer[64];

	g_string_append_printf( item, "<item id=\"leiads:local:%s\">", md5cpy( buffer, filename ) );
	xml_string_append_boxed( item, "upnp:class", "object.item.audioItem.musicTrack", NULL );

	encoded_filename = url_encoded_path( filename );
	*location = g_strconcat( "http://", server_address, "/file//", encoded_filename, NULL );
	g_free( encoded_filename );
	sprintf( buffer, "http-get:*:audio/%s:*", protocol_info );
	xml_string_append_boxed( item, "res", *location, "protocolInfo", buffer, NULL );
}

void end_get_metadata( const char *filename, GString *item, gsize item_len, const char *server_address, const char *album_art_filename )
{
	if ( strstr( item->str + item_len, "<dc:title>" ) == NULL )
	{
		gchar *basename = g_path_get_basename( filename );
		gchar *s = strrchr( basename, '.' );
		if ( s != NULL ) *s = '\0';
		xml_string_append_boxed( item, "dc:title", basename, NULL );
		g_free( basename );
	}

	if ( album_art_filename != NULL )
	{
		gchar *encoded_filename = url_encoded_path( album_art_filename );
		gchar *location = g_strconcat( "http://", server_address, "/file//", encoded_filename, NULL );

		xml_string_append_boxed( item, "upnp:albumArtURI", location, NULL );

		g_free( location );
		g_free( encoded_filename );
	}

	g_string_append( item, "</item>" );
}

gboolean GetMetadataFromFile( const char *filename, const char *album_art_filename, GString *item, gboolean playlists, GError **error )
{
	gboolean result = FALSE;

	TRACE(( "GetMetadataFromFile( %s )...\n", (filename != NULL) ? filename : "<NULL>" ));

	if ( filename != NULL )
	{
		FILE *fp = fopen_utf8( filename, "rb" );

		if ( fp != NULL )
		{
			gchar *server_address = http_get_server_address( GetAVTransport() );
			unsigned char header[4];

			if ( fread( header, 1, 4, fp ) == 4 )
			{
				gsize item_len = item->len;
				gchar *location;
				int x;

				if ( header[0] == 'f' && header[1] == 'L' && header[2] == 'a' && header[3] == 'C' )
				{
					start_get_metadata( filename, item, server_address, &location, "x-flac" );
					get_flac_metadata( item, fp, location, album_art_filename != NULL );
					end_get_metadata( filename, item, item_len, server_address, album_art_filename );

					result = TRUE;
				}
				else if ( header[0] == 'O' && header[1] == 'g' && header[2] == 'g' && header[3] == 'S' )
				{
					start_get_metadata( filename, item, server_address, &location, "ogg" );
					get_ogg_metadata( item, fp );
					end_get_metadata( filename, item, item_len, server_address, album_art_filename );

					result = TRUE;
				}
				else if ( header[0] == 'I' && header[1] == 'D' && header[2] == '3' && header[3] != '\xFF' )
				{
					start_get_metadata( filename, item, server_address, &location, "mpeg" );

					switch ( header[3] )
					{
					case 2:
						get_id3v2_2_metadata( item, fp, location, album_art_filename != NULL );
						break;
					case 3:
						get_id3v2_3_metadata( item, fp, location, album_art_filename != NULL );
						break;
					case 4:
						get_id3v2_4_metadata( item, fp, location, album_art_filename != NULL );
						break;
					default:
						TRACE(( "******* GetMetadataFromFile(): Unsupported version ID3v2.%d\n", header[3] ));
					}

					end_get_metadata( filename, item, item_len, server_address, album_art_filename );

					result = TRUE;
				}
				else if ( header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F' )
				{
					unsigned char buf[0x20];
					if ( fread( buf, 1, 0x20, fp ) == 0x20 && strncmp( (char *)buf + 4, "WAVEfmt", 7 ) == 0 &&
					     LITTLE_WORD( buf + 0x14 - 4 ) == 1  /* wFormatTag == PCM */ &&
					     LITTLE_WORD( buf + 0x16 - 4 ) == 2  /* wChannels  == 2 */   &&
					     LITTLE_WORD( buf + 0x22 - 4 ) == 16 /* wBitsPerSample == 16 */ )
					{
						start_get_metadata( filename, item, server_address, &location, "L16" );
						end_get_metadata( filename, item, item_len, server_address, album_art_filename );

						result = TRUE;
					}
					else
					{
						g_set_error( error, 0, -1, Text(ERRMSG_UNKNOWN_FILE_FORMAT), filename );
					}
				}
				else if ( playlists && header[0] == '<' &&
				        ((header[1] == 'P' && header[2] == 'l' && header[3] == 'a') ||
				         (header[1] == '?' && header[2] == 'x' && header[3] == 'm') ||
				         (header[1] == 'l' && header[2] == 'i' && header[3] == 'n')) )
				{
					struct xml_content *playlist;
					int tracks_not_added;
					GError *error = NULL;

					playlist = LoadPlaylist( filename, &tracks_not_added, &error );
					if ( playlist != NULL )
					{
						g_string_append( item, playlist->str );
						free_xml_content( playlist );

						HandleNotAddedTracks( tracks_not_added );
					}

					HandleError( error, Text(ERROR_PLAYLIST_LOAD) );
				}
				else if ( header[0] == '\x89' && header[1] == 'P' && header[2] == 'N' && header[3] == 'G' )
				{
					;  /* PNG, nothing to do */
					g_set_error( error, 0, -1, Text(ERRMSG_UNKNOWN_FILE_FORMAT), filename );
				}
				else if ( (x = fseek( fp, -128, SEEK_END )) == 0 )
				{
					char tag[128], enhanced_tag[227];

					if ( fread( tag, 1, 128, fp ) == 128 && tag[0] == 'T' && tag[1] == 'A' && tag[2] == 'G' )
					{
						if ( fseek( fp, -(128+227), SEEK_CUR ) == 0 && fread( enhanced_tag, 1, 227, fp ) == 227 &&
						     enhanced_tag[0] == 'T' && enhanced_tag[1] == 'A' && enhanced_tag[2] == 'G' && enhanced_tag[3] == '+' )
							;
						else
							memset( enhanced_tag, 0, sizeof( enhanced_tag ) );

						start_get_metadata( filename, item, server_address, &location, "mpeg" );
						get_id3v1_metadata( item, tag, enhanced_tag );
						end_get_metadata( filename, item, item_len, server_address, album_art_filename );

						result = TRUE;
					}
					else
					{
						const char *s = strrchr( filename, '\0' ) - 4;
						if ( s[0] == '.' && (s[1] == 'm' || s[1] == 'M') && (s[2] == 'p' || s[2] == 'P') && s[3] == '3' )
						{
							start_get_metadata( filename, item, server_address, &location, "mpeg" );
							end_get_metadata( filename, item, item_len, server_address, album_art_filename );

							result = TRUE;
						}
						else
						{
							g_set_error( error, 0, -1, Text(ERRMSG_UNKNOWN_FILE_FORMAT), filename );
						}
					}
				}
				else
				{
					g_set_error( error, 0, -1, Text(ERRMSG_UNKNOWN_FILE_FORMAT), filename );
				}
			}

			g_free( server_address );

			fclose( fp );
		}
		else
		{
			g_set_error( error, 0, -1, Text(ERRMSG_OPEN_FILE), filename );
		}
	}

	return result;
}

/*-----------------------------------------------------------------------------------------------*/
/* Drag & Drop support */

void GetMetadataFromFolder( const char *path, GString *items, GError **error )
{
	const gchar *album_art_filename;
	GList *dir, *l;

	TRACE(( "GetMetadataFromFolder( %s )...\n", path ));

	dir = get_sorted_dir_content( Settings->browser.local_folder_path, &album_art_filename, error );

	for ( l = dir; l != NULL; l = l->next )
	{
		struct dir_entry *entry = (struct dir_entry *)l->data;

		if ( (entry->st_mode & S_IFDIR) != 0 )
		{
			GetMetadataFromFolder( entry->filename, items, error );
		}
		else if ( (entry->st_mode & S_IFREG) != 0 )
		{
			GetMetadataFromFile( entry->filename, album_art_filename, items, FALSE, NULL );
		}
	}

	free_sorted_dir_content( dir );
}

/*-----------------------------------------------------------------------------------------------*/

static gint compare_func( struct dir_entry *a, struct dir_entry *b )
{
	const char *name1, *name2;
	int result;

	if ( (a->st_mode & S_IFDIR) != 0 && (b->st_mode & S_IFDIR) == 0 )
		return -1;

	if ( (a->st_mode & S_IFDIR) == 0 && (b->st_mode & S_IFDIR) != 0 )
		return 1;

	name1 = a->name;
	name2 = b->name;
	result = 0;

	for (;;)
	{
		char c1, c2;

		c1 = *name1++;
		c2 = *name2++;

		if ( c1 >= '0' && c1 <= '9' && c2 >= '0' && c2 <= '9' )
		{
			/* Compare numbers */
			unsigned long l1 = strtoul( --name1, (char **)&name1, 10 );
			unsigned long l2 = strtoul( --name2, (char **)&name2, 10 );
			if ( l1 < l2 ) return -1;
			if ( l1 != l2 ) return 1;
		}
		else if ( c1 != c2 )
		{
			/* Treat '.' < ' ', so "a.png" < "a-2.png" */
			if ( c1 == '.' ) return -1;
			if ( c2 == '.' ) return 1;

			/* If the names are the same except case, the first different character counts */
			if ( result == 0 ) result = c1 - c2;

			/* Compare characters case-insensitive */
			/* Note: ' ' will get '\0' here, so we have to test for '\0' before */
			if ( c1 == '\0' || c2 == '\0' ) break;
			c1 &= ~0x20;
			c2 &= ~0x20;
			if ( c1 != c2 ) return c1 - c2;
		}
		else if ( c1 == '\0' )
		{
			break;
		}
	}

	return result;
}

static const char *album_art_filenames[] =
{
	"cover.png",  "folder.png",
	"cover.gif",  "folder.gif",
	"cover.jpg",  "folder.jpg",
	"cover.jpeg",  "folder.jpeg"
};

GList *get_sorted_dir_content( const gchar *path, const gchar **album_art_filename, GError **error )
{
	GList *result = NULL;
	const gchar *name;
	GDir *dir;
	int i;

	TRACE(( "get_sorted_dir_content( \"%s\" )...\n", path ));

	*album_art_filename = NULL;

	dir = g_dir_open( path, 0 /* flags: Currently must be set to 0. Reserved for future use. */, error );
	if ( dir != NULL )
	{
		while ( (name = g_dir_read_name( dir )) != NULL )
		{
			if ( name[0] != '.' )
			{
				gchar *filename = g_build_filename( path, name, NULL );
				struct stat s;

				if ( g_stat( filename, &s ) == 0 )
				{
					struct dir_entry *entry = g_new( struct dir_entry, 1 );

					strncpy( entry->name, name, sizeof( entry->name ) - 1 )[sizeof( entry->name ) - 1] = '\0';
					entry->filename = filename;
					entry->st_mode = s.st_mode;

					result = g_list_append( result, entry );
				}
				else
				{
					TRACE(( "*** get_sorted_dir_content(): g_stat( %s ) failed\n", filename ));
					g_free( filename );
				}
			}
		}

		g_dir_close( dir );

		for ( i = 0; i < G_N_ELEMENTS(album_art_filenames); i++ )
		{
			GList *l;

			for ( l = result; l != NULL; l = l->next )
			{
				struct dir_entry *entry = (struct dir_entry *)l->data;

				if ( (entry->st_mode & S_IFREG) != 0 && stricmp( entry->name, album_art_filenames[i] ) == 0 )
				{
					*album_art_filename = entry->filename;
					break;
				}
			}
			if ( *album_art_filename != NULL ) break;
		}
	}

	return g_list_sort( result, (GCompareFunc)compare_func );
}

static void _free_sorted_dir_content( gpointer data, gpointer user_data )
{
	if ( data != NULL )
	{
		struct dir_entry *entry = (struct dir_entry *)data;
		g_free( entry->filename );
		g_free( entry );
	}
}

void free_sorted_dir_content( GList *l )
{
	g_list_foreach( l, _free_sorted_dir_content, NULL );
	g_list_free( l );
}

void free_dir_entry( GList *l )
{
	if ( l != NULL && l->data != NULL )
	{
		_free_sorted_dir_content( l->data, NULL );
		l->data = NULL;
	}
}

/*-----------------------------------------------------------------------------------------------*/
#endif

