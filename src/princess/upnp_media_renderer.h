/*
	princess/upnp_media_renderer.h
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

extern const char upnp_device_MediaRenderer_1[] /*= "urn:schemas-upnp-org:device:MediaRenderer:1"*/;

/*--- urn:schemas-upnp-org:service:AVTransport:1 ------------------------------------------------*/

extern const char upnp_serviceId_AVTransport[] /*= "urn:upnp-org:serviceId:AVTransport"*/;

gboolean AVTransport_SetAVTransportURI( const upnp_device *device, upnp_ui4 InstanceID, const char *CurrentURI, const char *CurrentURIMetaData, GError **error );
gboolean AVTransport_SetNextAVTransportURI( const upnp_device *device, upnp_ui4 InstanceID, const char *NextURI, const char *NextURIMetaData, GError **error );
gboolean AVTransport_GetMediaInfo( const upnp_device *device, upnp_ui4 InstanceID, upnp_ui4 *NrTracks, upnp_string *MediaDuration, upnp_string *CurrentURI, upnp_string *CurrentURIMetaData, upnp_string *NextURI, upnp_string *NextURIMetaData, upnp_string *PlayMedium, upnp_string *RecordMedium, upnp_string *WriteStatus, GError **error );
gboolean AVTransport_GetTransportInfo( const upnp_device *device, upnp_ui4 InstanceID, char **CurrentTransportState, upnp_string *CurrentTransportStatus, upnp_string *CurrentSpeed, GError **error );
gboolean AVTransport_GetPositionInfo( const upnp_device *device, upnp_ui4 InstanceID, upnp_ui4 *Track, upnp_string *TrackDuration, upnp_string *TrackMetaData, upnp_string *TrackURI, upnp_string *RelTime, upnp_string *AbsTime, upnp_i4 *RelCount, upnp_i4 *AbsCount, GError **error );
gboolean AVTransport_GetDeviceCapabilities( const upnp_device *device, upnp_ui4 InstanceID, upnp_string *PlayMedia, upnp_string *RecMedia, upnp_string *RecQualityModes, GError **error );
gboolean AVTransport_GetTransportSettings( const upnp_device *device, upnp_ui4 InstanceID, upnp_string *PlayMode, upnp_string *RecQualityMode, GError **error );
gboolean AVTransport_Stop( const upnp_device *device, upnp_ui4 InstanceID, GError **error );
gboolean AVTransport_Play( const upnp_device *device, upnp_ui4 InstanceID, const char *Speed, GError **error );
gboolean AVTransport_Pause( const upnp_device *device, upnp_ui4 InstanceID, GError **error );

/*--- urn:schemas-upnp-org:service:RenderingControl:1 -------------------------------------------*/

extern const char upnp_serviceId_RenderingControl[] /*= "urn:upnp-org:serviceId:RenderingControl"*/;

extern const char Master[] /*= "Master"*/;

gboolean RenderingControl_GetMute( const upnp_device *device, upnp_ui4 InstanceID, const char *Channel, upnp_boolean *CurrentMute, GError **error );
gboolean RenderingControl_SetMute( const upnp_device *device, upnp_ui4 InstanceID, const char *Channel, upnp_boolean DesiredMute, GError **error );
gboolean RenderingControl_GetVolume( const upnp_device *device, upnp_ui4 InstanceID, const char *Channel, upnp_ui2 *CurrentVolume, GError **error );
gboolean RenderingControl_SetVolume( const upnp_device *device, upnp_ui4 InstanceID, const char *Channel, upnp_ui2 DesiredVolume, GError **error );

/*--- urn:schemas-upnp-org:service:ConnectionManager:1 ------------------------------------------*/

extern const char upnp_serviceId_ConnectionManager[] /*= "urn:upnp-org:serviceId:ConnectionManager"*/;

gboolean ConnectionManager_GetProtocolInfo( const upnp_device *device, upnp_string *Source, upnp_string *Sink, GError **error );
gboolean ConnectionManager_PrepareForConnection( const upnp_device *device, const char *RemoteProtocolInfo, const char *PeerConnectionManager, upnp_i4 PeerConnectionID, const char *Direction, upnp_i4 *ConnectionID, upnp_i4 *AVTransportID, upnp_i4 *RcsID, GError **error );
gboolean ConnectionManager_ConnectionComplete( const upnp_device *device, upnp_i4 ConnectionID, GError **error );

/*===============================================================================================*/
