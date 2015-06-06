/*
	leia-ds.h
	Copyright (C) 2008-2011 Axel Sommerfeldt (axel.sommerfeldt@f-m.fm)

	--------------------------------------------------------------------

	This file is part of the Leia application.

	Leia is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Leia is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without eveGlovvn the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Leia.  If not, see <http://www.gnu.org/licenses/>.
*/

#define G_LOG_DOMAIN "Leia"

#ifdef MAEMO
#include <hildon/hildon.h>
#endif

/*#define GTK_DISABLE_DEPRECATED*/
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 12
#define gtk_tool_item_set_tooltip_text(tool_item,text) set_tool_item_tooltip_text(tool_item,text)
#endif

#define gtk_widget_get_window(widget) get_widget_window(widget)  /* since 2.14 */
#define gtk_cell_renderer_set_visible(cell,visible) set_cell_renderer_visible(cell,visible)  /* since 2.18 */
#define gtk_widget_set_visible(widget,visible) set_widget_visible(widget,visible)  /* since 2.18 */

#include <string.h>
#ifndef WIN32
  #include <strings.h>
  #define stricmp strcasecmp
  #define strnicmp strncasecmp
#endif

#include "princess/leia.h"

#define NAME      "Leia"
#define VERSION   "0.6"
#define CODENAME  "alpha 2"  /*"("__DATE__")"*/
#define COPYRIGHT "Copyright (C) 2008-2011 Axel Sommerfeldt"
#define WEBSITE   "http://leia.sommerfeldt.f-m.fm/"

#define BORDER    3
#define SPACING   3

#define ENTER_GDK_THREADS GUINT_TO_POINTER(1)

#define LEIADS_STOCK_BROWSER      GTK_STOCK_DIRECTORY
#define LEIADS_STOCK_PLAYLIST     GTK_STOCK_CDROM
#define LEIADS_STOCK_NOW_PLAYING  "leiads-now-playing"

struct keybd_function;

/*-----------------------------------------------------------------------------------------------*/
/* leia-ds.c */

#define LEIADS_STOCK_LEIADS "leiads-leiads"

#ifdef OSSO_SERVICE
/*extern osso_context_t *osso;*/
extern const gchar *osso_product_name, *osso_product_hardware;
#endif
#ifdef MAEMO
extern HildonProgram *Program;
extern HildonWindow *MainWindow;
#else
extern GtkWindow *MainWindow;
extern GtkStatusbar *Statusbar;
extern guint StatusbarContext;
extern GtkMenuItem *StatusMenuItem;
#endif

extern int NumServer, NumRenderer;

gboolean GetFullscreen( void );
void SetFullscreen( gboolean active );

const upnp_device *GetRenderer( void );
void SetRenderer( const upnp_device *device, gboolean refresh /*= FALSE*/ );

enum { BROWSER_PAGE = 0, PLAYLIST_PAGE, NOW_PLAYING_PAGE, NUM_PAGES };
int  GetCurrentPage( void );
void SetCurrentPage( int page_num );

void AppendDeviceMenuItems( GtkMenuShell *menu, const upnp_device *device );
void AppendTwonkyMenuItems( GtkMenuShell *menu, const upnp_device *device );
void HandleError( GError *error, const gchar *format, ... );
void RegisterLeiaStockIcon( void );
void ShowUri( GtkWidget *widget, const gchar *link );
void SetTransportState( enum TransportState new_state, gpointer user_data );
gboolean Quit( void );

gboolean is_twonky( const upnp_device *device );
void twonky_rescan( GtkMenuItem *menu_item, const upnp_device *device );
void twonky_restart( GtkMenuItem *menu_item, const upnp_device *device );
void twonky_rebuild( GtkMenuItem *menu_item, const upnp_device *device );

#ifndef MAEMO  /* Drag & Drop support */
extern GtkTargetEntry TargetList[2];
void drag_data_received( GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint time, gpointer user_data );
#endif

/*-----------------------------------------------------------------------------------------------*/
/* browser.c */

enum { BROWSER_NO_ACTION = -1, BROWSER_POPUP_MENU, BROWSER_OPEN, BROWSER_REPLACE_PLAYLIST, BROWSER_APPEND_TO_PLAYLIST, BROWSER_SET_CHANNEL, BROWSER_PLAY_LASTFM };

extern const char localId[], radioId[], radiotimeId[], lastfmId[];

void CreateBrowserToolItems( GtkToolbar *toolbar );
void CreateBrowserMenuItems( GtkMenuShell *menu );

GtkWidget *CreateBrowser( void );
int  OpenBrowser( void );
gboolean UpdateBrowser( void );
void CloseBrowser( void );
gboolean BrowserIsOpen( void );

void OnSelectBrowser( void );
void OnUnselectBrowser( void );
void OnBrowserKey( GdkEventKey *event, struct keybd_function **func );
void OnBrowserSettingsChanged( void );

void BrowserReset( const upnp_device *device );
void ClearBrowserCache( const upnp_device *device );

void GetLastfmRadioTracks( void );

/* Drag & Drop support */
extern const char *get_upnp_class_id( const char *didl_item );
extern gboolean is_dragable_item( const char *didl_item, const char *class_id );
extern gboolean browser_action( int action, gchar **didl_item, gchar **path_str, gfloat row_align );

/*-----------------------------------------------------------------------------------------------*/
/* playlist.c */

#define PLAYLIST_FILE_SUPPORT
#define PLAYLIST_REPEAT_SUPPORT  /* "Repeat Mode" for UPnP, Auskerry & Bute devices */

enum { PLAYLIST_NO_TOOLBAR = 0, PLAYLIST_LEFT_TOOLBAR, PLAYLIST_RIGHT_TOOLBAR };

void CreatePlaylistToolItems( GtkToolbar *toolbar );
void CreatePlaylistMenuItems( GtkMenuShell *menu );

GtkWidget *CreatePlaylist( void );
int  OpenPlaylist( void );
void ClosePlaylist( void );
gboolean PlaylistIsOpen( void );

void SetPlaylistRendererMenu( GtkMenu *menu );

void OnSelectPlaylist( void );
void OnUnselectPlaylist( void );
void OnPlaylistKey( GdkEventKey *event, struct keybd_function **func );
void OnPlaylistSettingsChanged( void );

void OnPlaylistTransportState( enum TransportState transport_state );
void OnPlaylistTrackIndex( int OldTrackIndex, int NewTrackIndex );

gboolean ClearPlaylist( void );
gboolean AddToPlaylist( char *playlist_data, const gchar *title );

void DoPlaylistClear( void );

/* Drag & Drop support */
extern void playlist_play_track( int index );

/*-----------------------------------------------------------------------------------------------*/
/* playlist_file.c */

struct xml_content *LoadPlaylist( const char *filename, int *tracks_not_added, GError **error );
gboolean SavePlaylist( const char *filename, GError **error );
void HandleNotAddedTracks( int tracks_not_added );

gchar *ChoosePlaylistFile( GtkWindow *parent, const char *current_name, gchar **current_folder );

FILE *CreatePlaylistFile( const char *filename, gboolean include_server_list, GError **error );
gboolean WritePlaylistItem( FILE *fp, const char *item, GError **error );
gboolean ClosePlaylistFile( FILE *fp, GError **error );

/*-----------------------------------------------------------------------------------------------*/
/* now_playing.c */

#define NOW_PLAYING_MINIMUM_IMAGE_SIZE   200
#define NOW_PLAYING_NOTEBOOK_IMAGE_SIZE  300
#define NOW_PLAYING_PANED_IMAGE_SIZE     200

void CreateNowPlayingToolItems( GtkToolbar *toolbar, gint icon_index );
void CreateNowPlayingMenuItems( GtkMenuShell *menu );

GtkWidget *CreateNowPlaying( void );
int  OpenNowPlaying( void );
void CloseNowPlaying( void );
void ClearNowPlaying( gboolean lastfm );

void OnSelectNowPlaying( void );
void OnUnselectNowPlaying( void );
void OnNowPlayingKey( GdkEventKey *event, struct keybd_function **func );
void OnNowPlayingSettingsChanged( void );

void OnNowPlayingTrackIndex( int OldTrackIndex, int NewTrackIndex );

gint GetNowPlayingWidth( gboolean fullscreen, int album_art_size );
void SetNowPlayingAttributes( int fullscreen, int album_art );

/*-----------------------------------------------------------------------------------------------*/
/* about.c */

void About( GdkPixbuf *logo );

/*-----------------------------------------------------------------------------------------------*/
/* info.c */

void Info( upnp_device *device );

gboolean IsValuableInfo( const char *value );
gchar *GetInfo( const gchar *item_or_container, const char *name, char *value, size_t sizeof_value );
gboolean AppendInfo( GtkListStore *store, const char *name, const char *value, int maxlen );

struct device_list_entry
{
	upnp_device *device;
	unsigned long product_id;
};

struct device_list_entry *GetServerList( int *n );
struct device_list_entry *GetRendererList( int *n );

gchar *GetAlbum( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetAlbumArtist( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetArtist( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetDate( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetDuration( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetGenre( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetOriginalTrackNumber( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetTitle( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetTitleAndArtist( const gchar *item_or_container, char *value, size_t sizeof_value );
gchar *GetYear( const gchar *item_or_container, char *value, size_t sizeof_value );

#define ALBUM_ART_SIZE_TOLERANCE 6  /* the album art is allowed to exceed the image size by this amount */

gchar *GetAlbumArtURI( const gchar *item_or_container );
gboolean GetAlbumArt( const char *albumArtURI, char **type, char **content, size_t *length, GError **error );
gboolean SetAlbumArt( GtkImage *image, gint image_size, gboolean shrink_only, const char *albumArtURI, char *content_type, char *content, size_t content_length, GError **error );

/*-----------------------------------------------------------------------------------------------*/
/* radiotime.c */

#define LEIADS_STOCK_RADIOTIME "leiads-radiotime"

void RegisterRadioTimeStockIcon( void );
gchar *GetRadioTimeLogoFilename( void );

const char *GetRadioTimeTitle( void );
gboolean IsRadioTimeItem( const char *didl_item );

struct xml_content *GetRadioTimeFolder( const char *id, const char *formats, gchar **title, GError **error );

/*-----------------------------------------------------------------------------------------------*/
/* lastfm.c */

#define LEIADS_STOCK_LASTFM "leiads-lastfm"

enum LastfmKey { SimilarArtists = 0, Fans, GlobalTags, Library, Neighbours, Loved, Recommended, NUM_LASTFM_KEYS };

void RegisterLastfmStockIcon( void );
gchar *GetLastfmLogoFilename( gboolean badge, gboolean small, const GdkColor *bg );

extern const char LastfmLeiaSessionKey[] /*= "leiads"*/;
void SetLastfmSessionKey( const char *name, const char *key );
const char *GetLastfmSessionKey( void );
gchar *GetLastfmSessionKeyFromTrack( int track_index );
gboolean IsOurLastfmSession( const char *key );

const char *GetLastfmRadioTitle( void );
const char *GetLastfmRadioItem( enum LastfmKey key, const char *value, GString *result );
gboolean IsLastfmItem( const char *didl_item );
gchar *LastfmRadioPlaylistToUpnpPlaylist( const char *title_strip, char *trackList );

struct xml_content *GetLastfmFolder( GError **error );
gboolean AddToLastfmFolder( const char *didl_item );

gboolean is_lastfm_url( const char *url );

gchar *lastfm_album_get_info_url( const char *artist, const char *album );
char *lastfm_album_get_info( const char *artist, const char *album, GError **error );

char *lastfm_auth_get_mobile_session( const char *username, const char *password, GError **error );

char *lastfm_radio_get_playlist( const char *sk, gboolean discovery, GError **error );
char *lastfm_radio_tune( const char *lang, const char *station, const char *sk, GError **error );

/*-----------------------------------------------------------------------------------------------*/
/* select_renderer.c */

const upnp_device *ChooseRenderer( void );
gboolean IsDefaultMediaRenderer( const upnp_device *device );
void SetDefaultMediaRenderer( const upnp_device *device );

/*-----------------------------------------------------------------------------------------------*/
/* settings.c */

extern struct settings
{
	gint screen_width, screen_height;
	gboolean MID;

	struct startup_settings
	{
		gboolean classic_ui, toolbar_at_bottom;
		gboolean fullscreen, maximize;
		gint     clock_format;
	} startup;

	struct browser_settings
	{
		gboolean activate_items_with_single_click;
		gboolean go_back_item;
		gint     playlist_container_action, music_album_action, music_track_action, audio_broadcast_action;
		gint     page_after_replacing_playlist;

		gboolean handle_storage_folder_with_artist_as_album;
		gboolean handle_storage_folder_with_genre_as_album;
		gboolean local_folder;
		gchar *local_folder_path;
	} browser;

	struct playlist_settings
	{
		gchar *current_folder;
		gint toolbar;
	} playlist;

	struct now_playing_settings
	{
		int album_art_size;
		gboolean shrink_album_art_only;
		gboolean show_track_number, show_track_duration, show_album_year;
		int double_click;
	} now_playing;

	struct volume_settings
	{
		enum { VOLUME_UI_NONE = 0, VOLUME_UI_PLUSMINUS, VOLUME_UI_BUTTON, VOLUME_UI_BAR } ui;
		gboolean show_level;
		int quiet, limit;
	} volume;

	struct keys_settings
	{
		int zoom, full_screen, up_down, left_right, enter;
		gboolean use_system_keys;
	} keys;

	struct radiotime_settings
	{
		int active;
		char username[32];
	} radiotime;

	struct lastfm_settings
	{
		gboolean active;
		char username[16], password[32+4];
		gboolean album_art, now_playing, scrobbling, discovery_mode;
	} lastfm;

	struct ssdp_settings
	{
		gboolean media_server, media_renderer, linn_products;
		int mx, timeout;
		gchar *default_media_renderer;
	} ssdp;

	struct debug_settings
	{
		gboolean keybd;
		int screenshot;
	} debug;

} *Settings;

void LoadSettings( void );
void SaveSettings( gboolean show_information );

void SetShareDir( const char *dirname );
void SetUserConfigDir( const char *dirname );

gchar *BuildUserConfigFilename( const char *filename );
gchar *BuildApplDataFilename( const char *filename );

void Customise( GtkMenuItem *menu_item, gpointer user_data );
void Options( GtkMenuItem *menu_item, gpointer user_data );

gboolean IsSmartQ( void );

/*-----------------------------------------------------------------------------------------------*/
/* transport.c */

extern gboolean IsRadioTimeTrack, IsLastfmTrack;

void CreateTransportToolItems( GtkToolbar *toolbar, gint icon_index );
void CreateTransportMenuItems( GtkMenuShell *menu );

int  OpenTransport( void );
void CloseTransport( void );

gboolean IsPlaying( void );
gboolean IsStopped( void );

gboolean DoPlay( void );
gboolean DoPause( void );
gboolean DoStop( void );
gboolean DoPreviousTrack( void );
gboolean DoNextTrack( void );

/*-----------------------------------------------------------------------------------------------*/
/* volume.c */

#ifdef MAEMO
#define STOCK_AUDIO_VOLUME_MUTED    "qgn_stat_volumemute"
#define STOCK_AUDIO_VOLUME_MINIMUM  "qgn_stat_volumelevel0"
#define STOCK_AUDIO_VOLUME_LOW      "qgn_stat_volumelevel1"
#define STOCK_AUDIO_VOLUME_MEDIUM   "qgn_stat_volumelevel2"
#define STOCK_AUDIO_VOLUME_HIGH     "qgn_stat_volumelevel3"
#define STOCK_AUDIO_VOLUME_MAXIMUM  "qgn_stat_volumelevel4"
#define STOCK_AUDIO_VOLUME_VOLUME   "qgn_indi_gene_volume"
#define STOCK_AUDIO_VOLUME_MUTE     "qgn_indi_gene_mute"
/*#define STOCK_AUDIO_VOLUME_MINUS    "qgn_indi_gene_minus"*/
/*#define STOCK_AUDIO_VOLUME_PLUS     "qgn_indi_gene_plus"*/
#else
#define STOCK_AUDIO_VOLUME_MUTED    "audio-volume-muted"
#define STOCK_AUDIO_VOLUME_MINIMUM  "audio-volume-low"
#define STOCK_AUDIO_VOLUME_LOW      "audio-volume-low"
#define STOCK_AUDIO_VOLUME_MEDIUM   "audio-volume-medium"
#define STOCK_AUDIO_VOLUME_HIGH     "audio-volume-high"
#define STOCK_AUDIO_VOLUME_MAXIMUM  "audio-volume-high"
#endif

void CreateVolumeToolItems( GtkToolbar *toolbar );
void CreateVolumeMenuItems( GtkMenuShell *menu );

int  OpenVolume( void );
void CloseVolume( void );

void OnVolumeSettingsChanged( void );

void ToggleMute( void );
void SetVolumeDelta( int delta );

/*-----------------------------------------------------------------------------------------------*/
/* keybd.c */

enum {
	KEYBD_NOP, KEYBD_FULLSCREEN, KEYBD_PAGE,
	KEYBD_PLAY_PAUSE, KEYBD_TRACK, KEYBD_MUTE, KEYBD_VOLUME,
	KEYBD_XXX, KEYBD_SELECTION, KEYBD_SELECT_RENDERER, KEYBD_REFRESH };

extern struct keybd_function
{
	void (*func)( int delta, gboolean long_press );
	enum { ALLOW_REPEAT, SUPPRESS_REPEAT, SUPPORTS_LONG_PRESS } repeat;
	/*  int description_text; */
} keybd_functions[];

gboolean KeyPressEvent( GtkWidget *widget, GdkEventKey *event, gpointer user_data );
gboolean KeyReleaseEvent( GtkWidget *widget, GdkEventKey *event, gpointer user_data );

/*-----------------------------------------------------------------------------------------------*/
/* metadata.c */

#ifndef MAEMO

gboolean GetMetadataFromFile( const char *filename, const char *album_art_filename, GString *item, gboolean playlists, GError **error );
void     GetMetadataFromFolder( const char *path, GString *items, GError **error );

struct dir_entry
{
	char name[256];
	gchar *filename;
	unsigned short st_mode;
};

GList *get_sorted_dir_content( const gchar *path, const gchar **album_art_filename, GError **error );
void free_sorted_dir_content( GList *l );
void free_dir_entry( GList *l );

#endif

/*-----------------------------------------------------------------------------------------------*/
/* miscellaneous.c */

extern const char hex[16] /*= {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' }*/;

char *md5cpy( char *dest, const char *src );
char *md5cat( char *dest, const char *src );

gchar *get_common_str_part( gchar **titles, guint n );
gchar *make_valid_filename( gchar *filename );

extern FILE *fopen_utf8( const char *filename, const char *mode );  /* princess/upnp.c */
int  file_put_int( FILE *fp, const char *name, int value );
int  file_put_string( FILE *fp, const char *name, const char *value /*, const char *attrib_name_1, ...*/ );

/*
	Workaround for flaw in older GTK+ versions:
	A double click on a selected row will produce *two* "row-activated" events
*/
extern int row_activated_flag;

void set_row_activated_flag( void );

GtkIconSize get_icon_size_for_toolbar( GtkToolbar *toolbar );
gint get_icon_width_for_toolbar( GtkToolbar *toolbar );

/*
	fix_hildon_tool_button() is a workaround
	for flaws in older GTK+ versions (MAEMO only!?):

	1. The toolbar item width is wrong when using an item image which must be scaled
	2. The toolbar item height is wrong in toolbars with GTK_ORIENTATION_VERTICAL
*/
#ifdef MAEMO
void fix_hildon_tool_button( GtkToolItem *item );
#else
#define fix_hildon_tool_button( item )
#endif

GtkIconFactory *get_icon_factory( void );
const gchar *register_stock_icon( const gchar *stock_id, const guint8 *data );
const gchar *register_bidi_stock_icon( const gchar *stock_id, const guint8 *data_ltr, const guint8 *data_rtl );
/*void register_sized_icon( GtkIconSet *icon_set, GtkIconSize size, const guint8 *data );*/
void register_di_icon( GtkIconSet *icon_set, GtkTextDirection direction, const guint8 *data );
void register_gtk_stock_icon( const gchar *stock_id );

void busy_enter( void );
void busy_leave( void );

void set_main_window_title( const char *title );
void set_screen_settings( void );

#define GTK_MESSAGE_QUESTION_WITH_CANCEL_AS_DEFAULT_BUTTON ((GtkMessageType)0x1000)
gint message_dialog( GtkMessageType type, const gchar *description );

struct animation
{
	const gchar *action;
	gchar *item;
	guint timeout_id;
	GtkWidget *widget;
};

struct animation *show_animation( const char *action, const gchar *item, guint interval );
void cancel_animation( struct animation *animation );
void clear_animation( struct animation *animation );

void show_information( const gchar *icon_name, const gchar *text_format, const gchar *text );
#ifndef MAEMO
guint push_to_statusbar( const gchar *text_format, const gchar *text );
void remove_from_statusbar( guint message_id );
#endif

void take_screenshot( guint delay, const char *filename );

void attach_to_table( GtkTable *table, GtkWidget *child, guint top_attach, guint left_attach, guint right_attach );
gboolean get_alternative_button_order( void );
GList *get_selected_tree_row_references( GtkTreeSelection *selection );
GtkSettings *get_settings_for_widget( GtkWidget *widget );
const GdkColor *get_widget_base_color( GtkWidget *widget, GtkStateType state );
GdkWindow *get_widget_window( GtkWidget *widget );
gboolean foreach_selected_tree_view_row( GtkTreeView *tree_view,
	gboolean (*func)( GtkTreeView *tree_view, GtkTreePath *path, gchar *path_str, gpointer user_data ), gpointer user_data );
GtkWidget *new_entry( gint max_length );
GtkWidget *new_file_chooser_dialog( const gchar *title, GtkWindow *parent, GtkFileChooserAction action );
GtkBox *new_hbox( void );
GtkWidget *new_label( const gchar *str );
GtkWidget *new_label_with_colon( const gchar *str );
GtkDialog *new_modal_dialog_with_ok_cancel( const gchar *title, GtkWindow *parent );
GtkWidget *new_scrolled_window( void );
GtkTable *new_table( guint rows, guint columns );
void set_cell_renderer_visible( GtkCellRenderer *cell, gboolean visible );
void set_dialog_size_request( GtkDialog *dialog, gboolean set_height );
void set_tool_item_tooltip_text( GtkToolItem *tool_item, const gchar *text );
void set_widget_visible( GtkWidget *widget, gboolean visible );
void tree_view_queue_resize( GtkTreeView *tree_view/*, int num_columns*/ );

/*-----------------------------------------------------------------------------------------------*/
/* intl.c */

enum
{
	T_TRANSLATOR_CREDITS, T_COMMENTS, T_LICENSE,

	T_MENU_FILE, T_MENU_QUIT, T_MENU_VIEW, T_MENU_TOOLS, T_MENU_INFO, T_MENU_ABOUT,

	T_BROWSER, T_PLAYLIST, T_NOW_PLAYING,
	T_MEDIA_PLAY, T_MEDIA_PAUSE, T_MEDIA_STOP, T_MEDIA_PREVIOUS, T_MEDIA_NEXT,
	T_MUTE, T_QUIET, T_LOUD, T_VOLUME, T_VOLUME_MINUS, T_VOLUME_PLUS, T_VOLUME_INFO,

	T_SEARCHING_SERVER, T_SEARCHING_RENDERER, T_SEARCHING_SERVER_AND_RENDERER,
	T_NO_SERVER_FOUND, T_NO_RENDERER_FOUND, T_NO_SERVER_AND_RENDERER_FOUND,
	T_SET_AS_DEFAULT,

	/* Info dialog */
	T_INFO, T_INFO_FRIENDLY_NAME,
	T_INFO_MODEL_NAME, T_INFO_MODEL_NUMBER, T_INFO_SERIAL_NUMBER, T_INFO_MANUFACTURER, T_INFO_URL,
	T_INFO_IP_ADDRESS, T_INFO_MAC_ADDRESS, T_INFO_PRODUCT_ID,
	T_INFO_SOFTWARE_VERSION, T_INFO_HARDWARE_VERSION,

	/* Media Browser page */
	T_BROWSER_MEDIA_SERVERS, T_BROWSER_OPENING, T_BROWSER_GO_BACK,
	T_BROWSER_POPUP_MENU, T_BROWSER_OPEN, T_BROWSER_REPLACE_PLAYLIST, T_BROWSER_APPEND_TO_PLAYLIST,
	T_BROWSER_SET_CHANNEL, T_BROWSER_REPLACE_PLAYLIST_QUESTION, T_BROWSER_CHANNEL_SET,
	T_BROWSER_LOCAL_FOLDER,

	/* Playlist page */
	T_PLAYLIST_REPLACED, T_PLAYLIST_ADDED,
	T_PLAYLIST_CLEAR, T_PLAYLIST_CLEAR_QUESTION, T_PLAYLIST_CLEARED,
	T_PLAYLIST_SHUFFLE, T_PLAYLIST_SHUFFLE_QUESTION,
	T_PLAYLIST_LOAD, T_PLAYLIST_SAVE_AS, T_PLAYLIST_LOAD_CAPTION, T_PLAYLIST_SAVE_CAPTION,
	T_PLAYLIST_FILTER_NAME, T_PLAYLIST_FILTER_NAME_1,
	T_PLAYLIST_PLAY, T_PLAYLIST_MOVE_UP, T_PLAYLIST_MOVE_DOWN,
	T_PLAYLIST_DELETE, T_PLAYLIST_DELETE_QUESTION,
	T_PLAYLIST_NOT_A_VALID_FILE, T_PLAYLIST_TRACKS_NOT_ADDED,
	T_PLAYLIST_READING, T_PLAYLIST_SHUFFLING, T_PLAYLIST_LOADING, T_PLAYLIST_SAVED, T_PLAYLIST_INFO,
	T_PLAYLIST_REPEAT_MODE, T_PLAYLIST_SHUFFLE_PLAY,
	T_PLAYLIST_NO_TOOLBAR, T_PLAYLIST_LEFT_TOOLBAR, T_PLAYLIST_RIGHT_TOOLBAR,

	/* Now Playing page */
	T_NOW_PLAYING_EOP,

	/* UPnP stuff */
	T_DC_TITLE, T_DC_CREATOR, T_UPNP_ARTIST, T_UPNP_ALBUM, T_UPNP_GENRE,
	T_DC_DATE, T_DC_YEAR, T_UPNP_ORIGINAL_TRACK_NUMBER, T_UPNP_COMPOSER,
	T_RES_DURATION, T_UNKNOWN,
	T_TRANSPORT_STATE_BUFFERING, T_TRANSPORT_STATE_PAUSED, T_TRANSPORT_STATE_PLAYING,
	T_TRANSPORT_STATE_TRANSITIONING, T_TRANSPORT_STATE_STOPPED,

	/* Server tools */
	T_SERVER_RESCAN, T_SERVER_RESTART, T_SERVER_REBUILD,
	T_SERVER_REBUILD_QUESTION, T_SERVER_REBUILD_QUESTION_X,

	/* Misc. */
	T_QUOTED, T_WLAN_CONNECTING,
	T_TOGGLE_FULLSCREEN, T_TOGGLE_MUTE, T_CLEAR_PLAYLIST, T_QUIT_APPLICATION,
	T_PRESENTATION_URL, T_MANUFACTURER_URL,

	/* Leia's own radio station list */
	T_RADIO, T_RADIO_NEW, T_RADIO_EDIT, T_RADIO_ALBUM_ART_URI,
	T_RADIO_IMPORT, T_RADIO_EXPORT,
	T_RADIO_EXPORT_WHICH, T_RADIO_EXPORT_ALL, T_RADIO_EXPORT_SELECTION,

	/* RadioTime */
	T_RADIOTIME_PRESETS, T_RADIOTIME_DS_PRESETS, T_RADIOTIME_HOME_PAGE,
	T_RADIOTIME_SUBTEXT, T_RADIOTIME_BITRATE, T_RADIOTIME_RELIABILITY, T_RADIOTIME_CURRENT_TRACK,

	/* Last.fm */
	T_LASTFM_RADIO, T_LASTFM_RADIO_NEW, T_LASTFM_RADIO_STOPPED, T_LASTFM_RADIO_TYPE,
	T_LASTFM_RADIO_SIMILAR_ARTISTS, T_LASTFM_RADIO_TOP_FANS, T_LASTFM_RADIO_GLOBAL_TAG,
	T_LASTFM_RADIO_LIBRARY, T_LASTFM_RADIO_NEIGHBOURS, T_LASTFM_RADIO_LOVED_TRACKS, T_LASTFM_RADIO_RECOMMENDATION,
	T_LASTFM_ARTIST, T_LASTFM_TAG, T_LASTFM_USER,
	T_LASTFM_HOME_PAGE, T_LASTFM_PROFILE_PAGE, T_LASTFM_RECOMMENDED_PAGE,
	T_LASTFM_LIBRARY_PAGE, T_LASTFM_EVENTS_PAGE, T_LASTFM_SETTINGS,
	T_LASTFM_ARTIST_PAGE, T_LASTFM_ALBUM_PAGE, T_LASTFM_TRACK_PAGE,
	T_LASTFM_BUY_TRACK_URL, T_LASTFM_BUY_ALBUM_URL, T_LASTFM_FREE_TRACK_URL,
	T_LASTFM_TITLE_STRIP,

	/* Menu (part 2) */
	T_MENU_DEVICES, T_MENU_DEVICE_INFO, T_MENU_CUSTOMISE, T_MENU_OPTIONS,

	/* Keys */
	T_KEYS, T_KEYBD_NOP, T_KEYBD_FULLSCREEN, T_KEYBD_PAGE, T_KEYBD_PLAY_PAUSE, T_KEYBD_TRACK, T_KEYBD_MUTE, T_KEYBD_VOLUME,
	T_KEYBD_REFRESH, T_KEYBD_SELECTION, T_KEYBD_SELECT_RENDERER,

	/* Customise & Options */
	T_CUSTOMISE, T_OPTIONS, T_SETTINGS_SAVED,

	T_STARTUP,
	T_SETTINGS_CLASSIC_UI, T_SETTINGS_TOOLBAR_AT_BOTTOM,
	T_SETTINGS_FULLSCREEN_MODE, T_SETTINGS_MAXIMIZE_WINDOW,

	T_GENERAL,
	T_SETTINGS_CLOCK, T_SETTINGS_CLOCK_NONE, T_SETTINGS_CLOCK_LOCALE, T_SETTINGS_CLOCK_12H, T_SETTINGS_CLOCK_24H,
	T_SETTINGS_PREFERRED_PAGE,

	T_SETTINGS_ACTIVATE_ITEMS_WITH_SINGLE_CLICK, T_SETTINGS_GO_BACK_ITEM,
	T_SETTINGS_PLAYLIST_CONTAINER_ACTION, T_SETTINGS_MUSIC_ALBUM_ACTION, T_SETTINGS_MUSIC_TRACK_ACTION, T_SETTINGS_AUDIO_BROADCAST_ACTION,
	T_SETTINGS_PAGE_AFTER_REPLACING_PLAYLIST,

	T_SETTINGS_HANDLE_STORAGE_FOLDER_WITH_ARTIST_AS_ALBUM, T_SETTINGS_HANDLE_STORAGE_FOLDER_WITH_GENRE_AS_ALBUM,

	T_SETTINGS_PLAYLIST_TOOLBAR, /* ... */

	T_SETTINGS_ALBUM_ART_SIZE, T_SETTINGS_SHRINK_ALBUM_ART_ONLY,
	T_SETTINGS_SHOW_TRACK_NUMBER, T_SETTINGS_SHOW_TRACK_DURATION, T_SETTINGS_SHOW_ALBUM_YEAR,
	T_SETTINGS_DOUBLE_CLICK,

	T_SETTINGS_VOLUME_UI, T_SETTINGS_VOLUME_UI_NONE, T_SETTINGS_VOLUME_UI_PLUSMINUS, T_SETTINGS_VOLUME_UI_BUTTON, T_SETTINGS_VOLUME_UI_BAR,
	T_SETTINGS_VOLUME_SHOW_LEVEL, T_SETTINGS_VOLUME_QUIET, T_SETTINGS_VOLUME_LIMIT,

	T_SETTINGS_ZOOM_KEY, T_SETTINGS_FULL_SCREEN_KEY, T_SETTINGS_LEFT_RIGHT_KEY,
	T_SETTINGS_UP_DOWN_KEY, T_SETTINGS_ENTER_KEY, T_SETTINGS_PAGE_UP_DOWN_KEY,
	T_SETTINGS_USE_SYSTEM_KEYS,

	T_SETTINGS_RADIOTIME_0, T_SETTINGS_RADIOTIME_1, T_SETTINGS_RADIOTIME_2,

	T_SETTINGS_LASTFM, T_SETTINGS_GET_MISSING_ALBUM_ART, T_SETTINGS_USERNAME,
	T_SETTINGS_PASSWORD, T_SETTINGS_NOW_PLAYING_SUBMISSION, T_SETTINGS_SCROBBLING,
	T_SETTINGS_DISCOVERY_MODE,

	T_SETTINGS_SSDP,
	T_SETTINGS_SSDP_MEDIA_SERVER, T_SETTINGS_SSDP_MEDIA_RENDERER, T_SETTINGS_SSDP_LINN_PRODUCTS,
	T_SETTINGS_SSDP_MX, T_SETTINGS_SSDP_TIMEOUT,
	T_SETTINGS_DEFAULT_RENDERER, T_SETTINGS_NO_DEFAULT_RENDERER,

	/* Questions */
	T_QUESTION_QUIT,

	/* Error messages */
	T_ERROR_MESSAGE, T_ERROR_CODE, T_FILE_NAME,
	T_ERROR_BROWSE_FOLDER, T_ERROR_BROWSE_ALBUM, T_ERROR_PLAY_LASTFM_RADIO, T_ERROR_GET_ALBUM_COVER,
	T_ERROR_CONNECT_DEVICE, T_ERROR_HANDLE_RENDERER_EVENT, T_ERROR_OPEN_URL, T_ERROR_PLAYLIST_CLEAR,
	T_ERROR_PLAYLIST_ADD, T_ERROR_PLAYLIST_DELETE, T_ERROR_PLAYLIST_MOVE, T_ERROR_PLAYLIST_SHUFFLE,
	T_ERROR_PLAYLIST_REPEAT_MODE, T_ERROR_PLAYLIST_LOAD, T_ERROR_PLAYLIST_SAVE,
	T_ERROR_PLAY, T_ERROR_PAUSE, T_ERROR_STOP, T_ERROR_SEEK_TRACK, T_ERROR_TOGGLE_MUTE, T_ERROR_VOLUME,
	T_ERROR_UPNP_STARTUP, T_ERROR_UPNP_SEARCH, T_ERROR_SAVE_SETTINGS, T_ERROR_SAVE_LASTFM_HISTORY,
	T_ERROR_HANDLING_ITEM, T_ERROR_DEVICE_INFO, T_ERROR_DEVICE_INFO_FROM, T_ERROR_SET_RADIO_CHANNEL,
	T_ERROR_GET_METADATA_FROM_FILE,

	/* Error message texts */
	T_ERRMSG_INTERNAL,
	T_ERRMSG_CREATE_FILE, T_ERRMSG_OPEN_FILE, T_ERRMSG_READ_FILE, T_ERRMSG_WRITE_FILE,
	T_ERRMSG_LASTFM_RADIO, T_ERRMSG_UNKNOWN_FORMAT,	T_ERRMSG_OUT_OF_MEMORY, T_ERRMSG_ITEM_ACTIVATED,
	T_ERRMSG_UNKNOWN_FILE_FORMAT,

	NUM_TEXT };

extern const char *Texts[NUM_TEXT];
#define TextIndex(a) T_##a
#define Text(a) Texts[TextIndex(a)]

extern const char *Music[], *Photos[], *Videos[];

const char *GetLanguage( void );
gchar *SetLanguage( const char *lang );

/*-----------------------------------------------------------------------------------------------*/
/* debug.c */

#if LOGLEVEL > 0
#define ASSERT(expr) g_assert(expr)
#define VERIFY(expr) g_assert(expr)
#else
#define ASSERT(expr)
#define VERIFY(expr) do { if G_LIKELY(expr) ; } while (0)
#endif

#if LOGLEVEL >= 2
#define TRACE(a) g_print a
#else
#define TRACE(a)
#endif

#if LOGLEVEL > 0
void debug_init( void );
void debug_exit( void );
#endif

gchar *build_logfilename( const char *basename, const char *extension );

/*--- EOF ---------------------------------------------------------------------------------------*/
