@ECHO ON
PATH=$PATH;C:\gtk\win32\bin

DEL /S pspbrwse.jbf

CALL make-h-toolbar qgn_list_filesys_audio_fldr qgn_list_filesys_audio_fldr
CALL make-h-toolbar qgn_list_gene_music_file qgn_list_gene_music_file
CALL make-h-toolbar qgn_list_gene_playlist qgn_list_gene_playlist

CALL make-h-toolbar gtk_media_forward_ltr_12 gtk+-2.12\gtk-media-forward-ltr
CALL make-h-toolbar gtk_media_forward_rtl_12 gtk+-2.12\gtk-media-forward-rtl
CALL make-h-toolbar gtk_media_next_ltr_12 gtk+-2.12\gtk-media-next-ltr
CALL make-h-toolbar gtk_media_next_rtl_12 gtk+-2.12\gtk-media-next-rtl
CALL make-h-toolbar gtk_media_pause_12 gtk+-2.12\gtk-media-pause
CALL make-h-toolbar gtk_media_play_ltr_12 gtk+-2.12\gtk-media-play-ltr
CALL make-h-toolbar gtk_media_play_rtl_12 gtk+-2.12\gtk-media-play-rtl
CALL make-h-toolbar gtk_media_previous_ltr_12 gtk+-2.12\gtk-media-previous-ltr
CALL make-h-toolbar gtk_media_previous_rtl_12 gtk+-2.12\gtk-media-previous-rtl
CALL make-h-toolbar gtk_media_record_12 gtk+-2.12\gtk-media-record
CALL make-h-toolbar gtk_media_rewind_ltr_12 gtk+-2.12\gtk-media-rewind-ltr
CALL make-h-toolbar gtk_media_rewind_rtl_12 gtk+-2.12\gtk-media-rewind-rtl
CALL make-h-toolbar gtk_media_stop_12 gtk+-2.12\gtk-media-stop

CALL make-h-toolbar leiads_media_paused_10 gtk+-2.10\leiads-media-paused
CALL make-h-toolbar leiads_media_paused_12 gtk+-2.12\leiads-media-paused

CALL make-h-toolbar leiads_media_playing_ltr_10 gtk+-2.10\leiads-media-playing-ltr
CALL make-h-toolbar leiads_media_playing_ltr_12 gtk+-2.12\leiads-media-playing-ltr
CALL make-h-toolbar leiads_media_playing_rtl_10 gtk+-2.10\leiads-media-playing-rtl
CALL make-h-toolbar leiads_media_playing_rtl_12 gtk+-2.12\leiads-media-playing-rtl

CALL make-h-toolbar leiads_media_stopped_10 gtk+-2.10\leiads-media-stopped
CALL make-h-toolbar leiads_media_stopped_12 gtk+-2.12\leiads-media-stopped

CALL make-h-toolbar leiads_media_transitioning_ltr_10 gtk+-2.10\leiads-media-transitioning-ltr
CALL make-h-toolbar leiads_media_transitioning_ltr_12 gtk+-2.12\leiads-media-transitioning-ltr
CALL make-h-toolbar leiads_media_transitioning_rtl_10 gtk+-2.10\leiads-media-transitioning-rtl
CALL make-h-toolbar leiads_media_transitioning_rtl_12 gtk+-2.12\leiads-media-transitioning-rtl

gdk-pixbuf-csource --raw --extern --name=audio_volume_muted  24x24\gtk+-2.12\audio-volume-muted.png  >24x24\gtk+-2.12\audio-volume-muted.h
gdk-pixbuf-csource --raw --extern --name=audio_volume_low    24x24\gtk+-2.12\audio-volume-low.png    >24x24\gtk+-2.12\audio-volume-low.h
gdk-pixbuf-csource --raw --extern --name=audio_volume_medium 24x24\gtk+-2.12\audio-volume-medium.png >24x24\gtk+-2.12\audio-volume-medium.h
gdk-pixbuf-csource --raw --extern --name=audio_volume_high   24x24\gtk+-2.12\audio-volume-high.png   >24x24\gtk+-2.12\audio-volume-high.h

gdk-pixbuf-csource --raw --extern --name=lastfm_16x16 16x16\lastfm.png >16x16\lastfm.h
gdk-pixbuf-csource --raw --extern --name=radiotime_16x16 16x16\radiotime.png >16x16\radiotime.h

PAUSE
