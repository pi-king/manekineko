#include <pebble.h>
#include <pebble_app_info.h>
#include <pebble_fonts.h>


static Window *window;

static TextLayer *text_time_layer;

static BitmapLayer *handLayer1;
static BitmapLayer *handLayer2;
static BitmapLayer *nekoLayer;
static BitmapLayer *lipLayer;
static BitmapLayer *mouthLayer;


static GBitmap *neko;
static GBitmap *mouth;
static GBitmap *lip;
static GBitmap *hand1;
static GBitmap *hand2;

static PropertyAnimation *mouth_animation_beg=NULL;
static PropertyAnimation *mouth_animation_end=NULL;
static PropertyAnimation *hand_animation_beg=NULL;
static PropertyAnimation *hand_animation_beg2=NULL;
static PropertyAnimation *hand_animation_end=NULL;
static PropertyAnimation *hand_animation_end2=NULL;

static GRect mouth_from_rect;
static GRect mouth_to_rect;
static GRect hand_from_rect;
static GRect hand_to_rect;
static GRect lip_rect;

static char time_text[] = "00:00";
static char time_text_buffer[] = "00:00";

void destroy_property_animation(PropertyAnimation **prop_animation) {
    if (*prop_animation == NULL) {
        return;
    }

    if (animation_is_scheduled((Animation*) *prop_animation)) {
        animation_unschedule((Animation*) *prop_animation);
    }

    property_animation_destroy(*prop_animation);
    *prop_animation = NULL;
}

void animation_started(Animation *animation, void *data) {
	(void)animation;
	(void)data;
}

void animation_stopped(Animation *animation, void *data) {
	(void)animation;
	(void)data;

	memcpy(time_text, time_text_buffer, strlen(time_text)+1);
        text_layer_set_text(text_time_layer, time_text);
	
	destroy_property_animation(&mouth_animation_end);
	destroy_property_animation(&hand_animation_end);
	destroy_property_animation(&hand_animation_end2);
	
	mouth_animation_end = property_animation_create_layer_frame(bitmap_layer_get_layer(mouthLayer), &mouth_to_rect, &mouth_from_rect);
	hand_animation_end = property_animation_create_layer_frame(bitmap_layer_get_layer(handLayer1), &hand_to_rect, &hand_from_rect);
	hand_animation_end2 = property_animation_create_layer_frame(bitmap_layer_get_layer(handLayer2), &hand_to_rect, &hand_from_rect);

	animation_set_duration((Animation*)mouth_animation_end, 500);
	animation_set_duration((Animation*)hand_animation_end, 1000);
	animation_set_duration((Animation*)hand_animation_end2, 1000);
	
	animation_set_curve((Animation*)mouth_animation_end,AnimationCurveEaseOut);
	animation_set_curve((Animation*)hand_animation_end,AnimationCurveEaseOut);
	animation_set_curve((Animation*)hand_animation_end2,AnimationCurveEaseOut);

	animation_schedule((Animation*)mouth_animation_end);
	animation_schedule((Animation*)hand_animation_end);
	animation_schedule((Animation*)hand_animation_end2);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	destroy_property_animation(&mouth_animation_beg);
	destroy_property_animation(&hand_animation_beg);
	destroy_property_animation(&hand_animation_beg2);

	mouth_animation_beg = property_animation_create_layer_frame(bitmap_layer_get_layer(mouthLayer), &mouth_from_rect, &mouth_to_rect);
	hand_animation_beg = property_animation_create_layer_frame(bitmap_layer_get_layer(handLayer1), &hand_from_rect, &hand_to_rect);
	hand_animation_beg2 = property_animation_create_layer_frame(bitmap_layer_get_layer(handLayer2), &hand_from_rect, &hand_to_rect);
	
	
	animation_set_duration((Animation*)mouth_animation_beg, 500);
	animation_set_duration((Animation*)hand_animation_beg, 1000);
	animation_set_duration((Animation*)hand_animation_beg2, 1000);
	
	animation_set_curve((Animation*)mouth_animation_beg,AnimationCurveEaseOut);
	animation_set_curve((Animation*)hand_animation_beg,AnimationCurveEaseOut);
	animation_set_curve((Animation*)hand_animation_beg2,AnimationCurveEaseOut);

	animation_set_handlers((Animation*)mouth_animation_beg, (AnimationHandlers) {
		.started = (AnimationStartedHandler) animation_started,
		.stopped = (AnimationStoppedHandler) animation_stopped
	}, 0);

	// section based on Simplicity by Pebble Team begins
	char *time_format;
	if (clock_is_24h_style()) {
      time_format = "%R";
    } else {
      time_format = "%I:%M";
    }

    strftime(time_text_buffer, sizeof(time_text_buffer), time_format, tick_time);

    if (!clock_is_24h_style() && (time_text_buffer[0] == '0')) {
      memmove(time_text_buffer, &time_text_buffer[1], sizeof(time_text_buffer) - 1);
    }
	// section ends

	animation_schedule((Animation*)mouth_animation_beg);
	animation_schedule((Animation*)hand_animation_beg);
	animation_schedule((Animation*)hand_animation_beg2);
}

static void window_load(Window *window) {
    Layer *rootLayer = window_get_root_layer(window);
	
	neko = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NEKO);
    mouth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOUTH);
    lip = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LIP);
	
	nekoLayer = bitmap_layer_create(layer_get_frame(rootLayer));
    bitmap_layer_set_bitmap(nekoLayer, neko);
    layer_add_child(rootLayer, bitmap_layer_get_layer(nekoLayer));
	
	mouth_from_rect = GRect(66, 57, 12, 8);
	mouth_to_rect = GRect(66, 65, 12, 8);
	hand_from_rect = GRect(97, 35, 37, 27);
	hand_to_rect = GRect(97, 50, 37, 27);
	lip_rect = GRect(64, 52, 16, 13);  //mouth x-2 y-5
	
	mouthLayer = bitmap_layer_create(mouth_from_rect);
    bitmap_layer_set_bitmap(mouthLayer, mouth);
    layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(mouthLayer));

	lipLayer = bitmap_layer_create(lip_rect);
    bitmap_layer_set_bitmap(lipLayer, lip);
	layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(lipLayer));
	
	//make "transparent" effect by creating black & white layer with different compsiting modes
	hand1 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HAND_WHITE);
	hand2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HAND_BLACK);
	
	handLayer1 = bitmap_layer_create(hand_from_rect);
	bitmap_layer_set_compositing_mode(handLayer1, GCompOpOr);
    bitmap_layer_set_bitmap(handLayer1, hand1);
	
	handLayer2 = bitmap_layer_create(hand_from_rect);
	bitmap_layer_set_compositing_mode(handLayer2, GCompOpClear);
    bitmap_layer_set_bitmap(handLayer2, hand2);
	
	layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(handLayer1));
	layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(handLayer2));
	
	text_time_layer = text_layer_create(GRect(0, 120, 144, 50));
    text_layer_set_text_color(text_time_layer, GColorBlack);
    text_layer_set_background_color(text_time_layer, GColorClear);
    text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
    text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    layer_add_child(bitmap_layer_get_layer(nekoLayer), text_layer_get_layer(text_time_layer));
}


static void window_unload(Window *window) {
    destroy_property_animation(&mouth_animation_beg);
    destroy_property_animation(&hand_animation_beg);
	destroy_property_animation(&hand_animation_beg2);
    destroy_property_animation(&mouth_animation_end);
    destroy_property_animation(&hand_animation_end);
	destroy_property_animation(&hand_animation_end2);

    bitmap_layer_destroy(handLayer1);
	bitmap_layer_destroy(handLayer2);
    text_layer_destroy(text_time_layer);
    
    bitmap_layer_destroy(nekoLayer);
	
    gbitmap_destroy(neko);
    gbitmap_destroy(mouth);
    gbitmap_destroy(lip);
	gbitmap_destroy(hand1);
	gbitmap_destroy(hand2);
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    const bool animated = true;
    window_stack_push(window, animated);
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    window_stack_remove(window, false);
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}