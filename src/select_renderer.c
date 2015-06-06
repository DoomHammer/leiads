/*
	select_renderer.c
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

#include "leia-ds.h"

enum { COL_UDN = 0, COL_FRIENDLY_NAME, COL_PRODUCT_ID };
struct select_renderer_user_data { GtkDialog *dialog; GtkToggleButton *set_as_default; const upnp_device *device; };

void select_renderer_selection_changed( GtkTreeSelection *selection, gpointer user_data );
void select_renderer_row_activated( GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data );

/*-----------------------------------------------------------------------------------------------*/

const upnp_device *ChooseRenderer( void )
{
	const upnp_device *device = GetAVTransport();

	gboolean show_product_id = FALSE;
	struct device_list_entry *sr_list;
	struct select_renderer_user_data ud;
	const gchar *cancel_button_text;
	int i, n;

	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkListStore *store;

	sr_list = GetRendererList( &n );
	if ( sr_list == NULL ) return NULL;

	for ( i = 0; i < n; i++ )
	{
		if ( device == NULL && IsDefaultMediaRenderer( sr_list[i].device ) )
		{
			device = sr_list[i].device;
			g_free( sr_list );
			return device;
		}

		if ( sr_list[i].product_id != 0 )
		{
			show_product_id = TRUE;
			break;
		}
	}

	/* Create dialog... */
	cancel_button_text = ( device == NULL ) ? GTK_STOCK_QUIT : GTK_STOCK_CANCEL;
	ud.dialog = GTK_DIALOG(( get_alternative_button_order() )
		? gtk_dialog_new_with_buttons( Text(KEYBD_SELECT_RENDERER),
			GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL,
			GTK_STOCK_OK, GTK_RESPONSE_OK, cancel_button_text, GTK_RESPONSE_CANCEL, NULL )
		: gtk_dialog_new_with_buttons( Text(KEYBD_SELECT_RENDERER),
			GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL,
			cancel_button_text, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL ));

	gtk_dialog_set_has_separator( ud.dialog, FALSE );
	gtk_dialog_set_default_response( ud.dialog, GTK_RESPONSE_OK );

	view = GTK_TREE_VIEW( gtk_tree_view_new() );
	gtk_tree_view_set_headers_visible( view, show_product_id );
	gtk_tree_view_set_enable_search( view, TRUE );
	gtk_tree_view_set_search_column( view, COL_FRIENDLY_NAME );

	selection = gtk_tree_view_get_selection( view );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_SINGLE );
/*
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "text", COL_UDN, NULL );
	gtk_tree_view_column_set_visible( column, FALSE );
	gtk_tree_view_append_column( view, column );
*/
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "text", COL_FRIENDLY_NAME, NULL );
	gtk_tree_view_column_set_title( column, Text(INFO_FRIENDLY_NAME) );
	gtk_tree_view_append_column( view, column );

	if ( show_product_id )
	{
		/* Show "ProductId" additionally so the user can distinguish his DS's */
		column = gtk_tree_view_column_new();
		renderer = gtk_cell_renderer_text_new();
		g_object_set( renderer, "xalign", 1.0, NULL );  /* right-aligned */
		gtk_tree_view_column_pack_start( column, renderer, TRUE );
		gtk_tree_view_column_set_attributes( column, renderer, "text", COL_PRODUCT_ID, NULL );
		gtk_tree_view_column_set_title( column, Text(INFO_PRODUCT_ID) );
		gtk_tree_view_append_column( view, column );

		store = gtk_list_store_new( 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
	}
	else
	{
		gtk_tree_view_column_set_min_width( column, 300 );  /* make the whole dialog title readable */

		store = gtk_list_store_new( 2, G_TYPE_STRING, G_TYPE_STRING );
	}

	gtk_tree_view_set_model( view, GTK_TREE_MODEL( store ) );
	g_object_unref( store );

	gtk_tree_view_set_enable_search( view, TRUE );
	gtk_tree_view_set_search_column( view, COL_FRIENDLY_NAME );

	/* Fill DS device list... */
	for ( i = 0; i < n; i++ )
	{
		const upnp_device *sr = sr_list[i].device;
		GtkTreeIter iter;

		ASSERT( sr != NULL );
		if ( sr == NULL ) continue;

		gtk_list_store_insert_with_values( store, &iter, i,
			COL_UDN, sr->UDN, COL_FRIENDLY_NAME, sr->friendlyName, -1 );

		if ( show_product_id )
		{
			if ( upnp_device_is( sr, upnp_serviceId_AVTransport ) )
			{
				gtk_list_store_set( store, &iter, COL_PRODUCT_ID, "A/V", -1 );
			}
			else if ( sr_list[i].product_id != 0 )
			{
				char value[16];
				sprintf( value, "%lu", sr_list[i].product_id );
				gtk_list_store_set( store, &iter, COL_PRODUCT_ID, value, -1 );
			}
			else
			{
				upnp_string value;

				if ( Linn_Volkano_ProductId( sr, &value, NULL ) )
				{
					gtk_list_store_set( store, &iter, COL_PRODUCT_ID, value, -1 );
					upnp_free_string( value );
				}
				else
				{
					gtk_list_store_set( store, &iter, COL_PRODUCT_ID, "-", -1 );
				}
			}
		}

		if ( sr == device )
		{
			gtk_tree_selection_select_iter( selection, &iter );
			;;;  /* TODO: Make sure selected item is visible on startup */
		}
	}

	g_free( sr_list );

	ud.set_as_default = GTK_TOGGLE_BUTTON( gtk_check_button_new_with_label( Text(SET_AS_DEFAULT) ) );

	gtk_box_pack_start( GTK_BOX(ud.dialog->vbox), GTK_WIDGET( view ), TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(ud.dialog->vbox), GTK_WIDGET( ud.set_as_default ), FALSE, FALSE, 0 );
	gtk_widget_show_all( GTK_WIDGET(ud.dialog) );

	ud.device = device;
	if ( device == NULL )
	{
		gtk_tree_selection_unselect_all( selection );  /* Note: must be called after gtk_widget_show_all() */
		gtk_dialog_set_response_sensitive( ud.dialog, GTK_RESPONSE_OK, FALSE );
	}
	else
	{
		select_renderer_selection_changed( selection, &ud );
	}

	g_signal_connect( G_OBJECT(selection), "changed", GTK_SIGNAL_FUNC(select_renderer_selection_changed), &ud );
	g_signal_connect( G_OBJECT(view), "row-activated", GTK_SIGNAL_FUNC(select_renderer_row_activated), &ud );

	if ( gtk_dialog_run( ud.dialog ) == GTK_RESPONSE_OK )
	{
		gboolean active;

		ASSERT( ud.device != NULL );

		active = gtk_toggle_button_get_active( ud.set_as_default );
		if ( !IsDefaultMediaRenderer( ud.device ) && active )
		{
			SetDefaultMediaRenderer( ud.device );
			SaveSettings( TRUE );
		}
		else if ( IsDefaultMediaRenderer( ud.device ) && !active )
		{
			SetDefaultMediaRenderer( NULL );
			SaveSettings( TRUE );
		}
	}
	else
	{
		ud.device = NULL;
	}

	gtk_widget_destroy( GTK_WIDGET(ud.dialog) );

	TRACE(( "ChooseRenderer() = %s\n", (ud.device != NULL) ? ud.device->friendlyName : "<NULL>" ));
	return ud.device;
}

/*-----------------------------------------------------------------------------------------------*/

gboolean IsDefaultMediaRenderer( const upnp_device *device )
{
	return Settings->ssdp.default_media_renderer != NULL &&
	       strcmp( device->UDN, Settings->ssdp.default_media_renderer ) == 0;
}

void SetDefaultMediaRenderer( const upnp_device *device )
{
	g_free( Settings->ssdp.default_media_renderer );
	Settings->ssdp.default_media_renderer = (device != NULL) ? g_strdup( device->UDN ) : NULL;
}

/*-----------------------------------------------------------------------------------------------*/

static void select_renderer( GtkTreeModel *model, GtkTreeIter *iter, struct select_renderer_user_data *ud )
{
	upnp_device *sr;
	gchar *UDN = NULL;

	ud->device = NULL;

	gtk_tree_model_get( model, iter, COL_UDN, &UDN, -1 );
	for ( sr = upnp_get_first_av_transport(); sr != NULL; sr = upnp_get_next_av_transport( sr ) )
	{
		if ( UDN != NULL && strcmp( sr->UDN, UDN ) == 0 )
		{
			ud->device = sr;
		}
	}
	if ( UDN != NULL ) g_free( UDN );

	gtk_toggle_button_set_active( ud->set_as_default, IsDefaultMediaRenderer( ud->device ) );
	gtk_dialog_set_response_sensitive( ud->dialog, GTK_RESPONSE_OK, ud->device != NULL );
}

void select_renderer_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	struct select_renderer_user_data *ud = (struct select_renderer_user_data *)user_data;

	GtkTreeModel *model;
	GtkTreeIter iter;

	TRACE(( "## select_renderer_selection_changed()\n" ));

	if ( gtk_tree_selection_get_selected( selection, &model, &iter ) )
	{
		select_renderer( model, &iter, ud );
	}
}

void select_renderer_row_activated( GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data )
{
	struct select_renderer_user_data *ud = (struct select_renderer_user_data *)user_data;

	GtkTreeModel *model = gtk_tree_view_get_model( view );
	GtkTreeIter iter;

	TRACE(( "## select_renderer_row_activated()\n" ));

	if ( gtk_tree_model_get_iter( model, &iter, path ) )
	{
		select_renderer( model, &iter, ud );
		gtk_dialog_response( ud->dialog, GTK_RESPONSE_OK );
	}
}

/*-----------------------------------------------------------------------------------------------*/
