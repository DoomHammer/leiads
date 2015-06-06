/*
	media_renderer.h
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

#include "upnp_media_renderer.h"
#include "linn_source.h"
#include "linn_preamp.h"

/*-----------------------------------------------------------------------------------------------*/

enum TransportState
{
	TRANSPORT_STATE_UNKNOWN = 0,
	TRANSPORT_STATE_BUFFERING, TRANSPORT_STATE_EOP, TRANSPORT_STATE_NO_MEDIA_PRESENT,
	TRANSPORT_STATE_PAUSED, TRANSPORT_STATE_PLAYING, TRANSPORT_STATE_STOPPED,
	TRANSPORT_STATE_TRANSITIONING
};


/* AVTransport stuff */
extern enum TransportState TransportState;
extern int CurrentTrackIndex;
extern gboolean RepeatMode, ShufflePlay;

/* RenderingControl stuff */
extern gboolean CurrentMute;
extern int VolumeLimit, CurrentVolume;

/*-----------------------------------------------------------------------------------------------*/

gboolean upnp_is_av_transport( const upnp_device *device );
upnp_device *upnp_get_first_av_transport( void );
upnp_device *upnp_get_next_av_transport( const upnp_device *device );
int upnp_num_av_transport( void );

gboolean upnp_is_rendering_control( const upnp_device *device );
upnp_device *upnp_get_first_rendering_control( void );
upnp_device *upnp_get_next_rendering_control( const upnp_device *device );

upnp_device *upnp_get_corresponding_rendering_control( const upnp_device *av_transport );
const char *upnp_get_ds_compatibility_family( const upnp_device *av_transport );
gboolean upnp_device_is_external_preamp( const upnp_device *device );

char *xml_first_e_property( char *e_propertyset, char **name, xml_iter *iter );
char *xml_next_e_property( char **name, xml_iter *iter );

/*===============================================================================================*/

int decode64( unsigned char *dest, const char *src );

/*===============================================================================================*/

const upnp_device *GetAVTransport( void );
gboolean SetAVTransport( const upnp_device *device );

const upnp_device *GetRenderingControl( void );
gboolean SetRenderingControl( const upnp_device *device );

gboolean PrepareForConnection( GError **error );
gboolean ConnectionComplete( gboolean standby, GError **error );

/*-----------------------------------------------------------------------------------------------*/

gboolean Play( GError **error );
gboolean Pause( GError **error );
gboolean Stop( GError **error );
gboolean SeekTracksAbsolute( int Index, GError **error );
gboolean SeekTracksRelative( int Delta, GError **error );
gboolean GetTransportState( enum TransportState *TransportState, GError **error );
gboolean GetTransportStateString( char **TransportState, GError **error );

void StartPlaying( void );

/*-----------------------------------------------------------------------------------------------*/

int GetTrackCount( void );
gchar *GetTrackItem( int index );
gchar *GetCurrentTrackItem( void );

/*int GetPlaylistTrackCount( void );*/
gboolean PlaylistAdd( char *MetaDataList, gpointer user_data, GError **error );
gboolean PlaylistInsert( int Index, char *MetaDataList, gpointer user_data, GError **error );
gboolean PlaylistDelete( int Index, int Count, gpointer user_data, GError **error );
gboolean PlaylistMove( int IndexFrom, int IndexTo, int Count, gpointer user_data, GError **error );
gboolean PlaylistClear( gpointer user_data, GError **error );

gboolean GetRepeat( gboolean *repeat, GError **error );
gboolean SetRepeat( gboolean repeat, GError **error );

gboolean CanShuffle( void );
gboolean GetShuffle( gboolean *shuffle, GError **error );
gboolean SetShuffle( gboolean shuffle, GError **error );

/*-----------------------------------------------------------------------------------------------*/

gboolean GetVolume( int *volume, GError **error );
gboolean SetVolume( int volume, GError **error );
gboolean GetMute( gboolean *mute, GError **error );
gboolean SetMute( gboolean mute, GError **error );
gboolean GetVolumeLimit( gboolean *volume_limit, GError **error );

/*-----------------------------------------------------------------------------------------------*/

gboolean SubscribeMediaRenderer( gboolean (*callback)( upnp_subscription *subscription, char *e_propertyset ), void *ref_data, GError **error );
gboolean UnsubscribeMediaRenderer( GError **error );

gboolean EnableVolumeEvents( GError **error );
gboolean DisableVolumeEvents( GError **error );

gboolean HandleMediaRendererEvent( const upnp_device *device, const char *serviceTypeOrId, upnp_ui4 SEQ, char *e_propertyset, gpointer user_data, GError **error );

void OnPlaylistClear( gpointer user_data );
void OnPlaylistInsert( int index, int count, gpointer user_data );
void OnPlaylistDelete( int index, int count, gpointer user_data );
void OnPlaylistMove( int index_from, int index_to, int count, gpointer user_data );

void OnPlaylistRepeat( gboolean is_active, gpointer user_data );
void OnPlaylistShuffle( gboolean is_active, gpointer user_data );

void OnCurrentTrackIndex( int old_track_index, int new_track_index, gboolean evented, gpointer user_data );
void OnTransportState( enum TransportState old_state, enum TransportState new_state, gpointer user_data );

void OnVolumeInit( gpointer user_data );
void OnVolume( int volume, gpointer user_data );
void OnMute( gboolean mute, gpointer user_data );

/*-----------------------------------------------------------------------------------------------*/

char *MetaDataList_GetItem( char **MetaDataList );

/*===============================================================================================*/

gboolean Preamp_SoftwareVersion( const upnp_device *device, char **software_version, GError **error );
gboolean Preamp_HardwareVersion( const upnp_device *device, char **hardware_version, GError **error );

/*-----------------------------------------------------------------------------------------------*/
