/*
 * Copyright (c) 2007, 2008 Czirkos Zoltan <cirix@fw.hu>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include "cavedb.h"
#include "gtkgfx.h"
#include "caveset.h"
#include "settings.h"
#include "util.h"
#include "gtkui.h"
#include "config.h"

/* pixbufs of icons and the like */
#include "icons.h"
/* title image and icon */
#include "title.h"

static char *caveset_filename=NULL;
static char *last_folder=NULL;



void
gd_create_stock_icons (void)
{
	static struct {
		const guint8 *data;
		char *stock_id;
	} icons[]={
		{ cave_editor, GD_ICON_CAVE_EDITOR},
		{ move, GD_ICON_EDITOR_MOVE},
		{ add_join, GD_ICON_EDITOR_JOIN},
		{ add_freehand, GD_ICON_EDITOR_FREEHAND},
		{ add_point, GD_ICON_EDITOR_POINT},
		{ add_line, GD_ICON_EDITOR_LINE},
		{ add_rectangle, GD_ICON_EDITOR_RECTANGLE},
		{ add_filled_rectangle, GD_ICON_EDITOR_FILLRECT},
		{ add_raster, GD_ICON_EDITOR_RASTER},
		{ add_fill_border, GD_ICON_EDITOR_FILL_BORDER},
		{ add_fill_replace, GD_ICON_EDITOR_FILL_REPLACE},
		{ add_maze, GD_ICON_EDITOR_MAZE},
		{ add_maze_uni, GD_ICON_EDITOR_MAZE_UNI},
		{ add_maze_braid, GD_ICON_EDITOR_MAZE_BRAID},
		{ snapshot, GD_ICON_SNAPSHOT},
		{ restart_level, GD_ICON_RESTART_LEVEL},
		{ random_fill, GD_ICON_RANDOM_FILL},
		{ award, GD_ICON_AWARD},
		{ to_top, GD_ICON_TO_TOP},
		{ to_bottom, GD_ICON_TO_BOTTOM},
		{ object_on_all, GD_ICON_OBJECT_ON_ALL},
		{ object_not_on_all, GD_ICON_OBJECT_NOT_ON_ALL},
		{ object_not_on_current, GD_ICON_OBJECT_NOT_ON_CURRENT},
	};

	GtkIconFactory *factory;
	int i;

	factory=gtk_icon_factory_new();
	for (i=0; i < G_N_ELEMENTS (icons); i++) {
		GtkIconSet *iconset;
		GdkPixbuf *pixbuf;

		pixbuf=gdk_pixbuf_new_from_inline (-1, icons[i].data, FALSE, NULL);
		iconset=gtk_icon_set_new_from_pixbuf(pixbuf);
		g_object_unref (pixbuf);
		gtk_icon_factory_add (factory, icons[i].stock_id, iconset);
	}
	gtk_icon_factory_add_default (factory);
	g_object_unref (factory);
}


GdkPixbuf *
gd_icon (void)
{
	return gdk_pixbuf_new_from_inline (-1, gdash, FALSE, NULL);
}

/* create and return an array of pixmaps, which contain the title animation.
   the array is one pointer larger than all frames; the last pointer is a null.
   up to the caller to free.
 */
GdkPixmap **
gd_create_title_animation (void)
{
	GdkPixbuf *screen;
	GdkPixbuf *tile;
	GdkPixbuf *frame;
	GdkPixbuf *bigone;
	GdkPixmap **pixmaps;
	int i;
	int x, y;
	int w, h, tw, th;

	/* the screen */
	screen=gdk_pixbuf_new_from_inline (-1, gdash_screen, FALSE, NULL);
	w=gdk_pixbuf_get_width(screen);
	h=gdk_pixbuf_get_height(screen);

	/* the tile to be put under the screen */
	tile=gdk_pixbuf_new_from_inline (-1, gdash_tile, FALSE, NULL);
	tw=gdk_pixbuf_get_width(tile);
	th=gdk_pixbuf_get_height(tile);

	/* do not allow more than 40 frames of animation */
	g_assert(th<40);

	/* create a big image, which is one tile larger than the title image size */
	bigone=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h+th);
	/* and fill it with the tile. */
	for (y=0; y<h+th; y+=th)
		for (x=0; x<w; x+=th)
			gdk_pixbuf_copy_area(tile, 0, 0, tw, th, bigone, x, y);

	g_object_unref(tile);

	pixmaps=g_new0(GdkPixmap *, th+1);	/* 'th' number of images, and a NULL to the end */
	frame=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
	for (i=0; i<th; i++) {
		GdkPixbuf *scaled;
		/* copy part of the big tiled image */
		gdk_pixbuf_copy_area(bigone, 0, i, w, h, frame, 0, 0);
		/* and composite it with the title image */
		gdk_pixbuf_composite(screen, frame, 0, 0, w, h, 0, 0, 1, 1, GDK_INTERP_NEAREST, 255);

		/* scale the title screen the same way as we scale the game */
		scaled=gd_pixbuf_scale(frame, gd_cell_scale_game);
		if (gd_pal_emulation_game)
			gd_pal_pixbuf(scaled);
		pixmaps[i]=gdk_pixmap_new (gdk_get_default_root_window(), gdk_pixbuf_get_width(scaled), gdk_pixbuf_get_height(scaled), -1);
		gdk_draw_pixbuf(pixmaps[i], NULL, scaled, 0, 0, 0, 0, gdk_pixbuf_get_width(scaled), gdk_pixbuf_get_height(scaled), GDK_RGB_DITHER_MAX, 0, 0);
		g_object_unref(scaled);
	}
	g_object_unref(bigone);
	g_object_unref(frame);

	g_object_unref(screen);

	return pixmaps;
}





/************
 *
 * functions for the settings window.
 * these implement the list view, which shows available graphics.
 *
 */
enum {
	THEME_COL_FILENAME,
	THEME_COL_NAME,
	THEME_COL_PIXBUF,
	NUM_THEME_COLS
};

static gboolean
find_image_in_store(GtkTreeModel *store, const gchar *find_name, GtkTreeIter *set_iter)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(store, &iter)) {
		do {
			char *stored_name;

			/* get filename from store */
			gtk_tree_model_get(store, &iter, THEME_COL_FILENAME, &stored_name, -1);
			/* if find_name is null and stored_name is null: that is the builtin theme, so they are equal! */
			/* otherwise, compare the strings. */
			if ((!find_name && !stored_name) || (find_name && stored_name && g_str_equal(find_name, stored_name))) {
				if (set_iter)
					*set_iter=iter;
				return TRUE;
			}
		} while (gtk_tree_model_iter_next(store, &iter));
	}

	/* not found: return false */
	return FALSE;
}

static GtkTreeIter *
add_image_to_store (GtkListStore *store, const char *filename)
{
	static GtkTreeIter iter;
	GdkPixbuf *pixbuf, *cell_pixbuf;
	char *seen_name;
	int cell_size;
	int cell_num=gd_elements[O_PLAYER].image_game;	/* load the image of the player from every pixbuf */
	GError *error=NULL;

	pixbuf=gdk_pixbuf_new_from_file(filename, &error);
	if (error) {
		/* unable to load image - silently ignore. */
		/* if we were called from add_dir_to_store, it is ok to ignore the error, as the file will not
		   be visible in the prefs window. */
		/* if we are called from the add_theme button, the file is already checked. */
		g_error_free(error);
		return NULL;
	}
	/* check if pixbuf is ok */
	if (gd_is_pixbuf_ok_for_theme(pixbuf)!=NULL) {
		g_object_unref(pixbuf);
		return NULL;
	}

	cell_size=gdk_pixbuf_get_width (pixbuf) / NUM_OF_CELLS_X;
	cell_pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, cell_size, cell_size);
	gdk_pixbuf_copy_area(pixbuf, (cell_num % NUM_OF_CELLS_X) * cell_size, (cell_num / NUM_OF_CELLS_X) * cell_size, cell_size, cell_size, cell_pixbuf, 0, 0);
	g_object_unref(pixbuf);

	/* check list store to find if this file is already added. may happen if a theme is overwritten by the add theme button */
	if (find_image_in_store(GTK_TREE_MODEL(store), filename, &iter))
		gtk_list_store_remove(store, &iter);

	/* add to list */
	seen_name=g_filename_display_basename(filename);
	if (strrchr(seen_name, '.'))	/* remove extension */
		*strrchr(seen_name, '.')='\0';
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, THEME_COL_FILENAME, filename, THEME_COL_NAME, seen_name, THEME_COL_PIXBUF, cell_pixbuf, -1);
	g_free(seen_name);
	g_object_unref(cell_pixbuf);	/* the store has its own ref */

	return &iter;
}

static void
add_dir_to_store(GtkListStore *store, const char *dirname)
{
	GDir *dir;
	GError *error=NULL;
	/* search directory */
	dir=g_dir_open(dirname, 0, &error);
	if (!error) {
		const char *name;

		while ((name=g_dir_read_name(dir))) {
			char *filename=g_build_filename(dirname, name, NULL);

			/* if image file can be loaded and proper size for theme */
			if (g_file_test(filename, G_FILE_TEST_IS_REGULAR) && gd_is_image_ok_for_theme(filename)==NULL) {
				add_image_to_store(store, filename);
			}
			g_free(filename);
		}
		g_dir_close(dir);
	} else {
		/* unable to open directory */
		g_warning("%s", error->message);
		g_error_free(error);
	}
}

GtkListStore *
gd_create_themes_list()
{
	GtkListStore *store;
	GtkTreeIter iter;

	store=gtk_list_store_new(NUM_THEME_COLS, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);

	/* default builtin theme */
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, THEME_COL_FILENAME, NULL, THEME_COL_NAME, _("Default"), THEME_COL_PIXBUF, gd_pixbuf_for_builtin, -1);

	/* add themes found in config directories */
	add_dir_to_store(store, gd_system_data_dir);
	add_dir_to_store(store, gd_user_config_dir);

	/* if the current theme cannot be found in the store for some reason, add it now. */
	if (!find_image_in_store(GTK_TREE_MODEL(store), gd_theme, NULL))
		add_image_to_store(store, gd_theme);

	return store;
}



/****************************
 *
 * settings window
 *
 */
typedef struct pref_info {
	GtkWidget *dialog;
	GtkWidget *treeview, *sizecombo_game, *sizecombo_editor;
	GtkWidget *image_game, *image_editor;
} pref_info;

static void
settings_window_update(pref_info* info)
{
	GdkPixbuf *orig_pb;
	GdkPixbuf *pb;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(info->treeview)), &model, &iter))
		return;
	gtk_tree_model_get(model, &iter, THEME_COL_PIXBUF, &orig_pb, -1);

	/* sample for game */
	pb=gd_pixbuf_scale(orig_pb, gtk_combo_box_get_active(GTK_COMBO_BOX(info->sizecombo_game)));
	if (gd_pal_emulation_game)
		gd_pal_pixbuf(pb);
	gtk_image_set_from_pixbuf(GTK_IMAGE(info->image_game), pb);
	g_object_unref(pb);

	/* sample for editor */
	pb=gd_pixbuf_scale(orig_pb, gtk_combo_box_get_active(GTK_COMBO_BOX(info->sizecombo_editor)));
	if (gd_pal_emulation_editor)
		gd_pal_pixbuf(pb);
	gtk_image_set_from_pixbuf(GTK_IMAGE(info->image_editor), pb);
	g_object_unref(pb);
}

static void
settings_window_changed(gpointer object, gpointer data)
{
	settings_window_update((pref_info *)data);
}

/* callback for a toggle button. data is a pointer to a gboolean. */
static void
settings_window_toggle(GtkWidget *widget, gpointer data)
{
	gboolean *bl=(gboolean *)data;

	*bl=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

/* callback for a spin button. data is a pointer to a gboolean. */
static void
settings_window_value_change(GtkWidget *widget, gpointer data)
{
	int *value=(int *) data;

	*value=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

/* return a list of image gtk_image_filter's. */
/* they have floating reference. */
/* the list is to be freed by the caller. */
static GList *
image_load_filters()
{
	GSList *formats=gdk_pixbuf_get_formats();
	GSList *iter;
	GtkFileFilter *all_filter;
	GList *filters=NULL;	/* new list of filters */

	all_filter=gtk_file_filter_new();
	gtk_file_filter_set_name(all_filter, _("All image files"));

	/* iterate the list of formats given by gdk. create file filters for each. */
	for (iter=formats; iter!=NULL; iter=iter->next) {
		GdkPixbufFormat *frm=iter->data;

		if (!gdk_pixbuf_format_is_disabled(frm)) {
			GtkFileFilter *filter;
			char **extensions;
			int i;

			filter=gtk_file_filter_new();
			gtk_file_filter_set_name(filter, gdk_pixbuf_format_get_description(frm));
			extensions=gdk_pixbuf_format_get_extensions(frm);
			for (i=0; extensions[i]!=NULL; i++) {
				char *pattern;

				pattern=g_strdup_printf("*.%s", extensions[i]);
				gtk_file_filter_add_pattern(filter, pattern);
				gtk_file_filter_add_pattern(all_filter, pattern);
				g_free(pattern);
			}
			g_strfreev(extensions);

			filters=g_list_append(filters, filter);
		}
	}
	g_slist_free(formats);

	/* add "all image files" filter */
	filters=g_list_prepend(filters, all_filter);

	return filters;
}

static void
add_theme_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	GList *filters;
	GList *iter;
	int result;
	char *filename=NULL;
	pref_info *info=(pref_info *)data;

	dialog=gtk_file_chooser_dialog_new (_("Add Theme from Image File"), GTK_WINDOW(info->dialog), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	/* obtain list of image filters, and add all to the window */
	filters=image_load_filters();
	for (iter=filters; iter!=NULL; iter=iter->next)
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), GTK_FILE_FILTER(iter->data));
	g_list_free(filters);

	result=gtk_dialog_run (GTK_DIALOG (dialog));
	if (result==GTK_RESPONSE_ACCEPT)
		filename=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));

	if (filename) {
		const char *error_msg;

		error_msg=gd_is_image_ok_for_theme(filename);
		if (error_msg==NULL) {
			/* make up new filename */
			char *basename, *new_filename;
			GError *error=NULL;
			gchar *contents;
			gsize length;

			basename=g_path_get_basename(filename);
			new_filename=g_build_path(G_DIR_SEPARATOR_S, gd_user_config_dir, basename, NULL);
			g_free(basename);

			/* if file not exists, or exists BUT overwrite allowed */
			if (!g_file_test(new_filename, G_FILE_TEST_EXISTS) || gd_ask_overwrite(new_filename)) {
				/* copy theme to user config directory */
				if (g_file_get_contents(filename, &contents, &length, &error) && g_file_set_contents(new_filename, contents, length, &error)) {
					GtkTreeIter *iter;

					iter=add_image_to_store(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(info->treeview))), new_filename);
					if (iter)						/* select newly added. if there was an error, the old selection was not cleared */
						gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(info->treeview)), iter);
				}
				else {
					gd_errormessage(error->message, NULL);
					g_error_free(error);
				}
			}
			g_free(new_filename);
		}
		else
			gd_errormessage(_("The selected image cannot be used as a GDash theme."), error_msg);
		g_free(filename);
	}

	gtk_widget_destroy (dialog);
}

static void
remove_theme_cb(GtkWidget *widget, gpointer data)
{
	pref_info *info=(pref_info *)data;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	char *filename, *seen_name;
	GtkWidget *dialog;
	int result;

	selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(info->treeview));
	/* if no row selected for some reason, don't care. return now. */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	gtk_tree_model_get(model, &iter, THEME_COL_FILENAME, &filename, THEME_COL_NAME, &seen_name, -1);
	/* do not thelete the built-in theme */
	if (!filename)
		return;
	dialog=gtk_message_dialog_new(GTK_WINDOW(info->dialog), 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, _("Do you really want to remove theme '%s'?"), seen_name);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dialog), _("The image file of the theme is '%s'."), filename);

	result=gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);
	if (result==GTK_RESPONSE_YES) {
		/* remove */
		if (g_unlink(filename)==0) {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			/* current file removed - select first iter found in list */
			if (gtk_tree_model_get_iter_first(model, &iter))
				gtk_tree_selection_select_iter(selection, &iter);
		} else
			gd_errormessage(_("Cannot delete the image file."), filename);
	}
}

GtkWidget *
gd_combo_box_new_from_stringv(const char **str)
{
	GtkWidget *combo;
	int i;
	
	combo=gtk_combo_box_new_text();
	for (i=0; str[i]!=NULL; i++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _(str[i]));	/* also translate */
	
	return combo;
}

/* when a combo box created from a stringv gets updated. data should point to the integer to be changed. */
static void
combo_box_stringv_changed(GtkWidget *widget, gpointer data)
{
	int *ptr=(int *) data;
	
	*ptr=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	/* if nothing selected (for some reason), set to zero. */
	if (*ptr==-1)
		*ptr=0;
}


void
gd_preferences (GtkWidget *parent)
{
	typedef enum _settingtype {
		TypeLabel,
		TypeBoolean,
		TypePercent,
		TypeStringv,
		TypeNewColumn,
	} SettingType;
	struct {
		SettingType type;
		const char *name;
		const char *description;
		gpointer value;
		gboolean update;			/* if sample pixbuf should be updated when changing this option */
		const char **stringv;
	} check_buttons[]={
		{TypeLabel, N_("<b>Language</b> (requires restart)"), NULL, NULL},
		{TypeStringv, NULL, N_("The language of the application. Requires restart!"), &gd_language, FALSE, gd_languages_names},
		{TypeLabel, N_("<b>Cave options</b>"), NULL, NULL},
		{TypeBoolean, N_("Ease levels"), N_("Make caves easier to play. Only one diamonds to collect, more time available, no expanding walls... This might render some caves unplayable!"), &gd_easy_play},
		{TypeBoolean, N_("Mouse play (experimental!)"), N_("Use the mouse to play. The player will follow the cursor!"), &gd_mouse_play, FALSE},
		{TypeBoolean, N_("All caves selectable"), N_("All caves and intermissions can be selected at game start."), &gd_all_caves_selectable, FALSE},
		{TypeBoolean, N_("Import as all caves selectable"), N_("Original, C64 games are imported not with A, E, I, M caves selectable, but all caves (ABCD, EFGH... excluding intermissions). This does not affect BDCFF caves."), &gd_import_as_all_caves_selectable, FALSE},
		{TypeBoolean, N_("Use BDCFF highscore"), N_("Use BDCFF highscores. GDash rather stores highscores in its own configuration directory, instead of saving them in "
			" the *.bd file."), &gd_use_bdcff_highscore, FALSE},
#ifdef GD_SOUND
		{TypeBoolean, N_("Time as min:sec"), N_("Show times in minutes and seconds, instead of seconds only."), &gd_time_min_sec, FALSE},
		{TypeLabel, N_("<b>Sound options</b> (require restart)"), NULL, NULL},
		{TypeBoolean, N_("Sound"), N_("Play sounds. Enabling this setting requires a restart!"), &gd_sdl_sound, FALSE},
		{TypeBoolean, N_("Classic sounds only"), N_("Play only classic sounds taken from the original game."), &gd_classic_sound, FALSE},
		{TypeBoolean, N_("16-bit mixing"), N_("Use 16-bit mixing of sounds. Try changing this setting if sound is clicky. Changing this setting requires a restart!"), &gd_sdl_16bit_mixing, FALSE},
		{TypeBoolean, N_("44kHz mixing"), N_("Use 44kHz mixing of sounds. Try changing this setting if sound is clicky. Changing this setting requires a restart!"), &gd_sdl_44khz_mixing, FALSE},
#endif
		{TypeNewColumn, },
		{TypeLabel, N_("<b>Display options</b>"), NULL, NULL},
		{TypeBoolean, N_("Random colours"), N_("Use randomly selected colours for C64 graphics."), &gd_random_colors, FALSE},
/*
   XXX currently dirt mod is not shown to the user.
		{N_("Allow dirt mod"), N_("Enable caves to use alternative dirt graphics. This applies only to imported caves, not BDCFF (*.bd) files."), &allow_dirt_mod, FALSE},
*/
		{TypeBoolean, N_("PAL emulation for game"), N_("Use PAL emulated graphics, ie. lines are striped."), &gd_pal_emulation_game, TRUE},
		{TypeBoolean, N_("PAL emulation for editor"), N_("Use PAL emulated graphics, ie. lines are striped."), &gd_pal_emulation_editor, TRUE},
		{TypePercent,    N_("PAL scanline shade (%)"), N_("Darker rows for PAL emulation."), &gd_pal_emu_scanline_shade, TRUE},
		{TypeLabel, N_("<b>C64 palette</b>"), NULL, NULL},
		{TypeStringv, NULL, N_("The colour palette for games imported from C64 files. Also affects BDCFF games, which have C64 colors!"), &gd_c64_palette, FALSE, gd_color_get_c64_palette_names()},
		{TypeLabel, N_("<b>Atari palette</b>"), NULL, NULL},
		{TypeStringv, NULL, N_("The colour palette for imported Atari games."), &gd_atari_palette, FALSE, gd_color_get_atari_palette_names()},
	};

	GtkWidget *dialog, *table, *label, *button;
	pref_info info;
	int i, row, col;
	GtkWidget *sw, *align, *hbox;
	GtkListStore *store;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	char *filename;

	dialog=gtk_dialog_new_with_buttons (_("GDash Preferences"), (GtkWindow *) parent, GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
	info.dialog=dialog;
	gtk_window_set_resizable (GTK_WINDOW(dialog), FALSE);

	/* remove theme button */
	button=gtk_button_new_with_mnemonic(_("_Remove theme"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remove_theme_cb), &info);

	/* add theme button */
	button=gtk_button_new_with_mnemonic(_("_Add theme"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(add_theme_cb), &info);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	hbox=gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox);
	table=gtk_table_new (1, 1, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, 0, 0, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);

	/* game booleans */
	row=0;
	col=0;
	for (i=0; i<G_N_ELEMENTS(check_buttons); i++) {
		GtkWidget *widget, *hbox;

		switch (check_buttons[i].type) {
			case TypeLabel:
				label=gtk_label_new(NULL);
				gtk_label_set_markup(GTK_LABEL(label), _(check_buttons[i].name));
				gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
				gtk_table_attach_defaults (GTK_TABLE (table), label, col, col+2, row, row+1);
				break;

			case TypeBoolean:
				widget=gtk_check_button_new_with_label(_(check_buttons[i].name));
				gtk_widget_set_tooltip_text(widget, _(check_buttons[i].description));
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), *(gboolean *)check_buttons[i].value);
				gtk_table_attach_defaults (GTK_TABLE (table), widget, col+1, col+2, row, row+1);
				g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(settings_window_toggle), check_buttons[i].value);
				/* if sample pixbuf should be updated */
				if (check_buttons[i].update)
					g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(settings_window_changed), &info);
				break;

			case TypePercent:
				hbox=gtk_hbox_new(FALSE, 6);
				widget=gtk_spin_button_new_with_range(0, 100, 5);
				gtk_widget_set_tooltip_text(widget, _(check_buttons[i].description));
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), *(int *)check_buttons[i].value);
				g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(settings_window_value_change), check_buttons[i].value);
				if (check_buttons[i].update)
					g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(settings_window_changed), &info);
				gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				gtk_box_pack_start_defaults(GTK_BOX(hbox), gd_label_new_printf(_(check_buttons[i].name)));

				gtk_table_attach_defaults (GTK_TABLE (table), hbox, col+1, col+2, row, row+1);
				break;
			
			case TypeStringv:
				g_assert(!check_buttons[i].update);
				widget=gd_combo_box_new_from_stringv(check_buttons[i].stringv);
				gtk_combo_box_set_active(GTK_COMBO_BOX(widget), *(int *)check_buttons[i].value);
				g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(combo_box_stringv_changed), check_buttons[i].value);
				gtk_table_attach_defaults(GTK_TABLE(table), widget, col+1, col+2, row, row+1);
				break;

			case TypeNewColumn:
				col+=2;
				row=-1;	/* so row++ under will result in zero */
				break;
		}
		row++;
	}

	/* gfx */
	store=gd_create_themes_list();
	info.treeview=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_widget_set_tooltip_text(info.treeview, _("This is the list of available themes. Use the Add Theme button to install a new one."));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(info.treeview), FALSE);	/* don't need headers as everything is self-explaining */
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(info.treeview), -1, "", gtk_cell_renderer_pixbuf_new(), "pixbuf", THEME_COL_PIXBUF, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(info.treeview), -1, "", gtk_cell_renderer_text_new(), "text", THEME_COL_NAME, NULL);
	selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(info.treeview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
	if (find_image_in_store(GTK_TREE_MODEL(store), gd_theme, &iter))
		gtk_tree_selection_select_iter(selection, &iter);

	sw=gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(sw), info.treeview);

	table=gtk_table_new (1, 1, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, 0, 0, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);

	label=gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Theme</b>"));
	gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
	gtk_table_attach(GTK_TABLE (table), label, 0, 3, 0, 1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

	gtk_table_attach_defaults(GTK_TABLE(table), sw, 1, 3, 1, 2);

	/* cells size combo for game */
	gtk_table_attach(GTK_TABLE(table), gd_label_new_printf("<b>Game</b>"), 1, 2, 2, 3, GTK_EXPAND|GTK_FILL, 0, 0, 0);
	info.sizecombo_game=gd_combo_box_new_from_stringv(gd_scaling_name);
	gtk_combo_box_set_active(GTK_COMBO_BOX(info.sizecombo_game), gd_cell_scale_game);
#if 0	
	gtk_combo_box_new_text();
	for (i=0; i<GD_SCALING_MAX; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(info.sizecombo_game), gettext(gd_scaling_name[i]));
		if (gd_cell_scale_game==i)
			gtk_combo_box_set_active(GTK_COMBO_BOX(info.sizecombo_game), i);
	}
#endif
	gtk_table_attach(GTK_TABLE(table), info.sizecombo_game, 1, 2, 3, 4, GTK_EXPAND|GTK_FILL, 0, 0, 0);
	/* image for game */
	align=gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_set_size_request(align, 64, 64);
	info.image_game=gtk_image_new();
	gtk_container_add(GTK_CONTAINER(align), info.image_game);
	gtk_table_attach(GTK_TABLE(table), align, 1, 2, 4, 5, GTK_EXPAND|GTK_FILL, 0, 0, 0);

	/* cells size combo for editor */
	gtk_table_attach(GTK_TABLE(table), gd_label_new_printf("<b>Editor</b>"), 2, 3, 2, 3, GTK_EXPAND|GTK_FILL, 0, 0, 0);
	info.sizecombo_editor=gd_combo_box_new_from_stringv(gd_scaling_name);
	gtk_combo_box_set_active(GTK_COMBO_BOX(info.sizecombo_editor), gd_cell_scale_editor);
	gtk_table_attach(GTK_TABLE(table), info.sizecombo_editor, 2, 3, 3, 4, GTK_EXPAND|GTK_FILL, 0, 0, 0);
	/* image for editor */
	align=gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_set_size_request(align, 64, 64);
	info.image_editor=gtk_image_new();
	gtk_container_add(GTK_CONTAINER(align), info.image_editor);
	gtk_table_attach(GTK_TABLE(table), align, 2, 3, 4, 5, GTK_EXPAND|GTK_FILL, 0, 0, 0);

	/* add the "changed" signal handlers only here, as by appending the texts, the changes signal whould have been called */
	g_signal_connect(G_OBJECT(info.sizecombo_game), "changed", G_CALLBACK(settings_window_changed), &info);
	g_signal_connect(G_OBJECT(info.sizecombo_editor), "changed", G_CALLBACK(settings_window_changed), &info);
	g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(settings_window_changed), &info);

	/* update images (for the first time) before showing the window */
	settings_window_update (&info);

	/* run dialog */
	gtk_widget_show_all (dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	/* get cell sizes */
	gd_cell_scale_game=gtk_combo_box_get_active(GTK_COMBO_BOX(info.sizecombo_game));
	gd_cell_scale_editor=gtk_combo_box_get_active(GTK_COMBO_BOX(info.sizecombo_editor));
	/* get theme file name from table */
	if (gtk_tree_selection_get_selected(selection, NULL, &iter))
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, THEME_COL_FILENAME, &filename, -1);
	else
		filename=NULL;
	gtk_widget_destroy (dialog);

	/* load new theme */
	if (filename) {
		if (gd_loadcells_file(filename)) {
			/* if successful, remember theme setting */
			g_free(gd_theme);
			gd_theme=g_strdup(filename);
		} else {
			/* unable to load; switch to default theme */
			g_warning("%s: unable to load theme, switching to built-in", filename);
			g_free(gd_theme);
			gd_theme=NULL;
		}
	}
	else {
		/* no filename means the builtin */
		g_free(gd_theme);
		gd_theme=NULL;
		gd_loadcells_default();
	}
}















enum {
	HS_COLUMN_RANK,
	HS_COLUMN_NAME,
	HS_COLUMN_SCORE,
	HS_COLUMN_BOLD,
	NUM_COLUMNS,
};

/* a cave name is selected, update the list store with highscore data */
#define GD_LISTSTORE "gd-liststore-for-combo"
#define GD_HIGHLIGHT_CAVE "gd-highlight-cave"
#define GD_HIGHLIGHT_RANK "gd-highlight-rank"
static void
hs_cave_combo_changed(GtkComboBox *widget, gpointer data)
{
	GtkListStore *store=GTK_LIST_STORE(g_object_get_data(G_OBJECT(widget), GD_LISTSTORE));
	int highlight_cave=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), GD_HIGHLIGHT_CAVE));
	int highlight_rank=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), GD_HIGHLIGHT_RANK));
	int i;
	GdHighScore *scores;

	gtk_list_store_clear(store);
	i=gtk_combo_box_get_active(widget);
	if (i==0)
		scores=gd_caveset_data->highscore;
	else
		scores=gd_return_nth_cave(i-1)->highscore;

	for (i=0; i<GD_HIGHSCORE_NUM; i++)
		if (scores[i].score>0) {
			GtkTreeIter iter;

			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, HS_COLUMN_RANK, i+1, HS_COLUMN_NAME, scores[i].name, HS_COLUMN_SCORE, scores[i].score,
				HS_COLUMN_BOLD, (i==highlight_cave && i==highlight_rank)?PANGO_WEIGHT_BOLD:PANGO_WEIGHT_NORMAL, -1);
		}
}

static void
hs_clear_highscore(GtkWidget *widget, gpointer data)
{
	GtkComboBox *combo=GTK_COMBO_BOX(data);
	GtkListStore *store=GTK_LIST_STORE(g_object_get_data(G_OBJECT(combo), GD_LISTSTORE));
	int i;
	GdHighScore *scores;

	i=gtk_combo_box_get_active(combo);
	if (i==0)
		scores=gd_caveset_data->highscore;
	else
		scores=gd_return_nth_cave(i-1)->highscore;

	/* if there is any entry, delete */
	gd_clear_highscore(scores);
	gtk_list_store_clear(store);
}

void
gd_show_highscore(GtkWidget *parent, Cave *cave, gboolean show_clear_button, Cave *highlight_cave, int highlight_rank)
{
	GtkWidget *dialog;
	int i;
	char *text;
	GtkListStore *store;
	GtkWidget *treeview, *sw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *combo;
	GList *iter;
	int hl_cave;

	/* dialog window */
	dialog=gtk_dialog_new_with_buttons(_("Highscores"), (GtkWindow *) parent, GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);

	store=gtk_list_store_new(NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
	treeview=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

	renderer=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes(_("Rank"), renderer, "text", HS_COLUMN_RANK, "weight", HS_COLUMN_BOLD, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	renderer=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", HS_COLUMN_NAME, "weight", HS_COLUMN_BOLD, NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	renderer=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes(_("Score"), renderer, "text", HS_COLUMN_SCORE, "weight", HS_COLUMN_BOLD, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	combo=gtk_combo_box_new_text();
	g_object_set_data(G_OBJECT(combo), GD_LISTSTORE, store);
	hl_cave=g_list_index(gd_caveset, highlight_cave);
	if (hl_cave==-1)
		hl_cave=0;
	g_object_set_data(G_OBJECT(combo), GD_HIGHLIGHT_CAVE, GINT_TO_POINTER(hl_cave));
	g_object_set_data(G_OBJECT(combo), GD_HIGHLIGHT_RANK, GINT_TO_POINTER(highlight_rank));
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(hs_cave_combo_changed), NULL);
	text=g_strdup_printf("[%s]", gd_caveset_data->name);
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	g_free(text);

	for (iter=gd_caveset, i=1; iter!=NULL; iter=iter->next, i++) {
		Cave *c=iter->data;

		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), c->name);

		/* if this one is the active, select it */
		if (c==cave)
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
	}

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), combo, FALSE, FALSE, 6);
	sw=gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sw), treeview);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), sw);

	/* clear button */
	if (show_clear_button) {
		GtkWidget *button;

		button=gtk_button_new_from_stock(GTK_STOCK_CLEAR);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(hs_clear_highscore), combo);
	}

	/* close button */
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);

	gtk_window_set_default_size(GTK_WINDOW(dialog), 240, 320);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
#undef GD_LISTSTORE
#undef GD_HIGHLIGHT_CAVE
#undef GD_HIGHLIGHT_RANK

static GtkWidget *
guess_active_toplevel()
{
    GtkWidget *parent=NULL;
    GList *toplevels, *iter;

    /* try to guess which window is active */
    toplevels=gtk_window_list_toplevels();
    for (iter=toplevels; iter!=NULL; iter=iter->next)
    	if (gtk_window_has_toplevel_focus(GTK_WINDOW(iter->data)))
    		parent=iter->data;
    g_list_free(toplevels);

    return parent;
}

/*
 * show a warning window
 */
static void
show_message (GtkMessageType type, const char *primary, const char *secondary)
{
    GtkWidget *dialog;

    dialog=gtk_message_dialog_new ((GtkWindow *) guess_active_toplevel(),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        type, GTK_BUTTONS_OK,
        primary);
    gtk_window_set_title(GTK_WINDOW(dialog), "GDash");
   	/* secondary message exists an is not empty string: */
    if (secondary && secondary[0]!=0)
        gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), secondary);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

void
gd_warningmessage (const char *primary, const char *secondary)
{
	show_message (GTK_MESSAGE_WARNING, primary, secondary);
}

void
gd_errormessage (const char *primary, const char *secondary)
{
	show_message (GTK_MESSAGE_ERROR, primary, secondary);
}

void
gd_infomessage (const char *primary, const char *secondary)
{
	show_message (GTK_MESSAGE_INFO, primary, secondary);
}

/* if necessary, ask the user if he doesn't want to save changes to cave */
gboolean
gd_discard_changes (GtkWidget *parent)
{
	GtkWidget *dialog, *button;
	gboolean discard;

	/* save highscore on every ocassion when the caveset is to be removed from memory */
	gd_save_highscore(gd_user_config_dir);

	/* caveset is not edited, so pretend user confirmed */
	if (!gd_caveset_edited)
		return TRUE;

	dialog=gtk_message_dialog_new ((GtkWindow *) parent, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("Cave set is edited. Discard changes?"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), gd_caveset_data->name);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
	/* create a discard button with a trash icon and Discard text */
	button=gtk_button_new_with_label (_("_Discard"));
	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_stock (GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON));
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_YES);

	discard=gtk_dialog_run (GTK_DIALOG (dialog))==GTK_RESPONSE_YES;
	gtk_widget_destroy (dialog);

	/* return button pressed */
	return discard;
}

gboolean
gd_ask_overwrite(const char *filename)
{
	gboolean result;
	char *sec;

	/* ask if overwrite file */
	sec=g_strdup_printf(_("The file (%s) already exists, and will be overwritten."), filename);
	result=gd_question_yesno(_("The file already exists. Do you want to overwrite it?"), sec);
	g_free(sec);

	return result;
}



static void
caveset_file_operation_successful(const char *filename)
{
	/* save successful, so remember filename */
	/* first we make a copy, as it is possible that filename==caveset_filename (the pointers!) */
	char *uri;

	/* add to recent chooser */
	if (g_path_is_absolute(filename))
		uri=g_filename_to_uri(filename, NULL, NULL);
	else {
		/* make an absolute filename if needed */
		char *absolute;
		char *currentdir;

		currentdir=g_get_current_dir();
		absolute=g_build_path(G_DIR_SEPARATOR_S, currentdir, filename, NULL);
		g_free(currentdir);
		uri=g_filename_to_uri(absolute, NULL, NULL);
		g_free(absolute);
	}
	gtk_recent_manager_add_item(gtk_recent_manager_get_default(), uri);
	g_free(uri);

	/* if it is a bd file, remember new filename */
	if (g_str_has_suffix(filename, ".bd")) {
		char *stored;

		/* first make copy, then free and set pointer. we might be called with filename=caveset_filename */
		stored=g_strdup(filename);
		g_free(caveset_filename);
		caveset_filename=stored;
	} else {
		g_free(caveset_filename);
		caveset_filename=NULL;
	}
}


/* save caveset to specified directory, and pop up error message if failed */
static void
caveset_save (const gchar *filename)
{
	gboolean saved;

	saved=gd_caveset_save(filename);
	if (!saved)
		gd_show_last_error(guess_active_toplevel());
	else
		caveset_file_operation_successful(filename);
}


void
gd_save_caveset_as (GtkWidget *parent)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	char *filename=NULL, *suggested_name;

	dialog=gtk_file_chooser_dialog_new (_("Save File As"), GTK_WINDOW(parent), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	filter=gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("BDCFF cave sets (*.bd)"));
	gtk_file_filter_add_pattern (filter, "*.bd");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);

	filter=gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files (*)"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);

	suggested_name=g_strdup_printf("%s.bd", gd_caveset_data->name);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), suggested_name);
	g_free(suggested_name);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		filename=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));

	/* check if .bd extension should be added */
	if (filename) {
		char *suffixed;

		/* if it has no .bd extension, add one */
		if (!g_str_has_suffix(filename, ".bd")) {
			suffixed=g_strdup_printf("%s.bd", filename);

			g_free(filename);
			filename=suffixed;
		}
	}

	/* if we have a filename, do the save */
	if (filename) {
		if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
			/* if exists, ask if overwrite */
			if (gd_ask_overwrite(filename))
				caveset_save(filename);
		} else
			/* if did not exist, simply save */
			caveset_save(filename);
	}
	g_free(filename);
	gtk_widget_destroy (dialog);
}

void
gd_save_caveset(GtkWidget *parent)
{
	if (!caveset_filename)
		/* if no filename remembered, rather start the save_as function, which asks for one. */
		gd_save_caveset_as(parent);
	else
		/* if given, save. */
		caveset_save(caveset_filename);
}




/* load a caveset, and remember its filename, it is a bdcff file. */
/* called after "open file" dialogs, and also from the main() of the gtk version */
gboolean
gd_open_caveset_in_ui(const char *filename, gboolean highscore_load_from_bdcff)
{
	gboolean loaded;

	gd_clear_error_flag();

	loaded=gd_caveset_load_from_file (filename, gd_user_config_dir);
	if (loaded)
		caveset_file_operation_successful(filename);

	/* if successful loading and this is a bd file, and we load highscores from our own config dir */
	if (!gd_has_new_error() && g_str_has_suffix(filename, ".bd") && !highscore_load_from_bdcff)
		gd_load_highscore(gd_user_config_dir);

	/* return true if successful, false if any error */
	return !gd_has_new_error();
}




void
gd_open_caveset (GtkWidget *parent, const char *directory)
{
	GtkWidget *dialog, *check;
	GtkFileFilter *filter;
	int result;
	char *filename=NULL;
	gboolean highscore_load_from_bdcff;
	int i;

	/* if caveset is edited, and user does not want to discard changes */
	if (!gd_discard_changes(parent))
		return;

	dialog=gtk_file_chooser_dialog_new (_("Open File"), (GtkWindow *) parent, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	check=gtk_check_button_new_with_mnemonic(_("Load _highscores from BDCFF file"));

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), check, FALSE, FALSE, 6);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), gd_use_bdcff_highscore);
	gtk_widget_show(check);

	filter=gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("GDash cave sets"));
	for (i=0; gd_caveset_extensions[i]!=NULL; i++)
		gtk_file_filter_add_pattern(filter, gd_caveset_extensions[i]);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	/* if callback shipped with a directory name, show that directory by default */
	if (directory)
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), directory);
	else
	if (last_folder)
		/* if we previously had an open command, the directory was remembered */
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), last_folder);
	else
		/* otherwise user home */
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), g_get_home_dir());

	result=gtk_dialog_run(GTK_DIALOG (dialog));
	if (result==GTK_RESPONSE_ACCEPT) {
		filename=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));
		g_free (last_folder);
		last_folder=gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(dialog));
	}
	/* read the state of the check button */
	highscore_load_from_bdcff=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
	gtk_widget_destroy (dialog);

	/* WINDOWS GTK+ 20080926 HACK XXX XXX XXX XXX */
	/* gtk bug - sometimes the above widget destroy creates an error message. */
	/* so we delete the error flag here. */
#ifdef G_OS_WIN32
	gd_clear_error_flag();
#endif

	if (filename)
		gd_open_caveset_in_ui(filename, highscore_load_from_bdcff);
	g_free (filename);

	if (gd_has_new_error())
		gd_show_last_error(parent);
}

/* load an internal caveset, after checking that the current one is to be saved or not */
void
gd_load_internal(GtkWidget *parent, int i)
{
	if (gd_discard_changes(parent)) {
		g_free(caveset_filename);
		caveset_filename=NULL;	/* forget cave filename, as this one is not loaded from a file... */

		gd_caveset_load_from_internal (i, gd_user_config_dir);
		gd_infomessage(_("Loaded game:"), gd_caveset_data->name);
	}
}



/* set a label's markup */
static void
label_set_markup_vprintf(GtkLabel *label, const char *format, va_list args)
{
	char *text;

	text=g_strdup_vprintf(format, args);
	gtk_label_set_markup(label, text);
	g_free(text);
}

/* create a label, and set its text by a printf; the text will be centered */
GtkWidget *
gd_label_new_printf_centered(const char *format, ...)
{
	va_list args;
	GtkWidget *label;

	label=gtk_label_new(NULL);

	va_start(args, format);
	label_set_markup_vprintf(GTK_LABEL(label), format, args);
	va_end(args);

	return label;
}

/* create a label, and set its text by a printf */
GtkWidget *
gd_label_new_printf(const char *format, ...)
{
	va_list args;
	GtkWidget *label;

	label=gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	va_start(args, format);
	label_set_markup_vprintf(GTK_LABEL(label), format, args);
	va_end(args);

	return label;
}

/* set a label's markup with a printf format string */
void
gd_label_set_markup_printf(GtkLabel *label, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	label_set_markup_vprintf(label, format, args);
	va_end(args);
}



void
gd_show_errors (GtkWidget *parent)
{
	/* create text buffer */
	GtkTextIter iter;
	GtkTextBuffer *buffer=gtk_text_buffer_new(NULL);
	GtkWidget *dialog, *sw, *view;
	GList *liter;
	int result;
	GdkPixbuf *pixbuf_error, *pixbuf_warning, *pixbuf_info;

	dialog=gtk_dialog_new_with_buttons (_("GDash Errors"), (GtkWindow *) parent, GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CLEAR, 1, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 512, 384);
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), sw);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* get text and show it */
	view=gtk_text_view_new_with_buffer (buffer);
	gtk_container_add (GTK_CONTAINER (sw), view);
	g_object_unref (buffer);

	pixbuf_error=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU, NULL);
	pixbuf_warning=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU, NULL);
	pixbuf_info=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU, NULL);
	for (liter=gd_errors; liter!=NULL; liter=liter->next) {
		GdErrorMessage *error=liter->data;

		gtk_text_buffer_get_iter_at_offset(buffer, &iter, -1);
		if (error->flags>=G_LOG_LEVEL_MESSAGE)
			gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_info);
		else
		if (error->flags<G_LOG_LEVEL_WARNING)
			gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_error);
		else
			gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_warning);
		gtk_text_buffer_insert(buffer, &iter, error->message, -1);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	}
	g_object_unref(pixbuf_error);
	g_object_unref(pixbuf_warning);
	g_object_unref(pixbuf_info);

	/* set some tags */
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
	gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (view), 3);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 6);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view), 6);
	gtk_widget_show_all (dialog);
	result=gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	/* the user has seen the errors, clear the "has new error" flag */
	gd_clear_error_flag();
	/* maybe clearing the whole error list requested? */
	if (result==1)
		gd_clear_errors();
}

void
gd_show_last_error(GtkWidget *parent)
{
    GtkWidget *dialog;
    int result;
    GdErrorMessage *m;

    if (!gd_errors)
    	return;

    /* set new error flag to false, as the user now knows that some error has happened */
	gd_clear_error_flag();

	m=g_list_last(gd_errors)->data;

    dialog=gtk_message_dialog_new ((GtkWindow *) parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
        m->message);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog), _("_Show all"), 1, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_title(GTK_WINDOW(dialog), "GDash");
    result=gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (result==1)
    	/* user requested to show all errors */
    	gd_show_errors(parent);
}


gboolean
gd_question_yesno(const char *primary, const char *secondary)
{
	GtkWidget *dialog;
	int response;

	dialog=gtk_message_dialog_new ((GtkWindow *) guess_active_toplevel(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, primary);
	if (secondary && !g_str_equal(secondary, ""))
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), secondary);
	response=gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	return response==GTK_RESPONSE_YES;
}


