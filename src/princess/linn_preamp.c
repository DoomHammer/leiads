/*
	princess/linn_preamp.c
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
#include "linn_preamp.h"

/*const char linn_device_Preamp_1[] = "urn:linn-co-uk:device:Preamp:1";*/

/*=== urn:linn-co-uk:serviceId:Preamp ===========================================================*/

const char linn_serviceId_Preamp[] = "urn:linn-co-uk:serviceId:Preamp";

/*--- urn:linn-co-uk:service:Preamp:1 -----------------------------------------------------------*/

gboolean Linn_Preamp_SetVolume( const upnp_device *device, upnp_ui4 aVolume, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetVolume" );
	upnp_add_ui4_arg( in, "aVolume", aVolume );

	return upnp_action( device, linn_serviceId_Preamp, in, NULL, NULL, error );
}

gboolean Linn_Preamp_Volume( const upnp_device *device, upnp_ui4 *aVolume, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Volume" );

	if ( upnp_action( device, linn_serviceId_Preamp, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aVolume != NULL );
		*aVolume = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Preamp_SetMute( const upnp_device *device, upnp_boolean aMute, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetMute" );
	upnp_add_boolean_arg( in, "aMute", aMute );

	return upnp_action( device, linn_serviceId_Preamp, in, NULL, NULL, error );
}

gboolean Linn_Preamp_Mute( const upnp_device *device, upnp_boolean *aMute, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Mute" );

	if ( upnp_action( device, linn_serviceId_Preamp, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aMute != NULL );
		*aMute = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Preamp_SetBalance( const upnp_device *device, upnp_i4 aBalance, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetBalance" );
	upnp_add_i4_arg( in, "aBalance", aBalance );

	return upnp_action( device, linn_serviceId_Preamp, in, NULL, NULL, error );
}

gboolean Linn_Preamp_Balance( const upnp_device *device, upnp_i4 *aBalance, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Balance" );

	if ( upnp_action( device, linn_serviceId_Preamp, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aBalance != NULL );
		*aBalance = upnp_get_i4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Preamp_SetVolumeLimit( const upnp_device *device, upnp_ui4 aVolumeLimit, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetVolumeLimit" );
	upnp_add_ui4_arg( in, "aVolumeLimit", aVolumeLimit );

	return upnp_action( device, linn_serviceId_Preamp, in, NULL, NULL, error );
}

gboolean Linn_Preamp_VolumeLimit( const upnp_device *device, upnp_ui4 *aVolumeLimit, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "VolumeLimit" );

	if ( upnp_action( device, linn_serviceId_Preamp, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aVolumeLimit != NULL );
		*aVolumeLimit = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*=== urn:linn-co-uk:serviceId:KlimaxKontrolConfig (Auskerry) ===================================*/

const char linn_serviceId_KlimaxKontrolConfig[] = "urn:linn-co-uk:serviceId:KlimaxKontrolConfig";

/*--- urn:linn-co-uk:service:KlimaxKontrolConfig:1 ----------------------------------------------*/

static gboolean Linn_Version( const upnp_device *device, const char *serviceTypeOrId, const char *actionName, upnp_string *aVersion, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, actionName );

	if ( upnp_action( device, serviceTypeOrId, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aVersion != NULL );
		*aVersion = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_KlimaxKontrolConfig_SoftwareVersion( const upnp_device *device, upnp_string *aSoftwareVersion, GError **error )
{
	return Linn_Version( device, linn_serviceId_KlimaxKontrolConfig, "SoftwareVersion", aSoftwareVersion, error );
}

gboolean Linn_KlimaxKontrolConfig_HardwareVersion( const upnp_device *device, upnp_string *aHardwareVersion, GError **error )
{
	return Linn_Version( device, linn_serviceId_KlimaxKontrolConfig, "HardwareVersion", aHardwareVersion, error );
}

/*=== urn:linn-co-uk:serviceId:AkurateKontrolConfig (Auskerry) ==================================*/

const char linn_serviceId_AkurateKontrolConfig[] = "urn:linn-co-uk:serviceId:AkurateKontrolConfig";

/*--- urn:linn-co-uk:service:AkurateKontrolConfig:1 ---------------------------------------------*/

gboolean Linn_AkurateKontrolConfig_SoftwareVersion( const upnp_device *device, upnp_string *aSoftwareVersion, GError **error )
{
	return Linn_Version( device, linn_serviceId_AkurateKontrolConfig, "SoftwareVersion", aSoftwareVersion, error );
}

gboolean Linn_AkurateKontrolConfig_HardwareVersion( const upnp_device *device, upnp_string *aHardwareVersion, GError **error )
{
	return Linn_Version( device, linn_serviceId_AkurateKontrolConfig, "HardwareVersion", aHardwareVersion, error );
}

/*=== urn:linn-co-uk:serviceId:Proxy (Bute) =====================================================*/

const char linn_serviceId_Proxy[] = "urn:linn-co-uk:serviceId:Proxy";

/*--- urn:linn-co-uk:service:Proxy:1 ------------------------------------------------------------*/

gboolean Linn_Proxy_SoftwareVersion( const upnp_device *device, upnp_string *aSoftwareVersion, GError **error )
{
	return Linn_Version( device, linn_serviceId_Proxy, "SoftwareVersion", aSoftwareVersion, error );
}

gboolean Linn_Proxy_HardwareVersion( const upnp_device *device, upnp_string *aHardwareVersion, GError **error )
{
	return Linn_Version( device, linn_serviceId_Proxy, "HardwareVersion", aHardwareVersion, error );
}

/*=== urn:av-openhome-org:serviceId:Volume (Davaar) =============================================*/

const char openhome_serviceId_Volume[] = "urn:av-openhome-org:serviceId:Volume";

/*--- urn:av-openhome-org:service:Volume:1 ------------------------------------------------------*/

gboolean Openhome_Volume_Characteristics( const upnp_device *device, upnp_ui4 *VolumeMax, upnp_ui4 *VolumeUnity, upnp_ui4 *VolumeSteps, upnp_ui4 *VolumeMilliDbPerStep, upnp_ui4 *BalanceMax, upnp_ui4 *FadeMax, GError **error )
{
	upnp_arg in[1], out[7];
	int outc = 7;

	upnp_set_action_name( in, "Characteristics" );

	if ( upnp_action( device, openhome_serviceId_Volume, in, out, &outc, error ) )
	{
		upnp_assert( outc == 7 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));
		upnp_debug(( "%s = \"%s\"\n", out[4].name, out[4].value ));
		upnp_debug(( "%s = \"%s\"\n", out[5].name, out[5].value ));
		upnp_debug(( "%s = \"%s\"\n", out[6].name, out[6].value ));

		if ( VolumeMax != NULL ) *VolumeMax = upnp_get_ui4_value( out, 1 );
		if ( VolumeUnity != NULL ) *VolumeUnity = upnp_get_ui4_value( out, 2 );
		if ( VolumeSteps != NULL ) *VolumeSteps = upnp_get_ui4_value( out, 3 );
		if ( VolumeMilliDbPerStep != NULL ) *VolumeMilliDbPerStep = upnp_get_ui4_value( out, 4 );
		if ( BalanceMax != NULL ) *BalanceMax = upnp_get_ui4_value( out, 5 );
		if ( FadeMax != NULL ) *FadeMax = upnp_get_ui4_value( out, 6 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Volume_SetVolume( const upnp_device *device, upnp_ui4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetVolume" );
	upnp_add_ui4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_VolumeInc( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "VolumeInc" );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_VolumeDec( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "VolumeDec" );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_Volume( const upnp_device *device, upnp_ui4 *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Volume" );

	if ( upnp_action( device, openhome_serviceId_Volume, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Volume_SetBalance( const upnp_device *device, upnp_i4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetBalance" );
	upnp_add_i4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_BalanceInc( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "BalanceInc" );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_BalanceDec( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "BalanceDec" );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_Balance( const upnp_device *device, upnp_i4 *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Balance" );

	if ( upnp_action( device, openhome_serviceId_Volume, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_i4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Volume_SetFade( const upnp_device *device, upnp_i4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetFade" );
	upnp_add_i4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_FadeInc( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "FadeInc" );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_FadeDec( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "FadeDec" );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_Fade( const upnp_device *device, upnp_i4 *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Fade" );

	if ( upnp_action( device, openhome_serviceId_Volume, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_i4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Volume_SetMute( const upnp_device *device, upnp_boolean Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetMute" );
	upnp_add_boolean_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Volume, in, NULL, NULL, error );
}

gboolean Openhome_Volume_Mute( const upnp_device *device, upnp_boolean *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Mute" );

	if ( upnp_action( device, openhome_serviceId_Volume, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Volume_VolumeLimit( const upnp_device *device, upnp_ui4 *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "VolumeLimit" );

	if ( upnp_action( device, openhome_serviceId_Volume, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*===============================================================================================*/
