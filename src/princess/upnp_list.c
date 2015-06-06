/*
	princess/upnp_list.c
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

/*--- List management ---------------------------------------------------------------------------*/

struct upnp_list { struct upnp_list *next; int dummy; };

int list_append( void *list, void *data )
{
	struct upnp_list *_list = (struct upnp_list*)list, *_data = (struct upnp_list*)data;
	int n = 0;

	for ( ; _list->next != NULL; _list = _list->next ) n++;
	_data->next = _list->next;
	_list->next = _data;

	return n;  /* Number of entries before list_append() */
}

void list_prepend( void *list, void *data )
{
	struct upnp_list *_list = (struct upnp_list*)list, *_data = (struct upnp_list*)data;

	_data->next = _list->next;
	_list->next = _data;
}

gboolean list_remove( void *list, void *data )
{
	struct upnp_list *_list = (struct upnp_list*)list, *_data = (struct upnp_list*)data;

	for ( ; _list->next != NULL; _list = _list->next )
	{
		if ( _list->next == _data )
		{
			_list->next = _data->next;
			return TRUE;
		}
	}

	upnp_critical(( "list_remove() failed" ));
	return FALSE;
}

/*===============================================================================================*/
