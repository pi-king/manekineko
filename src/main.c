#include <pebble.h>
#include <pebble_fonts.h>

// Settings
#define KEY_SHOW_DATE 1
#define KEY_SHOW_BATTERY 2
#define KEY_SHOW_BT 3
#define KEY_VIBE_BT 4
#define KEY_BKGND_COLOR 5
#define KEY_TEXT_COLOR 6

static Window *window;

static TextLayer *text_time_layer;
static TextLayer *text_date_layer;
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
static GBitmap *nothing;

static PropertyAnimation *mouth_animation_beg=NULL;
static PropertyAnimation *mouth_animation_end=NULL;
static PropertyAnimation *hand_animation_beg=NULL;
static PropertyAnimation *hand_animation_beg2=NULL;
static PropertyAnimation *hand_animation_end=NULL;
static PropertyAnimation *hand_animation_end2=NULL;

static GRect neko_rect;
static GRect mouth_from_rect;
static GRect mouth_to_rect;
static GRect hand_from_rect;
static GRect hand_to_rect;
static GRect lip_rect;
static GRect battery_rect;
static GRect bluetooth_rect;
static char time_text[] = "00:00";
static char time_text_buffer[] = "00:00";
static char date_text_string[12];
static int current_battery_level=-1;

static GColor text_color;
static GColor background_color;

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
	
	if (current_battery_level == -1){
		current_battery_level = charge_state.charge_percent;
	}

	static char battery_text[] = "100 ";
	if (charge_state.is_charging) {
		snprintf(battery_text, sizeof(battery_text), "+%d", charge_state.charge_percent);
		current_battery_level = charge_state.charge_percent;
	} else {
		snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
		if (current_battery_level > charge_state.charge_percent){
			current_battery_level = charge_state.charge_percent;
		}
	}
	text_layer_set_text(batterytext_layer, battery_text);
}

static void handle_bluetooth(bool connected) {
	Layer *rootLayer = window_get_root_layer(window);
	
	layer_remove_from_parent(bitmap_layer_get_layer(bluetoothLayer));
    bitmap_layer_destroy(bluetoothLayer);
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
	#ifdef PBL_PLATFORM_BASALT
	bitmap_layer_set_compositing_mode(bluetoothLayer, GCompOpSet);
	#endif
	if (persist_read_bool(KEY_SHOW_BT)) {
		if (connected) {
			bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
		} else {
			bitmap_layer_set_bitmap(bluetoothLayer, disconnect);
			if (persist_read_bool(KEY_VIBE_BT)) {
			vibes_long_pulse();
			APP_LOG(APP_LOG_LEVEL_DEBUG, "vibrate");
			}
		}
	}else{
		bitmap_layer_set_bitmap(bluetoothLayer, nothing);
	}
	layer_add_child(rootLayer, bitmap_layer_get_layer(bluetoothLayer));
	layer_mark_dirty(bitmap_layer_get_layer(bluetoothLayer));
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
	
	strftime(date_text_string, sizeof(date_text_string), "%d-%b-%Y", tick_time);
	text_layer_set_text(text_date_layer, date_text_string);

	
	animation_schedule((Animation*)mouth_animation_beg);
	animation_schedule((Animation*)hand_animation_beg);
	animation_schedule((Animation*)hand_animation_beg2);
	
}

static void window_load(Window *window) {

    Layer *rootLayer = window_get_root_layer(window);
	
	neko = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NEKO_SMALL);
    mouth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOUTH);
    lip = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LIP);
	
	bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_2);
	disconnect = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT_2);
	battery = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_2);
	nothing = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NOTHING_2);
	
	//don't know why but position is different in APLITE&BASALT
	#ifdef PBL_PLATFORM_APLITE
		neko_rect = GRect(23, 0, 105, 130); 
	#elif PBL_PLATFORM_BASALT
		neko_rect = GRect(19, 0, 105, 130);
	#endif
	
	mouth_from_rect = GRect(66, 52, 12, 8);
	mouth_to_rect = GRect(66, 60, 12, 8);
	hand_from_rect = GRect(97, 30, 37, 27);
	hand_to_rect = GRect(97, 45, 37, 27);
	lip_rect = GRect(64, 47, 16, 13);  //mouth x-2 y-5
	battery_rect = GRect(8,5,20,20);
	bluetooth_rect = GRect(124,5,20,20);
	
	batteryLayer = bitmap_layer_create(battery_rect);
	#ifdef PBL_PLATFORM_BASALT
	bitmap_layer_set_compositing_mode(batteryLayer, GCompOpSet);
	#endif
    bitmap_layer_set_bitmap(batteryLayer, battery);
	layer_add_child(rootLayer, bitmap_layer_get_layer(batteryLayer));
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
	#ifdef PBL_PLATFORM_BASALT
	bitmap_layer_set_compositing_mode(bluetoothLayer, GCompOpSet);
	#endif
    bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
	layer_add_child(rootLayer, bitmap_layer_get_layer(bluetoothLayer));
	
	nekoLayer = bitmap_layer_create(neko_rect);
	#ifdef PBL_PLATFORM_BASALT
	bitmap_layer_set_compositing_mode(nekoLayer, GCompOpSet);
	#endif
    bitmap_layer_set_bitmap(nekoLayer, neko);
    layer_add_child(rootLayer, bitmap_layer_get_layer(nekoLayer));
	
	mouthLayer = bitmap_layer_create(mouth_from_rect);
    bitmap_layer_set_bitmap(mouthLayer, mouth);
    layer_add_child(rootLayer, bitmap_layer_get_layer(mouthLayer));
	lipLayer = bitmap_layer_create(lip_rect);
    bitmap_layer_set_bitmap(lipLayer, lip);
	layer_add_child(rootLayer, bitmap_layer_get_layer(lipLayer));
	
	//make "transparent" effect by creating black & white layer with different compsiting modes
	hand1 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HAND_WHITE);
	hand2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HAND_BLACK);
	
	handLayer1 = bitmap_layer_create(hand_from_rect);
	bitmap_layer_set_compositing_mode(handLayer1, GCompOpOr);
    bitmap_layer_set_bitmap(handLayer1, hand1);
	
	handLayer2 = bitmap_layer_create(hand_from_rect);
	bitmap_layer_set_compositing_mode(handLayer2, GCompOpClear);
    bitmap_layer_set_bitmap(handLayer2, hand2);
	
	layer_add_child(rootLayer, bitmap_layer_get_layer(handLayer1));
	layer_add_child(rootLayer, bitmap_layer_get_layer(handLayer2));
		
	text_date_layer = text_layer_create(GRect(0, 145, 144, 35));
    text_layer_set_text_color(text_date_layer, text_color);
    text_layer_set_background_color(text_date_layer, GColorClear);
    text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
    text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(rootLayer, text_layer_get_layer(text_date_layer));
	
	
	text_time_layer = text_layer_create(GRect(0, 115, 144, 53));
    text_layer_set_text_color(text_time_layer, text_color);
    text_layer_set_background_color(text_time_layer, GColorClear);
    text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
    text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
    layer_add_child(rootLayer, text_layer_get_layer(text_time_layer));

	
	batterytext_layer = text_layer_create(GRect(3,18,30,20));
    text_layer_set_text_color(batterytext_layer, text_color);
    text_layer_set_background_color(batterytext_layer, GColorClear);
    text_layer_set_font(batterytext_layer, fonts_get_system_font(FONT_KEY_FONT_FALLBACK));
    text_layer_set_text_alignment(batterytext_layer, GTextAlignmentCenter);
	layer_add_child(rootLayer, text_layer_get_layer(batterytext_layer));
	
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
	text_layer_destroy(text_date_layer);
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

void update_settings() {
	#ifdef PBL_PLATFORM_APLITE
		text_color = GColorWhite;
		background_color = GColorBlack;
	#elif PBL_PLATFORM_BASALT
		GColor8 color;
		color.argb = persist_read_int(KEY_TEXT_COLOR);
		text_color = color;
		color.argb = persist_read_int(KEY_BKGND_COLOR);
		background_color = color;
	#endif
	
	text_layer_set_text_color(text_time_layer, text_color);
	
	window_set_background_color(window, background_color);
	
	if (persist_read_bool(KEY_SHOW_DATE)) {
		text_layer_set_text_color(text_date_layer, text_color);
		text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
	} else {
		text_layer_set_text_color(text_date_layer, background_color);
		text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
	}

	Layer *rootLayer = window_get_root_layer(window);
	layer_remove_from_parent(bitmap_layer_get_layer(batteryLayer));
	bitmap_layer_destroy(batteryLayer);
	
	batteryLayer = bitmap_layer_create(battery_rect);
	#ifdef PBL_PLATFORM_BASALT
	bitmap_layer_set_compositing_mode(batteryLayer, GCompOpSet);
	#endif
	if (persist_read_bool(KEY_SHOW_BATTERY)) {
		bitmap_layer_set_bitmap(batteryLayer, battery);
		text_layer_set_text_color(batterytext_layer, text_color);
	}else{
		bitmap_layer_set_bitmap(batteryLayer, nothing);
		text_layer_set_text_color(batterytext_layer, background_color);
	}
	layer_add_child(rootLayer, bitmap_layer_get_layer(batteryLayer));
	
	handle_bluetooth(bluetooth_connection_service_peek());
	
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
	// Get tuple
	Tuple *t = dict_read_first(iterator);
	
	while (t != NULL) {
		switch (t->key) {
			case KEY_SHOW_DATE:
				if (strcmp(t->value->cstring, "true") == 0) {
					persist_write_bool(KEY_SHOW_DATE, true);
				} else if (strcmp(t->value->cstring, "false") == 0) {
					persist_write_bool(KEY_SHOW_DATE, false);
				}
				break;
			case KEY_SHOW_BT:
				if (strcmp(t->value->cstring, "true") == 0) {
					persist_write_bool(KEY_SHOW_BT, true);
				} else if (strcmp(t->value->cstring, "false") == 0) {
					persist_write_bool(KEY_SHOW_BT, false);
				}
				break;
			case KEY_SHOW_BATTERY:
				if (strcmp(t->value->cstring, "true") == 0) {
					persist_write_bool(KEY_SHOW_BATTERY, true);
				} else if (strcmp(t->value->cstring, "false") == 0) {
					persist_write_bool(KEY_SHOW_BATTERY, false);
				}
				break;
			case KEY_VIBE_BT:
				if (strcmp(t->value->cstring, "true") == 0) {
					persist_write_bool(KEY_VIBE_BT, true);
				} else if (strcmp(t->value->cstring, "false") == 0) {
					persist_write_bool(KEY_VIBE_BT, false);
				}
				break;
			case KEY_TEXT_COLOR:
				persist_write_int(KEY_TEXT_COLOR, t->value->int32);
				break;
			case KEY_BKGND_COLOR:
				persist_write_int(KEY_BKGND_COLOR, t->value->int32);
				break;
		}
		
		// Get next pair, if any
    	t = dict_read_next(iterator);
	}
	
	update_settings();
}

static void init(void) {
	
	// Initialyze settings
	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	// Set default settings if needed
	if (persist_exists(KEY_SHOW_DATE) == false) {
		persist_write_bool(KEY_SHOW_DATE, true);
	}
	if (persist_exists(KEY_SHOW_BT) == false) {
		persist_write_bool(KEY_SHOW_BT, true);
	}
	if (persist_exists(KEY_SHOW_BATTERY) == false) {
		persist_write_bool(KEY_SHOW_BATTERY, true);
	}
	if (persist_exists(KEY_VIBE_BT) == false) {
		persist_write_bool(KEY_VIBE_BT, true);
	}
	
	#ifdef PBL_PLATFORM_BASALT
	if (persist_exists(KEY_TEXT_COLOR) == false) {
		persist_write_int(KEY_TEXT_COLOR, GColorWhiteARGB8);
	}
	if (persist_exists(KEY_BKGND_COLOR) == false) {
		persist_write_int(KEY_BKGND_COLOR, GColorOrangeARGB8);
	}
	#endif
	
    window = window_create();
	window_set_background_color(window,COLOR_FALLBACK(GColorOrange,GColorBlack));
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
	
	// Initially show the correct settings
	update_settings();
	
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