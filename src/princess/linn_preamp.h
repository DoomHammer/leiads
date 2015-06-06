/*
	princess/linn_preamp.h
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

/*extern const char linn_device_Preamp_1[]*/ /*= "urn:linn-co-uk:device:Preamp:1"*/ /*;*/

/*=== urn:linn-co-uk:serviceId:Preamp ===========================================================*/

extern const char linn_serviceId_Preamp[] /*= "urn:linn-co-uk:serviceId:Preamp"*/;

/*--- urn:linn-co-uk:service:Preamp:1 -----------------------------------------------------------*/

gboolean Linn_Preamp_SetVolume( const upnp_device *device, upnp_ui4 aVolume, GError **error );
gboolean Linn_Preamp_Volume( const upnp_device *device, upnp_ui4 *aVolume, GError **error );

gboolean Linn_Preamp_SetMute( const upnp_device *device, upnp_boolean aMute, GError **error );
gboolean Linn_Preamp_Mute( const upnp_device *device, upnp_boolean *aMute, GError **error );

gboolean Linn_Preamp_SetBalance( const upnp_device *device, upnp_i4 aBalance, GError **error );
gboolean Linn_Preamp_Balance( const upnp_device *device, upnp_i4 *aBalance, GError **error );

gboolean Linn_Preamp_SetVolumeLimit( const upnp_device *device, upnp_ui4 aVolumeLimit, GError **error );
gboolean Linn_Preamp_VolumeLimit( const upnp_device *device, upnp_ui4 *aVolumeLimit, GError **error );

/*=== urn:linn-co-uk:serviceId:KlimaxKontrolConfig (Auskerry) ===================================*/

extern const char linn_serviceId_KlimaxKontrolConfig[] /*= "urn:linn-co-uk:serviceId:KlimaxKontrolConfig"*/;

/*--- urn:linn-co-uk:service:KlimaxKontrolConfig:1 ----------------------------------------------*/

gboolean Linn_KlimaxKontrolConfig_SoftwareVersion( const upnp_device *device, upnp_string *aSoftwareVersion, GError **error );
gboolean Linn_KlimaxKontrolConfig_HardwareVersion( const upnp_device *device, upnp_string *aHardwareVersion, GError **error );

/*=== urn:linn-co-uk:serviceId:AkurateKontrolConfig (Auskerry) ==================================*/

extern const char linn_serviceId_AkurateKontrolConfig[] /*= "urn:linn-co-uk:serviceId:AkurateKontrolConfig"*/;

/*--- urn:linn-co-uk:service:AkurateKontrolConfig:1 ---------------------------------------------*/

gboolean Linn_AkurateKontrolConfig_SoftwareVersion( const upnp_device *device, upnp_string *aSoftwareVersion, GError **error );
gboolean Linn_AkurateKontrolConfig_HardwareVersion( const upnp_device *device, upnp_string *aHardwareVersion, GError **error );

/*=== urn:linn-co-uk:serviceId:Proxy (Bute) =====================================================*/

extern const char linn_serviceId_Proxy[] /*= "urn:linn-co-uk:serviceId:Proxy"*/;

/*--- urn:linn-co-uk:service:Proxy:1 ------------------------------------------------------------*/

gboolean Linn_Proxy_SoftwareVersion( const upnp_device *device, upnp_string *aSoftwareVersion, GError **error );
gboolean Linn_Proxy_HardwareVersion( const upnp_device *device, upnp_string *aHardwareVersion, GError **error );

/*=== urn:av-openhome-org:serviceId:Volume (Davaar) =============================================*/

extern const char openhome_serviceId_Volume[] /*= "urn:av-openhome-org:serviceId:Volume"*/;

/*--- urn:av-openhome-org:service:Volume:1 ------------------------------------------------------*/

gboolean Openhome_Volume_Characteristics( const upnp_device *device, upnp_ui4 *VolumeMax, upnp_ui4 *VolumeUnity, upnp_ui4 *VolumeSteps, upnp_ui4 *VolumeMilliDbPerStep, upnp_ui4 *BalanceMax, upnp_ui4 *FadeMax, GError **error );

gboolean Openhome_Volume_SetVolume( const upnp_device *device, upnp_ui4 Value, GError **error );
gboolean Openhome_Volume_VolumeInc( const upnp_device *device, GError **error );
gboolean Openhome_Volume_VolumeDec( const upnp_device *device, GError **error );
gboolean Openhome_Volume_Volume( const upnp_device *device, upnp_ui4 *Value, GError **error );

gboolean Openhome_Volume_SetBalance( const upnp_device *device, upnp_i4 Value, GError **error );
gboolean Openhome_Volume_BalanceInc( const upnp_device *device, GError **error );
gboolean Openhome_Volume_BalanceDec( const upnp_device *device, GError **error );
gboolean Openhome_Volume_Balance( const upnp_device *device, upnp_i4 *Value, GError **error );

gboolean Openhome_Volume_SetFade( const upnp_device *device, upnp_i4 Value, GError **error );
gboolean Openhome_Volume_FadeInc( const upnp_device *device, GError **error );
gboolean Openhome_Volume_FadeDec( const upnp_device *device, GError **error );
gboolean Openhome_Volume_Fade( const upnp_device *device, upnp_i4 *Value, GError **error );

gboolean Openhome_Volume_SetMute( const upnp_device *device, upnp_boolean Value, GError **error );
gboolean Openhome_Volume_Mute( const upnp_device *device, upnp_boolean *Value, GError **error );

gboolean Openhome_Volume_VolumeLimit( const upnp_device *device, upnp_ui4 *Value, GError **error );

/*===============================================================================================*/
