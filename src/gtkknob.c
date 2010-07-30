/*****************************************************************************
 *
 * gtkknob.c
 *
 * PHASEX:  [P]hase [H]armonic [A]dvanced [S]ynthesis [EX]periment
 *
 * Most of this code comes from gAlan 0.2.0, copyright (C) 1999
 * Tony Garnock-Jones, with modifications from Sean Bolton,
 * copyright (C) 2004, William Weston copyright (C) 2007, and
 * Pete Shorthose copyright (C) 2007.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *****************************************************************************/
#include <math.h>
#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

#include "config.h"
#include "gtkknob.h"


#ifndef M_PI
# define M_PI		3.14159265358979323846	/* pi */
#endif
#ifndef M_1_PI
# define M_1_PI		0.31830988618379067154	/* 1/pi */
#endif


#define SCROLL_DELAY_LENGTH	250

#define STATE_IDLE		0
#define STATE_PRESSED		1
#define STATE_DRAGGING		2

static void gtk_knob_class_init(GtkKnobClass *klass);
static void gtk_knob_init(GtkKnob *knob);
static void gtk_knob_destroy(GtkObject *object);
static void gtk_knob_realize(GtkWidget *widget);
static void gtk_knob_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void gtk_knob_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static gint gtk_knob_expose(GtkWidget *widget, GdkEventExpose *event);
static gint gtk_knob_scroll(GtkWidget *widget, GdkEventScroll *event);
static gint gtk_knob_button_press(GtkWidget *widget, GdkEventButton *event);
static gint gtk_knob_button_release(GtkWidget *widget, GdkEventButton *event);
static gint gtk_knob_motion_notify(GtkWidget *widget, GdkEventMotion *event);
static gint gtk_knob_timer(GtkKnob *knob);

static void gtk_knob_update_mouse_update(GtkKnob *knob);
static void gtk_knob_update_mouse(GtkKnob *knob, gint x, gint y, gboolean absolute);
static void gtk_knob_update(GtkKnob *knob);
static void gtk_knob_adjustment_changed(GtkAdjustment *adjustment, gpointer data);
static void gtk_knob_adjustment_value_changed(GtkAdjustment *adjustment, gpointer data);

GError *gerror;

/* Local data */

static GtkWidgetClass *parent_class = NULL;


/*****************************************************************************
 *
 * GTK_KNOB_GET_TYPE()
 *
 *****************************************************************************/
GType
gtk_knob_get_type(void) {
    static GType knob_type = 0;

    if (!knob_type) {
	static const GTypeInfo info = {
	    sizeof (GtkKnobClass),
	    NULL,
	    NULL,
	    (GClassInitFunc) gtk_knob_class_init,
	    NULL,
	    NULL,
	    sizeof (GtkKnob),
	    0,
	    (GInstanceInitFunc) gtk_knob_init,
	};

	knob_type = g_type_register_static (GTK_TYPE_WIDGET,
					    "GtkKnob",
					    &info,
					    0);
    }

    return knob_type;
}


/*****************************************************************************
 *
 * GTK_KNOB_CLASS_INIT()
 *
 *****************************************************************************/
static void
gtk_knob_class_init (GtkKnobClass *class) {
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass*) class;
    widget_class = (GtkWidgetClass*) class;

    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = gtk_knob_destroy;

    widget_class->realize              = gtk_knob_realize;
    widget_class->expose_event         = gtk_knob_expose;
    widget_class->size_request         = gtk_knob_size_request;
    widget_class->size_allocate        = gtk_knob_size_allocate;
    widget_class->scroll_event         = gtk_knob_scroll;
    widget_class->button_press_event   = gtk_knob_button_press;
    widget_class->button_release_event = gtk_knob_button_release;
    widget_class->motion_notify_event  = gtk_knob_motion_notify;
}


/*****************************************************************************
 *
 * GTK_KNOB_INIT()
 *
 *****************************************************************************/
static void
gtk_knob_init (GtkKnob *knob) {
    knob->policy     = GTK_UPDATE_CONTINUOUS;
    knob->state      = STATE_IDLE;
    knob->saved_x    = 0;
    knob->saved_y    = 0;
    knob->timer      = 0;
    knob->anim       = NULL;
    knob->mask       = NULL;
    knob->mask_gc    = NULL;
    knob->red_gc     = NULL;
    knob->old_value  = 0.0;
    knob->old_lower  = 0.0;
    knob->old_upper  = 0.0;
    knob->adjustment = NULL;
}


/*****************************************************************************
 *
 * GTK_KNOB_NEW()
 *
 *****************************************************************************/
GtkWidget *
gtk_knob_new(GtkAdjustment *adjustment, GtkKnobAnim *anim) {
    GtkKnob *knob;

    g_return_val_if_fail (anim != NULL, NULL);
    g_return_val_if_fail (GDK_IS_PIXBUF (anim->pixbuf), NULL);

    knob = gtk_type_new (gtk_knob_get_type ());

    gtk_knob_set_animation (knob, anim);

    if (!adjustment) {
	adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0,
							  0.0, 0.0, 0.0);
    }

    gtk_knob_set_adjustment (knob, adjustment);

    return GTK_WIDGET (knob);
}


/*****************************************************************************
 *
 * GTK_KNOB_DESTROY()
 *
 *****************************************************************************/
static void
gtk_knob_destroy(GtkObject *object) {
    GtkKnob *knob;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GTK_IS_KNOB (object));

    knob = GTK_KNOB (object);

    if (knob->adjustment) {
	gtk_object_unref (GTK_OBJECT (knob->adjustment));
	knob->adjustment = NULL;
    }

    /* FIXME: needs ref counting for automatic GtkKnobAnim cleanup
    if (knob->anim) {
        gtk_knob_anim_unref (knob->anim);
        knob->anim = NULL;
    }
    */

    if (knob->mask) {
	gdk_bitmap_unref (knob->mask);
	knob->mask = NULL;
    }

    if (knob->mask_gc) {
	gdk_gc_unref (knob->mask_gc);
	knob->mask_gc = NULL;
    }
    if (knob->red_gc) {
	gdk_gc_unref (knob->red_gc);
	knob->red_gc = NULL;
    }

    if (GTK_OBJECT_CLASS (parent_class)->destroy) {
	(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
    }
}


/*****************************************************************************
 *
 * GTK_KNOB_GET_ADJUSTMENT()
 *
 *****************************************************************************/
GtkAdjustment*
gtk_knob_get_adjustment(GtkKnob *knob) {

    g_return_val_if_fail (knob != NULL, NULL);
    g_return_val_if_fail (GTK_IS_KNOB (knob), NULL);

    return knob->adjustment;
}


/*****************************************************************************
 *
 * GTK_KNOB_SET_UPDATE_POLICY()
 *
 *****************************************************************************/
void
gtk_knob_set_update_policy(GtkKnob *knob, GtkUpdateType policy) {

    g_return_if_fail (knob != NULL);
    g_return_if_fail (GTK_IS_KNOB (knob));

    knob->policy = policy;
}


/*****************************************************************************
 *
 * GTK_KNOB_SET_ADJUSTMENT()
 *
 *****************************************************************************/
void
gtk_knob_set_adjustment(GtkKnob *knob, GtkAdjustment *adjustment) {

    g_return_if_fail (knob != NULL);
    g_return_if_fail (GTK_IS_KNOB (knob));

    if (knob->adjustment) {
	gtk_signal_disconnect_by_data (GTK_OBJECT (knob->adjustment),
				       (gpointer)knob);
	gtk_object_unref (GTK_OBJECT (knob->adjustment));
    }

    knob->adjustment = adjustment;
    gtk_object_ref (GTK_OBJECT (knob->adjustment));
    gtk_object_sink (GTK_OBJECT (knob->adjustment));

    gtk_signal_connect (GTK_OBJECT (adjustment), "changed",
			GTK_SIGNAL_FUNC (gtk_knob_adjustment_changed),
			(gpointer)knob);
    gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			GTK_SIGNAL_FUNC (gtk_knob_adjustment_value_changed),
			(gpointer)knob);

    knob->old_value = adjustment->value;
    knob->old_lower = adjustment->lower;
    knob->old_upper = adjustment->upper;

    if (knob->anim != NULL) {
	knob->frame_offset = (int)(
	    (((gfloat)(knob->anim->width) /
	      (gfloat)(knob->anim->frame_width)) - 1) *
	    (knob->adjustment->value - knob->adjustment->lower) *
	    (1.0 / ((gfloat)(adjustment->upper) -
		    (gfloat)(adjustment->lower)))
	    ) * knob->width;
    }

    gtk_knob_update (knob);
}


/*****************************************************************************
 *
 * GTK_KNOB_REALIZE()
 *
 *****************************************************************************/
static void
gtk_knob_realize(GtkWidget *widget) {
    GtkKnob *knob;
    GdkWindowAttr attributes;
    gint attributes_mask;
    GdkColor color = { 0, 0xffff, 0, 0 };
//    GdkDisplay	*display;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_KNOB (widget));

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
    knob = GTK_KNOB (widget);

    attributes.x           = widget->allocation.x;
    attributes.y           = widget->allocation.y;
    attributes.width       = widget->allocation.width;
    attributes.height      = widget->allocation.height;
    attributes.wclass      = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask  =
	gtk_widget_get_events (widget) |
	GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
	GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
	GDK_POINTER_MOTION_HINT_MASK;
    attributes.visual   = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new (widget->parent->window,
				     &attributes, attributes_mask);

//    display = gtk_widget_get_display (widget);
//    if (gdk_display_supports_composite (display)) {
//	gdk_window_set_composited (widget->window, TRUE);
//    }

    widget->style = gtk_style_attach (widget->style, widget->window);

    gdk_window_set_user_data (widget->window, widget);

    gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

    knob->mask_gc = gdk_gc_new (widget->window);
    gdk_gc_copy (knob->mask_gc, widget->style->bg_gc[GTK_STATE_NORMAL]);
    gdk_gc_set_clip_mask (knob->mask_gc, knob->mask);

    knob->red_gc = gdk_gc_new (widget->window);
    gdk_gc_copy (knob->red_gc, widget->style->bg_gc[GTK_STATE_NORMAL]);
    gdk_colormap_alloc_color (attributes.colormap, &color, FALSE, TRUE);
    gdk_gc_set_foreground (knob->red_gc, &color);
}


/*****************************************************************************
 *
 * GTK_KNOB_SIZE_REQUEST()
 *
 *****************************************************************************/
static void
gtk_knob_size_request (GtkWidget *widget, GtkRequisition *requisition) {

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_KNOB (widget));

    requisition->width  = GTK_KNOB (widget)->width;
    requisition->height = GTK_KNOB (widget)->height;
}


/*****************************************************************************
 *
 * GTK_KNOB_SIZE_ALLOCATE()
 *
 *****************************************************************************/
static void
gtk_knob_size_allocate (GtkWidget *widget, GtkAllocation *allocation) {
    GtkKnob *knob;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_KNOB (widget));
    g_return_if_fail (allocation != NULL);

    widget->allocation = *allocation;
    knob = GTK_KNOB (widget);

    if (GTK_WIDGET_REALIZED (widget)) {
	gdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);
    }
}


/*****************************************************************************
 *
 * GTK_KNOB_EXPOSE()
 *
 *****************************************************************************/
static gint
gtk_knob_expose(GtkWidget *widget, GdkEventExpose *event) {
    GtkKnob *knob;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_KNOB (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (event->count > 0)
	return FALSE;

    knob = GTK_KNOB (widget);

    gdk_draw_pixbuf (widget->window, knob->mask_gc, knob->anim->pixbuf,
		     knob->frame_offset, 0, 0, 0, knob->width, knob->height,
		     GDK_RGB_DITHER_NONE, 0, 0);

    return FALSE;
}


/*****************************************************************************
 *
 * GTK_KNOB_SCROLL()
 *
 *****************************************************************************/
static gint
gtk_knob_scroll(GtkWidget *widget, GdkEventScroll *event) {
    GtkKnob *knob;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_KNOB (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    knob = GTK_KNOB (widget);

    switch (event->direction) {
    case GDK_SCROLL_UP:
	knob->adjustment->value += knob->adjustment->step_increment;
	gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
				 "value_changed");
	break;
    case GDK_SCROLL_DOWN:
	knob->adjustment->value -= knob->adjustment->step_increment;
	gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
				 "value_changed");
	break;
    default:
	break;
    }

    return FALSE;
}


/*****************************************************************************
 *
 * GTK_KNOB_BUTTON_PRESS()
 *
 *****************************************************************************/
static gint
gtk_knob_button_press(GtkWidget *widget, GdkEventButton *event) {
    GtkKnob *knob;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_KNOB (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    knob = GTK_KNOB (widget);

    switch (knob->state) {
    case STATE_IDLE:
	switch (event->button) {
	case 1:
	case 3:
	    gtk_grab_add (widget);
	    knob->state   = STATE_PRESSED;
	    knob->saved_x = event->x;
	    knob->saved_y = event->y;
	    break;
	case 2:
	    knob->adjustment->value = floor ((knob->adjustment->lower +
					      knob->adjustment->upper + 1.0)
					     * 0.5);
	    gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
	    				 "value_changed");
	    break;
	}
	break;
    }

    return FALSE;
}


/*****************************************************************************
 *
 * GTK_KNOB_BUTTON_RELEASE()
 *
 *****************************************************************************/
static gint
gtk_knob_button_release(GtkWidget *widget, GdkEventButton *event) {
    GtkKnob *knob;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_KNOB (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    knob = GTK_KNOB (widget);

    switch (knob->state) {

    case STATE_PRESSED:
	gtk_grab_remove (widget);
	knob->state = STATE_IDLE;

	switch (event->button) {
	case 1:
	    knob->adjustment->value -= knob->adjustment->page_increment;
	    gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
				     "value_changed");
	    break;
	case 3:
	    knob->adjustment->value += knob->adjustment->page_increment;
	    gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
				     "value_changed");
	    break;
	}
	break;

    case STATE_DRAGGING:
	gtk_grab_remove (widget);
	knob->state = STATE_IDLE;

	switch (event->button) {
	case 1:
	case 3:
	    if (knob->policy != GTK_UPDATE_CONTINUOUS
		&& knob->old_value != knob->adjustment->value)
	    {
		gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
					 "value_changed");
	    }
	    break;
	}

	break;
    }

    return FALSE;
}


/*****************************************************************************
 *
 * GTK_KNOB_MOTION_NOTIFY()
 *
 *****************************************************************************/
static gint
gtk_knob_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
    GtkKnob *knob;
    GdkModifierType mods;
    gint x, y;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_KNOB (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    knob = GTK_KNOB (widget);

    x = event->x;
    y = event->y;

    if (event->is_hint || (event->window != widget->window)) {
	gdk_window_get_pointer (widget->window, &x, &y, &mods);
    }

    switch (knob->state) {

    case STATE_PRESSED:
	knob->state = STATE_DRAGGING;
	/* fall through */

    case STATE_DRAGGING:
	if (mods & GDK_BUTTON1_MASK) {
	    gtk_knob_update_mouse (knob, x, y, TRUE);
	    return TRUE;
	}
	else if (mods & GDK_BUTTON3_MASK) {
	    gtk_knob_update_mouse (knob, x, y, FALSE);
	    return TRUE;
	}
	break;
    }

    return FALSE;
}


/*****************************************************************************
 *
 * GTK_KNOB_TIMER()
 *
 *****************************************************************************/
static gint
gtk_knob_timer(GtkKnob *knob) {

    g_return_val_if_fail (knob != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_KNOB (knob), FALSE);

    if (knob->policy == GTK_UPDATE_DELAYED) {
	gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
				 "value_changed");
    }

    /* don't keep running this timer */
    return FALSE;
}


/*****************************************************************************
 *
 * GTK_KNOB_UPDATE_MOUSE_UPDATE()
 *
 *****************************************************************************/
static void
gtk_knob_update_mouse_update(GtkKnob *knob) {

    if (knob->policy == GTK_UPDATE_CONTINUOUS) {
	gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
				 "value_changed");
    }
    else {
	knob->frame_offset = (int)(
	    (((gfloat)(knob->anim->width) /
	      (gfloat)(knob->anim->frame_width)) - 1) *
	    (knob->adjustment->value - knob->adjustment->lower) *
	    (1.0 / ((gfloat)(knob->adjustment->upper) -
		    (gfloat)(knob->adjustment->lower)))
	    ) * knob->width;
	gtk_widget_draw (GTK_WIDGET (knob), NULL);

	if (knob->policy == GTK_UPDATE_DELAYED) {
	    if (knob->timer) {
		gtk_timeout_remove (knob->timer);
	    }
	    knob->timer = gtk_timeout_add (SCROLL_DELAY_LENGTH,
					   (GtkFunction) gtk_knob_timer,
					   (gpointer) knob);
	}
    }
}


/*****************************************************************************
 *
 * GTK_KNOB_UPDATE_MOUSE()
 *
 *****************************************************************************/
static void
gtk_knob_update_mouse(GtkKnob *knob, gint x, gint y, gboolean absolute) {
    gfloat old_value, new_value, dv, dh;
    gfloat angle;

    g_return_if_fail (knob != NULL);
    g_return_if_fail (GTK_IS_KNOB (knob));

    old_value = knob->adjustment->value;

    angle = atan2f (-y + (knob->height >> 1), x - (knob->width >> 1));

    if (absolute) {
	/* map [1.25pi, -0.25pi] onto [0, 1] */
	angle *= M_1_PI;
	if (angle < -0.5) {
	    angle += 2.0;
	}
	new_value = -0.66666666666666666666 * (angle - 1.25);
	new_value *= knob->adjustment->upper - knob->adjustment->lower;
	new_value += knob->adjustment->lower;
    }
    else {
	/* inverted cartesian graphics coordinate system */
	dv = knob->saved_y - y;
	dh = x - knob->saved_x;
	knob->saved_x = x;
	knob->saved_y = y;

	if ((x >= 0) && (x <= knob->width)) {
	    dh = 0;  /* dead zone */
	}
	else {
	    angle = cosf (angle);
	    dh *= angle * angle;
	}

	new_value = knob->adjustment->value +
	    dv * knob->adjustment->step_increment +
	    dh * (knob->adjustment->upper -
		  knob->adjustment->lower) * 0.005; /* 0.005 == (1 / 200) */
    }

    new_value = MAX (MIN (new_value, knob->adjustment->upper),
		     knob->adjustment->lower);

    knob->adjustment->value = new_value;

    if (knob->adjustment->value != old_value) {
	gtk_knob_update_mouse_update (knob);
    }
}


/*****************************************************************************
 *
 * GTK_KNOB_UPDATE()
 *
 *****************************************************************************/
static void
gtk_knob_update(GtkKnob *knob) {
    gfloat new_value;

    g_return_if_fail (knob != NULL);
    g_return_if_fail (GTK_IS_KNOB (knob));

    if (knob->adjustment->step_increment == 1) {
	new_value = floor (knob->adjustment->value + 0.5);
    }
    else {
	new_value = knob->adjustment->value;
    }

    if (new_value < knob->adjustment->lower) {
	new_value = knob->adjustment->lower;
    }

    if (new_value > knob->adjustment->upper) {
	new_value = knob->adjustment->upper;
    }

    if (new_value != knob->adjustment->value) {
	knob->adjustment->value = new_value;
	gtk_signal_emit_by_name (GTK_OBJECT (knob->adjustment),
				 "value_changed");
    }

    gtk_widget_draw (GTK_WIDGET (knob), NULL);
}


/*****************************************************************************
 *
 * GTK_KNOB_ADJUSTMENT_CHANGED()
 *
 *****************************************************************************/
static void
gtk_knob_adjustment_changed(GtkAdjustment *adjustment, gpointer data) {
    GtkKnob *knob;

    g_return_if_fail (adjustment != NULL);
    g_return_if_fail (data != NULL);

    knob = GTK_KNOB (data);

    if ((knob->old_value != adjustment->value) ||
	(knob->old_lower != adjustment->lower) ||
	(knob->old_upper != adjustment->upper))
    {
	if (knob->anim != NULL) {
	    knob->frame_offset = (int)(
		(((gfloat)(knob->anim->width) /
		  (gfloat)(knob->anim->frame_width)) - 1) *
		(knob->adjustment->value - knob->adjustment->lower) *
		(1.0 / ((gfloat)(adjustment->upper) -
			(gfloat)(adjustment->lower)))
		) * knob->width;
	}
	gtk_knob_update (knob);

	knob->old_value = adjustment->value;
	knob->old_lower = adjustment->lower;
	knob->old_upper = adjustment->upper;

    }
}


/*****************************************************************************
 *
 * GTK_KNOB_ADJUSTMENT_VALUE_CHANGED()
 *
 *****************************************************************************/
static void
gtk_knob_adjustment_value_changed (GtkAdjustment *adjustment, gpointer data) {
    GtkKnob *knob;

    g_return_if_fail (adjustment != NULL);
    g_return_if_fail (data != NULL);

    knob = GTK_KNOB (data);

    if (adjustment->value > adjustment->upper) {
	adjustment->value = adjustment->upper;
    }
    if (adjustment->value < adjustment->lower) {
	adjustment->value = adjustment->lower;
    }
    if (knob->old_value != adjustment->value) {
	if (knob->anim != NULL) {
	    knob->frame_offset = (int)(
		(((gfloat)(knob->anim->width) /
		  (gfloat)(knob->anim->frame_width)) - 1) *
		(knob->adjustment->value - knob->adjustment->lower) *
		(1.0 / ((gfloat)(adjustment->upper) -
			(gfloat)(adjustment->lower)))
		) * knob->width;
	}
	gtk_knob_update (knob);
	knob->old_value = adjustment->value;
    }
}


/*****************************************************************************
 *
 * GTK_KNOB_SET_ANIMATION()
 *
 *****************************************************************************/
void
gtk_knob_set_animation (GtkKnob *knob, GtkKnobAnim *anim) {
    g_return_if_fail (knob != NULL);
    g_return_if_fail (anim != NULL);
    g_return_if_fail (GTK_IS_KNOB (knob));
    g_return_if_fail (GDK_IS_PIXBUF (anim->pixbuf));

    knob->anim   = (GtkKnobAnim *)anim;
    knob->width  = anim->frame_width;
    knob->height = anim->height;
    if (knob->adjustment != NULL) {
	knob->frame_offset = (int)(
	    (((gfloat)(knob->anim->width) /
	      (gfloat)(knob->anim->frame_width)) - 1) *
	    (knob->adjustment->value - knob->adjustment->lower) *
	    (1.0 / ((gfloat)(knob->adjustment->upper) -
		    (gfloat)(knob->adjustment->lower)))
	    ) * knob->width;
    }

    if (GTK_WIDGET_REALIZED (knob)) {
    	gtk_widget_queue_resize (GTK_WIDGET (knob));
    }
}


/*****************************************************************************
 *
 * GTK_KNOB_ANIMATION_NEW_FROM_FILE()
 *
 *****************************************************************************/
GtkKnobAnim *
gtk_knob_animation_new_from_file(gchar *filename) {
    GtkKnobAnim *anim;

    anim = gtk_knob_animation_new_from_file_full (filename, KNOB_WIDTH, -1, KNOB_HEIGHT);
    return anim;
}


/*****************************************************************************
 *
 * GTK_KNOB_ANIMATION_NEW_FROM_FILE_FULL()
 *
 * frame_width: overrides the frame width (to make rectangular frames)
 * but doesn't affect the image size width and height cause optional
 * scaling if not set to -1 when they are derived from the native
 * image size.
 *
 * FIXME: account for any problems where (width % frame_width != 0)
 *
 *****************************************************************************/
GtkKnobAnim *
gtk_knob_animation_new_from_file_full(gchar *filename, gint frame_width,
				      gint width, gint height) {
    GtkKnobAnim *anim = g_new0 (GtkKnobAnim, 1);

    g_return_val_if_fail ((filename != NULL), NULL);

#if GTK_MINOR_VERSION < 10
    if (!(anim->pixbuf = gdk_pixbuf_new_from_file (filename, &gerror))) {
	return NULL;
    }
#else /* GTK_MINOR_VERSION >= 10 */
    if (!(anim->pixbuf = gdk_pixbuf_new_from_file_at_size (filename, width,
							   height, &gerror))) {
	return NULL;
    }
#endif /* GTK_MINOR_VERSION < 10 */
    else {
	anim->height      = gdk_pixbuf_get_height (anim->pixbuf);
	anim->width       = gdk_pixbuf_get_width (anim->pixbuf);
	anim->frame_width = (frame_width != -1) ? frame_width : anim->height;
    }

    return anim;
}
