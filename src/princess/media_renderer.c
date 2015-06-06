/*
	media_renderer.c
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
#include "upnp_private.h"  /* for debug stuff */
#include "media_renderer.h"

/* Workaround for bug in gmediarender:
   AVTransport::SetAVTransportURI() has no effect when TransportState == "PLAYING",
   so we call AVTransport::Stop() first */
#define GMEDIARENDER_WORKAROUND

#define START_PLAYING_DELAY   1000 /* ms */

/* NOTE: A time value of -1 works fine with "Auskerry", but not with "Bute": "HTTP/1.1 400 Bad Request" */
#define SUBSCRIPTION_TIMEOUT  7200 /* seconds */

/*-----------------------------------------------------------------------------------------------*/

int TracksMax;
char **ItemArray;
enum TransportState TransportState;

/* UPnP stuff */
const upnp_device *AVTransport, *RenderingControl;
upnp_i4 ConnectionID, AVTransportID, RcsID;
static char *AVTransportURI;
static int NextTrackIndex = -1;

/* Auskerry & Bute stuff */
upnp_ui4 Sequence;
int CurrentTrackIndex = -1;

/* Cara stuff */
upnp_ui4 *IdArray;
upnp_ui4 CurrentTrackId;
gboolean RepeatMode, ShufflePlay;

/* Preamp stuff */
int VolumeLimit, CurrentVolume;
gboolean CurrentMute;

/* Error messages */
static const char upnp_err_service_not_found[] = "No appropriate device or service available";

/*-----------------------------------------------------------------------------------------------*/

void playlist_insert_track( int index, upnp_ui4 new_id, char *item );
void playlist_insert_tracks( int index, int count, upnp_ui4 *new_id_array, char *meta_data_list );
void playlist_delete_tracks( int index, int count );
void playlist_delete_all_tracks( void );
/*void playlist_move_tracks( int index_from, int index_to, int count );*/

gboolean handle_av_transport_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error );
gboolean handle_rendering_control_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error );

gboolean handle_media_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error );
void     handle_playlist_clear( gpointer user_data );
gboolean handle_playlist_insert( upnp_ui4 sequence, int index, int count, gpointer user_data, GError **error );
void     handle_playlist_delete( upnp_ui4 sequence, int index, int count, gpointer user_data );
/*void   handle_playlist_move( upnp_ui4 sequence, int index_from, int index_to, int count, gpointer user_data );*/
void     handle_playlist_current_track_index( const char *item, gpointer user_data );
void     handle_current_track_index( int index, gboolean evented, gpointer user_data );
void     handle_transport_state( int media_event, const char *state, gpointer user_data );

gboolean handle_playlist_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error );
int      handle_id_array( upnp_ui4 *new_id_array, gpointer user_data, GError **error );
gboolean handle_id_array_insert( int index, int count, upnp_ui4 *new_id_array, gpointer user_data, GError **error );
void     handle_id_array_delete( int index, int count, upnp_ui4 id, gpointer user_data );
/*void   handle_id_array_move( int index_from, int index_to, int count, gpointer user_data );*/

void     handle_ds_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data );
void     handle_track_id( gboolean evented, gpointer user_data );

void     handle_preamp_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data );

/*-----------------------------------------------------------------------------------------------*/

gboolean upnp_is_av_transport( const upnp_device *device )
{
	return upnp_device_is( device, upnp_serviceId_AVTransport )    /* UPnP */
	    || upnp_device_is( device, linn_serviceId_Media  )         /* Auskerry & Bute */
	    || upnp_device_is( device, linn_serviceId_Playlist )       /* Cara */
	    || upnp_device_is( device, openhome_serviceId_Playlist );  /* Davaar */
}

upnp_device *upnp_get_first_av_transport( void )
{
	upnp_device *device = upnp_get_first_device( NULL );
	if ( device == NULL || upnp_is_av_transport( device ) ) return device;
	return upnp_get_next_av_transport( device );
}

upnp_device *upnp_get_next_av_transport( const upnp_device *device )
{
	do device = upnp_get_next_device( device, NULL );
	while ( device != NULL && !upnp_is_av_transport( device ) );
	return (upnp_device *)device;
}

static int upnp_num_linn_transport( void )
{
	return upnp_num_devices( linn_serviceId_Media )
	     + upnp_num_devices( linn_serviceId_Playlist )
		 + upnp_num_devices( openhome_serviceId_Playlist );
}

int upnp_num_av_transport( void )
{
	return upnp_num_devices( upnp_serviceId_AVTransport )
	     + upnp_num_linn_transport();
}


gboolean upnp_is_rendering_control( const upnp_device *device )
{
	return upnp_device_is( device, upnp_serviceId_RenderingControl )  /* UPnP */
	    || upnp_device_is( device, linn_serviceId_Preamp )            /* Auskerry, Bute, and Cara */
	    || upnp_device_is( device, openhome_serviceId_Volume );       /* Davaar */
}

upnp_device *upnp_get_first_rendering_control( void )
{
	upnp_device *device = upnp_get_first_device( NULL );
	if ( device == NULL || upnp_is_rendering_control( device ) ) return device;
	return upnp_get_next_rendering_control( device );
}

upnp_device *upnp_get_next_rendering_control( const upnp_device *device )
{
	do device = upnp_get_next_device( device, NULL );
	while ( device != NULL && !upnp_is_rendering_control( device ) );
	return (upnp_device *)device;
}

upnp_device *upnp_get_corresponding_rendering_control( const upnp_device *av_transport )
{
	upnp_device *rendering_control;

	if ( upnp_device_is( av_transport, upnp_serviceId_AVTransport ) )
	{
		rendering_control = (upnp_device *)av_transport;
	}
	else if ( av_transport != NULL )
	{
		for ( rendering_control = upnp_get_first_device( NULL ); rendering_control != NULL; rendering_control = upnp_get_next_device( rendering_control, NULL ) )
		{
			if ( upnp_device_is( rendering_control, linn_serviceId_Preamp ) || upnp_device_is( rendering_control, openhome_serviceId_Volume ) )
			{
				if ( strcmp( rendering_control->serialNumber, av_transport->serialNumber ) == 0 ) break;
			}
		}

		if ( rendering_control == NULL )
		{
			const char *colon = strchr( av_transport->friendlyName, ':' );
			size_t n = (colon + 1) - av_transport->friendlyName;
			upnp_device *rc;
			int match = -1;

			for ( rc = upnp_get_first_device( NULL ); rc != NULL; rc = upnp_get_next_device( rc, NULL ) )
			{
				if ( upnp_device_is( rc, linn_serviceId_Preamp ) || upnp_device_is( rc, openhome_serviceId_Volume ) )
				{
					if ( colon != NULL && strncmp( rc->friendlyName, av_transport->friendlyName, n ) == 0 )
					{
						int i;

						for ( i = 0;; i++ )
						{
							char ch = rc->friendlyName[n + i];
							if ( ch != av_transport->friendlyName[n + i] ) break;
							if ( ch == '\0' ) break;
						}

						if ( match < i ) { match = i; rendering_control = rc; }  /* best match so far */
					}
				}
			}
		}

		if ( rendering_control == NULL && upnp_num_linn_transport() == 1 )
		{
			rendering_control = upnp_get_first_device( linn_serviceId_Preamp );
			if ( rendering_control == NULL )
				rendering_control = upnp_get_first_device( openhome_serviceId_Volume );
		}
	}
	else
	{
		rendering_control = NULL;
	}

	return rendering_control;
}

const char *upnp_get_ds_compatibility_family( const upnp_device *av_transport )
{
	const char *compatibility_family = NULL;

	if ( upnp_device_is( av_transport, openhome_serviceId_Playlist ) )
	{
		compatibility_family = "Davaar";
	}
	else if ( upnp_device_is( av_transport, linn_serviceId_Playlist ) )
	{
		if ( upnp_device_is( av_transport, linn_serviceId_Radio ) )
			compatibility_family = "Cara w/ RadioTime support";
		else
			compatibility_family = "Cara";
	}
	else if ( upnp_device_is( av_transport, linn_serviceId_Media ) )
	{
		if ( upnp_device_is( av_transport, linn_serviceId_Product ) )
			compatibility_family = "Bute";
		else
			compatibility_family = "Auskerry";
	}
	else if ( upnp_device_is( av_transport, linn_serviceId_Product ) )
	{
		for ( av_transport = av_transport->deviceList; av_transport != NULL; av_transport = av_transport->next )
		{
			if ( upnp_device_is( av_transport, linn_serviceId_Media ) )
			{
				compatibility_family = "Auskerry";
				break;
			}
		}
	}

	return compatibility_family;
}

gboolean upnp_device_is_external_preamp( const upnp_device *device )
{
	return upnp_device_is( device, linn_serviceId_KlimaxKontrolConfig )  || /* Auskerry Preamp */
	       upnp_device_is( device, linn_serviceId_AkurateKontrolConfig ) || /* Auskerry Preamp */
	       upnp_device_is( device, linn_serviceId_Proxy );                  /* (External) Bute Preamp */
	/* TODO: Needs to be adapted to Cara & Davaar */
}

/*-----------------------------------------------------------------------------------------------*/

static char *xml_unbox_e_property( char *e_property, char **name )
{
	char *s = e_property;

	if ( s != NULL )
	{
		s = xml_unbox( &s, name, NULL );
		upnp_assert( s != NULL && strcmp( *name, "e:property" ) == 0 );
		if ( s == NULL || strcmp( *name, "e:property" ) != 0 ) return NULL;

		s = xml_unbox( &s, name, NULL );
		upnp_assert( s != NULL );
	}

	return s;
}

char *xml_first_e_property( char *e_propertyset, char **name, xml_iter *iter )
{
	return xml_unbox_e_property( xml_first( e_propertyset, iter ), name );
}

char *xml_next_e_property( char **name, xml_iter *iter )
{
	return xml_unbox_e_property( xml_next( iter ), name );
}

/*-----------------------------------------------------------------------------------------------*/

const upnp_device *GetAVTransport( void )
{
	return AVTransport;
}

gboolean SetAVTransport( const upnp_device *device )
{
	if ( device != NULL && !upnp_is_av_transport( device ) )
	{
		upnp_warning(( "SetAVTransport( \"%s\" ): Unsupported device type\n", device->friendlyName ));
		return FALSE;
	}

	/* Select Linn DS */
	AVTransport = device;
	upnp_message(( "SetAVTransport(): AVTransport = %s\n", (AVTransport != NULL) ? AVTransport->friendlyName : "<NULL>" ));

	/* Select corresponding Linn preamp */
	return SetRenderingControl( upnp_get_corresponding_rendering_control( device ) );
}

const upnp_device *GetRenderingControl( void )
{
	return RenderingControl;
}

gboolean SetRenderingControl( const upnp_device *device )
{
	if ( device != NULL && !upnp_is_rendering_control( device ) )
	{
		upnp_warning(( "SetRenderingControl( \"%s\" ): Unsupported device type\n", device->friendlyName ));
		return FALSE;
	}

	RenderingControl = device;
	upnp_message(( "SetRenderingControl(): RenderingControl = %s\n", (RenderingControl != NULL) ? RenderingControl->friendlyName : "<NULL>" ));

	return TRUE;
}

static void set_AVTransportURI( const char *Uri )
{
	if ( AVTransportURI != NULL ) g_free( AVTransportURI );
	AVTransportURI = ( Uri != NULL ) ? g_strdup( Uri ) : NULL;
}

static void clear_connection()
{
	playlist_delete_all_tracks();
	g_free( ItemArray );
	ItemArray = NULL;

	TracksMax = 0;
	TransportState = TRANSPORT_STATE_UNKNOWN;

	/* UPnP stuff */
	ConnectionID = AVTransportID = RcsID = -1L;
	set_AVTransportURI( NULL );
	NextTrackIndex = -1;

	/* Auskerry & Bute stuff */
	Sequence = 0L;
	CurrentTrackIndex = -1;

	/* Cara stuff */
	if ( IdArray != NULL )
	{
		g_free( IdArray );
		IdArray = NULL;
	}
	CurrentTrackId = 0UL;

	/* Preamp stuff */
	VolumeLimit = 0;
	CurrentMute = CurrentVolume = 0;
	RepeatMode = ShufflePlay = FALSE;
}

gboolean PrepareForConnection( GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean success;

	clear_connection();

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		GError *err = NULL;

		success = ConnectionManager_PrepareForConnection( device, "http-get", "", -1L, "Output", &ConnectionID, &AVTransportID, &RcsID, &err );
		upnp_message(( "PrepareForConnection(): ConnectionManager_PrepareForConnection() = %s, %d, %d, %d\n", (success) ? "TRUE" : "FALSE", ConnectionID, AVTransportID, RcsID ));
		if ( err != NULL )
		{
			if ( err->code == 401 /* Invalid action */ /*|| err->code == 702*/ /* Rhythmbox */ )
			{
				AVTransportID = RcsID = 0L;
				success = TRUE;
			}
			else if ( error != NULL && *error == NULL )
			{
				*error = err;
				err = NULL;
			}

			if ( err != NULL ) g_error_free( err );
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Product ) )
	{
		upnp_boolean standby;

		success = Linn_Product_Standby( device, &standby, error );
		upnp_message(( "PrepareForConnection(): Product_Standby() = %s, %d\n", (success) ? "TRUE" : "FALSE", standby ));
		if ( success && standby )
		{
			success = Linn_Product_SetStandby( device, FALSE, error );
			upnp_message(( "PrepareForConnection(): Product_SetStandby( FALSE ) = %s\n", (success) ? "TRUE" : "FALSE" ));
		}
	}
	else if ( upnp_device_is( device, openhome_serviceId_Product ) )
	{
		upnp_boolean standby;

		success = Openhome_Standby( device, &standby, error );
		upnp_message(( "PrepareForConnection(): Openhome_Standby() = %s, %d\n", (success) ? "TRUE" : "FALSE", standby ));
		if ( success && standby )
		{
			success = Openhome_SetStandby( device, FALSE, error );
			upnp_message(( "PrepareForConnection(): Openhome_SetStandby( FALSE ) = %s\n", (success) ? "TRUE" : "FALSE" ));
		}
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		success = FALSE;
	}

	return success;
}

gboolean ConnectionComplete( gboolean standby, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean success;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		if ( ConnectionID >= 0L )
		{
			success = ConnectionManager_ConnectionComplete( device, ConnectionID, error );
			upnp_message(( "ConnectionComplete(): ConnectionManager_ConnectionComplete() = %s\n", (success) ? "TRUE" : "FALSE" ));
		}
		else
		{
			success = TRUE;
			upnp_message(( "ConnectionComplete(): nothing to do\n" ));
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Product ) )
	{
		if ( standby )
		{
			success = Linn_Product_Standby( device, &standby, error );
			upnp_message(( "ConnectionComplete(): Product_Standby() = %s, %d\n", (success) ? "TRUE" : "FALSE", standby ));
			if ( success && !standby )
			{
				if ( TransportState == TRANSPORT_STATE_UNKNOWN )
				{
					success = GetTransportState( &TransportState, error );
					upnp_message(( "ConnectionComplete(): GetTransportState() = %s,%d\n", (success) ? "TRUE" : "FALSE", TransportState ));
				}

				if ( success && (TransportState == TRANSPORT_STATE_UNKNOWN ||
					 TransportState == TRANSPORT_STATE_STOPPED || TransportState == TRANSPORT_STATE_EOP ||
					 TransportState == TRANSPORT_STATE_NO_MEDIA_PRESENT) )
				{
					success = Linn_Product_SetStandby( device, TRUE, error );
					upnp_message(( "ConnectionComplete(): Product_SetStandby( TRUE ) = %s\n", (success) ? "TRUE" : "FALSE" ));
				}
			}
		}
	}
	else if ( upnp_device_is( device, openhome_serviceId_Product ) )
	{
		if ( standby )
		{
			success = Openhome_Standby( device, &standby, error );
			upnp_message(( "ConnectionComplete(): Openhome_Standby() = %s, %d\n", (success) ? "TRUE" : "FALSE", standby ));
			if ( success && !standby )
			{
				if ( TransportState == TRANSPORT_STATE_UNKNOWN )
				{
					success = GetTransportState( &TransportState, error );
					upnp_message(( "ConnectionComplete(): GetTransportState() = %s,%d\n", (success) ? "TRUE" : "FALSE", TransportState ));
				}

				if ( success && (TransportState == TRANSPORT_STATE_UNKNOWN ||
					 TransportState == TRANSPORT_STATE_STOPPED || TransportState == TRANSPORT_STATE_EOP ||
					 TransportState == TRANSPORT_STATE_NO_MEDIA_PRESENT) )
				{
					success = Openhome_SetStandby( device, TRUE, error );
					upnp_message(( "ConnectionComplete(): Openhome_SetStandby( TRUE ) = %s\n", (success) ? "TRUE" : "FALSE" ));
				}
			}
		}
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		success = FALSE;
	}

	clear_connection();
	return success;
}

/*-----------------------------------------------------------------------------------------------*/

gboolean Play( GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		if ( CurrentTrackIndex < 0 && GetTrackCount() > 0 )
		{
			result = SeekTracksAbsolute( 0, error );
		}
		else
		{
			NextTrackIndex = -1;  /* allow track transition */
			result = AVTransport_Play( device, AVTransportID, "1", error );
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		result = Linn_Media_Play( device, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Ds ) )
	{
		result = Linn_Ds_Play( device, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_Play( device, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean Pause( GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		result = AVTransport_Pause( device, AVTransportID, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		result = Linn_Media_Pause( device, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Ds ) )
	{
		result = Linn_Ds_Pause( device, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_Pause( device, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean Stop( GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		NextTrackIndex = TracksMax;  /* prevent track transition */
		result = AVTransport_Stop( device, AVTransportID, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		result = Linn_Media_Stop( device, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Ds ) )
	{
		result = Linn_Ds_Stop( device, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_Stop( device, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

static gboolean AVTransport_SetURI( const upnp_device *device, const char *Uri, const char *MetaData, gboolean TrackTransition, GError **error )
{
	gboolean result;

#ifdef GMEDIARENDER_WORKAROUND
	if ( !TrackTransition && TransportState != TRANSPORT_STATE_STOPPED && TransportState != TRANSPORT_STATE_NO_MEDIA_PRESENT )
	{
		set_AVTransportURI( NULL );  /* prevent track transition */

		result = AVTransport_Stop( device, AVTransportID, error );
		upnp_message(( "AVTransport_SetURI: AVTransport_Stop() = %s\n", (result) ? "TRUE" : "FALSE" ));
		if ( !result ) return result;
	}
#endif

	set_AVTransportURI( Uri );
	result = AVTransport_SetAVTransportURI( device, AVTransportID, Uri, MetaData, error );
	upnp_message(( "AVTransport_SetURI: SetAVTransportURI() = %s\n", (result) ? "TRUE" : "FALSE" ));

	if ( Uri != NULL && result )
	{
	#ifndef GMEDIARENDER_WORKAROUND
		if ( TrackTransition || (TransportState != TRANSPORT_STATE_TRANSITIONING && TransportState != TRANSPORT_STATE_PLAYING) )
	#endif
		{
			result = AVTransport_Play( device, AVTransportID, "1", error );
			upnp_message(( "AVTransport_SetURI: AVTransport_Play() = %s\n", (result) ? "TRUE" : "FALSE" ));
		}
	}
	else
	{
		set_AVTransportURI( NULL );
	}

	return result;
}

static gboolean AVTransport_SeekTrackAbsolute( const upnp_device *device, int Track, gboolean TrackTransition, GError **error )
{
	gboolean result = FALSE;

	if ( Track >= 0 && Track < TracksMax && ItemArray != NULL )
	{
		const char *item = ItemArray[Track];
		if ( item != NULL )
		{
			struct PlaylistData *playlist_data = PlaylistData_New( 1, error );
			upnp_assert( playlist_data != NULL );
			if ( playlist_data != NULL )
			{
				char *Uri, *MetaData;

				if ( PlaylistData_AddTrack( playlist_data, item, error ) )
				{
					MetaData = PlaylistData_Finalize( playlist_data, error );
					if ( MetaData != NULL )
					{
						Uri = NULL;
						MetaData = PlaylistData_GetTrack( &MetaData, &Uri, NULL );
						upnp_assert( MetaData != NULL && Uri != NULL );
						if ( MetaData != NULL && Uri != NULL )
						{
							NextTrackIndex = Track;
							result = AVTransport_SetURI( device, Uri, MetaData, TrackTransition, error );
							if ( !result ) NextTrackIndex = -1;

							upnp_message(( "AVTransport_SeekTrackAbsolute(%d): AVTransport_SetURI() = %s\n", Track, (result) ? "TRUE" : "FALSE" ));
						}
						else
						{
							g_set_error( error, UPNP_ERROR, -3, "Internal Error" );
						}
					}
				}

				PlaylistData_Free( playlist_data );
			}
		}
		else
		{
			g_set_error( error, UPNP_ERROR, -2, "Internal Error" );
		}
	}
	else
	{
		g_set_error( error, UPNP_ERROR, -1, "Internal Error" );
	}

	return result;
}

gboolean SeekTracksAbsolute( int Index, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		result = AVTransport_SeekTrackAbsolute( device, Index, FALSE, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		result = Linn_Media_SeekTracks( device, Index, Relativity_Absolute, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Ds ) )
	{
		if ( IdArray != NULL )
			result = Linn_Ds_SeekTrackId( device, IdArray[Index], error );
		else
			result = Linn_Ds_SeekTrackAbsolute( device, Index, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		if ( IdArray != NULL )
			result = Openhome_Playlist_SeekId( device, IdArray[Index], error );
		else
			result = Openhome_Playlist_SeekIndex( device, Index, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean SeekTracksRelative( int Delta, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		int Index;

		if ( RepeatMode )
		{
			int track_count = GetTrackCount();
			if ( track_count > 0 )
			{
				while ( CurrentTrackIndex + Delta < 0 )
					Delta += track_count;
				while ( CurrentTrackIndex + Delta >= track_count )
					Delta -= track_count;
			}
		}

		Index = CurrentTrackIndex + Delta;
		if ( Index >= 0 && Index < TracksMax && ItemArray[Index] != NULL )
		{
			result = AVTransport_SeekTrackAbsolute( device, Index, FALSE, error );
		}
		else
		{
			result = AVTransport_SetURI( device, NULL, NULL, FALSE, error );
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		if ( RepeatMode )
		{
			int track_count = GetTrackCount();
			if ( track_count > 0 )
			{
				int current_track_index = -1;
				if ( !Linn_Media_Status( device, NULL, &current_track_index, NULL, NULL, NULL, NULL, error ) )
					return FALSE;

				upnp_message(( "SeekTracksRelative( %d ): CurrentTrackIndex = %d, GetTrackCount() = %d\n",
					Delta, current_track_index, track_count ));

				if ( current_track_index >= 0 )
				{
					while ( current_track_index + Delta < 0 )
						Delta += track_count;
					while ( current_track_index + Delta >= track_count )
						Delta -= track_count;
				}
			}
		}

		result = Linn_Media_SeekTracks( device, Delta, Relativity_Relative, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Ds ) )
	{
		result = Linn_Ds_SeekTrackRelative( device, Delta, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = ((Delta < 0) ? Openhome_Playlist_Previous : Openhome_Playlist_Next)( device, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

static enum TransportState get_transport_state_from_string( const char *state )
{
	/* "STOPPED", "PLAYING", "TRANSITIONING", "PAUSED_PLAYBACK",
	   "PAUSED_RECORDING", "RECORDING", "NO_MEDIA_PRESENT" (UPnP A/V)
	   "Playing", "Paused", "Stopped", "Eop" (Auskerry+Bute), "Buffering" (Cara+Davaar) */

	enum TransportState result;

	if ( state == NULL )
	{
		upnp_warning(( "* Transport State is <NULL>\n" ));
		result = TRANSPORT_STATE_UNKNOWN;
	}
	else if ( stricmp( state, "BUFFERING" ) == 0 )
	{
		result = TRANSPORT_STATE_BUFFERING;
	}
	else if ( stricmp( state, "EOP" ) == 0 )
	{
		result = TRANSPORT_STATE_EOP;
	}
	else if ( stricmp( state, "NO_MEDIA_PRESENT" ) == 0 )
	{
		result = TRANSPORT_STATE_NO_MEDIA_PRESENT;
	}
	else if ( stricmp( state, "PAUSED" ) == 0 || stricmp( state, "PAUSED_PLAYBACK" ) == 0 )
	{
		result = TRANSPORT_STATE_PAUSED;
	}
	else if ( stricmp( state, "PLAYING" ) == 0 )
	{
		result = TRANSPORT_STATE_PLAYING;
	}
	else if ( stricmp( state, "STOPPED" ) == 0 )
	{
		result = TRANSPORT_STATE_STOPPED;
	}
	else if ( stricmp( state, "TRANSITIONING" ) == 0 )
	{
		result = TRANSPORT_STATE_TRANSITIONING;
	}
	else
	{
		upnp_warning(( "* Unknown Transport State \"%s\"\n", state ));
		result = TRANSPORT_STATE_UNKNOWN;
	}

	upnp_debug(( "TransportState \"%s\" = %d\n", ( state != NULL ) ? state : "<NULL>", result ));
	return result;
}

gboolean GetTransportState( enum TransportState *TransportState, GError **error )
{
	char *transport_state = NULL;

	if ( !GetTransportStateString( &transport_state, error ) )
		return FALSE;

	if ( TransportState != NULL )
		*TransportState = get_transport_state_from_string( transport_state );

	upnp_free_string( transport_state );
	return TRUE;
}

gboolean GetTransportStateString( char **TransportState, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		result = AVTransport_GetTransportInfo( device, AVTransportID, TransportState, NULL, NULL, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		result = Linn_Media_Status( device, TransportState, NULL, NULL, NULL, NULL, NULL, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Ds ) )
	{
		result = Linn_Ds_State( device, TransportState, NULL, NULL, NULL, NULL, NULL, NULL, NULL, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_TransportState( device, TransportState, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

/*-----------------------------------------------------------------------------------------------*/

struct start_playing_data
{
	gchar *message;
	void (*error_handler)( const gchar *message, GError *error );
};

static gboolean start_playing( gpointer data )
{
	struct start_playing_data *spd = (struct start_playing_data *)data;
	GError *error = NULL;

	gboolean success = Play( &error );
	if ( !success && spd->error_handler != NULL )
		(*spd->error_handler)( spd->message, error );

	g_free( spd->message );
	g_free( spd );
	return FALSE;
}

void StartPlaying( void )
{
	/* Unfortunately Media_Play() has often no effect here,
	   so we do it with a little delay instead */

	struct start_playing_data *spd = g_new( struct start_playing_data, 1 );

	spd->message = NULL;        /* TODO */
	spd->error_handler = NULL;  /* TODO */

#ifdef START_PLAYING_DELAY
	g_timeout_add_full( G_PRIORITY_LOW, START_PLAYING_DELAY, start_playing, spd, NULL );
#else
	start_playing( spd );
#endif
}

/*-----------------------------------------------------------------------------------------------*/

static upnp_ui4 add_to_sequence( upnp_ui4 sequence, int count )
{
	return (sequence + count) & 0xFFFFFFFFUL;
}

static struct sequence
{
	struct sequence *next;
	upnp_ui4 value;
} *sequence_list;

static GMutex* sequence_list_mutex;

upnp_ui4 append_to_sequence_list( int count )
{
	struct sequence *seq = g_new( struct sequence, 1 );
	upnp_assert( seq != NULL );
	seq->value = add_to_sequence( Sequence, count );

	upnp_message(( "# append_to_sequence_list( %u, %d ) = %u\n", Sequence, count, seq->value ));

	upnp_assert( sequence_list_mutex != NULL );
	g_mutex_lock( sequence_list_mutex );

	list_append( &sequence_list, seq );

	g_mutex_unlock( sequence_list_mutex );
	return seq->value;
}

void remove_from_sequence_list( upnp_ui4 value )
{
	struct sequence *seq;
	int success = FALSE;

	upnp_message(( "# remove_from_sequence_list( %u )\n", value ));

	upnp_assert( sequence_list_mutex != NULL );
	g_mutex_lock( sequence_list_mutex );

	for ( seq = sequence_list; seq != NULL; seq = seq->next )
	{
		if ( seq->value == value )
		{
			success = list_remove( &sequence_list, seq );
			if ( success )
				g_free( seq );
			else
				upnp_critical(( "remove_from_sequence_list( %u ): list_remove() failed\n", value ));

			break;
		}
	}

	g_mutex_unlock( sequence_list_mutex );
	if ( !success ) upnp_critical(( "remove_from_sequence_list( %u ): no success\n", value ));
}

static gboolean was_in_sequence_list( upnp_ui4 value )
{
	gboolean result = FALSE;

	upnp_assert( sequence_list_mutex != NULL );
	g_mutex_lock( sequence_list_mutex );

	do
	{
		struct sequence *seq = sequence_list;
		if ( seq == NULL ) break;
		sequence_list = seq->next;

		if ( seq->value == value )
			result = TRUE;
		else
			upnp_warning(( "was_in_sequence_list(): invalid sequence number %u\n", seq->value ));

		g_free( seq );
	}
	while ( !result );

	g_mutex_unlock( sequence_list_mutex );

	if ( !result ) Sequence = value;

	upnp_message(( "# was_in_sequence_list( %u ) = %s\n", value, (result) ? "TRUE" : "FALSE" ));
	return result;
}

static void clear_sequence_list( void )
{
	upnp_message(( "# clear_sequence_list()\n" ));

	upnp_assert( sequence_list_mutex != NULL );
	g_mutex_lock( sequence_list_mutex );

	for (;;)
	{
		struct sequence *seq = sequence_list;
		if ( seq == NULL ) break;
		sequence_list = seq->next;

		g_free( seq );
	}

	g_mutex_unlock( sequence_list_mutex );
}

/*-----------------------------------------------------------------------------------------------*/

int GetTrackCount( void )
{
	int track_count;

	if ( ItemArray == NULL ) return UPNP_ERR_NOT_SUBSCRIBED;

	if ( IdArray != NULL )
	{
		for ( track_count = 0; IdArray[track_count] != 0; track_count++ ) ;
	}
	else
	{
		for ( track_count = 0; ItemArray[track_count] != NULL; track_count++ ) ;
	}

	return track_count;
}

static const char *get_track_item( int index )
{
	return (ItemArray != NULL && index >= 0) ? ItemArray[index] : NULL;
}

gchar *GetTrackItem( int index )
{
	const char *item = get_track_item( index );
	return ( item != NULL ) ? g_strdup( item ) : NULL;
}

static const char *get_current_track_item( void )
{
	return get_track_item( CurrentTrackIndex );
}

gchar *GetCurrentTrackItem( void )
{
	return GetTrackItem( CurrentTrackIndex );
}

/*
int GetPlaylistTrackCount( void )
{
	upnp_device *device = AVTransport;
	int result = -1;

	upnp_assert( Count != NULL );

	if ( upnp_device_is( device, upnp_service_AVTransport_1 ) )
	{
		result = GetTrackCount();
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		Media_PlaylistTrackCount( Sequence, &result );
		upnp_message(( "Media_PlaylistTrackCount() = %d\n", result ));
	}
	else if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		upnp_string id_array;
		if ( Playlist_IdArray( NULL, &id_array ) )
		{
			result = (((strlen( id_array ) + 3) / 4) * 3) / 4;
			upnp_free_string( id_array );
		}
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		upnp_string id_array;
		if ( Openhome_IdArray( NULL, &id_array ) )
		{
			result = (((strlen( id_array ) + 3) / 4) * 3) / 4;
			upnp_free_string( id_array );
		}
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
	}

	return result;
}
*/

gboolean PlaylistAdd( char *MetaDataList, gpointer user_data, GError **error )
{
	return PlaylistInsert( GetTrackCount(), MetaDataList, user_data, error );
}

static gboolean playlist_insert( int Index, char *MetaDataList, gpointer user_data, GError **error, gboolean event_callback )
{
	const upnp_device *device = AVTransport;
	gboolean result = FALSE;
	int count = 0;

	upnp_assert( MetaDataList != NULL );

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		const char *current_item = get_current_track_item();

		for (;;)
		{
			char *item = MetaDataList_GetItem( &MetaDataList );
			if ( item == NULL ) { result = TRUE; break; }

			playlist_insert_track( Index + count, 0, item );

			count++;
		}

		if ( event_callback && count > 0 )
		{
			OnPlaylistInsert( Index, count, user_data );
			handle_playlist_current_track_index( current_item, user_data );
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		struct PlaylistData *playlist_data = PlaylistData_New( 50, NULL /*error*/ );

		upnp_assert( playlist_data != NULL );
		if ( playlist_data != NULL )
		{
			const char *current_item = get_current_track_item();
			int c = 0;

			for (;;)
			{
				char *item = MetaDataList_GetItem( &MetaDataList );
				if ( item != NULL )
				{
					result = PlaylistData_AddTrack( playlist_data, item, error );
					if ( !result ) break;
					c++;
				}

				if ( c == 50 || (item == NULL && c > 0) )
				{
					upnp_ui4 next_sequence;
					char *meta_data;

					meta_data = PlaylistData_Finalize( playlist_data, error );
					if ( meta_data == NULL ) { result = FALSE; break; }

					next_sequence = append_to_sequence_list( c );
					result = Linn_Media_PlaylistInsert( device, Sequence, Index + count, meta_data, error );
					upnp_message(( "PlaylistInsert(): Linn_Media_PlaylistInsert( %u, %d ) = %s\n", Sequence, Index + count, (result) ? "TRUE" : "FALSE" ));
					if ( !result )
					{
						remove_from_sequence_list( next_sequence );
						break;
					}

					playlist_insert_tracks( Index + count, c, NULL, meta_data );
					Sequence = next_sequence;

					PlaylistData_Reset( playlist_data );
					count += c;
					c = 0;
				}

				if ( item == NULL ) { result = TRUE; break; }
			}

			PlaylistData_Free( playlist_data );

			if ( event_callback && count > 0 )
			{
				OnPlaylistInsert( Index, count, user_data );
				handle_playlist_current_track_index( current_item, user_data );
			}
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Playlist ) && IdArray != NULL )
	{
		struct PlaylistData *playlist_data = PlaylistData_New( 1, error );
		upnp_ui4 AfterId = (Index == 0) ? 0 : IdArray[Index-1];

		upnp_assert( playlist_data != NULL );
		if ( playlist_data != NULL )
		{
			for (;;)
			{
				char *item, *Uri, *MetaData;
				upnp_ui4 NewId;

				item = MetaDataList_GetItem( &MetaDataList );
				if ( item == NULL ) { result = TRUE; break; }

				result = PlaylistData_AddTrack( playlist_data, item, error );
				if ( !result ) break;

				MetaData = PlaylistData_Finalize( playlist_data, error );
				if ( MetaData == NULL ) { result = FALSE; break; }

				Uri = NULL;
				MetaData = PlaylistData_GetTrack( &MetaData, &Uri, NULL );
				upnp_assert( MetaData != NULL && Uri != NULL );
				if ( MetaData == NULL || Uri == NULL ) continue;

				result = Linn_Playlist_Insert( device, AfterId, Uri, MetaData, &NewId, error );
				upnp_message(( "PlaylistInsert(): Linn_Playlist_Insert( %u ) = %s, %u\n", AfterId, (result) ? "TRUE" : "FALSE", NewId ));
				if ( !result ) break;

				playlist_insert_track( Index + count, NewId, item );

				PlaylistData_Reset( playlist_data );
				count++;

				AfterId = NewId;
			}

			PlaylistData_Free( playlist_data );

			if ( event_callback && count > 0 )
			{
				OnPlaylistInsert( Index, count, user_data );
				handle_track_id( FALSE, user_data );
			}
		}
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) && IdArray != NULL )
	{
		struct PlaylistData *playlist_data = PlaylistData_New( 1, error );
		upnp_ui4 AfterId = (Index == 0) ? 0 : IdArray[Index-1];

		upnp_assert( playlist_data != NULL );
		if ( playlist_data != NULL )
		{
			for (;;)
			{
				char *item, *Uri, *MetaData;
				upnp_ui4 NewId;

				item = MetaDataList_GetItem( &MetaDataList );
				if ( item == NULL ) { result = TRUE; break; }

				result = PlaylistData_AddTrack( playlist_data, item, error );
				if ( !result ) break;

				MetaData = PlaylistData_Finalize( playlist_data, error );
				if ( MetaData == NULL ) { result = FALSE; break; }

				Uri = NULL;
				MetaData = PlaylistData_GetTrack( &MetaData, &Uri, NULL );
				upnp_assert( MetaData != NULL && Uri != NULL );
				if ( MetaData == NULL || Uri == NULL ) continue;

				result = Openhome_Playlist_Insert( device, AfterId, Uri, MetaData, &NewId, error );
				upnp_message(( "PlaylistInsert(): Openhome_Playlist_Insert( %u ) = %s, %u\n", AfterId, (result) ? "TRUE" : "FALSE", NewId ));
				if ( !result ) break;

				playlist_insert_track( Index + count, NewId, item );

				PlaylistData_Reset( playlist_data );
				count++;

				AfterId = NewId;
			}

			PlaylistData_Free( playlist_data );

			if ( event_callback && count > 0 )
			{
				OnPlaylistInsert( Index, count, user_data );
				handle_track_id( FALSE, user_data );
			}
		}
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
	}

	upnp_message(( "PlaylistInsert( %d, %d, %s ) = %s\n", Index, count, (event_callback) ? "TRUE" : "FALSE", (result) ? "TRUE" : "FALSE" ));
	return result;
}

gboolean PlaylistInsert( int Index, char *MetaDataList, gpointer user_data, GError **error )
{
	return playlist_insert( Index, MetaDataList, user_data, error, TRUE );
}

static gboolean playlist_delete( int Index, int Count, gpointer user_data, GError **error, gboolean event_callback )
{
	const upnp_device *device = AVTransport;
	int result = FALSE;

	if ( Count == 0 ) return TRUE;
	if ( Count < 0 ) return FALSE;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		const char *current_item = get_current_track_item();

		playlist_delete_tracks( Index, Count );
		result = TRUE;

		if ( event_callback )
		{
			OnPlaylistDelete( Index, Count, user_data );
			handle_playlist_current_track_index( current_item, user_data );
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		const char *current_item = get_current_track_item();
		upnp_ui4 next_sequence = append_to_sequence_list( 1 );

		result = Linn_Media_PlaylistDelete( device, Sequence, Index, Count, error );
		upnp_message(( "PlaylistDelete(): Linn_Media_PlaylistDelete( %u, %d, %d ) = %s\n", Sequence, Index, Count, (result) ? "TRUE" : "FALSE" ));
		if ( result )
		{
			playlist_delete_tracks( Index, Count );
			Sequence = next_sequence;

			if ( event_callback )
			{
				OnPlaylistDelete( Index, Count, user_data );
				handle_playlist_current_track_index( current_item, user_data );
			}
		}
		else
		{
			remove_from_sequence_list( next_sequence );
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		int i;

		for ( i = 0; i < Count; i++ )
		{
			result = Linn_Playlist_Delete( device, IdArray[Index], error );
			upnp_message(( "PlaylistDelete(): Linn_Playlist_Delete( %u ) = %s\n", IdArray[Index], (result) ? "TRUE" : "FALSE" ));
			if ( !result ) break;

			playlist_delete_tracks( Index, 1 );
		}

		if ( event_callback )
		{
			OnPlaylistDelete( Index, i, user_data );
			handle_track_id( FALSE, user_data );
		}
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		int i;

		for ( i = 0; i < Count; i++ )
		{
			result = Openhome_Playlist_DeleteId( device, IdArray[Index], error );
			upnp_message(( "PlaylistDelete(): Openhome_Playlist_Delete( %u ) = %s\n", IdArray[Index], (result) ? "TRUE" : "FALSE" ));
			if ( !result ) break;

			playlist_delete_tracks( Index, 1 );
		}

		if ( event_callback )
		{
			OnPlaylistDelete( Index, i, user_data );
			handle_track_id( FALSE, user_data );
		}
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
	}

	upnp_message(( "PlaylistDelete( %d, %d, %s ) = %s\n", Index, Count, (event_callback) ? "TRUE" : "FALSE", (result) ? "TRUE" : "FALSE" ));
	return result;
}

gboolean PlaylistDelete( int Index, int Count, gpointer user_data, GError **error )
{
	return playlist_delete( Index, Count, user_data, error, TRUE );
}

gboolean PlaylistMove( int IndexFrom, int IndexTo, int Count, gpointer user_data, GError **error )
{
	const char *current_item = get_current_track_item();
	GString *item_array = g_string_new( "" );
	gboolean result;
	int i;

	if ( IndexFrom == IndexTo || Count == 0 ) return TRUE;  /* nothing to do */
	if ( Count < 0 ) return FALSE;

	/* don't delete current track */
	if ( CurrentTrackIndex >= IndexFrom && CurrentTrackIndex < IndexFrom + Count )
	{
		int index_from = IndexFrom;
		int index_to   = IndexTo;

		IndexTo = index_from;
		if ( index_to > index_from )
		{
			IndexFrom += Count;
			Count = index_to - index_from;
		}
		else
		{
			IndexFrom -= Count;
			Count = index_from - index_to;
		}
	}

	for ( i = 0; i < Count; i++ )
	{
		char *item = ItemArray[IndexFrom + i];
		upnp_assert( item != NULL );
		if ( item == NULL )
		{
			upnp_warning(( "PlaylistMove(): ItemArray[%d] == NULL", IndexFrom + i ));
			g_string_free( item_array, TRUE );
			return FALSE;
		}
		g_string_append( item_array, item );
	}

	result = playlist_delete( IndexFrom, Count, user_data, error, FALSE );
	if ( result )
	{
		const upnp_device *device;

		result = playlist_insert( IndexTo, item_array->str, user_data, error, FALSE );

		if ( result )
		{
			OnPlaylistMove( IndexFrom, IndexTo, Count, user_data );
		}
		else
		{
			OnPlaylistDelete( IndexFrom, Count, user_data );
		}

		device = AVTransport;
		if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
		{
			handle_playlist_current_track_index( current_item, user_data );
		}
		else if ( upnp_device_is( device, linn_serviceId_Media ) )
		{
			handle_playlist_current_track_index( current_item, user_data );
		}
		else if ( upnp_device_is( device, linn_serviceId_Playlist ) || upnp_device_is( device, openhome_serviceId_Playlist ) )
		{
			handle_track_id( FALSE, user_data );
		}
		else if ( result )
		{
			g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
			result = FALSE;
		}
	}

	g_string_free( item_array, TRUE );
	return result;
}

gboolean PlaylistClear( gpointer user_data, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		result = AVTransport_SetURI( device, NULL, NULL, FALSE, error );
		upnp_message(( "PlaylistClear(): AVTransport_SetURI( NULL ) = %s\n", (result) ? "TRUE" : "FALSE" ));
		if ( result )
		{
			handle_playlist_clear( user_data );
			handle_current_track_index( -1, FALSE, user_data );
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		/* We use this opportunity to re-sync ourself with the Linn DS */
		result = Linn_Media_PlaylistSequence( device, &Sequence, error );
		upnp_message(( "PlaylistClear(): Linn_Media_PlaylistSequence() = %s, %u\n", (result) ? "TRUE" : "FALSE", Sequence ));
		if ( result )
		{
			upnp_ui4 track_count;
			result = Linn_Media_PlaylistTrackCount( device, Sequence, &track_count, error );
			upnp_message(( "PlaylistClear(): Linn_Media_PlaylistTrackCount() = %s, %u\n", (result) ? "TRUE" : "FALSE", track_count ));
			if ( result )
			{
				if ( track_count > 0 )
					result = playlist_delete( 0, track_count, user_data, error, FALSE );

				if ( result )
				{
					handle_playlist_clear( user_data );
					handle_current_track_index( -1, FALSE, user_data );
				}
			}
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		result = Linn_Playlist_DeleteAll( device, error );
		upnp_message(( "PlaylistClear(): Linn_Playlist_DeleteAll() = %s\n", (result) ? "TRUE" : "FALSE" ));
		if ( result )
		{
			handle_playlist_clear( user_data );
			handle_track_id( FALSE, user_data );
		}
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_DeleteAll( device, error );
		upnp_message(( "PlaylistClear(): Openhome_Playlist_DeleteAll() = %s\n", (result) ? "TRUE" : "FALSE" ));
		if ( result )
		{
			handle_playlist_clear( user_data );
			handle_track_id( FALSE, user_data );
		}
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean GetRepeat( gboolean *repeat, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) || upnp_device_is( device, linn_serviceId_Media ) )
	{
		*repeat = RepeatMode;
		result = TRUE;
	}
	else if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		result = Linn_Playlist_Repeat( device, repeat, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_Repeat( device, repeat, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean SetRepeat( gboolean repeat, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) || upnp_device_is( device, linn_serviceId_Media ) )
	{
		if ( RepeatMode != repeat )
		{
			RepeatMode = repeat;
			if ( ItemArray != NULL ) OnPlaylistRepeat( RepeatMode, NULL /* user_data */ );
		}

		result = TRUE;
	}
	else if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		result = Linn_Playlist_SetRepeat( device, repeat, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_SetRepeat( device, repeat, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean CanShuffle( void )
{
	const upnp_device *device = AVTransport;
	return upnp_device_is( device, linn_serviceId_Playlist ) || upnp_device_is( device, openhome_serviceId_Playlist );
}

gboolean GetShuffle( gboolean *shuffle, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		result = Linn_Playlist_Shuffle( device, shuffle, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_Shuffle( device, shuffle, error );
	}
	else
	{
		*shuffle = FALSE;
		result = TRUE;
	}

	return result;
}

gboolean SetShuffle( gboolean shuffle, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		result = Linn_Playlist_SetShuffle( device, shuffle, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = Openhome_Playlist_SetShuffle( device, shuffle, error );
	}
	else if ( !shuffle )
	{
		result = TRUE;
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

/*-----------------------------------------------------------------------------------------------*/

gboolean GetVolume( int *volume, GError **error )
{
	const upnp_device *device = RenderingControl;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_RenderingControl ) )
	{
		upnp_ui2 _volume;
		result = RenderingControl_GetVolume( device, RcsID, NULL, &_volume, error );
		*volume = (int)_volume;
	}
	else if ( upnp_device_is( device, linn_serviceId_Preamp ) )
	{
		upnp_ui4 _volume;
		result = Linn_Preamp_Volume( device, &_volume, error );
		*volume = (int)_volume;
	}
	else if ( upnp_device_is( device, openhome_serviceId_Volume ) )
	{
		upnp_ui4 _volume;
		result = Openhome_Volume_Volume( device, &_volume, error );
		*volume = (int)_volume;
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean SetVolume( int volume, GError **error )
{
	const upnp_device *device = RenderingControl;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_RenderingControl ) )
	{
		result = RenderingControl_SetVolume( device, RcsID, NULL, (upnp_ui2)volume, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Preamp ) )
	{
		result = Linn_Preamp_SetVolume( device, (upnp_ui4)volume, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Volume ) )
	{
		result = Openhome_Volume_SetVolume( device, (upnp_ui4)volume, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean GetMute( gboolean *mute, GError **error )
{
	const upnp_device *device = RenderingControl;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_RenderingControl ) )
	{
		result = RenderingControl_GetMute( device, RcsID, NULL, mute, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Preamp ) )
	{
		result = Linn_Preamp_Mute( device, mute, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Volume ) )
	{
		result = Openhome_Volume_Mute( device, mute, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean SetMute( gboolean mute, GError **error  )
{
	const upnp_device *device = RenderingControl;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_RenderingControl ) )
	{
		result = RenderingControl_SetMute( device, RcsID, NULL, mute, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Preamp ) )
	{
		result = Linn_Preamp_SetMute( device, mute, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Volume ) )
	{
		result = Openhome_Volume_SetMute( device, mute, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean GetVolumeLimit( int *volume_limit, GError **error )
{
	const upnp_device *device = RenderingControl;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_RenderingControl ) )
	{
		char *content = upnp_get_allowedValueRange( device, upnp_serviceId_RenderingControl, "Volume", error );
		result = FALSE;

		if ( content != NULL )
		{
			char *s, *name;
			xml_iter iter;

			for ( s = xml_first( content, &iter ); s != NULL; s = xml_next( &iter ) )
			{
				s = xml_unbox( &s, &name, NULL );
				if ( s == NULL ) break;

				if ( strcmp( name, "maximum" ) == 0 )
				{
					*volume_limit = atoi( s );
					result = TRUE;
					break;
				}
			}

			upnp_free_string( content );
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Preamp ) )
	{
		upnp_ui4 _volume_limit;
		result = Linn_Preamp_VolumeLimit( device, &_volume_limit, error );
		*volume_limit = (int)_volume_limit;
	}
	else if ( upnp_device_is( device, openhome_serviceId_Volume ) )
	{
		upnp_ui4 _volume_limit;
		result = Openhome_Volume_VolumeLimit( device, &_volume_limit, error );
		*volume_limit = (int)_volume_limit;
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	upnp_message(( "GetVolumeLimit() = %s, %d\n", (result) ? "TRUE" : "FALSE", *volume_limit ));
	return result;
}

/*-----------------------------------------------------------------------------------------------*/

void playlist_insert_track( int index, upnp_ui4 new_id, char *item )
{
	int i;

	upnp_message(( "playlist_insert_track( %d, id %u )\n", index, new_id ));
	if ( ItemArray == NULL ) return;

	for ( i = TracksMax; --i > index;)
	{
		if ( IdArray != NULL ) IdArray[i] = IdArray[i - 1];
		ItemArray[i] = ItemArray[i - 1];
	}

	if ( IdArray != NULL ) { upnp_assert( new_id != 0 ); IdArray[index] = new_id; }
	ItemArray[index] = g_strdup( item );
}

void playlist_insert_tracks( int index, int count, upnp_ui4 *new_id_array, char *meta_data_list )
{
	int i;

	for ( i = 0; i < count; i++ )
	{
		upnp_ui4 id = 0;
		char *meta_data = PlaylistData_GetTrack( &meta_data_list, NULL, &id );
		char *name = NULL, *item;

		item = xml_unbox( &meta_data, &name, NULL );
		if ( name != NULL && strcmp( name, "DIDL-Lite" ) == 0 && item != NULL )
		{
			if ( new_id_array != NULL )
			{
				upnp_assert( new_id_array[i] != 0 && id != 0 );

				while ( new_id_array[i] != id )
				{
					upnp_warning(( "playlist_insert_tracks(): Id %u expected but got %u instead\n", new_id_array[i], id ));
					playlist_insert_track( index + i, new_id_array[i], "<item></item>" );

					if ( ++i >= count ) return;
				}
			}
		}
		else
		{
			upnp_warning(( "playlist_insert_tracks(): invalid item\n" ));
			item = "<item></item>";
		}

		playlist_insert_track( index + i, (new_id_array != NULL) ? new_id_array[i] : id, item );
	}
}

void playlist_delete_tracks( int index, int count )
{
	int i;

	upnp_message(( "playlist_delete_tracks( %d, %d )\n", index, count ));
	if ( ItemArray == NULL ) return;

	for ( i = index; i < index + count; i++ )
	{
		if ( ItemArray[i] != NULL ) g_free( ItemArray[i] );
	}
	for ( i = index; i < TracksMax - count; i++ )
	{
		if ( IdArray != NULL ) IdArray[i] = IdArray[count + i];
		ItemArray[i] = ItemArray[count + i];
	}
	for ( ; i < TracksMax; i++ )
	{
		if ( IdArray != NULL ) IdArray[i] = 0;
		ItemArray[i] = NULL;
	}
}

void playlist_delete_all_tracks( void )
{
	int i;

	upnp_message(( "playlist_delete_all_tracks()\n" ));
	if ( ItemArray == NULL ) return;

	for ( i = 0; i < TracksMax; i++ )
	{
		if ( IdArray != NULL ) IdArray[i] = 0;

		if ( ItemArray[i] != NULL )
		{
			g_free( ItemArray[i] );
			ItemArray[i] = NULL;
		}
	}
}

#if 0
void playlist_move_tracks( int index_from, int index_to, int count )
{
	char *item;

	upnp_message(( "playlist_move_tracks( %d => %d, %d )\n", index_from, index_to, count ));
	if ( ItemArray == NULL ) return;

	upnp_assert( count == 1 );

	if ( IdArray != NULL )
	{
		upnp_ui4 id = IdArray[index_from];
		IdArray[index_from] = IdArray[index_to];
		IdArray[index_to] = id;
	}

	item = ItemArray[index_from];
	ItemArray[index_from] = ItemArray[index_to];
	ItemArray[index_to] = item;
}
#endif

/*-----------------------------------------------------------------------------------------------*/
/* See also: http://en.wikipedia.org/wiki/Base64 */

static const unsigned char d64[0x50] =
{
	62,  0,  0,  0, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
	 0,  0,  0,  0,  0,  0,  0,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
	 0,  0,  0,  0,  0,  0,
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

int decode64( unsigned char *dest, const char *src )
{
	unsigned char *t = dest;
	int n = 0;

	upnp_assert( strlen( src ) % 4 == 0 );

	for ( ; *src != '\0'; src += 4 )
	{
		upnp_assert( src[0] >= '+' && src[0] <= 'z' );
		upnp_assert( src[1] >= '+' && src[1] <= 'z' );
		upnp_assert( src[2] >= '+' && src[2] <= 'z' );
		upnp_assert( src[3] >= '+' && src[3] <= 'z' );

		upnp_assert( src[1] != '\0' && src[1] != '=' );
		if ( src[1] == '\0' || src[1] == '=' ) break;
		if ( t != NULL ) *t++ = (unsigned char)((d64[src[0]-'+'] << 2) | (d64[src[1]-'+'] >> 4));
		n++;

		if ( src[2] == '\0' || src[2] == '=' ) break;
		if ( t != NULL ) *t++ = (unsigned char)((d64[src[1]-'+'] << 4) | (d64[src[2]-'+'] >> 2));
		n++;

		if ( src[3] == '\0' || src[3] == '=' ) break;
		if ( t != NULL ) *t++ = (unsigned char)((d64[src[2]-'+'] << 6) | (d64[src[3]-'+'] >> 0));
		n++;
	}

	return n;
}

int decode_id_array( upnp_ui4 *id_array, const char *content )
{
	int tracks = decode64( (unsigned char *)id_array, content );
	int i;

	upnp_assert( sizeof( upnp_ui4 ) == 4 );
	upnp_assert( tracks % 4 == 0 );
	tracks /= 4;

	for ( i = 0; i < tracks; i++ )
		id_array[i] = GUINT32_FROM_BE( id_array[i] );
	for ( ; i <= TracksMax; i++ )
		id_array[i] = 0;

	return tracks;
}

/*-----------------------------------------------------------------------------------------------*/

static gboolean (*_callback)( upnp_subscription *subscription, char *e_propertyset );
static void *_ref_data;

gboolean SubscribeMediaRenderer( gboolean (*callback)( upnp_subscription *subscription, char *e_propertyset ), void *ref_data, GError **error )
{
	const upnp_subscription *subscription = NULL;
	const upnp_device *device = AVTransport;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		subscription = upnp_subscribe( device, upnp_serviceId_AVTransport, SUBSCRIPTION_TIMEOUT, callback, ref_data, NULL );
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		if ( sequence_list_mutex == NULL )
		{
			sequence_list_mutex = g_mutex_new();
			sequence_list = NULL;
		}

		subscription = upnp_subscribe( device, linn_serviceId_Media, SUBSCRIPTION_TIMEOUT, callback, ref_data, NULL );
	}
	else if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		subscription = upnp_subscribe( device, linn_serviceId_Playlist, SUBSCRIPTION_TIMEOUT, callback, ref_data, error );
		if ( subscription != NULL )
		{
			subscription = upnp_subscribe( device, linn_serviceId_Ds, SUBSCRIPTION_TIMEOUT, callback, ref_data, error );
			if ( subscription == NULL )
			{
				upnp_unsubscribe( device, linn_serviceId_Playlist, NULL );
			}
		}
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		subscription = upnp_subscribe( device, openhome_serviceId_Playlist, SUBSCRIPTION_TIMEOUT, callback, ref_data, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
	}

	_ref_data = ref_data;
	_callback = callback;
	return (subscription != NULL);
}

gboolean UnsubscribeMediaRenderer( GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean result;

	DisableVolumeEvents( error );

	_ref_data = NULL;
	_callback = NULL;

	if ( upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		result = upnp_unsubscribe( device, upnp_serviceId_AVTransport, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Media ) )
	{
		result = upnp_unsubscribe( device, linn_serviceId_Media, error );
		if ( result )
		{
			clear_sequence_list();
			g_mutex_free( sequence_list_mutex );
			sequence_list_mutex = NULL;
		}
	}
	else if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		result = upnp_unsubscribe( device, linn_serviceId_Ds, error );
		if ( !upnp_unsubscribe( device, linn_serviceId_Playlist, error ) ) result = FALSE;
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		result = upnp_unsubscribe( device, openhome_serviceId_Playlist, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

gboolean EnableVolumeEvents( GError **error )
{
	const upnp_device *device = RenderingControl;
	upnp_subscription *subscription;

	if ( upnp_device_is( device, upnp_serviceId_RenderingControl ) )
	{
		subscription = upnp_subscribe( device, upnp_serviceId_RenderingControl, SUBSCRIPTION_TIMEOUT, _callback, _ref_data, NULL );
	}
	else if ( upnp_device_is( device, linn_serviceId_Preamp ) )
	{
		subscription = upnp_subscribe( device, linn_serviceId_Preamp, SUBSCRIPTION_TIMEOUT, _callback, _ref_data, NULL );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Volume ) )
	{
		subscription = upnp_subscribe( device, openhome_serviceId_Volume, SUBSCRIPTION_TIMEOUT, _callback, _ref_data, NULL );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		subscription = NULL;
	}

	return (subscription != NULL);
}

gboolean DisableVolumeEvents( GError **error )
{
	const upnp_device *device = RenderingControl;
	gboolean result;

	if ( upnp_device_is( device, upnp_serviceId_RenderingControl ) )
	{
		result = upnp_unsubscribe( device, upnp_serviceId_RenderingControl, NULL );
	}
	else if ( upnp_device_is( device, linn_serviceId_Preamp ) )
	{
		result = upnp_unsubscribe( device, linn_serviceId_Preamp, NULL );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Volume ) )
	{
		result = upnp_unsubscribe( device, openhome_serviceId_Volume, NULL );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		result = FALSE;
	}

	return result;
}

/*-----------------------------------------------------------------------------------------------*/

struct media_renderer_event
{
	char *TransportState, *CurrentTrack;
	char *CurrentTrackURI, *CurrentTrackEmbeddedMetaData;
	char *AVTransportURI, *AVTransportURIMetaData;

	char *Mute, *Volume;
};

static char *unbox_upnp_event_val( char *content, char **name, char **val, char **channel )
{
	char *attrib;
	char *s = xml_unbox( &content, name, &attrib );
	upnp_assert( s != NULL );
	if ( s == NULL ) return NULL;

	*val = NULL;
	*channel = NULL;

	for (;;)
	{
		char *name4, *s4;

		s4 = xml_unbox_attribute( &attrib, &name4 );
		if ( s4 == NULL ) break;

		if ( strcmp( name4, "val" ) == 0 ) *val = s4;
		else if ( strcmp( name4, "channel" ) == 0 ) *channel = s4;
	}

	return s;
}

static void get_upnp_event( char *e_propertyset, upnp_i4 InstanceID, struct media_renderer_event* event )
{
	/*gboolean success = TRUE;*/
	char *name, *content;
	xml_iter iter;

	memset( event, 0, sizeof( struct media_renderer_event ) );

	/* "<e:property><LastChange>&lt;Event xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/AVT/&quot;&gt;&lt;InstanceID val=&quot;0&quot;&gt;&lt;TransportState val=&quot;NO_MEDIA_PRESENT&quot;/&gt;..." */
	for ( content = xml_first_e_property( e_propertyset, &name, &iter ); content != NULL; content = xml_next_e_property( &name, &iter ) )
	{
		if ( strcmp( name, "LastChange" ) == 0 )
		{
			xml_iter iter;
			char *s;

			content = xml_unbox( &content, &name, NULL );
			/*upnp_assert( content != NULL && strcmp( name, "Event" ) == 0 );*/
			if ( content == NULL || strcmp( name, "Event" ) != 0 )
			{
				/*success = FALSE;*/
				break;
			}

			for ( s = xml_first( content, &iter ); s != NULL; s = xml_next( &iter ) )
			{
				char *s2, *val, *channel;
				xml_iter iter;

				s2 = unbox_upnp_event_val( s, &name, &val, &channel );
				upnp_assert( s2 != NULL && val != NULL );
				if ( s2 == NULL || val == NULL )
				{
					/*success = FALSE;*/
					break;
				}

				upnp_message(( "get_upnp_event(): %s = \"%s\"\n", name, val ));

				if ( strcmp( name, "InstanceID" ) == 0 && strtol( val, NULL, 10 ) == InstanceID )
				{
					for ( s2 = xml_first( s2, &iter ); s2 != NULL; s2 = xml_next( &iter ) )
					{
						char *s3 = unbox_upnp_event_val( s2, &name, &val, &channel );
						upnp_assert( s3 != NULL && val != NULL );
						if ( s3 == NULL || val == NULL ) break;

						upnp_message(( "get_upnp_event(): %s = \"%s\"\n", name, val ));
						if ( strcmp( val, "NOT_IMPLEMENTED" ) == 0 )
							continue;
					/*
						TransportState = "NO_MEDIA_PRESENT"
						TransportStatus = "OK"
						CurrentMediaCategory = "NO_MEDIA"
						PlaybackStorageMedium = "NONE"
						NumberOfTracks = "0"
						CurrentTrack = "0"
						CurrentTrackDuration = "0:00:00"
						CurrentMediaDuration = "0:00:00"
						CurrentTrackURI = ""
						AVTransportURI = ""
						CurrentTrackMetaData = ""
						AVTransportURIMetaData = ""
						PossiblePlaybackStorageMedia = "NETWORK"
						CurrentPlayMode = "NORMAL"
						TransportPlaySpeed = "1"
						NextAVTransportURI = "NOT_IMPLEMENTED"
						NextAVTransportURIMetaData = "NOT_IMPLEMENTED"
						RecordStorageMedium = "NOT_IMPLEMENTED"
						PossibleRecordStorageMedia = "NOT_IMPLEMENTED"
						RecordMediumWriteStatus = "NOT_IMPLEMENTED"
						CurrentRecordQualityMode = "NOT_IMPLEMENTED"
						PossibleRecordQualityModes = "NOT_IMPLEMENTED"
					*/
						if ( strcmp( name, "TransportState" ) == 0 )
							event->TransportState = val;
						else if ( strcmp( name, "CurrentTrack" ) == 0 )
							event->CurrentTrack = val;
						else if ( strcmp( name, "CurrentTrackURI" ) == 0 )
							event->CurrentTrackURI = val;
						else if ( strcmp( name, "CurrentTrackEmbeddedMetaData" ) == 0 )
							event->CurrentTrackEmbeddedMetaData = val;
						else if ( strcmp( name, "AVTransportURI" ) == 0 )
							event->AVTransportURI = val;
						else if ( strcmp( name, "AVTransportURIMetaData" ) == 0 )
							event->AVTransportURIMetaData = val;

					/*
						InstanceID = "0"
						PresetNameList = "FactoryDefaults"
						Mute = "0"
						Volume = "50"
						VolumeDB = "-7680"
					*/
						else if ( strcmp( name, "Volume" ) == 0 && (channel == NULL || strcmp( channel, Master ) == 0) )
							event->Volume = val;
						else if ( strcmp( name, "Mute" ) == 0 )
							event->Mute = val;
					}
				}
			}
		}
	}

	/*return success;*/
}

gboolean HandleMediaRendererEvent( const upnp_device *device, const char *serviceTypeOrId, upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error )
{
	gboolean success = TRUE;

	upnp_message(( "HandleMediaRendererEvent( %s, %u )...\n", serviceTypeOrId, SEQ ));

	if ( device != AVTransport && device != RenderingControl )
	{
		upnp_warning(( "HandleMediaRendererEvent( %s, %u ): unknown device\n", serviceTypeOrId, SEQ ));
		return FALSE;
	}

	if ( upnp_get_subscription( device, serviceTypeOrId ) == NULL )
	{
		upnp_warning(( "HandleMediaRendererEvent( %s, %u ): not subscribed (anymore)\n", serviceTypeOrId, SEQ ));
		return FALSE;
	}

	if ( strcmp( serviceTypeOrId, upnp_serviceId_AVTransport ) == 0 )
		success = handle_av_transport_event( SEQ, e_propertyset, user_data, error );
	else if ( strcmp( serviceTypeOrId, linn_serviceId_Media ) == 0 )
		success = handle_media_event( SEQ, e_propertyset, user_data, error );
	else if ( strcmp( serviceTypeOrId, linn_serviceId_Playlist ) == 0 )
		success = handle_playlist_event( SEQ, e_propertyset, user_data, error );
	else if ( strcmp( serviceTypeOrId, linn_serviceId_Ds ) == 0 )
		handle_ds_event( SEQ, e_propertyset, user_data );
	else if ( strcmp( serviceTypeOrId, openhome_serviceId_Playlist ) == 0 )
		success = handle_playlist_event( SEQ, e_propertyset, user_data, error );

	else if ( strcmp( serviceTypeOrId, upnp_serviceId_RenderingControl ) == 0 )
		success = handle_rendering_control_event( SEQ, e_propertyset, user_data, error );
	else if ( strcmp( serviceTypeOrId, linn_serviceId_Preamp ) == 0 )
		handle_preamp_event( SEQ, e_propertyset, user_data );
	else if ( strcmp( serviceTypeOrId, openhome_serviceId_Volume ) == 0 )
		handle_preamp_event( SEQ, e_propertyset, user_data );

	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		success = FALSE;
	}

	upnp_message(( "HandleMediaRendererEvent() = %s\n", (success) ? "TRUE" : "FALSE" ));
	return success;
}

static gboolean is_track_uri( int track, const char *uri )
{
	gboolean result = FALSE;

	upnp_assert( uri != NULL && *uri != '\0' );

	if ( track >= 0 && track < TracksMax )
	{
		const char *item = ItemArray[track];
		if ( item != NULL )
		{
			gchar *res = xml_get_named_str( item, "res" );
			if ( res != NULL )
			{
				upnp_assert( uri != NULL );
				if ( strcmp( res, uri ) == 0 )
					result = TRUE;

				g_free( res );
			}
		}
	}

	return result;
}

gboolean handle_av_transport_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error )
{
	struct media_renderer_event event = {0};
	gboolean success = TRUE;
	char *buffer = NULL;
	int i;

	Sequence = SEQ;

	if ( SEQ == 0 )
	{
		if ( ItemArray == NULL ) ItemArray = g_new0( char *, TracksMax = 1000 );

		handle_playlist_clear( user_data );
		handle_current_track_index( -1, TRUE, user_data );
	}

	get_upnp_event( e_propertyset, AVTransportID, &event );

	if ( SEQ == 0 )
	{
		if ( event.AVTransportURI == NULL )
			event.AVTransportURI = event.CurrentTrackURI;
		if ( event.AVTransportURIMetaData == NULL )
			event.AVTransportURIMetaData = event.CurrentTrackEmbeddedMetaData;

		if ( event.AVTransportURI == NULL || event.AVTransportURIMetaData == NULL )
		{
			char *current_uri, *current_uri_meta_data;

			success = AVTransport_GetMediaInfo( AVTransport, AVTransportID, NULL, NULL, &current_uri, &current_uri_meta_data, NULL, NULL, NULL, NULL, NULL, error );
			upnp_message(( "handle_av_transport_event(): AVTransport_GetMediaInfo() = %s\n", (success) ? "TRUE" : "FALSE" ));
			if ( success )
			{
				char *s;

				buffer = g_new( char, strlen( current_uri ) + strlen( current_uri_meta_data ) + 2 );
				upnp_assert( buffer != NULL );
				s = buffer;

				if ( event.AVTransportURI == NULL )
				{
					event.AVTransportURI = strcpy( s, current_uri );
					s += strlen( current_uri ) + 1;
				}
				if ( event.AVTransportURIMetaData == NULL )
				{
					event.AVTransportURIMetaData = strcpy( s, current_uri_meta_data );
					/*s += strlen( current_uri_meta_data ) + 1;*/
				}

				upnp_free_string( current_uri_meta_data );
				upnp_free_string( current_uri );
			}
		}

		if ( event.TransportState == NULL )
		{
			upnp_string transport_state;

			success = AVTransport_GetTransportInfo( AVTransport, AVTransportID, &transport_state, NULL, NULL, error );
			upnp_message(( "handle_av_transport_event(): AVTransport_GetTransportInfo() = %s\n", (success) ? "TRUE" : "FALSE" ));
			if ( success )
			{
				event.TransportState = strcpy( e_propertyset, transport_state );
				upnp_free_string( transport_state );
			}
		}
	}
	else if ( CurrentTrackIndex >= 0 && NextTrackIndex < 0 &&
		AVTransportURI != NULL && (event.AVTransportURI == NULL || event.AVTransportURI[0] == '\0' || strcmp( event.AVTransportURI, AVTransportURI ) == 0) &&
		TransportState == TRANSPORT_STATE_PLAYING && event.TransportState != NULL && stricmp( event.TransportState, "STOPPED" ) == 0 )
	{
		int next_track = CurrentTrackIndex + 1;
		int track_count = GetTrackCount();

		if ( RepeatMode && next_track == track_count ) next_track = 0;

		if ( next_track < track_count )
		{
			/* Current track has been finished, start the next one */
			upnp_message(( "\n" ));
			upnp_message(( "****************************************************\n" ));
			upnp_message(( "*                                                  *\n" ));
			upnp_message(( "* handle_av_transport_event(): Start next track... *\n" ));
			upnp_message(( "*                                                  *\n" ));
			upnp_message(( "****************************************************\n" ));
			upnp_message(( "\n" ));

			success = AVTransport_SeekTrackAbsolute( AVTransport, next_track, TRUE, error );
			if ( success )
			{
				TransportState = get_transport_state_from_string( event.TransportState );
				return TRUE;
			}
		}
	}

	if ( event.AVTransportURI != NULL && event.AVTransportURI[0] != '\0' && SEQ != 0 )
	{
		for ( i = 0; i < TracksMax; i++ )
		{
			if ( is_track_uri( i, event.AVTransportURI ) ) break;
		}
		if ( i == TracksMax )  /* not our AVTransportURI */
		{
			handle_playlist_clear( user_data );
		}
	}

	if ( event.AVTransportURIMetaData != NULL && event.AVTransportURIMetaData[0] != '\0' && GetTrackCount() == 0 )
	{
		for ( i = 0;; i++ )
		{
			char *item = MetaDataList_GetItem( &event.AVTransportURIMetaData );
			if ( item == NULL ) break;
			playlist_insert_track( i, 0, item );
		}

		OnPlaylistInsert( 0, i, user_data );
	}

	/* Some media renderer supports only AVTransportURI */
	if ( event.CurrentTrackURI == NULL )
		event.CurrentTrackURI = event.AVTransportURI;

	if ( event.CurrentTrackURI != NULL )
	{
		int track = -1;

		if ( event.CurrentTrackURI[0] != '\0' )
		{
			if ( is_track_uri( NextTrackIndex, event.CurrentTrackURI ) )
			{
				track = NextTrackIndex;
				NextTrackIndex = -1;
			}
			else if ( is_track_uri( CurrentTrackIndex, event.CurrentTrackURI ) )
			{
				track = CurrentTrackIndex;
			}
			else if ( event.CurrentTrack != NULL && is_track_uri( i = atoi( event.CurrentTrack ) - 1, event.CurrentTrackURI ) )
			{
				track = i;
			}
			else
			{
				for ( i = 0; i < TracksMax; i++ )
				{
					if ( is_track_uri( i, event.CurrentTrackURI ) )
					{
						track = i;
						break;
					}
				}
			}
		}

		handle_current_track_index( track, TRUE, user_data );
	}

	if ( event.TransportState != NULL )
	{
		handle_transport_state( TRUE, event.TransportState, user_data );
	}

	if ( buffer != NULL ) g_free( buffer );
	return success;
}

gboolean handle_rendering_control_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error )
{
	struct media_renderer_event event = {0};
	gboolean success = TRUE;
	int volume, mute;

	if ( SEQ == 0 )
	{
		if ( !GetVolumeLimit( &VolumeLimit, NULL ) )
			VolumeLimit = 100;

		OnVolumeInit( user_data );
	}

	get_upnp_event( e_propertyset, RcsID, &event );

	volume = (event.Volume != NULL) ? atoi( event.Volume ) : -1;
	mute   = (event.Mute   != NULL) ? atoi( event.Mute   ) : -1;

	if ( SEQ == 0 )
	{
		if ( volume < 0 ) success = GetVolume( &volume, error );
		if ( mute   < 0 ) success = GetMute( &mute, error );
	}

	if ( volume >= 0 ) OnVolume( CurrentVolume = volume, user_data );
	if ( mute   >= 0 ) OnMute  ( CurrentMute   = mute  , user_data );

	return success;
}

gboolean handle_media_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error )
{
	const upnp_device *device = AVTransport;
	static upnp_ui4 sequence;
	gboolean success = TRUE;

	if ( SEQ == 0 )
	{
		upnp_ui4 track_count;
		upnp_i4 current_track_index;
		upnp_string transport_state;

		if ( ItemArray == NULL ) ItemArray = g_new0( char *, TracksMax = 1000 );
		handle_playlist_clear( user_data );

		success = Linn_Media_PlaylistSequence( device, &sequence, error );
		if ( success )
		{
			upnp_message(( "handle_media_event(): Linn_Media_PlaylistSequence() = %u\n", sequence ));

			success = Linn_Media_PlaylistTrackCount( device, sequence, &track_count, error );
			if ( success )
			{
				upnp_message(( "handle_media_event(): Linn_Media_PlaylistTrackCount() = %d\n", track_count ));

				clear_sequence_list();
				Sequence = sequence;

				if ( track_count > 0 )
				{
					success = handle_playlist_insert( sequence, 0, track_count, user_data, error );
				}
			}
			else
			{
				upnp_warning(( "handle_media_event(): Media_PlaylistTrackCount() = FALSE\n" ));
			}
		}
		else
		{
			upnp_warning(( "handle_media_event(): Media_PlaylistSequence() = FALSE\n" ));
		}

		CurrentTrackIndex = -1;
		CurrentTrackId = 0;
		TransportState = TRANSPORT_STATE_UNKNOWN;

		success = Linn_Media_Status( device, &transport_state, &current_track_index, NULL, NULL, NULL, NULL, error );
		if ( success )
		{
			upnp_message(( "handle_media_event(): Linn_Media_Status() = %s, %d\n", transport_state, current_track_index ));

			handle_current_track_index( current_track_index, TRUE, user_data );
			handle_transport_state( FALSE, transport_state, user_data );

			upnp_free_string( transport_state );
		}
		else
		{
			upnp_warning(( "handle_media_event(): Linn_Media_Status() = FALSE\n" ));
		}

		OnPlaylistRepeat( RepeatMode, user_data );
	}
	else
	{
	/*
		e.g. "<e:property><TransportState>Eop</TransportState></e:property>
			  <e:property><CurrentTrackIndex>&lt;CurrentTrackIndex&gt;&lt;Sequence&gt;35&lt;/Sequence&gt;&lt;Index&gt;-1&lt;/Index&gt;&lt;/CurrentTrackIndex&gt;</CurrentTrackIndex></e:property>"
		e.g. "<e:property><PlaylistInsert>&lt;Insert&gt;&lt;Sequence&gt;37&lt;/Sequence&gt;&lt;Index&gt;0&lt;/Index&gt;&lt;Count&gt;1&lt;/Count&gt;&lt;/Insert&gt;</PlaylistInsert></e:property>
			  <e:property><PlaylistDelete>&lt;Delete&gt;&lt;Sequence&gt;36&lt;/Sequence&gt;&lt;Index&gt;0&lt;/Index&gt;&lt;Count&gt;1&lt;/Count&gt;&lt;/Delete&gt;</PlaylistDelete></e:property>"
	*/

		#define MAX_MEDIA_EVENTS 10
		struct e_property
		{
			const char *name, *value;
			upnp_ui4 Sequence;
			int Index, IndexFrom, IndexTo, Count;
		} events[MAX_MEDIA_EVENTS];
		xml_iter iter;
		int i, n = 0;
		char *s;

		/* First we collect all the events... */
		for ( s = xml_first( e_propertyset, &iter ); s != NULL; s = xml_next( &iter ) )
		{
			char *name, *content;

			s = xml_unbox( &s, &name, NULL );
			upnp_assert( s != NULL && strcmp( name, "e:property" ) == 0 );
			if ( s == NULL || strcmp( name, "e:property" ) != 0 ) break;

			s = xml_unbox( &s, &name, NULL );
			upnp_assert( s != NULL );
			if ( s == NULL ) break;

			upnp_message(( "handle_media_event(): %s = \"%s\"\n", name, s ));

			upnp_assert( n < MAX_MEDIA_EVENTS );
			if ( n == MAX_MEDIA_EVENTS )
			{
				g_set_error( error, UPNP_ERROR, UPNP_ERR_OUT_OF_MEMORY, "Too many media events; can't handle that amount" );
				success = FALSE;
				break;
			}

			upnp_assert( name != NULL );

			events[n].name = name;
			events[n].Sequence = sequence;
			events[n].Index = events[n].IndexFrom = events[n].IndexTo = -1;
			events[n].Count = 0;

			content = xml_unbox( &s, &name, NULL );
			upnp_assert( s != NULL );
			if ( content != NULL )
			{
				xml_iter iter;
				char *s;

				events[n].value = name;

				for ( s = xml_first( content, &iter ); s != NULL; s = xml_next( &iter ) )
				{
					s = xml_unbox( &s, &name, NULL );
					upnp_assert( s != NULL );
					if ( s == NULL ) break;

					if ( strcmp( name, "Sequence" ) == 0 )
						events[n].Sequence = strtoul( s, NULL, 10 );
					else if ( strcmp( name, "Index" ) == 0 )
						events[n].Index = atoi( s );
					else if ( strcmp( name, "IndexFrom" ) == 0 )
						events[n].IndexFrom = atoi( s );
					else if ( strcmp( name, "IndexTo" ) == 0 )
						events[n].IndexTo = atoi( s );
					else if ( strcmp( name, "Count" ) == 0 )
						events[n].Count = atoi( s );
				}
			}
			else
			{
				events[n].value = s;
			}

			n++;
		}

		/* ...and afterwards we handle them in the correct (sequence) order */
		for (;;)
		{
			int unhandled_events = 0;

			upnp_debug(( "handle_media_event(): Try Sequence #%u (n = %d)...\n", Sequence, n ));

			for ( i = 0; i < n; i++ )
			{
				struct e_property *event = &events[i];
				if ( event->name == NULL ) continue;  /* already handled event */

				if ( event->Sequence == sequence )
				{
					upnp_message(( "handle_media_event(): event[%d]: name=%s, value=%s, Sequence=%u, Index=%d, IndexFrom=%d, IndexTo=%d, Count=%d\n",
						 i, event->name, event->value, event->Sequence, event->Index, event->IndexFrom, event->IndexTo, event->Count ));

					if ( strcmp( event->name, "TransportState" ) == 0 )
					{
						handle_transport_state( TRUE, event->value, user_data );
					}
					else if ( strcmp( event->name, "CurrentTrackIndex" ) == 0 )
					{
						if ( strcmp( event->value, "CurrentTrackIndex" ) == 0 )
							handle_current_track_index( event->Index, TRUE, user_data );
					}
					else if ( strcmp( event->name, "PlaylistInsert" ) == 0 )
					{
						if ( strcmp( event->value, "Insert" ) == 0 )
							success = handle_playlist_insert( event->Sequence, event->Index, event->Count, user_data, error );
					}
					else if ( strcmp( event->name, "PlaylistDelete" ) == 0 )
					{
						if ( strcmp( event->value, "Delete" ) == 0 )
							handle_playlist_delete( event->Sequence, event->Index, event->Count, user_data );
					}
				#if 0  /* was never implemented by Linn */
					else if ( strcmp( event->name, "PlaylistMove" ) == 0 )
					{
						if ( strcmp( event->value, "Move" ) == 0 )
							handle_playlist_move( event->Sequence, event->IndexFrom, event->IndexTo, event->Count, user_data );
					}
				#endif

					event->name = NULL;   /* mark event as handled */
				}
				else
				{
					unhandled_events++;
				}
			}

			upnp_debug(( "Media_HandleEvent(): unhandled_events = %d\n", unhandled_events ));
			if ( unhandled_events == 0 ) break;  /* All events have been handled */

			sequence = add_to_sequence( sequence, 1 );
		}
	}

	return success;
}

void handle_playlist_clear( gpointer user_data )
{
	playlist_delete_all_tracks();
	OnPlaylistClear( user_data );
}

gboolean handle_playlist_insert( upnp_ui4 sequence, int index, int count, gpointer user_data, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean success = TRUE;

	if ( was_in_sequence_list( sequence ) )
	{
		upnp_message(( "* handle_playlist_insert(): Sequence %u already handled\n", sequence ));
	}
	else
	{
		const char *current_item = get_current_track_item();
		upnp_ui4 sequence;

		/* The Sequence number could already be outdated,
		   so we use the actual one here instead of 'Sequence' */
		success = Linn_Media_PlaylistSequence( device, &sequence, error );
		if ( success )
		{
			upnp_string playlist_data = NULL;

			upnp_message(( "handle_playlist_insert(): Linn_Media_PlaylistSequence() = %u\n", sequence ));

			success = Linn_Media_PlaylistRead( device, sequence, index, count, &playlist_data, error );
			if ( success )
			{
				playlist_insert_tracks( index, count, NULL, playlist_data );
				upnp_free_string( playlist_data );

				OnPlaylistInsert( index, count, user_data );
			}
			else
			{
				upnp_warning(( "handle_playlist_insert(): Linn_Media_PlaylistRead() = FALSE\n" ));
			}
		}
		else
		{
			upnp_warning(( "handle_playlist_insert(): Linn_Media_PlaylistSequence() = FALSE\n" ));
		}

		handle_playlist_current_track_index( current_item, user_data );
	}

	return success;
}

void handle_playlist_delete( upnp_ui4 sequence, int index, int count, gpointer user_data )
{
	if ( was_in_sequence_list( sequence ) )
	{
		upnp_message(( "* handle_playlist_delete(): Sequence %u already handled\n", sequence ));
	}
	else
	{
		const char *current_item = get_current_track_item();

		playlist_delete_tracks( index, count );
		OnPlaylistDelete( index, count, user_data );

		handle_playlist_current_track_index( current_item, user_data );
	}
}

#if 0
void handle_playlist_move( upnp_ui4 sequence, int index_from, int index_to, int count, gpointer user_data )
{
	if ( was_in_sequence_list( sequence ) )
	{
		upnp_message(( "* handle_playlist_move(): Sequence %u already handled\n", sequence ));
	}
	else
	{
		char *current_item = get_current_track_item();

		playlist_move_tracks( index_from, index_to, count );
		OnPlaylistMove( index_from, index_to, count, user_data );

		handle_playlist_current_track_index( current_item, user_data );
	}
}
#endif

void handle_playlist_current_track_index( const char *item, gpointer user_data )
{
	if ( item != NULL )
	{
		int i;

		for ( i = 0; i < TracksMax; i++ )
		{
			if ( ItemArray != NULL && ItemArray[i] == item )
			{
				if ( item != get_current_track_item() )
				{
					handle_current_track_index( i, FALSE, user_data );
				}

				break;
			}
		}
	}
}

void handle_current_track_index( int index, gboolean evented, gpointer user_data )
{
	int OldTrackIndex = CurrentTrackIndex;
	OnCurrentTrackIndex( OldTrackIndex, CurrentTrackIndex = index, evented, user_data );
}

void handle_transport_state( int media_event, const char *state, gpointer user_data )
{
	enum TransportState old_state = TransportState;
	enum TransportState new_state = get_transport_state_from_string( state );

	TransportState = new_state;
	OnTransportState( old_state, new_state, user_data );

	if ( media_event && RepeatMode && old_state == TRANSPORT_STATE_PLAYING && new_state == TRANSPORT_STATE_EOP )
	{
		upnp_message(( "handle_transport_state(): Repeat mode, start playing (again)...\n" ));
		StartPlaying();
	}
}

static gboolean atob( const char *string )
{
	return ( strcmp( string, "true" ) == 0 || strcmp( string, "1" ) == 0 );
}

gboolean handle_playlist_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error )
{
	const upnp_device *device = AVTransport;
	gboolean success = TRUE;
	char *name, *content;
	xml_iter iter;

	Sequence = SEQ;

	if ( SEQ == 0 )
	{
		if ( IdArray == NULL )
		{
			gchar *value = xml_get_named_str( e_propertyset, "TracksMax" );
			if ( value != NULL )
			{
				TracksMax = atoi( value );
				upnp_message(( "TracksMax = %d\n", TracksMax ));
				g_free( value );
			}
			else
			{
				TracksMax = 1000;
			}

			IdArray = g_new0( upnp_ui4, TracksMax + 1 );

			upnp_assert( ItemArray == NULL );
			ItemArray = g_new0( char *, TracksMax + 1 );
		}

		handle_playlist_clear( user_data );

		if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
		{
			CurrentTrackIndex = -1;
			CurrentTrackId = 0;
			TransportState = TRANSPORT_STATE_UNKNOWN;
		}
	}

	/* "<e:property><IdArray>AAAAAQAAAAIAAAADAAAABAAAAAUAAAAGAAAABwAAAAgAAAAJAAAACgAAAAs=</IdArray></e:property><e:property><Repeat>false</Repeat></e:property><e:property><Shuffle>false</Shuffle></e:property><e:property><TracksMax>1000</TracksMax></e:property>" */
	for ( content = xml_first_e_property( e_propertyset, &name, &iter ); content != NULL; content = xml_next_e_property( &name, &iter ) )
	{
		upnp_message(( "handle_playlist_event(): %s = \"%s\"\n", name, content ));

		/* Cara & Davaar stuff */
		if ( strcmp( name, "IdArray" ) == 0 )
		{
			upnp_ui4 *new_id_array = g_new0( upnp_ui4, TracksMax + 1 );
			int tracks = decode_id_array( new_id_array, content );
			int i;

			if ( SEQ != 0 )
			{
				/* The IdArray could already be outdated,
				   so we may use the actual one here instead of 'IdArray' */
				for ( i = 0; i < TracksMax; i++ )
				{
					if ( new_id_array[i] != IdArray[i] )
					{
						if ( upnp_device_is( device, linn_serviceId_Playlist ) )
						{
							success = Linn_Playlist_IdArray( device, NULL, &content, error );
						}
						else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
						{
							success = Openhome_Playlist_IdArray( device, NULL, &content, error );
						}
						else
						{
							g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
							success = FALSE;
						}

						if ( success )
						{
							upnp_message(( "handle_playlist_event(): Playlist_IdArray() = TRUE\n" ));

							tracks = decode_id_array( new_id_array, content );
							upnp_free_string( content );
						}
						else
						{
							upnp_warning(( "handle_playlist_event(): Playlist_IdArray() = FALSE\n" ));
						}

						break;
					}
				}
			}

			for (;;)
			{
				int x = handle_id_array( new_id_array, user_data, error );
				upnp_message(( "handle_id_array() = %d\n", x ));
				if ( x < 0 ) break;

				if ( !x )  /* an error occurred => abort */
				{
					success = FALSE;
					break;
				}
			}

			g_free( new_id_array );

			handle_track_id( FALSE, user_data );
		}
		else if ( strcmp( name, "Repeat" ) == 0 )
		{
			OnPlaylistRepeat( RepeatMode = atob( content ), user_data );
		}
		else if ( strcmp( name, "Shuffle" ) == 0 )
		{
			OnPlaylistShuffle( ShufflePlay = atob( content ), user_data );
		}

		/* Davaar stuff */
		else if ( strcmp( name, "Id" ) == 0 )
		{
			CurrentTrackId = strtoul( content, NULL, 10 );
			handle_track_id( TRUE, user_data );
		}
		else if ( strcmp( name, "TransportState" ) == 0 )
		{
			handle_transport_state( FALSE, content, user_data );
		}
	}

	return success;
}

int handle_id_array( upnp_ui4 *new_id_array, gpointer user_data, GError **error )
{
	int n = 0, i;

	for ( i = 0; i <= TracksMax; i++ )
	{
		upnp_ui4 id = new_id_array[i];
		if ( id != IdArray[i] )
		{
			int j;

			for ( j = i; j <= TracksMax; j++ )
			{
				if ( id == IdArray[j] )
				{
					handle_id_array_delete( i - n, n + j - i, id, user_data );
					return TRUE;
				}
			}

			id = IdArray[i];
			for ( j = i; j <= TracksMax; j++ )
			{
				if ( id == new_id_array[j] )
				{
					if ( n > 0 )
					{
						handle_id_array_delete( i - n, n, id, user_data );
						return TRUE;
					}

					return handle_id_array_insert( i, j - i, new_id_array + i, user_data, error );
				}
			}

			n++;
		}
		else if ( n > 0 )
		{
			handle_id_array_delete( i - n, n, id, user_data );
			return TRUE;
		}
	}

	upnp_assert( n == 0 );
	if ( n != 0 )
	{
		upnp_critical(( "handle_id_array(): IdArray[] seems to be corrupt!?\n" ));
		handle_playlist_clear( user_data );
		return TRUE;
	}

	return -1;  /* nothing to do */
}

gboolean handle_id_array_insert( int index, int count, upnp_ui4 *new_id_array, gpointer user_data, GError **error )
{
	const upnp_device *device = AVTransport;
	char *id_list = g_new( char, TracksMax * (10 /*strlen("4294967295")*/ + 1 /*strlen(",")*/) );
	char *meta_data_list, *s;
	gboolean success;
	int i;

	s = id_list; *s = '\0';

	if ( upnp_device_is( device, linn_serviceId_Playlist ) )
	{
		for ( i = 0; i < count; i++ )
		{
			if ( s != id_list ) *s++ = ',';
			s += sprintf( s, "%lu", (unsigned long)new_id_array[i] );
		}

		success = Linn_Playlist_ReadList( device, id_list, &meta_data_list, error );
	}
	else if ( upnp_device_is( device, openhome_serviceId_Playlist ) )
	{
		for ( i = 0; i < count; i++ )
		{
			if ( s != id_list ) *s++ = ' ';
			s += sprintf( s, "%lu", (unsigned long)new_id_array[i] );
		}

		success = Openhome_Playlist_ReadList( device, id_list, &meta_data_list, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, upnp_err_service_not_found );
		success = FALSE;
	}


	if ( success )
	{
		upnp_message(( "handle_id_array_insert(): Playlist_ReadList( %s ) = TRUE\n", id_list ));
		g_free( id_list );

		playlist_insert_tracks( index, count, new_id_array, meta_data_list );
		upnp_free_string( meta_data_list );

		OnPlaylistInsert( index, count, user_data );
	}
	else
	{
		upnp_warning(( "handle_id_array_insert(): Playlist_ReadList( %s ) = FALSE\n", id_list ));
		g_free( id_list );
	}

	return success;
}

void handle_id_array_delete( int index, int count, upnp_ui4 id, gpointer user_data )
{
	if ( index == 0 && id == 0 )
	{
		handle_playlist_clear( user_data );
	}
	else
	{
		playlist_delete_tracks( index, count );
		OnPlaylistDelete( index, count, user_data );
	}
}

#if 0
void handle_id_array_move( int index_from, int index_to, int count, gpointer user_data )
{
	;;; /* not used yet */
}
#endif

void handle_ds_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data )
{
	char *name, *content;
	xml_iter iter;

	if ( SEQ == 0 )
	{
		CurrentTrackIndex = -1;
		CurrentTrackId = 0;
		TransportState = TRANSPORT_STATE_UNKNOWN;
	}

	/* "<e:property><SupportedProtocols>http-get:*:audio/x-flac:*,http-get:*:audio/wav:*,http-get:*:audio/wave:*,http-get:*:audio/x-wav:*,http-get:*:audio/mpeg:*,http-get:*:audio/x-mpeg:*,http-get:*:audio/mp1:*,http-get:*:audio/aiff:*,http-get:*:audio/x-aiff:*,http-get:*:audio/x-m4a:*</SupportedProtocols></e:property><e:property><TrackDuration>237</TrackDuration></e:property><e:property><TrackBitRate>1411200</TrackBitRate></e:property><e:property><TrackLossless>true</TrackLossless></e:property><e:property><TrackBitDepth>16</TrackBitDepth></e:property><e:property><TrackSampleRate>44100</TrackSampleRate></e:property><e:property><TrackCodecName>FLAC</TrackCodecName></e:property><e:property><TrackId>24</TrackId></e:property><e:property><TransportState>Paused</TransportState></e:property>" */
	for ( content = xml_first_e_property( e_propertyset, &name, &iter ); content != NULL; content = xml_next_e_property( &name, &iter ) )
	{
		upnp_message(( "handle_ds_event(): %s = \"%s\"\n", name, content ));

		if ( strcmp( name, "TrackId" ) == 0 )
		{
			CurrentTrackId = strtoul( content, NULL, 10 );
			handle_track_id( TRUE, user_data );
		}
		else if ( strcmp( name, "TransportState" ) == 0 )
		{
			handle_transport_state( FALSE, content, user_data );
		}
	}
}

void handle_track_id( gboolean evented, gpointer user_data )
{
	int i;

	for ( i = 0; i < TracksMax; i++ )
	{
		if ( IdArray[i] == CurrentTrackId ) break;
	}
	if ( CurrentTrackId == 0 || i == TracksMax ) i = -1;

	if ( CurrentTrackIndex != i || evented )
	{
		handle_current_track_index( i, evented, user_data );
	}
}

void handle_preamp_event( upnp_ui4 SEQ, char *e_propertyset, gpointer user_data )
{
	char *name, *content;
	xml_iter iter;

	if ( SEQ == 0 )
	{
		gchar *value = xml_get_named_str( e_propertyset, "VolumeLimit" );
		if ( value != NULL )
		{
			VolumeLimit = atoi( value );
			upnp_message(( "VolumeLimit = %d\n", VolumeLimit ));
			g_free( value );
		}
		else
		{
			VolumeLimit = 100;
		}

		OnVolumeInit( user_data );
	}

	/* "<e:property><Volume>50</Volume></e:property><e:property><Mute>true</Mute></e:property><e:property><Balance>0</Balance></e:property><e:property><VolumeLimit>100</VolumeLimit></e:property><e:property><StartupVolume>50</StartupVolume></e:property><e:property><StartupVolumeEnabled>true</StartupVolumeEnabled></e:property><e:property><AnySourceVolumeOffset>0</AnySourceVolumeOffset></e:property><e:property><AnySourceUnityGain>0</AnySourceUnityGain></e:property><e:property><AnySpeakerEnabled>0</AnySpeakerEnabled></e:property><e:property><AnySpeakerVolumeOffset>0</AnySpeakerVolumeOffset></e:property><e:property><AnySpeakerDelay>0</AnySpeakerDelay></e:property><e:property><AnySpeakerPassive>0</AnySpeakerPassive></e:property><e:property><AnySpeakerDriveUnitEnabled>0</AnySpeakerDriveUnitEnabled></e:property><e:property><AnySpeakerDriveUnitVolumeOffset>0</AnySpeakerDriveUnitVolumeOffset></e:property><e:property><AnySpeakerDriveUnitDelay>0</AnySpeakerDriveUnitDelay></e:property>" */
	for ( content = xml_first_e_property( e_propertyset, &name, &iter ); content != NULL; content = xml_next_e_property( &name, &iter ) )
	{
		upnp_message(( "handle_preamp_event(): %s = \"%s\"\n", name, content ));

		if ( strcmp( name, "Volume" ) == 0 )
		{
			OnVolume( CurrentVolume = atoi( content ), user_data );
		}
		else if ( strcmp( name, "Mute" ) == 0 )
		{
			OnMute( CurrentMute = atob( content ), user_data );
		}
	}
}

/*-----------------------------------------------------------------------------------------------*/

char *MetaDataList_GetItem( char **MetaDataList )
{
	char *item = NULL;

	upnp_assert( MetaDataList != NULL );
	if ( MetaDataList == NULL || *MetaDataList == NULL ) return NULL;

	if ( strncmp( *MetaDataList, "<PlaylistData", 13 ) == 0 || strncmp( *MetaDataList, "<MetaDataList", 13 ) == 0 ||
	     strncmp( *MetaDataList, "<Track", 6 ) == 0 || strncmp( *MetaDataList, "<Entry", 6 ) == 0 )
	{
		char *meta_data = PlaylistData_GetTrack( MetaDataList, NULL, NULL );
		char *name = NULL;

		item = xml_unbox( &meta_data, &name, NULL );
		if ( name == NULL || strcmp( name, "DIDL-Lite" ) != 0 ) item = "";
	}
	else
	{
		if ( strncmp( *MetaDataList, "<DIDL-Lite", 10 ) == 0 )
		{
			char *content = xml_unbox( MetaDataList, NULL, NULL );
			*MetaDataList = content;
		}
		else if ( strncmp( *MetaDataList, "item", 4 ) == 0 )
		{
			*--(*MetaDataList) = '<';  /* prepend the missing '<' */
		}

		if ( strncmp( *MetaDataList, "<item", 5 ) == 0 )
		{
			struct xml_info info;
			if ( xml_get_info( *MetaDataList, &info ) )
			{
				item = *MetaDataList;
				if ( *info.next != '\0' ) *info.next++ = '\0';
				*MetaDataList = info.next;
			}
			else
			{
				upnp_warning(( "MetaDataList_GetItem(): invalid \"item\" format\n" ));
			}
		}
		else if ( **MetaDataList != '\0' )
		{
			upnp_warning(( "MetaDataList_GetItem(): unknown format\n" ));
		}
	}

	return item;
}

/*===============================================================================================*/

gboolean Preamp_SoftwareVersion( const upnp_device *device, char **software_version, GError **error )
{
	/* TODO: Needs to be adapted to Cara & Davaar */
	gboolean result;

	if ( upnp_device_is( device, linn_serviceId_KlimaxKontrolConfig ) )       /* "Auskerry" */
	{
		result = Linn_KlimaxKontrolConfig_SoftwareVersion( device, software_version, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_AkurateKontrolConfig ) ) /* "Auskerry" */
	{
		result = Linn_AkurateKontrolConfig_SoftwareVersion( device, software_version, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Proxy ) )                /* "Bute" */
	{
		result = Linn_Proxy_SoftwareVersion( device, software_version, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, "The given UPnP device is no pre-amp" );
		result = FALSE;
	}

	return result;
}

gboolean Preamp_HardwareVersion( const upnp_device *device, char **hardware_version, GError **error )
{
	/* TODO: Needs to be adapted to Cara & Davaar */
	gboolean result;

	if ( upnp_device_is( device, linn_serviceId_KlimaxKontrolConfig ) )       /* "Auskerry" */
	{
		result = Linn_KlimaxKontrolConfig_HardwareVersion( device, hardware_version, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_AkurateKontrolConfig ) ) /* "Auskerry" */
	{
		result = Linn_AkurateKontrolConfig_HardwareVersion( device, hardware_version, error );
	}
	else if ( upnp_device_is( device, linn_serviceId_Proxy ) )                /* "Bute" */
	{
		result = Linn_Proxy_HardwareVersion( device, hardware_version, error );
	}
	else
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, "The given UPnP device is no pre-amp" );
		result = FALSE;
	}

	return result;
}

/*===============================================================================================*/
