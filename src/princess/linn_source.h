/*
	princess/linn_source.h
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

/*=== urn:linn-co-uk:device:Source:1 ============================================================*/

/*extern const char linn_device_Source_1[]*/ /*= "urn:linn-co-uk:device:Source:1"*/ /*;*/

/*=== urn:linn-co-uk:serviceId:Volkano ==========================================================*/

extern const char linn_serviceId_Volkano[] /*= "urn:linn-co-uk:serviceId:Volkano"*/;

/*--- urn:linn-co-uk:service:Volkano:1 ----------------------------------------------------------*/

gboolean Linn_Volkano_UglyName( const upnp_device *device, upnp_string *aUglyName, GError **error );
gboolean Linn_Volkano_MacAddress( const upnp_device *device, upnp_string *aMacAddress, GError **error );
gboolean Linn_Volkano_ProductId( const upnp_device *device, upnp_string *aProductNumber, GError **error );
gboolean Linn_Volkano_BoardId( const upnp_device *device, upnp_ui4 aIndex, upnp_string *aBoardNumber, GError **error );
gboolean Linn_Volkano_BoardType( const upnp_device *device, upnp_ui4 aIndex, upnp_string *aBoardNumber, GError **error );
gboolean Linn_Volkano_MaxBoards( const upnp_device *device, upnp_ui4 *aMaxBoards, GError **error );
gboolean Linn_Volkano_SoftwareVersion( const upnp_device *device, upnp_string *aSoftwareVersion, GError **error );

/*=== urn:linn-co-uk:serviceId:Product ==========================================================*/

extern const char linn_serviceId_Product[] /*= "urn:linn-co-uk:serviceId:Product"*/;

/*--- urn:linn-co-uk:service:Product:1 ----------------------------------------------------------*/

gboolean Linn_Product_Room( const upnp_device *device, upnp_string *aRoom, GError **error );
gboolean Linn_Product_SetRoom( const upnp_device *device, const char *aRoom, GError **error );
gboolean Linn_Product_Standby( const upnp_device *device, upnp_boolean *aStandby, GError **error );
gboolean Linn_Product_SetStandby( const upnp_device *device, upnp_boolean aStandby, GError **error );

/* ... */

/*=== urn:linn-co-uk:serviceId:Media (Auskerry & Bute) ==========================================*/

extern const char linn_serviceId_Media[] /*= "urn:linn-co-uk:serviceId:Media"*/;

extern const char Relativity_Absolute[] /*= "Absolute"*/;
extern const char Relativity_Relative[] /*= "Relative"*/;

/*--- urn:linn-co-uk:service:Media:1 ------------------------------------------------------------*/

gboolean Linn_Media_Play( const upnp_device *device, GError **error );
gboolean Linn_Media_Pause( const upnp_device *device, GError **error );
gboolean Linn_Media_Stop( const upnp_device *device, GError **error );
gboolean Linn_Media_SeekSeconds( const upnp_device *device, upnp_i4 aOffset, const char *aRelativity, GError **error );
gboolean Linn_Media_SeekTracks( const upnp_device *device, upnp_i4 aOffset, const char *aRelativity, GError **error );
gboolean Linn_Media_Status( const upnp_device *device, upnp_string *aTransportState,
	upnp_i4 *aCurrentTrackIndex, upnp_ui4 *aCurrentTrackSeconds, upnp_ui4 *aCurrentTrackDuration,
	upnp_ui4 *aCurrentTrackBitDepth, upnp_ui4 *aCurrentTrackSampleRate, GError **error );

gboolean Linn_Media_PlaylistInsert( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 aIndex, char *aPlaylistData, GError **error );
gboolean Linn_Media_PlaylistDelete( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 aIndex, upnp_ui4 aCount, GError **error );
/*gboolean Linn_Media_PlaylistMove( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 aIndexFrom, upnp_ui4 aIndexTo, upnp_ui4 aCount, GError **error );*/
gboolean Linn_Media_PlaylistRead( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 aIndex, upnp_ui4 aCount, upnp_string *aPlaylistData, GError **error );

gboolean Linn_Media_PlaylistSequence( const upnp_device *device, upnp_ui4 *aSeq, GError **error );
gboolean Linn_Media_PlaylistTrackCount( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 *aTrackCount, GError **error );

/* Bute 09/2008 only */
gboolean Linn_Media_SetNonSeekableBuffer( const upnp_device *device, upnp_ui4 aBytes, GError **error );
gboolean Linn_Media_NonSeekableBuffer( const upnp_device *device, upnp_ui4 *aBytes, GError **error );

/*-----------------------------------------------------------------------------------------------*/

extern const char beginPlaylistData[] /*= "<PlaylistData>"*/;
extern const char endPlaylistData[]  /*= "</PlaylistData>"*/;
#define strlen_beginPlaylistData 14
#define strlen_endPlaylistData   15

struct PlaylistData;
struct PlaylistData *PlaylistData_New( int Count, GError **error );
gboolean PlaylistData_Reset( struct PlaylistData *PlaylistData );
gboolean PlaylistData_AddTrack( struct PlaylistData *PlaylistData, const char *MetaData_Contents, GError **error );
char *PlaylistData_Finalize( struct PlaylistData *PlaylistData, GError **error );
void PlaylistData_Free( struct PlaylistData *PlaylistData );

char *PlaylistData_GetTrack( char **PlaylistData, char **Uri, upnp_ui4 *Id );

/*=== urn:linn-co-uk:serviceId:Playlist (Cara) ==================================================*/

extern const char linn_serviceId_Playlist[] /*= "urn:linn-co-uk:serviceId:Playlist"*/;
extern const char TransportState_Buffering[] /*= "Buffering"*/;

/*--- urn:linn-co-uk:service:Playlist:1 ---------------------------------------------------------*/

gboolean Linn_Playlist_Read( const upnp_device *device, upnp_ui4 aId, upnp_string *aUri, upnp_string *aMetaData, GError **error );
gboolean Linn_Playlist_ReadList( const upnp_device *device, const char *aIdList, upnp_string *aMetaDataList, GError **error );
gboolean Linn_Playlist_Insert( const upnp_device *device, upnp_ui4 aAfterId, const char *aUri, const char *aMetaData, upnp_ui4 *aNewId, GError **error );
gboolean Linn_Playlist_Delete( const upnp_device *device, upnp_ui4 aId, GError **error );
gboolean Linn_Playlist_DeleteAll( const upnp_device *device, GError **error );

gboolean Linn_Playlist_SetRepeat( const upnp_device *device, upnp_boolean aRepeat, GError **error );
gboolean Linn_Playlist_Repeat( const upnp_device *device, upnp_boolean *aRepeat, GError **error );
gboolean Linn_Playlist_SetShuffle( const upnp_device *device, upnp_boolean aShuffle, GError **error );
gboolean Linn_Playlist_Shuffle( const upnp_device *device, upnp_boolean *aShuffle, GError **error );

gboolean Linn_Playlist_TracksMax( const upnp_device *device, upnp_ui4 *aTracksMax, GError **error );
gboolean Linn_Playlist_IdArray( const upnp_device *device, upnp_ui4 *aIdArrayToken, upnp_string *aIdArray, GError **error );
gboolean Linn_Playlist_IdArrayChanged( const upnp_device *device, upnp_ui4 aIdArrayToken, upnp_boolean *aIdArrayChanged, GError **error );

/*=== urn:linn-co-uk:serviceId:Ds (Cara) ========================================================*/

extern const char linn_serviceId_Ds[] /*= "urn:linn-co-uk:serviceId:Ds"*/;

/*--- urn:linn-co-uk:service:Ds:1 ---------------------------------------------------------------*/

gboolean Linn_Ds_Play( const upnp_device *device, GError **error );
gboolean Linn_Ds_Pause( const upnp_device *device, GError **error );
gboolean Linn_Ds_Stop( const upnp_device *device, GError **error );
gboolean Linn_Ds_SeekSecondAbsolute( const upnp_device *device, upnp_ui4 aSecond, GError **error );
gboolean Linn_Ds_SeekSecondRelative( const upnp_device *device, upnp_i4 aSecond, GError **error );
gboolean Linn_Ds_SeekTrackId( const upnp_device *device, upnp_ui4 aTrackId, GError **error );
gboolean Linn_Ds_SeekTrackAbsolute( const upnp_device *device, upnp_ui4 aTrack, GError **error );
gboolean Linn_Ds_SeekTrackRelative( const upnp_device *device, upnp_i4 aTrack, GError **error );
gboolean Linn_Ds_State( const upnp_device *device, upnp_string *aTransportState,
	upnp_ui4 *aTrackDuration, upnp_ui4 *aTrackBitRate, upnp_boolean *aTrackLossless,
	upnp_ui4 *aTrackBitDepth, upnp_ui4 *aTrackSampleRate, upnp_string *aTrackCodecName,
	upnp_ui4 *aTrackId, GError **error );

gboolean Linn_Ds_ProtocolInfo( const upnp_device *device, upnp_string *aSupportedProtocols, GError **error );

/*=== urn:linn-co-uk:serviceId:Radio (Cara 6) ===================================================*/

extern const char linn_serviceId_Radio[] /*= "urn:linn-co-uk:serviceId:Radio"*/;

/*--- urn:linn-co-uk:service:Radio:1 ------------------------------------------------------------*/

gboolean Linn_Radio_Play( const upnp_device *device, GError **error );
gboolean Linn_Radio_Pause( const upnp_device *device, GError **error );
gboolean Linn_Radio_Stop( const upnp_device *device, GError **error );
gboolean Linn_Radio_SeekSecondAbsolute( const upnp_device *device, upnp_ui4 aSecond, GError **error );
gboolean Linn_Radio_SeekSecondRelative( const upnp_device *device, upnp_i4 aSecond, GError **error );
gboolean Linn_Radio_Channel( const upnp_device *device, upnp_string *aUri, upnp_string *aMetadata, GError **error );
gboolean Linn_Radio_SetChannel( const upnp_device *device, const char *aUri, const char *aMetadata, GError **error );
gboolean Linn_Radio_ProtocolInfo( const upnp_device *device, upnp_string *aInfo, GError **error );
gboolean Linn_Radio_TransportState( const upnp_device *device, upnp_string *aState, GError **error );
gboolean Linn_Radio_Id( const upnp_device *device, upnp_ui4 *aId, GError **error );
gboolean Linn_Radio_SetId( const upnp_device *device, upnp_ui4 aId, const char *aUri, GError **error );
gboolean Linn_Radio_Read( const upnp_device *device, upnp_ui4 aId, upnp_string *aMetadata, GError **error );
gboolean Linn_Radio_ReadList( const upnp_device *device, const char *aIdList, upnp_string *aMetadataList, GError **error );
gboolean Linn_Radio_IdArray( const upnp_device *device, upnp_ui4 *aIdArrayToken, upnp_string *aIdArray, GError **error );
gboolean Linn_Radio_IdArrayChanged( const upnp_device *device, upnp_ui4 aIdArrayToken, upnp_boolean *aIdArrayChanged, GError **error );
gboolean Linn_Radio_IdsMax( const upnp_device *device, upnp_ui4 *aIdsMax, GError **error );

/*=== urn:av-openhome-org:serviceId:Product (Davaar) ============================================*/

extern const char openhome_serviceId_Product[] /*= "urn:av-openhome-org:serviceId:Product"*/;

/*--- urn:av-openhome-org:service:Product:1 -----------------------------------------------------*/

gboolean Openhome_Manufacturer( const upnp_device *device, upnp_string *Name, upnp_string *Info, upnp_string *Url, upnp_string *ImageUri, GError **error );
gboolean Openhome_Model( const upnp_device *device, upnp_string *Name, upnp_string *Info, upnp_string *Url, upnp_string *ImageUri, GError **error );
gboolean Openhome_Product( const upnp_device *device, upnp_string *Room, upnp_string *Name, upnp_string *Info, upnp_string *Url, upnp_string *ImageUri, GError **error );
gboolean Openhome_Standby( const upnp_device *device, upnp_boolean *Value, GError **error );
gboolean Openhome_SetStandby( const upnp_device *device, upnp_boolean Value, GError **error );

/* ... */

/*=== urn:av-openhome-org:serviceId:Playlist (Davaar) ===========================================*/

extern const char openhome_serviceId_Playlist[] /*= "urn:av-openhome-org:serviceId:Playlist"*/;

/*--- urn:av-openhome-org:service:Playlist:1 ----------------------------------------------------*/

gboolean Openhome_Playlist_Play( const upnp_device *device, GError **error );
gboolean Openhome_Playlist_Pause( const upnp_device *device, GError **error );
gboolean Openhome_Playlist_Stop( const upnp_device *device, GError **error );
gboolean Openhome_Playlist_Next( const upnp_device *device, GError **error );
gboolean Openhome_Playlist_Previous( const upnp_device *device, GError **error );

gboolean Openhome_Playlist_SetRepeat( const upnp_device *device, upnp_boolean Value, GError **error );
gboolean Openhome_Playlist_Repeat( const upnp_device *device, upnp_boolean *Value, GError **error );
gboolean Openhome_Playlist_SetShuffle( const upnp_device *device, upnp_boolean Value, GError **error );
gboolean Openhome_Playlist_Shuffle( const upnp_device *device, upnp_boolean *Value, GError **error );

gboolean Openhome_Playlist_SeekSecondAbsolute( const upnp_device *device, upnp_ui4 Value, GError **error );
gboolean Openhome_Playlist_SeekSecondRelative( const upnp_device *device, upnp_i4 Value, GError **error );
gboolean Openhome_Playlist_SeekId( const upnp_device *device, upnp_ui4 Value, GError **error );
gboolean Openhome_Playlist_SeekIndex( const upnp_device *device, upnp_ui4 Value, GError **error );

gboolean Openhome_Playlist_TransportState( const upnp_device *device, upnp_string *Value, GError **error );

gboolean Openhome_Playlist_Id( const upnp_device *device, upnp_ui4 *Value, GError **error );
gboolean Openhome_Playlist_Read( const upnp_device *device, upnp_ui4 Id, upnp_string *Uri, upnp_string *Metadata, GError **error );
gboolean Openhome_Playlist_ReadList( const upnp_device *device, upnp_string IdList, upnp_string *TrackList, GError **error );
gboolean Openhome_Playlist_Insert( const upnp_device *device, upnp_ui4 AfterId, upnp_string Uri, upnp_string Metadata, upnp_ui4 *NewId, GError **error );
gboolean Openhome_Playlist_DeleteId( const upnp_device *device, upnp_ui4 Value, GError **error );
gboolean Openhome_Playlist_DeleteAll( const upnp_device *device, GError **error );
gboolean Openhome_Playlist_TracksMax( const upnp_device *device, upnp_ui4 *Value, GError **error );
gboolean Openhome_Playlist_IdArray( const upnp_device *device, upnp_ui4 *Token, upnp_string *Array, GError **error );
gboolean Openhome_Playlist_IdArrayChanged( const upnp_device *device, upnp_ui4 Token, upnp_boolean *Value, GError **error );

gboolean Openhome_Playlist_ProtocolInfo( const upnp_device *device, upnp_string *Value, GError **error );

/*=== urn:av-openhome-org:serviceId:Radio (Davaar) ===================================================*/

extern const char openhome_serviceId_Radio[] /*= "urn:av-openhome-org:serviceId:Radio"*/;

/*--- urn:av-openhome-org:service:Radio:1 ------------------------------------------------------------*/

gboolean Openhome_Radio_Play( const upnp_device *device, GError **error );
gboolean Openhome_Radio_Pause( const upnp_device *device, GError **error );
gboolean Openhome_Radio_Stop( const upnp_device *device, GError **error );
gboolean Openhome_Radio_SeekSecondAbsolute( const upnp_device *device, upnp_ui4 Value, GError **error );
gboolean Openhome_Radio_SeekSecondRelative( const upnp_device *device, upnp_i4 Value, GError **error );
gboolean Openhome_Radio_Channel( const upnp_device *device, upnp_string *Uri, upnp_string *Metadata, GError **error );
gboolean Openhome_Radio_SetChannel( const upnp_device *device, const char *Uri, const char *Metadata, GError **error );
gboolean Openhome_Radio_ProtocolInfo( const upnp_device *device, upnp_string *aInfo, GError **error );
gboolean Openhome_Radio_TransportState( const upnp_device *device, upnp_string *Value, GError **error );
gboolean Openhome_Radio_Id( const upnp_device *device, upnp_ui4 *Value, GError **error );
gboolean Openhome_Radio_SetId( const upnp_device *device, upnp_ui4 Value, const char *Uri, GError **error );
gboolean Openhome_Radio_Read( const upnp_device *device, upnp_ui4 Id, upnp_string *Metadata, GError **error );
gboolean Openhome_Radio_ReadList( const upnp_device *device, const char *IdList, upnp_string *ChannelList, GError **error );
gboolean Openhome_Radio_IdArray( const upnp_device *device, upnp_ui4 *Token, upnp_string *Array, GError **error );
gboolean Openhome_Radio_IdArrayChanged( const upnp_device *device, upnp_ui4 Token, upnp_boolean *Value, GError **error );
gboolean Openhome_Radio_ChannelsMax( const upnp_device *device, upnp_ui4 *Value, GError **error );
gboolean Openhome_Radio_ProtocolInfo( const upnp_device *device, upnp_string *Value, GError **error );

/*===============================================================================================*/
