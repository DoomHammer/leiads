/*
	intl.c
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
	along with Leia D  If not, see <http://www.gnu.org/licenses/>.
*/

#include "leia-ds.h"
#include <ctype.h>

#ifdef WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN  /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#endif

const char *Texts[NUM_TEXT];

/*-----------------------------------------------------------------------------------------------*/

/* Folder names in Twonky Media
   (These will be used to enter the "Music" directory automatically) */

const char *Music[] =
{
	"Music",   /* English */
	"Musik",   /* German */
	"Musique", /* French */
	"Música" , /* Spanish */
	NULL
};

const char *Photos[] =
{
	"Photos",  /* English */
	"Bilder",  /* German */
	"Fotos",   /* German */
	"Images",  /* French */
	"Fótos",   /* Spanish */	
	NULL
};

const char *Videos[] =
{
	"Video",   /* English & German & Spanish */
	"Videos",  /* English & German & Spanish */
	"Vidéos",  /* French */
	NULL
};

const char *Folder[] =
{
	"Folder",       /* English */
	"Verzeichnis",  /* German */
	"Répertoire",  /* French */
	"Directorio",   /* Spanish */
	NULL
};

/*-----------------------------------------------------------------------------------------------*/

static char language[4];

static const char *set_language( const char *lang )
{
	language[0] = lang[0];
	language[1] = lang[1];
	language[2] = '\0';
	return language;
}

const char *GetLanguage( void )
{
	if ( language[0] == '\0' )
	{
	#ifdef WIN32

		static const char lang_codes[] =
			/* see http://en.wikipedia.org/wiki/ISO_639-1 */
			"??"  /* LANG_NEUTRAL */
			"ar"  /* LANG_ARABIC                      0x01 */
			"bg"  /* LANG_BULGARAN                    0x02 */
			"ca"  /* LANG_CATALAN                     0x03 */
			"zh"  /* LANG_CHINESE                     0x04 */
			"cs"  /* LANG_CZECH                       0x05 */
			"da"  /* LANG_DANISH                      0x06 */
			"de"  /* LANG_GERMAN                      0x07 */
			"el"  /* LANG_GREEK                       0x08 */
			"en"  /* LANG_ENGLISH                     0x09 */
			"es"  /* LANG_SPANISH                     0x0a */
			"fi"  /* LANG_FINNISH                     0x0b */
			"fr"  /* LANG_FRENCH                      0x0c */
			"he"  /* LANG_HEBREW                      0x0d */
			"hu"  /* LANG_HUNGARIAN                   0x0e */
			"is"  /* LANG_ICELANDIC                   0x0f */
			"it"  /* LANG_ITALIAN                     0x10 */
			"ja"  /* LANG_JAPANESE                    0x11 */
			"ko"  /* LANG_KOREAN                      0x12 */
			"nl"  /* LANG_DUTCH                       0x13 */
			"no"  /* LANG_NORWEGIAN                   0x14 */
			"pl"  /* LANG_POLISH                      0x15 */
			"pt"  /* LANG_PORTUGUESE                  0x16 */
			"??"  /* 0x17 */
			"ro"  /* LANG_ROMANIAN                    0x18 */
			"ru"  /* LANG_RUSSIAN                     0x19 */
			"hr"  /* LANG_SERBIAN & LANG_CROATIAN     0x1a */
			"sk"  /* LANG_SLOVAK                      0x1b */
			"sq"  /* LANG_ALBANIAN                    0x1c */
			"sv"  /* LANG_SWEDISH                     0x1d */
			"th"  /* LANG_THAI                        0x1e */
			"tr"  /* LANG_TURKISH                     0x1f */
			"ur"  /* LANG_URDU                        0x20 */
			"id"  /* LANG_INDONESIAN                  0x21 */
			"uk"  /* LANG_UKRAINIAN                   0x22 */
			"be"  /* LANG_BELARUSIAN                  0x23 */
			"sl"  /* LANG_SLOVENIAN                   0x24 */
			"et"  /* LANG_ESTONIAN                    0x25 */
			"lv"  /* LANG_LATVIAN                     0x26 */
			"lt"  /* LANG_LITHUANIAN                  0x27 */
			"??"  /* 0x28 */
			"fa"  /* LANG_FARSI                       0x29 */
			"vi"  /* LANG_VIETNAMESE                  0x2a */
			"hy"  /* LANG_ARMENIAN                    0x2b */
			"az"  /* LANG_AZERI                       0x2c */
			"eu"  /* LANG_BASQUE                      0x2d */
			"??"  /* 0x2e */
			"mk"  /* LANG_MACEDONIAN                  0x2f */
			"??"  /* 0x30 */
			"??"  /* 0x31 */
			"??"  /* 0x32 */
			"??"  /* 0x33 */
			"??"  /* 0x34 */
			"??"  /* 0x35 */
			"af"  /* LANG_AFRIKAANS                   0x36 */
			"ka"  /* LANG_GEORGIAN                    0x37 */
			"fo"  /* LANG_FAEROESE                    0x38 */
			"hi"  /* LANG_HINDI                       0x39 */
			"??"  /* 0x3a */
			"??"  /* 0x3b */
			"??"  /* 0x3c */
			"??"  /* 0x3d */
			"ml"  /* LANG_MALAY                       0x3e */
			"kk"  /* LANG_KAZAK                       0x3f */
			"ky"  /* LANG_KYRGYZ                      0x40 */
			"sw"  /* LANG_SWAHILI                     0x41 */
			"??"  /* 0x42 */
			"uz"  /* LANG_UZBEK                       0x43 */
			"tt"  /* LANG_TATAR                       0x44 */
			"bn"  /* LANG_BENGALI                     0x45 */
			"pa"  /* LANG_PUNJABI                     0x46 */
			"gu"  /* LANG_GUJARATI                    0x47 */
			"or"  /* LANG_ORIYA                       0x48 */
			"ta"  /* LANG_TAMIL                       0x49 */
			"te"  /* LANG_TELUGU                      0x4a */
			"kn"  /* LANG_KANNADA                     0x4b */
			"ml"  /* LANG_MALAYALAM                   0x4c */
			"as"  /* LANG_ASSAMESE                    0x4d */
			"mr"  /* LANG_MARATHI                     0x4e */
			"sa"  /* LANG_SANSKRIT                    0x4f */
			"mn"  /* LANG_MONGOLIAN                   0x50 */
			"??"  /* 0x51 */
			"??"  /* 0x52 */
			"??"  /* 0x53 */
			"??"  /* 0x54 */
			"??"  /* 0x55 */
			"gl"  /* LANG_GALICIAN                    0x56 */
			"??"  /* LANG_KONKANI                     0x57 */
			"??"  /* LANG_MANIPURI                    0x58 */
			"sd"  /* LANG_SINDHI                      0x59 */
			"??"  /* LANG_SYRIAC                      0x5a */
			"??"  /* 0x5b */
			"??"  /* 0x5c */
			"??"  /* 0x5d */
			"??"  /* 0x5e */
			"??"  /* 0x5f */
			"ks"  /* LANG_KASHMIRI                    0x60 */
			"ne"  /* LANG_NEPALI                      0x61 */
		;

		LANGID lang_id;
		WORD   primary_lang_id;
		const char *lang;

	#if 0  /* Needs Platform SDK installed or a compiler newer than Visual Studio 6.0 */
		lang_id = GetSystemDefaultUILanguage();
	#else
		HMODULE hModule = LoadLibrary( "kernel32.dll" );
		LANGID (*GetSystemDefaultUILanguageProc)( void );
		ASSERT( hModule != 0 );
		GetSystemDefaultUILanguageProc = (LANGID (*)( void ))GetProcAddress( hModule, "GetSystemDefaultUILanguage" );
		lang_id = (*GetSystemDefaultUILanguageProc)();
		FreeLibrary( hModule );
	#endif

		TRACE(( "lang_id = %#x\n", lang_id ));

		if ( lang_id == MAKELANGID( LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC ) )
		{
			lang = "sr";
		}
		else
		{
			primary_lang_id = (WORD)PRIMARYLANGID( lang_id );
			if ( primary_lang_id > LANG_NEPALI ) primary_lang_id = 0;
			lang = lang_codes + primary_lang_id * 2;
			if ( *lang == '?' ) lang = "en";
		}

	#else

		/* See http://opengroup.org/onlinepubs/007908799/xbd/envvar.html */
		const char *lang = g_getenv( "LC_ALL" );
		if ( lang == NULL || *lang == '\0' )
		{
			lang = g_getenv( "LC_MESSAGES" );
			if ( lang == NULL || *lang == '\0' )
			{
				lang = g_getenv( "GDM_LANG" );
				if ( lang == NULL || *lang == '\0' )
				{
					lang = g_getenv( "LANG" );
					if ( lang == NULL || *lang == '\0' )
					{
						lang = "en";
					}
				}
			}
		}

	#endif

		set_language( lang );
	}

	return language;
}

gchar *SetLanguage( const char *lang )
{
	gchar filename[14], *pathname, *buf = NULL;
	static const char xx[] = "??";
	int i;

	TRACE(( "SetLanguage( %s )\n", (lang != NULL) ? lang : "<NULL>" ));

	if ( lang == NULL ) lang = GetLanguage();
	else lang = set_language( lang );

	for ( i = 0; i < NUM_TEXT; i++ ) Texts[i] = xx;

	TRACE(( "lang = %s\n", lang ));

	strcat( strncat( strcpy( filename, "leiads_" ), lang, 2 ), ".txt" );
	pathname = BuildApplDataFilename( filename );
	g_file_get_contents( pathname, &buf, NULL, NULL );
	g_free( pathname );
	if ( buf != NULL )
	{
		const char *s;

		TRACE(( "Parsing %s...\n", filename ));

		if ( (s = strstr( buf, "[leiads language file]" )) != NULL )
		{
			char inside = '\0';
			char *t = buf;

			s += 22;
			i = 0;

			for (;;)
			{
				char ch = *s++;
				if ( ch == '\0' ) break;

				if ( inside != '\0' )
				{
					if ( ch == inside )
					{
						inside = FALSE;
					}
					else if ( ch == '\\' )
					{
						ch = *s++;
						if ( ch == '\0' ) break;

						if ( ch == '\r' || ch == '\n' )
						{
							while ( isspace( *s ) ) s++;
						}
						else
						{
							switch ( ch )
							{
							case '0':
								ch = '\0';
								break;
							case 'b':
								ch = '\b';
								break;
							case 'n':
								ch = '\n';
								break;
							case 'r':
								ch = '\r';
								break;
							case 't':
								ch = '\t';
								break;
							}

							*t++ = ch;
						}
					}
					else
					{
						*t++ = ch;
					}
				}
				else
				{
					if ( isspace( ch ) )
					{
						;
					}
					else if ( ch == '{' )
					{
						s = strchr( s, '}' );
						if ( s == NULL ) break;
						s += 1;
					}
					else if ( ch == '(' && *s == '*' )
					{
						s = strstr( s, "*)" );
						if ( s == NULL ) break;
						s += 2;
					}
					else if ( ch == '/' && *s == '*' )
					{
						s = strstr( s, "*/" );
						if ( s == NULL ) break;
						s += 2;
					}
					else if ( ch == '#' )
					{
						if ( Texts[i] != xx )
						{
							*t++ = '\0';
							if ( ++i == NUM_TEXT ) break;
						}

						while ( *s != '\0' && *s != '\r' && *s != '\n' ) s++;
					}
					else if ( ch == '/' && *s == '/' )
					{
						while ( *s != '\0' && *s != '\r' && *s != '\n' ) s++;
					}
					else if ( ch == '\'' || ch == '\"' )
					{
						if ( Texts[i] == xx ) Texts[i] = t;
						inside = ch;
					}
					else if ( ch == ',' )
					{
						*t++ = '\0';
						if ( ++i == NUM_TEXT ) break;
					}
					else if ( ch == 'N' && strncmp( s, "ULL", 3 ) == 0 )
					{
						if ( Texts[i] == xx ) Texts[i] = NULL;
						s += 3;
					}
					else
					{
						TRACE(( "*** SYNTAX ERROR\n" ));
						ASSERT( FALSE );
						/*break;*/
					}
				}
			}

			*t++ = '\0';

		#if LOGLEVEL

			TRACE(( "Test language texts...\n" ));

			for ( i = 0; i < NUM_TEXT; i++ )
			{
				if ( Texts[i] == xx )
					g_warning( "invalid string [%d]\n", i );
				else if ( Texts[i] != NULL && !g_utf8_validate( Texts[i], -1, NULL ) )
					g_error( "invalid UTF-8 string [%d] \"%s\"\n", i, Texts[i] );
			}

		#endif
		}
		else
		{
			g_free( buf );
			buf = NULL;
		}
	}

	return buf;
}

/*-----------------------------------------------------------------------------------------------*/
/*

http://oriya.sarovar.org/docs/gettext/memo.html
http://library.gnome.org/devel/gtkmm-tutorial/unstable/chapter-internationalization.html.en
http://www.gnu.org/software/gettext/manual/gettext.html
http://live.gnome.org/TranslationProject/DevGuidelines
http://www.gnome.org/~malcolm/i18n/
  
*/
/*-----------------------------------------------------------------------------------------------*/
