/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007  Audacious development team.
 *
 * Based on:
 * BMP - Cross-platform multimedia player
 * Copyright (C) 2003-2004  BMP development team.
 * XMMS:
 * Copyright (C) 1998-2003  XMMS development team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include "widgets/widgetcore.h"
#include "ui_skinned_number.h"
#include "main.h"
#include "util.h"
#include "strings.h"
#include <string.h>
#include <ctype.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmarshal.h>

#define UI_TYPE_SKINNED_NUMBER           (ui_skinned_number_get_type())

enum {
    DOUBLED,
    LAST_SIGNAL
};

static void ui_skinned_number_class_init         (UiSkinnedNumberClass *klass);
static void ui_skinned_number_init               (UiSkinnedNumber *number);
static void ui_skinned_number_destroy            (GtkObject *object);
static void ui_skinned_number_realize            (GtkWidget *widget);
static void ui_skinned_number_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_number_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_number_expose         (GtkWidget *widget, GdkEventExpose *event);
static void ui_skinned_number_toggle_doublesize  (UiSkinnedNumber *number);

static GtkWidgetClass *parent_class = NULL;
static guint number_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_number_get_type() {
    static GType number_type = 0;
    if (!number_type) {
        static const GTypeInfo number_info = {
            sizeof (UiSkinnedNumberClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_number_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedNumber),
            0,
            (GInstanceInitFunc) ui_skinned_number_init,
        };
        number_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedNumber", &number_info, 0);
    }

    return number_type;
}

static void ui_skinned_number_class_init(UiSkinnedNumberClass *klass) {
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_number_destroy;

    widget_class->realize = ui_skinned_number_realize;
    widget_class->expose_event = ui_skinned_number_expose;
    widget_class->size_request = ui_skinned_number_size_request;
    widget_class->size_allocate = ui_skinned_number_size_allocate;

    klass->doubled = ui_skinned_number_toggle_doublesize;

    number_signals[DOUBLED] = 
        g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedNumberClass, doubled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void ui_skinned_number_init(UiSkinnedNumber *number) {
    number->width = 9;
    number->height = 13;
}

GtkWidget* ui_skinned_number_new(GtkWidget *fixed, gint x, gint y, SkinPixmapId si) {
    UiSkinnedNumber *number = g_object_new (ui_skinned_number_get_type (), NULL);

    number->x = x;
    number->y = y;
    number->num = 0;
    number->skin_index = si;

    number->fixed = fixed;
    number->double_size = FALSE;

    gtk_fixed_put(GTK_FIXED(number->fixed), GTK_WIDGET(number), number->x, number->y);

    return GTK_WIDGET(number);
}

static void ui_skinned_number_destroy(GtkObject *object) {
    UiSkinnedNumber *number;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_NUMBER (object));

    number = UI_SKINNED_NUMBER (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_number_realize(GtkWidget *widget) {
    UiSkinnedNumber *number;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_NUMBER(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    number = UI_SKINNED_NUMBER(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new(widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach(widget->style, widget->window);

    gdk_window_set_user_data(widget->window, widget);
}

static void ui_skinned_number_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedNumber *number = UI_SKINNED_NUMBER(widget);

    requisition->width = number->width*(1+number->double_size);
    requisition->height = number->height*(1+number->double_size);
}

static void ui_skinned_number_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedNumber *number = UI_SKINNED_NUMBER (widget);

    widget->allocation = *allocation;
    widget->allocation.x *= (1+number->double_size);
    widget->allocation.y *= (1+number->double_size);
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    number->x = widget->allocation.x/(number->double_size ? 2 : 1);
    number->y = widget->allocation.y/(number->double_size ? 2 : 1);
}

static gboolean ui_skinned_number_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_NUMBER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedNumber *number = UI_SKINNED_NUMBER (widget);

    GdkPixmap *obj = NULL;
    GdkGC *gc;
    obj = gdk_pixmap_new(NULL, number->width, number->height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    if (number->num > 11 || number->num < 0)
        number->num = 10;

    skin_draw_pixmap(bmp_active_skin, obj, gc,
                     number->skin_index, number->num * 9, 0,
                     0, 0, number->width, number->height);

    GdkPixmap *image;
    image = gdk_pixmap_new(NULL, number->width*(1+number->double_size),
                                 number->height*(1+number->double_size),
                                 gdk_rgb_get_visual()->depth);

    if (number->double_size) {
        image = create_dblsize_pixmap(obj);
    } else
        gdk_draw_drawable (image, gc, obj, 0, 0, 0, 0, number->width, number->height);

    g_object_unref(obj);

    gdk_draw_drawable (widget->window, gc, image, 0, 0, 0, 0,
                       number->width*(1+number->double_size), number->height*(1+number->double_size));
    g_object_unref(gc);
    g_object_unref(image);

    return FALSE;
}

static void ui_skinned_number_toggle_doublesize(UiSkinnedNumber *number) {
    GtkWidget *widget = GTK_WIDGET (number);
    number->double_size = !number->double_size;

    gtk_widget_set_size_request(widget, number->width*(1+number->double_size), number->height*(1+number->double_size));

    gtk_widget_queue_draw(GTK_WIDGET(number));
}

void ui_skinned_number_set_number(GtkWidget *widget, gint num) {
    UiSkinnedNumber *number = UI_SKINNED_NUMBER (widget);

    if (number->num == num)
         return;

    number->num = num;
    gtk_widget_queue_draw(GTK_WIDGET(number));
}
