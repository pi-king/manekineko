#include <pebble.h>
#include <pebble_app_info.h>
#include <pebble_fonts.h>
	
static Window *window;

static TextLayer *text_time_layer;
static TextLayer *batterytext_layer;

static BitmapLayer *handLayer1;
static BitmapLayer *handLayer2;
static BitmapLayer *nekoLayer;
static BitmapLayer *lipLayer;
static BitmapLayer *mouthLayer;
static BitmapLayer *batteryLayer;
static BitmapLayer *bluetoothLayer;

static GBitmap *neko;
static GBitmap *mouth;
static GBitmap *lip;
static GBitmap *hand1;
static GBitmap *hand2;
static GBitmap *battery;
static GBitmap *bluetooth;
static GBitmap *disconnect;

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
static GRect battery_rect;
static GRect bluetooth_rect;
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

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100 ";
  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "+%d", charge_state.charge_percent);
  } else {
	  snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
	  if (charge_state.charge_percent==20){
	  	vibes_double_pulse();
	  }else if(charge_state.charge_percent==10){
	  	vibes_long_pulse();
	  }
  }
  text_layer_set_text(batterytext_layer, battery_text);
}

static void handle_bluetooth(bool connected) {
	layer_remove_from_parent(bitmap_layer_get_layer(bluetoothLayer));
    bitmap_layer_destroy(bluetoothLayer);
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
	if (connected) {
    bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
	} else {
	bitmap_layer_set_bitmap(bluetoothLayer, disconnect);
	vibes_long_pulse();
	}
	layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(bluetoothLayer));	
}

static void handle_appfocus(bool in_focus){
	if (in_focus){
	handle_bluetooth(bluetooth_connection_service_peek());
    }
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
	
	bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
	disconnect = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT);
	battery = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
	
	nekoLayer = bitmap_layer_create(layer_get_frame(rootLayer));
    bitmap_layer_set_bitmap(nekoLayer, neko);
    layer_add_child(rootLayer, bitmap_layer_get_layer(nekoLayer));
	
	mouth_from_rect = GRect(66, 57, 12, 8);
	mouth_to_rect = GRect(66, 65, 12, 8);
	hand_from_rect = GRect(97, 35, 37, 27);
	hand_to_rect = GRect(97, 50, 37, 27);
	lip_rect = GRect(64, 52, 16, 13);  //mouth x-2 y-5
	battery_rect = GRect(10,10,16,16);
	bluetooth_rect = GRect(124,10,20,20);
	
	mouthLayer = bitmap_layer_create(mouth_from_rect);
    bitmap_layer_set_bitmap(mouthLayer, mouth);
    layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(mouthLayer));
	lipLayer = bitmap_layer_create(lip_rect);
    bitmap_layer_set_bitmap(lipLayer, lip);
	layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(lipLayer));
	
	batteryLayer = bitmap_layer_create(battery_rect);
    bitmap_layer_set_bitmap(batteryLayer, battery);
	layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(batteryLayer));
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
    bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
	layer_add_child(bitmap_layer_get_layer(nekoLayer), bitmap_layer_get_layer(bluetoothLayer));
	
	
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
	
	batterytext_layer = text_layer_create(GRect(3,20,30,20));
    text_layer_set_text_color(batterytext_layer, GColorBlack);
    text_layer_set_background_color(batterytext_layer, GColorClear);
    text_layer_set_font(batterytext_layer, fonts_get_system_font(FONT_KEY_FONT_FALLBACK));
    text_layer_set_text_alignment(batterytext_layer, GTextAlignmentCenter);
	layer_add_child(bitmap_layer_get_layer(nekoLayer), text_layer_get_layer(batterytext_layer));
    
	handle_battery(battery_state_service_peek());
    handle_bluetooth(bluetooth_connection_service_peek());
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
	bitmap_layer_destroy(batteryLayer);
	bitmap_layer_destroy(bluetoothLayer);
    text_layer_destroy(text_time_layer);
	text_layer_destroy(batterytext_layer);
    
    bitmap_layer_destroy(nekoLayer);
	
    gbitmap_destroy(neko);
    gbitmap_destroy(mouth);
    gbitmap_destroy(lip);
	gbitmap_destroy(hand1);
	gbitmap_destroy(hand2);
	gbitmap_destroy(battery);
	gbitmap_destroy(bluetooth);
	gbitmap_destroy(disconnect);
	
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
	battery_state_service_subscribe(&handle_battery);
    bluetooth_connection_service_subscribe(&handle_bluetooth);
    app_focus_service_subscribe(&handle_appfocus);
}

static void deinit(void) {
	tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
	app_focus_service_unsubscribe();
    window_stack_remove(window, false);
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}