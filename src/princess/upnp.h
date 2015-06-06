/*
	princess/upnp.h
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

	--------------------------------------------------------------------
*/

#include <stdlib.h>  /* for size_t */
#include <glib.h>

typedef int upnp_boolean;
typedef signed short upnp_i2;
typedef unsigned short upnp_ui2;
typedef signed int upnp_i4;
typedef unsigned int upnp_ui4;
typedef char *upnp_string;

/* Start resp. shutdown the UPnP support */
gboolean upnp_startup( GError **error );
void upnp_cleanup( gboolean connected );

/*--- The UPnP device structure ------------------------------------------------------------------*/

typedef struct _upnp_device
{
	struct _upnp_device *next;     /* pointer to the next UPnP device in list */
	struct _upnp_device *parent;   /* pointer to the parent UPnP device or NULL if none */

	const char *URLBase;           /* e.g. "http://192.168.178.21:55178/" */

	const char *deviceType;        /* e.g. "urn:linn-co-uk:device:Source:1" */
	const char *friendlyName;      /* e.g. "Main Room:Sneaky Music DS" */
	const char *manufacturer;      /* e.g. "Linn Products Ltd" */
	const char *manufacturerURL;   /* e.g. "http://www.linn.co.uk" */
	const char *modelDescription;  /* e.g. "Linn High Fidelity System Component" */
	const char *modelName;         /* e.g. "Sneaky Music DS" */
	const char *modelNumber;       /* e.g. "Unknown" */
	const char *modelURL;          /* e.g. "http://www.linn.co.uk" */
	const char *presentationURL;   /* e.g. "http://192.168.178.21/Ds/index.html" */
	const char *serialNumber;      /* e.g. "100000032d3eb114" */
	const char *UDN;               /* e.g. "uuid:4c494e4e-0050-c221-75a9-10000003013f" */
	const char *UPC;               /* e.g. "Unknown" */

	struct _upnp_service *serviceList;
	struct _upnp_device *deviceList;

	void *user_data;               /* for own use */
	void (*free_user_data)( void *user_data );

	const char *USN;               /* USN, e.g. "uuid:7076436f-6e65-1063-8074-00104b4584f4::urn:schemas-upnp-org:device:MediaServer:1" */
	const char *LOCATION;          /* location URL, e.g. "http://192.168.0.10:55178/Ds/device.xml" */

	struct _GTimer *_timer;        /* internal: timeout for retrieval of description */
	gboolean _got_description;     /* internal: flag for retrieval of description */
	char *_buffer;                 /* internal: pointer to string buffer */
} upnp_device;

typedef struct _upnp_service
{
	struct _upnp_service *next;    /* pointer to the next UPnP service in list */
	struct _upnp_device *device;   /* pointer to the corresponding UPnP device */

	const char *serviceType;       /* e.g. "urn:linn-co-uk:service:Media:1" or "urn:linn-co-uk:service:Media:2" */
	const char *serviceId;         /* e.g. "urn:linn-co-uk:serviceId:Media" */
	const char *SCPDURL;           /* e.g. "dev02/Media/scpd.xml" or "/Ds/Media/service.xml" */
	const char *controlURL;        /* e.g. "dev02/Media/control" or "/Ds/Media/control" */
	const char *eventSubURL;       /* e.g. "dev02/Media/event" or "/Ds/Media/event" */

	/*void *user_data;*/           /* for own use */
} upnp_service;

gboolean upnp_device_is_valid( const upnp_device *device );
void upnp_device_mark_invalid( upnp_device *device );

/* Test if a device offers the given interface (device or service type/id),
   returns TRUE or FALSE */
gboolean upnp_device_is( const upnp_device *device, const char *type_or_id );
gboolean upnp_service_is( const upnp_service *service, const char *type_or_id );

/* Get the IP (and port) address of a given UPnP device */
gchar *upnp_get_ip_address( const upnp_device *device );
unsigned upnp_get_ip_port( const upnp_device *device );

/* Get a service which offers the given interface,
   returns NULL if not available */
upnp_service *upnp_get_service( const upnp_device *device, const char *service_type_or_id );
char *upnp_get_allowedValueRange( const upnp_device *device, const char *service_type_or_id, const char *state_variable_name, GError **error );

/* Get first/next device which offers the given interface */
upnp_device *upnp_get_first_device( const char *type_or_id );
upnp_device *upnp_get_next_device( const upnp_device *device, const char *type_or_id );
int upnp_num_devices( const char *type_or_id );

/*--- SSDP --------------------------------------------------------------------------------------*/

extern int SSDP_TTL /*= 4*/, SSDP_LOOP /*= 0*/, SSDP_MX /*= 2*/;
extern double SSDP_2ND_ATTEMPT   /*= 1.0s*/,
	SSDP_TIMEOUT                 /*= 30.0s*/,
	SSDP_GET_DESCRIPTION_TIMEOUT /*= 30.0s*/;

/* Do a UPnP search */
gboolean upnp_search_devices( const char *stv[], int stc,
	gboolean (*callback)( struct _GTimer *timer, void *user_data ), void *user_data, GError **error );

/* Add/Remove devices */
upnp_device *upnp_add_device( const char *USN, const char *LOCATION, GError **error );
void upnp_remove_device( const char *USN );

/* Free all devices */
void upnp_free_all_devices( void );

/*--- SOAP --------------------------------------------------------------------------------------*/

typedef struct _upnp_arg
{
	const char *name, *value;
	union
	{
		int argc;
		char value_buf[12];  /* must be sufficient for "upnp_ui4" in ASCII */
	} u;
} upnp_arg;

gboolean upnp_action( const upnp_device *device, const char* service_type_or_id,
                      const upnp_arg inv[], upnp_arg outv[], int *outc, GError **error );
void upnp_release_action( upnp_arg outv[] );

void upnp_set_action_name( upnp_arg inv[], const char *name );
void upnp_add_boolean_arg( upnp_arg inv[], const char *name, upnp_boolean value );
void upnp_add_i2_arg( upnp_arg inv[], const char *name, upnp_i2 value );
void upnp_add_ui2_arg( upnp_arg inv[], const char *name, upnp_ui2 value );
void upnp_add_i4_arg( upnp_arg inv[], const char *name, upnp_i4 value );
void upnp_add_ui4_arg( upnp_arg inv[], const char *name, upnp_ui4 value );
void upnp_add_string_arg( upnp_arg inv[], const char *name, const char *value );

upnp_boolean upnp_get_boolean_value( upnp_arg outv[], int index );
upnp_i2      upnp_get_i2_value( upnp_arg outv[], int index );
upnp_ui2     upnp_get_ui2_value( upnp_arg outv[], int index );
upnp_i4      upnp_get_i4_value( upnp_arg outv[], int index );
upnp_ui4     upnp_get_ui4_value( upnp_arg outv[], int index );
upnp_string  upnp_get_string_value( upnp_arg outv[], int index );

upnp_string upnp_shrink_to_fit( upnp_string str );
void upnp_free_string( upnp_string str );

/*--- GENA --------------------------------------------------------------------------------------*/

#define UPNP_MAX_HOST_ADDRESS  64
#define UPNP_MAX_SID          128

typedef struct _upnp_subscription
{
	struct _upnp_subscription *next;

	const upnp_device *device;
	const char *serviceTypeOrId;
	int timeout;
	gboolean (*callback)( struct _upnp_subscription *subscription, char *e_propertyset );
	void *user_data;
	GMutex *mutex;

	char sock_addr[UPNP_MAX_HOST_ADDRESS];
	unsigned timeout_id;

	char SID[UPNP_MAX_SID];  /* e.g. "uuid:47a32076-d9f5-5aef-46b0-88456fad2f2e" or "uuid:a2ef0ac4-e37f-4efa-b40f-52f566c8f8f9-urn:schemas-upnp-org:service:AVTransport-2" */
	upnp_ui4 SEQ;
} upnp_subscription;
extern upnp_subscription *subscriptionList;

upnp_subscription *upnp_subscribe( const upnp_device *device, const char *serviceTypeOrId, int timeout,
	gboolean (*callback)( upnp_subscription *subscription, char *e_propertyset ), void *user_data, GError **error );
gboolean upnp_unsubscribe( const upnp_device *device, const char *serviceTypeOrId, GError **error );
const upnp_subscription *upnp_get_subscription( const upnp_device *device, const char *serviceTypeOrId );

void upnp_unsubscribe_all( gboolean connected );

/*--- HTTP --------------------------------------------------------------------------------------*/

extern gint http_connections;

void http_set_user_agent( const char *user_agent );
gboolean http_get( const char *url, size_t initial_size, char **type, char **content, size_t *length, GError **error );
gboolean http_post( const char *url, size_t initial_size, char **type, char **content, size_t *length, GError **error );

gchar *http_get_server_address( const upnp_device *device );
gboolean http_start_server( GError **error );
void http_stop_server( void );
void http_end_server( void );

void http_server_add_to_url_list( gchar *str );
gboolean http_server_remove_from_url_list( const gchar *str );
void http_server_remove_all_from_url_list( void );

gboolean url_is_unreserved_char( char ch );
gchar *url_encoded( const char *arg );
gchar *url_encoded_path( const char *arg );
gchar *url_decoded( const char *url );

/*--- List management ---------------------------------------------------------------------------*/

int  list_append( void *list, void *data );
void list_prepend( void *list, void *data );
gboolean list_remove( void *list, void *data );

/*--- XML ---------------------------------------------------------------------------------------*/

struct xml_info { char *name, *end_of_name, *attributes, *end_of_attributes, *content, *end_of_content, *next; };
gboolean xml_get_info( const char *element, struct xml_info *info );
char *xml_strcpy( char *dest, size_t sizeof_dest, const char *src, const char *src_end );
gchar *xml_gstrcpy( const char *src, const char *src_end );

char *xml_get_name( const char *element, char *name, size_t sizeof_name );
gchar *xml_get_name_str( const char *element );
GString *xml_get_name_string( const char *element );

/*char* xml_get_attributes( const char *element, char *attribute, size_t sizeof_attribute );*/
/*gchar *xml_get_attributes_str( const char *element );*/
/*GString *xml_get_attributes_string( const char *element );*/

/*char* xml_get_content( const char *element, char *value, size_t sizeof_value );*/
/*gchar *xml_get_content_str( const char *element );*/
/*GString *xml_get_content_string( const char *element );*/

gchar *xml_box( const char *name, const char *content, const char *attribute_name_1, ... );
#ifdef va_start
gchar *xml_box_v( const char *name, const char *content, const char *attribute_name_1, va_list args );
#endif
void xml_string_append_boxed( GString *str, const char *name, const char *content, const char *attribute_name_1, ... );

char *xml_unbox( char **element, char **name /*= NULL*/, char **attributes /*= NULL*/ );
char *xml_unbox_attribute( char **attribute, char **name );

gchar *xml_wrap_delimiters( const char *src );
gchar *xml_unwrap_delimiters( const char *src );

/* Iteration */
typedef struct xml_iter_t { char *next, x; } xml_iter;
char *xml_first( char *content, xml_iter *iter );
char *xml_next( xml_iter *iter );
void  xml_abort_iteration( xml_iter *iter );

/* Search */
char *xml_find_named( const char *xml, const char *name, struct xml_info *info );
char *xml_get_named( const char *xml, const char *name, char *content, size_t sizeof_content );
gchar *xml_get_named_str( const char *xml, const char *name );
GString *xml_get_named_string( const char *xml, const char *name );

char *xml_get_named_with_attributes( const char *xml, const char *name,
	char *content, size_t sizeof_content, char *attributes, size_t sizeof_attributes );
gchar *xml_get_named_with_attributes_str( const char *xml, const char *name, gchar **attributes );
GString *xml_get_named_with_attributes_string( const char *xml, const char *name, GString **attributes );

char *xml_get_named_attribute( const char *element, const char *name, char *value, size_t sizeof_value );
gchar *xml_get_named_attribute_str( const char *element, const char *name );
GString *xml_get_named_attribute_string( const char *element, const char *name );

/* Management */
struct xml_content
{
	char *str;
	unsigned n;
	gint ref_count;
	enum { UPNP_FREE, G_FREE } free;
};
struct xml_content *new_xml_content( char *str, int n, int free );
struct xml_content *xml_content_from_string( GString *string, int n );
struct xml_content *copy_xml_content( struct xml_content *content );
struct xml_content *append_xml_content( struct xml_content *dest, struct xml_content *src, GError **error );
struct xml_content *append_to_xml_content( struct xml_content *dest, const char *src, GError **error );
struct xml_content *prepend_to_xml_content( struct xml_content *dest, const char *src, GError **error );
void free_xml_content( struct xml_content *content );
int num_xml_contents();

/*--- Error values ------------------------------------------------------------------------------*/

#define UPNP_ERROR (upnp_error_quark())
GQuark upnp_error_quark( void );

#define UPNP_LOG_DOMAIN "upnp"

#define UPNP_ERR_TIMEOUT               -1000
#define UPNP_ERR_SOCKET_CLOSED         -1001
#define UPNP_ERR_OUT_OF_MEMORY         -1002
#define UPNP_ERR_DEVICE_NOT_FOUND      -1003
#define UPNP_ERR_SERVICE_NOT_FOUND     -1004
#define UPNP_ERR_INVALID_PARAMS        -1006
#define UPNP_ERR_INVALID_URL           -1007
#define UPNP_ERR_UNKNOWN_HOST          -1008
#define UPNP_ERR_NO_VALID_XML          -1013
#define UPNP_ERR_NOT_SUBSCRIBED        -1014

/*-----------------------------------------------------------------------------------------------*/
