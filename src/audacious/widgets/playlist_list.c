/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

/*
 *  A note about Pango and some funky spacey fonts: Weirdly baselined
 *  fonts, or fonts with weird ascents or descents _will_ display a
 *  little bit weird in the playlist widget, but the display engine
 *  won't make it look too bad, just a little deranged.  I honestly
 *  don't think it's worth fixing (around...), it doesn't have to be
 *  perfectly fitting, just the general look has to be ok, which it
 *  IMHO is.
 *
 *  A second note: The numbers aren't perfectly aligned, but in the
 *  end it looks better when using a single Pango layout for each
 *  number.
 */

#include "widgetcore.h"

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "input.h"
#include "strings.h"
#include "playback.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "util.h"

#include "debug.h"

static PangoFontDescription *playlist_list_font = NULL;
static gint ascent, descent, width_delta_digit_one;
static gboolean has_slant;
static guint padding;

/* FIXME: the following globals should not be needed. */
static gint width_approx_letters;
static gint width_colon, width_colon_third;
static gint width_approx_digits, width_approx_digits_half;

void playlist_list_draw(Widget * w);

static gboolean
playlist_list_auto_drag_down_func(gpointer data)
{
    PlayList_List *pl = data;

    if (pl->pl_auto_drag_down) {
        playlist_list_move_down(pl);
        pl->pl_first++;
        playlistwin_update_list(playlist_get_active());
        return TRUE;
    }
    return FALSE;
}

static gboolean
playlist_list_auto_drag_up_func(gpointer data)
{
    PlayList_List *pl = data;

    if (pl->pl_auto_drag_up) {
        playlist_list_move_up(pl);
        pl->pl_first--;
        playlistwin_update_list(playlist_get_active());
        return TRUE;

    }
    return FALSE;
}

void
playlist_list_move_up(PlayList_List * pl)
{
    GList *list;
    Playlist *playlist = playlist_get_active();

    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist->mutex);
    if ((list = playlist->entries) == NULL) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }
    if (PLAYLIST_ENTRY(list->data)->selected) {
        /* We are at the top */
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }
    while (list) {
        if (PLAYLIST_ENTRY(list->data)->selected)
            glist_moveup(list);
        list = g_list_next(list);
    }
    PLAYLIST_UNLOCK(playlist->mutex);
    if (pl->pl_prev_selected != -1)
        pl->pl_prev_selected--;
    if (pl->pl_prev_min != -1)
        pl->pl_prev_min--;
    if (pl->pl_prev_max != -1)
        pl->pl_prev_max--;
}

void
playlist_list_move_down(PlayList_List * pl)
{
    GList *list;
    Playlist *playlist = playlist_get_active();

    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist->mutex);

    if (!(list = g_list_last(playlist->entries))) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }

    if (PLAYLIST_ENTRY(list->data)->selected) {
        /* We are at the bottom */
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }

    while (list) {
        if (PLAYLIST_ENTRY(list->data)->selected)
            glist_movedown(list);
        list = g_list_previous(list);
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    if (pl->pl_prev_selected != -1)
        pl->pl_prev_selected++;
    if (pl->pl_prev_min != -1)
        pl->pl_prev_min++;
    if (pl->pl_prev_max != -1)
        pl->pl_prev_max++;
}

static void
playlist_list_button_press_cb(GtkWidget * widget,
                              GdkEventButton * event,
                              PlayList_List * pl)
{
    gint nr;
    Playlist *playlist = playlist_get_active();

	nr = playlist_list_get_playlist_position(pl, event->x, event->y);
	if (nr == -1)
		return;

	if (event->button == 3)
	{
		GList* selection = playlist_get_selected(playlist);
		if (g_list_find(selection, GINT_TO_POINTER(nr)) == NULL)
		{
			playlist_select_all(playlist, FALSE);
			playlist_select_range(playlist, nr, nr, TRUE);
		}
	}
	else if (event->button == 1)
	{
        if (!(event->state & GDK_CONTROL_MASK))
            playlist_select_all(playlist, FALSE);

        if (event->state & GDK_SHIFT_MASK && pl->pl_prev_selected != -1) {
            playlist_select_range(playlist, pl->pl_prev_selected, nr, TRUE);
            pl->pl_prev_min = pl->pl_prev_selected;
            pl->pl_prev_max = nr;
            pl->pl_drag_pos = nr - pl->pl_first;
        }
        else {
            if (playlist_select_invert(playlist, nr)) {
                if (event->state & GDK_CONTROL_MASK) {
                    if (pl->pl_prev_min == -1) {
                        pl->pl_prev_min = pl->pl_prev_selected;
                        pl->pl_prev_max = pl->pl_prev_selected;
                    }
                    if (nr < pl->pl_prev_min)
                        pl->pl_prev_min = nr;
                    else if (nr > pl->pl_prev_max)
                        pl->pl_prev_max = nr;
                }
                else
                    pl->pl_prev_min = -1;
                pl->pl_prev_selected = nr;
                pl->pl_drag_pos = nr - pl->pl_first;
            }
        }
        if (event->type == GDK_2BUTTON_PRESS) {
            /*
             * Ungrab the pointer to prevent us from
             * hanging on to it during the sometimes slow
             * playback_initiate().
             */
            gdk_pointer_ungrab(GDK_CURRENT_TIME);
            gdk_flush();
            playlist_set_position(playlist, nr);
            if (!playback_get_playing())
                playback_initiate();
        }

        pl->pl_dragging = TRUE;
    }

	playlistwin_update_list(playlist);
}

gint
playlist_list_get_playlist_position(PlayList_List * pl,
                                    gint x,
                                    gint y)
{
    gint iy, length;
    gint ret;
    Playlist *playlist = playlist_get_active();

    if (!widget_contains(WIDGET(pl), x, y) || !pl->pl_fheight)
        return -1;

    if ((length = playlist_get_length(playlist)) == 0)
        return -1;
    iy = y - pl->pl_widget.y;

    ret = (iy / pl->pl_fheight) + pl->pl_first;

    if (ret > length - 1)
	    ret = -1;

    return ret;
}

static void
playlist_list_motion_cb(GtkWidget * widget,
                        GdkEventMotion * event,
                        PlayList_List * pl)
{
    gint nr, y, off, i;

    if (pl->pl_dragging) {
        y = event->y - pl->pl_widget.y;
        nr = (y / pl->pl_fheight);
        if (nr < 0) {
            nr = 0;
            if (!pl->pl_auto_drag_up) {
                pl->pl_auto_drag_up = TRUE;
                pl->pl_auto_drag_up_tag =
                    g_timeout_add(100, playlist_list_auto_drag_up_func, pl);
            }
        }
        else if (pl->pl_auto_drag_up)
            pl->pl_auto_drag_up = FALSE;

        if (nr >= pl->pl_num_visible) {
            nr = pl->pl_num_visible - 1;
            if (!pl->pl_auto_drag_down) {
                pl->pl_auto_drag_down = TRUE;
                pl->pl_auto_drag_down_tag =
                    g_timeout_add(100, playlist_list_auto_drag_down_func,
                                    pl);
            }
        }
        else if (pl->pl_auto_drag_down)
            pl->pl_auto_drag_down = FALSE;

        off = nr - pl->pl_drag_pos;
        if (off) {
            for (i = 0; i < abs(off); i++) {
                if (off < 0)
                    playlist_list_move_up(pl);
                else
                    playlist_list_move_down(pl);

            }
            playlistwin_update_list(playlist_get_active());
        }
        pl->pl_drag_pos = nr;
    }
}

static void
playlist_list_button_release_cb(GtkWidget * widget,
                                GdkEventButton * event,
                                PlayList_List * pl)
{
    pl->pl_dragging = FALSE;
    pl->pl_auto_drag_down = FALSE;
    pl->pl_auto_drag_up = FALSE;
}

static void
playlist_list_draw_string(PlayList_List * pl,
                          PangoFontDescription * font,
                          gint line,
                          gint width,
                          const gchar * text,
                          guint ppos)
{
    guint plist_length_int;
    Playlist *playlist = playlist_get_active();

    PangoLayout *layout;

    REQUIRE_LOCK(playlist->mutex);

    if (cfg.show_numbers_in_pl) {
        gchar *pos_string = g_strdup_printf(cfg.show_separator_in_pl == TRUE ? "%d" : "%d.", ppos);
        plist_length_int =
            gint_count_digits(playlist_get_length(playlist)) + !cfg.show_separator_in_pl + 1; /* cf.show_separator_in_pl will be 0 if false */

        padding = plist_length_int;
        padding = ((padding + 1) * width_approx_digits);

        layout = gtk_widget_create_pango_layout(playlistwin, pos_string);
        pango_layout_set_font_description(layout, playlist_list_font);
        pango_layout_set_width(layout, plist_length_int * 100);

        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
        gdk_draw_layout(pl->pl_widget.parent, pl->pl_widget.gc,
                        pl->pl_widget.x +
                        (width_approx_digits *
                         (-1 + plist_length_int - strlen(pos_string))) +
                        (width_approx_digits / 4),
                        pl->pl_widget.y + (line - 1) * pl->pl_fheight +
                        ascent + abs(descent), layout);
        g_free(pos_string);
        g_object_unref(layout);

        if (!cfg.show_separator_in_pl)
            padding -= (width_approx_digits * 1.5);
    }
    else {
        padding = 3;
    }

    width -= padding;

    layout = gtk_widget_create_pango_layout(playlistwin, text);

    pango_layout_set_font_description(layout, playlist_list_font);
    pango_layout_set_width(layout, width * PANGO_SCALE);
    pango_layout_set_single_paragraph_mode(layout, TRUE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    gdk_draw_layout(pl->pl_widget.parent, pl->pl_widget.gc,
                    pl->pl_widget.x + padding + (width_approx_letters / 4),
                    pl->pl_widget.y + (line - 1) * pl->pl_fheight +
                    ascent + abs(descent), layout);

    g_object_unref(layout);
}

void
playlist_list_draw(Widget * w)
{
    Playlist *playlist = playlist_get_active();
    PlayList_List *pl = PLAYLIST_LIST(w);
    GList *list;
    GdkGC *gc;
    GdkPixmap *obj;
    PangoLayout *layout;
    gchar *title;
    gint width, height;
    gint i, max_first;
    guint padding, padding_dwidth, padding_plength;
    guint max_time_len = 0;
    gfloat queue_tailpadding = 0;
    gint tpadding; 
    gsize tpadding_dwidth = 0;
    gint x, y;
    guint tail_width;
    guint tail_len;

    gchar tail[100];
    gchar queuepos[255];
    gchar length[40];

    gchar **frags;
    gchar *frag0;

    gint plw_w, plw_h;

    GdkRectangle *playlist_rect;

    gc = pl->pl_widget.gc;

    width = pl->pl_widget.width;
    height = pl->pl_widget.height;

    obj = pl->pl_widget.parent;

    plw_w = playlistwin_get_width();
    plw_h = playlistwin_get_height();

    playlist_rect = g_new0(GdkRectangle, 1);

    playlist_rect->x = 0;
    playlist_rect->y = 0;
    playlist_rect->width = plw_w - 17;
    playlist_rect->height = plw_h - 36;

    gdk_gc_set_clip_origin(gc, 31, 58);
    gdk_gc_set_clip_rectangle(gc, playlist_rect);

    gdk_gc_set_foreground(gc,
                          skin_get_color(bmp_active_skin,
                                         SKIN_PLEDIT_NORMALBG));
    gdk_draw_rectangle(obj, gc, TRUE, pl->pl_widget.x, pl->pl_widget.y,
                       width, height);

    if (!playlist_list_font) {
        g_critical("Couldn't open playlist font");
        return;
    }

    pl->pl_fheight = (ascent + abs(descent));
    pl->pl_num_visible = height / pl->pl_fheight;

    max_first = playlist_get_length(playlist) - pl->pl_num_visible;
    max_first = MAX(max_first, 0);

    pl->pl_first = CLAMP(pl->pl_first, 0, max_first);

    PLAYLIST_LOCK(playlist->mutex);
    list = playlist->entries;
    list = g_list_nth(list, pl->pl_first);

    /* It sucks having to run the iteration twice but this is the only
       way you can reliably get the maximum width so we can get our
       playlist nice and aligned... -- plasmaroo */

    for (i = pl->pl_first;
         list && i < pl->pl_first + pl->pl_num_visible;
         list = g_list_next(list), i++) {
        PlaylistEntry *entry = list->data;

        if (entry->length != -1)
        {
            g_snprintf(length, sizeof(length), "%d:%-2.2d",
                       entry->length / 60000, (entry->length / 1000) % 60);
            tpadding_dwidth = MAX(tpadding_dwidth, strlen(length));
        }
    }

    /* Reset */
    list = playlist->entries;
    list = g_list_nth(list, pl->pl_first);

    for (i = pl->pl_first;
         list && i < pl->pl_first + pl->pl_num_visible;
         list = g_list_next(list), i++) {
        gint pos;
        PlaylistEntry *entry = list->data;

        if (entry->selected) {
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_SELECTEDBG));
            gdk_draw_rectangle(obj, gc, TRUE, pl->pl_widget.x,
                               pl->pl_widget.y +
                               ((i - pl->pl_first) * pl->pl_fheight),
                               width, pl->pl_fheight);
        }

        /* FIXME: entry->title should NEVER be NULL, and there should
           NEVER be a need to do a UTF-8 conversion. Playlist title
           strings should be kept properly. */

        if (!entry->title) {
            gchar *realfn = g_filename_from_uri(entry->filename, NULL, NULL);
            gchar *basename = g_path_get_basename(realfn ? realfn : entry->filename);
            title = filename_to_utf8(basename);
            g_free(basename); g_free(realfn);
        }
        else
            title = str_to_utf8(entry->title);

        title = convert_title_text(title);

        pos = playlist_get_queue_position(playlist, entry);

        tail[0] = 0;
        queuepos[0] = 0;
        length[0] = 0;

        if (pos != -1)
            g_snprintf(queuepos, sizeof(queuepos), "%d", pos + 1);

        if (entry->length != -1)
        {
            g_snprintf(length, sizeof(length), "%d:%-2.2d",
                       entry->length / 60000, (entry->length / 1000) % 60);
        }

        strncat(tail, length, sizeof(tail) - 1);
        tail_len = strlen(tail);

        max_time_len = MAX(max_time_len, tail_len);

        if (pos != -1 && tpadding_dwidth <= 0)
            tail_width = width - (width_approx_digits * (strlen(queuepos) + 2.25));
        else if (pos != -1)
            tail_width = width - (width_approx_digits * (tpadding_dwidth + strlen(queuepos) + 4));
        else if (tpadding_dwidth > 0)
            tail_width = width - (width_approx_digits * (tpadding_dwidth + 2.5));
        else
            tail_width = width;

        if (i == playlist_get_position_nolock(playlist))
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_CURRENT));
        else
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_NORMAL));
        playlist_list_draw_string(pl, playlist_list_font,
                                  i - pl->pl_first, tail_width, title,
                                  i + 1);

        x = pl->pl_widget.x + width - width_approx_digits * 2;
        y = pl->pl_widget.y + ((i - pl->pl_first) -
                               1) * pl->pl_fheight + ascent;

        frags = NULL;
        frag0 = NULL;

        if ((strlen(tail) > 0) && (tail != NULL)) {
            frags = g_strsplit(tail, ":", 0);
            frag0 = g_strconcat(frags[0], ":", NULL);

            layout = gtk_widget_create_pango_layout(playlistwin, frags[1]);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_width(layout, tail_len * 100);
            pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
            gdk_draw_layout(obj, gc, x - (0.5 * width_approx_digits),
                            y + abs(descent), layout);
            g_object_unref(layout);

            layout = gtk_widget_create_pango_layout(playlistwin, frag0);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_width(layout, tail_len * 100);
            pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
            gdk_draw_layout(obj, gc, x - (0.75 * width_approx_digits),
                            y + abs(descent), layout);
            g_object_unref(layout);

            g_free(frag0);
            g_strfreev(frags);
        }

        if (pos != -1) {

            /* DON'T remove the commented code yet please     -- Milosz */

            if (tpadding_dwidth > 0)
                queue_tailpadding = tpadding_dwidth + 1;
            else
                queue_tailpadding = -0.75;

            gdk_draw_rectangle(obj, gc, FALSE,
                               x -
                               (((queue_tailpadding +
                                  strlen(queuepos)) *
                                 width_approx_digits) +
                                (width_approx_digits / 4)),
                               y + abs(descent),
                               (strlen(queuepos)) *
                               width_approx_digits +
                               (width_approx_digits / 2),
                               pl->pl_fheight - 2);

            layout =
                gtk_widget_create_pango_layout(playlistwin, queuepos);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

            gdk_draw_layout(obj, gc,
                            x -
                            ((queue_tailpadding +
                              strlen(queuepos)) * width_approx_digits) +
                            (width_approx_digits / 4),
                            y + abs(descent), layout);
            g_object_unref(layout);
        }

        g_free(title);
    }


    /*
     * Drop target hovering over the playlist, so draw some hint where the
     * drop will occur.
     *
     * This is (currently? unfixably?) broken when dragging files from Qt/KDE apps,
     * probably due to DnD signaling problems (actually i have no clue).
     *
     */

    if (pl->pl_drag_motion) {
        guint pos, plength, lpadding;
	gint x, y, plx, ply;

        if (cfg.show_numbers_in_pl) {
            lpadding = gint_count_digits(playlist_get_length(playlist)) + 1;
            lpadding = ((lpadding + 1) * width_approx_digits);
        }
        else {
            lpadding = 3;
        };

        /* We already hold the mutex and have the playlist locked, so call
           the non-locking function. */
        plength = playlist_get_length(playlist);

        x = pl->drag_motion_x;
        y = pl->drag_motion_y;

        plx = pl->pl_widget.x;
        ply = pl->pl_widget.y;

        if ((x > pl->pl_widget.x) && !(x > pl->pl_widget.width)) {

            if ((y > pl->pl_widget.y)
                && !(y > (pl->pl_widget.height + ply))) {

                pos = ((y - ((Widget *) pl)->y) / pl->pl_fheight) +
                    pl->pl_first;

                if (pos > (plength)) {
                    pos = plength;
                }

                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));

                gdk_draw_line(obj, gc, pl->pl_widget.x,
			      pl->pl_widget.y + ((pos - pl->pl_first) * pl->pl_fheight),
                              pl->pl_widget.width + pl->pl_widget.x - 1,
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight));
            }

        }

        /* When dropping on the borders of the playlist, outside the text area,
         * files get appended at the end of the list. Show that too.
         */

        if ((y < ply) || (y > pl->pl_widget.height + ply)) {
            if ((y >= 0) || (y <= (pl->pl_widget.height + ply))) {
                pos = plength;
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));

                gdk_draw_line(obj, gc, pl->pl_widget.x,
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight),
                              pl->pl_widget.width + pl->pl_widget.x - 1,
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight));

            }
        }
    }

    gdk_gc_set_foreground(gc,
                          skin_get_color(bmp_active_skin,
                                         SKIN_PLEDIT_NORMAL));

    if (cfg.show_numbers_in_pl)
    {
        padding_plength = playlist_get_length(playlist);

        if (padding_plength == 0) {
            padding_dwidth = 0;
        }
        else {
            padding_dwidth = gint_count_digits(playlist_get_length(playlist));
        }

        padding =
            (padding_dwidth *
             width_approx_digits) + width_approx_digits;


        /* For italic or oblique fonts we add another half of the
         * approximate width */
        if (has_slant)
            padding += width_approx_digits_half;

        if (cfg.show_separator_in_pl) {
            gdk_draw_line(obj, gc,
                          pl->pl_widget.x + padding,
                          pl->pl_widget.y,
                          pl->pl_widget.x + padding,
                          pl->pl_widget.y + pl->pl_widget.height - 1);
        }
    }

    if (tpadding_dwidth != 0)
    {
        tpadding = (tpadding_dwidth * width_approx_digits) + (width_approx_digits * 1.5);

        if (has_slant)
            tpadding += width_approx_digits_half;

        if (cfg.show_separator_in_pl) {
            gdk_draw_line(obj, gc,
                          pl->pl_widget.x + pl->pl_widget.width - tpadding,
                          pl->pl_widget.y,
                          pl->pl_widget.x + pl->pl_widget.width - tpadding,
                          pl->pl_widget.y + pl->pl_widget.height - 1);
        }
    }

    gdk_gc_set_clip_origin(gc, 0, 0);
    gdk_gc_set_clip_rectangle(gc, NULL);

    PLAYLIST_UNLOCK(playlist->mutex);

    gdk_flush();

    g_free(playlist_rect);
}


PlayList_List *
create_playlist_list(GList ** wlist,
                     GdkPixmap * parent,
                     GdkGC * gc,
                     gint x, gint y,
                     gint w, gint h)
{
    PlayList_List *pl;

    pl = g_new0(PlayList_List, 1);
    widget_init(&pl->pl_widget, parent, gc, x, y, w, h, TRUE);

    pl->pl_widget.button_press_cb =
        (WidgetButtonPressFunc) playlist_list_button_press_cb;
    pl->pl_widget.button_release_cb =
        (WidgetButtonReleaseFunc) playlist_list_button_release_cb;
    pl->pl_widget.motion_cb = (WidgetMotionFunc) playlist_list_motion_cb;
    pl->pl_widget.draw = playlist_list_draw;

    pl->pl_prev_selected = -1;
    pl->pl_prev_min = -1;
    pl->pl_prev_max = -1;

    widget_list_add(wlist, WIDGET(pl));

    return pl;
}

void
playlist_list_set_font(const gchar * font)
{

    /* Welcome to bad hack central 2k3 */

    gchar *font_lower;
    gint width_temp;
    gint width_temp_0;

    playlist_list_font = pango_font_description_from_string(font);

    text_get_extents(font,
                     "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz ",
                     &width_approx_letters, NULL, &ascent, &descent);

    width_approx_letters = (width_approx_letters / 53);

    /* Experimental: We don't weigh the 1 into total because it's width is almost always
     * very different from the rest
     */
    text_get_extents(font, "023456789", &width_approx_digits, NULL, NULL,
                     NULL);
    width_approx_digits = (width_approx_digits / 9);

    /* Precache some often used calculations */
    width_approx_digits_half = width_approx_digits / 2;

    /* FIXME: We assume that any other number is broader than the "1" */
    text_get_extents(font, "1", &width_temp, NULL, NULL, NULL);
    text_get_extents(font, "2", &width_temp_0, NULL, NULL, NULL);

    if (abs(width_temp_0 - width_temp) < 2) {
        width_delta_digit_one = 0;
    }
    else {
        width_delta_digit_one = ((width_temp_0 - width_temp) / 2) + 2;
    }

    text_get_extents(font, ":", &width_colon, NULL, NULL, NULL);
    width_colon_third = width_colon / 4;

    font_lower = g_utf8_strdown(font, strlen(font));
    /* This doesn't take any i18n into account, but i think there is none with TTF fonts
     * FIXME: This can probably be retrieved trough Pango too
     */
    has_slant = g_strstr_len(font_lower, strlen(font_lower), "oblique")
        || g_strstr_len(font_lower, strlen(font_lower), "italic");

    g_free(font_lower);
}
