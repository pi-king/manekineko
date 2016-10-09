/****
Getting battery level part is from source of 11weeks watchface
Copyright (c) 2015 Programus
https://github.com/programus/pebble-watchface-11weeks/blob/master/LICENSE
*/

#include <pebble.h>
#include <pebble_fonts.h>


// Settings
#define KEY_SHOW_DATE 1
#define KEY_SHOW_BATTERY 2
#define KEY_SHOW_BT 3
#define KEY_VIBE_BT 4
#define KEY_BKGND_COLOR 5
#define KEY_TEXT_COLOR 6
#define KEY_SHOW_PHONE_BATT    7
#define KEY_PHONE_BATTERY    8
#define KEY_HOURLY_VIBE 9

#define UNKNOW_LEVEL    "XX"
#define FULL_LEVEL      "FL"

#define CHARGING_MASK   0x80
#define LEVEL_MASK      0x7f
#define BATTERY_API_UNSUPPORTED   0x70
#define LEVEL_UNKNOWN   0x7f


static Window *window;

static TextLayer *text_time_layer;
static TextLayer *text_date_layer;
static TextLayer *batterytext_layer;
static TextLayer *phone_batterytext_layer;

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
static GRect batterytext_rect;
static GRect phone_batterytext_rect;		
static GRect text_date_rect;
static GRect text_time_rect;

static char time_text[] = "00:00";
static char time_text_buffer[] = "00:00";
static char date_text_string[12];
static int current_battery_level=-1;

static bool is_phone_charging;
static AppSync s_sync;
static uint8_t* s_sync_buffer;
static bool s_battery_api_supported = false;
static void init_sync();
static void deinit_sync();
static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context);
static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context);
static void update_phone_battery(uint8_t state);


static GColor text_color;
static GColor background_color;

void destroy_property_animation(PropertyAnimation **prop_animation) {
    if (*prop_animation == NULL) {
        return;
    }
    if (animation_is_scheduled((Animation*) *prop_animation)) {
        animation_unschedule((Animation*) *prop_animation);
    }
	
	#ifdef PBL_BW
    property_animation_destroy(*prop_animation);
	#endif
	
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
	
	animation_set_duration((Animation*)mouth_animation_end, 800);
	animation_set_duration((Animation*)hand_animation_end, 800);
	animation_set_duration((Animation*)hand_animation_end2, 800);

	//animation_set_curve((Animation*)mouth_animation_end,AnimationCurveEaseOut);
	//animation_set_curve((Animation*)hand_animation_end,AnimationCurveEaseOut);
	//animation_set_curve((Animation*)hand_animation_end2,AnimationCurveEaseOut);
	
	
	animation_schedule((Animation*)mouth_animation_end);
	animation_schedule((Animation*)hand_animation_end);
	animation_schedule((Animation*)hand_animation_end2);

}

//This part is from source of 11weeks watchface
//Copyright (c) 2015 Programus
//https://github.com/programus/pebble-watchface-11weeks/blob/master/LICENSE
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
	
	if (!connected && persist_read_bool(KEY_VIBE_BT)){
		vibes_long_pulse();
	}
	if (!persist_read_bool(KEY_SHOW_BT)){
		if (bitmap_layer_get_bitmap(bluetoothLayer) == nothing){
			return;
		}
	}
	
	Layer *rootLayer = window_get_root_layer(window);
	layer_remove_from_parent(bitmap_layer_get_layer(bluetoothLayer));
	bitmap_layer_destroy(bluetoothLayer);
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
	#if defined(PBL_COLOR)
	bitmap_layer_set_compositing_mode(bluetoothLayer, GCompOpSet);
	#endif
	
	if (persist_read_bool(KEY_SHOW_BT)) {
		if (connected) {
			bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
		} else{ 
			bitmap_layer_set_bitmap(bluetoothLayer, disconnect);
		}
	}else {
		bitmap_layer_set_bitmap(bluetoothLayer, nothing);
	}
	
	layer_add_child(rootLayer, bitmap_layer_get_layer(bluetoothLayer));
	layer_mark_dirty(bitmap_layer_get_layer(bluetoothLayer));
	
}

void phone_battery_layer_update(uint8_t state) {
  
  // get number
    int num = state & LEVEL_MASK;
	if (num != BATTERY_API_UNSUPPORTED) {
		static char phone_battery_text[] = "100 ";
		if (state & CHARGING_MASK) {
			//show charging icon in bluetooth layer
			is_phone_charging = true;
			snprintf(phone_battery_text, sizeof(phone_battery_text), "+%d", num);
		} else {
			//show bluetooth icon in bluetooth layer
			is_phone_charging = false;
			snprintf(phone_battery_text, sizeof(phone_battery_text), "%d", num);
		}
		
		text_layer_set_text(phone_batterytext_layer, phone_battery_text);
		layer_mark_dirty( text_layer_get_layer(phone_batterytext_layer));
    }
}

static void handle_appfocus(bool in_focus){
	if (in_focus){
	#ifdef PBL_SDK_2
	handle_bluetooth(bluetooth_connection_service_peek());
	#elif PBL_SDK_3
	handle_bluetooth (connection_service_peek_pebble_app_connection());
	#endif
	
    }
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
		
	destroy_property_animation(&mouth_animation_beg);
	destroy_property_animation(&hand_animation_beg);
	destroy_property_animation(&hand_animation_beg2);
	
	mouth_animation_beg = property_animation_create_layer_frame(bitmap_layer_get_layer(mouthLayer), &mouth_from_rect, &mouth_to_rect);
	hand_animation_beg = property_animation_create_layer_frame(bitmap_layer_get_layer(handLayer1), &hand_from_rect, &hand_to_rect);
	hand_animation_beg2 = property_animation_create_layer_frame(bitmap_layer_get_layer(handLayer2), &hand_from_rect, &hand_to_rect);
	
	animation_set_duration((Animation*)mouth_animation_beg, 800);
	animation_set_duration((Animation*)hand_animation_beg, 800);
	animation_set_duration((Animation*)hand_animation_beg2, 800);
	
	//animation_set_curve((Animation*)mouth_animation_beg,AnimationCurveEaseOut);
	//animation_set_curve((Animation*)hand_animation_beg,AnimationCurveEaseOut);
	//animation_set_curve((Animation*)hand_animation_beg2,AnimationCurveEaseOut);
	
	animation_set_handlers((Animation*)hand_animation_beg, (AnimationHandlers) {
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
	
	if (persist_read_bool(KEY_HOURLY_VIBE) && tick_time->tm_min == 0) {
		vibes_double_pulse();
	}
	
	animation_schedule((Animation*)hand_animation_beg);
	animation_schedule((Animation*)hand_animation_beg2);
	animation_schedule((Animation*)mouth_animation_beg);
	
}

void updateLayers(){
	layer_set_frame(text_layer_get_layer(text_time_layer), text_time_rect);
	layer_set_frame(text_layer_get_layer(text_date_layer), text_date_rect);
	layer_set_frame(text_layer_get_layer(batterytext_layer), batterytext_rect);
	layer_set_frame(text_layer_get_layer(phone_batterytext_layer), phone_batterytext_rect);
	layer_set_frame(bitmap_layer_get_layer(handLayer1), hand_from_rect);
	layer_set_frame(bitmap_layer_get_layer(handLayer2), hand_from_rect);
	layer_set_frame(bitmap_layer_get_layer(nekoLayer), neko_rect);
	layer_set_frame(bitmap_layer_get_layer(lipLayer), lip_rect);
	layer_set_frame(bitmap_layer_get_layer(mouthLayer), mouth_from_rect);
	layer_set_frame(bitmap_layer_get_layer(batteryLayer), battery_rect);
	layer_set_frame(bitmap_layer_get_layer(bluetoothLayer), bluetooth_rect);
}

void calLayersPosition() {
	bool quickViewAppeared = false;
	int quickViewHeightDiff = 0;
	
    Layer *rootLayer = window_get_root_layer(window);
	
	GRect fullscreen = layer_get_bounds(rootLayer);
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(rootLayer);
	quickViewHeightDiff = fullscreen.size.h - unobstructed_bounds.size.h;
	if (quickViewHeightDiff > 0){
		quickViewAppeared = true;
	}
	
  if (!quickViewAppeared){
		neko_rect = GRect(PBL_IF_ROUND_ELSE(39,19), 0, 105, 130);
		mouth_from_rect = GRect(PBL_IF_ROUND_ELSE(86,66), 52, 12, 8);
		mouth_to_rect = GRect(PBL_IF_ROUND_ELSE(86,66), 59, 12, 8);
		hand_from_rect = GRect(PBL_IF_ROUND_ELSE(117,97), 30, 37, 27);
		hand_to_rect = GRect(PBL_IF_ROUND_ELSE(117,97), 45, 37, 27);
		lip_rect = GRect(PBL_IF_ROUND_ELSE(84,64), 47, 16, 13);  //mouth x-2 y-5
		
		battery_rect = GRect(PBL_IF_ROUND_ELSE(15,8),PBL_IF_ROUND_ELSE(75,1),20,20);
		bluetooth_rect = GRect(PBL_IF_ROUND_ELSE(150,121),PBL_IF_ROUND_ELSE(82,2),20,20);
    	batterytext_rect = GRect(PBL_IF_ROUND_ELSE(10,3),PBL_IF_ROUND_ELSE(90,18),30,20);
    	phone_batterytext_rect = GRect(PBL_IF_ROUND_ELSE(145,119),PBL_IF_ROUND_ELSE(90,18),25,20);
		
		text_date_rect = GRect(0, 145, PBL_IF_ROUND_ELSE(180,144), 35);
    	text_time_rect = GRect(0, 115, PBL_IF_ROUND_ELSE(180,144), 53);
	}else{
		neko_rect = GRect(PBL_IF_ROUND_ELSE(39,19), 0 + quickViewHeightDiff, 105, 130);
		mouth_from_rect = GRect(PBL_IF_ROUND_ELSE(86,66), 52 + quickViewHeightDiff, 12, 8);
		mouth_to_rect = GRect(PBL_IF_ROUND_ELSE(86,66), 59 + quickViewHeightDiff, 12, 8);
		hand_from_rect = GRect(PBL_IF_ROUND_ELSE(117,97), 30 + quickViewHeightDiff, 37, 27);
		hand_to_rect = GRect(PBL_IF_ROUND_ELSE(117,97), 45 + quickViewHeightDiff, 37, 27);
		lip_rect = GRect(PBL_IF_ROUND_ELSE(84,64), 47 + quickViewHeightDiff, 16, 13);  //mouth x-2 y-5
		
		battery_rect = GRect(PBL_IF_ROUND_ELSE(15,8),PBL_IF_ROUND_ELSE(75,1 + quickViewHeightDiff),20,20);
		bluetooth_rect = GRect(PBL_IF_ROUND_ELSE(150,121),PBL_IF_ROUND_ELSE(82,2 + quickViewHeightDiff),20,20);
    	batterytext_rect = GRect(PBL_IF_ROUND_ELSE(10,3),PBL_IF_ROUND_ELSE(90,18 + quickViewHeightDiff),30,20);
    	phone_batterytext_rect = GRect(PBL_IF_ROUND_ELSE(145,119),PBL_IF_ROUND_ELSE(90,18 + quickViewHeightDiff),25,20);
		
		text_date_rect = GRect(0, 30, PBL_IF_ROUND_ELSE(180,144), 35);
    	text_time_rect = GRect(0, 0, PBL_IF_ROUND_ELSE(180,144), 53);
	}
}

static void prv_unobstructed_did_change(void *context) {
	calLayersPosition();
	updateLayers();
}

static void window_load(Window *window) {
	Layer *rootLayer = window_get_root_layer(window);
	calLayersPosition();
	
	neko = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NEKO_SMALL);
    mouth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOUTH);
    lip = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LIP);
	
	bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_2);
	disconnect = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT_2);
	battery = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_2);
	nothing = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NOTHING_2);
	
	batteryLayer = bitmap_layer_create(battery_rect);
	#if defined(PBL_COLOR)
	bitmap_layer_set_compositing_mode(batteryLayer, GCompOpSet);
	#endif
    bitmap_layer_set_bitmap(batteryLayer, battery);
	layer_add_child(rootLayer, bitmap_layer_get_layer(batteryLayer));
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
	#if defined(PBL_COLOR)
	bitmap_layer_set_compositing_mode(bluetoothLayer, GCompOpSet);
	#endif
    bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
	layer_add_child(rootLayer, bitmap_layer_get_layer(bluetoothLayer));
	
	nekoLayer = bitmap_layer_create(neko_rect);
	#if defined(PBL_COLOR)
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
	
	batterytext_layer = text_layer_create(batterytext_rect);
    phone_batterytext_layer = text_layer_create(phone_batterytext_rect);
		
	text_date_layer = text_layer_create(text_date_rect);
    text_time_layer = text_layer_create(text_time_rect);
	
	text_layer_set_text_color(text_date_layer, text_color);
    text_layer_set_background_color(text_date_layer, GColorClear);
    text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
    text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(rootLayer, text_layer_get_layer(text_date_layer));
	
	
	text_layer_set_text_color(text_time_layer, text_color);
    text_layer_set_background_color(text_time_layer, GColorClear);
    text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
    text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
    layer_add_child(rootLayer, text_layer_get_layer(text_time_layer));
	
	text_layer_set_text_color(batterytext_layer, text_color);
    text_layer_set_background_color(batterytext_layer, GColorClear);
    text_layer_set_font(batterytext_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(batterytext_layer, GTextAlignmentCenter);
	layer_add_child(rootLayer, text_layer_get_layer(batterytext_layer));
	
	
	text_layer_set_text_color(phone_batterytext_layer, text_color);
    text_layer_set_background_color(phone_batterytext_layer, GColorClear);
    text_layer_set_font(phone_batterytext_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(phone_batterytext_layer, GTextAlignmentCenter);
	layer_add_child(rootLayer, text_layer_get_layer(phone_batterytext_layer));
	
	handle_battery(battery_state_service_peek());
	
	#ifdef PBL_SDK_2
	handle_bluetooth(bluetooth_connection_service_peek());
	#elif PBL_SDK_3
	handle_bluetooth(connection_service_peek_pebble_app_connection());
	#endif

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
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Update setting");
	
	#if defined(PBL_BW)
		text_color = GColorWhite;
		background_color = GColorBlack;
	#elif defined(PBL_COLOR)
		GColor8 color;
		color.argb = persist_read_int(KEY_TEXT_COLOR);
		text_color = color;
		color.argb = persist_read_int(KEY_BKGND_COLOR);
		background_color = color;
	#endif
	
	text_layer_set_text_color(text_time_layer, text_color);
	text_layer_set_text_color(phone_batterytext_layer, text_color);
	
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
	#if defined(PBL_COLOR)
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
		
	if (s_battery_api_supported && persist_read_bool(KEY_SHOW_PHONE_BATT)) {
		text_layer_set_text_color(phone_batterytext_layer, text_color);
	}else{
		text_layer_set_text_color(phone_batterytext_layer, background_color);
	}
	layer_mark_dirty( text_layer_get_layer(phone_batterytext_layer));
	
	#ifdef PBL_SDK_2
	handle_bluetooth(bluetooth_connection_service_peek());
	#elif PBL_SDK_3
	handle_bluetooth(connection_service_peek_pebble_app_connection());
	#endif
	
}


static void init_sync() {
  
	
  // setup initial value
  Tuplet initial_values[] = {
    
	  TupletInteger(KEY_SHOW_DATE, persist_read_bool(KEY_SHOW_DATE) ? 1 : 0),
	  TupletInteger(KEY_SHOW_BATTERY, persist_read_bool(KEY_SHOW_BATTERY)  ? 1 : 0),
	  TupletInteger(KEY_SHOW_BT, persist_read_bool(KEY_SHOW_BT)  ? 1 : 0),
	  TupletInteger(KEY_VIBE_BT, persist_read_bool(KEY_VIBE_BT)  ? 1 : 0), 
	  TupletInteger(KEY_BKGND_COLOR, persist_read_int(KEY_BKGND_COLOR)),
	  TupletInteger(KEY_TEXT_COLOR, persist_read_int(KEY_TEXT_COLOR)),
	  TupletInteger(KEY_SHOW_PHONE_BATT, persist_read_bool(KEY_SHOW_PHONE_BATT)  ? 1 : 0),
	  TupletInteger(KEY_PHONE_BATTERY, BATTERY_API_UNSUPPORTED),
	  TupletInteger(KEY_HOURLY_VIBE, persist_read_bool(KEY_HOURLY_VIBE)  ? 1 : 0)
	  
  };
	
  
  // create buffer
  uint32_t size = dict_calc_buffer_size_from_tuplets(initial_values, ARRAY_LENGTH(initial_values));
  s_sync_buffer = malloc(size * sizeof(uint8_t));
  
  app_message_open(size << 1, 0);
	
  // Begin using AppSync
  app_sync_init(&s_sync, s_sync_buffer, size, initial_values, ARRAY_LENGTH(initial_values), sync_changed_handler, sync_error_handler, NULL);
}

static void deinit_sync() {
  app_sync_deinit(&s_sync);
}

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_changed_handler called %lu", (unsigned long)key);
	switch (key) {
		case KEY_PHONE_BATTERY:
		{
		  update_phone_battery(new_tuple->value->int8);
		}
		break;
    
	case KEY_SHOW_DATE:
		if (persist_read_bool(KEY_SHOW_DATE) != new_tuple->value->int8){
			if (new_tuple->value->int8 > 0) {
				persist_write_bool(KEY_SHOW_DATE, true);
			} else {
				persist_write_bool(KEY_SHOW_DATE, false);
			}
			update_settings();
		}
		break;
	case KEY_SHOW_BT:
		if (persist_read_bool(KEY_SHOW_BT) != new_tuple->value->int8){
			if (new_tuple->value->int8 > 0) {
				persist_write_bool(KEY_SHOW_BT, true);
			} else {
			persist_write_bool(KEY_SHOW_BT, false);
			}
			update_settings();
		}
		break;
	case KEY_SHOW_BATTERY:
		if (persist_read_bool(KEY_SHOW_BATTERY) != new_tuple->value->int8){
			if (new_tuple->value->int8 > 0) {
				persist_write_bool(KEY_SHOW_BATTERY, true);
			} else {
				persist_write_bool(KEY_SHOW_BATTERY, false);
			}
			update_settings();
		}
		break;
	case KEY_VIBE_BT:
		if (persist_read_bool(KEY_VIBE_BT) != new_tuple->value->int8){
			if (new_tuple->value->int8 > 0) {
				persist_write_bool(KEY_VIBE_BT, true);
			} else {
				persist_write_bool(KEY_VIBE_BT, false);
			}
			update_settings();
		}
		break;
	case KEY_SHOW_PHONE_BATT:
		if (persist_read_bool(KEY_SHOW_PHONE_BATT) != new_tuple->value->int8){
			if (new_tuple->value->int8 > 0) {
				persist_write_bool(KEY_SHOW_PHONE_BATT, true);
			} else {
				persist_write_bool(KEY_SHOW_PHONE_BATT, false);
			}
			update_settings();
		}
				break;
	  case KEY_HOURLY_VIBE:
		if (persist_read_bool(KEY_HOURLY_VIBE) != new_tuple->value->int8){
			if (new_tuple->value->int8 > 0) {
				persist_write_bool(KEY_HOURLY_VIBE, true);
			} else {
				persist_write_bool(KEY_HOURLY_VIBE, false);
			}
			update_settings();
		}
		break;
	case KEY_TEXT_COLOR:
		if (persist_read_int(KEY_TEXT_COLOR) != new_tuple->value->int32){
			persist_write_int(KEY_TEXT_COLOR, new_tuple->value->int32);
			update_settings();
		}
		break;
	case KEY_BKGND_COLOR:
		if (persist_read_int(KEY_BKGND_COLOR) != new_tuple->value->int32){
			persist_write_int(KEY_BKGND_COLOR, new_tuple->value->int32);
			update_settings();
		}
		break;  		
	}
	
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_error %d", dict_error);
}

static void update_phone_battery(uint8_t state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "get battery state: %d, %d", state & CHARGING_MASK, state & LEVEL_MASK);
  s_battery_api_supported = !(state == BATTERY_API_UNSUPPORTED);
  if (s_battery_api_supported && persist_read_bool(KEY_SHOW_PHONE_BATT)) {
	text_layer_set_text_color(phone_batterytext_layer, text_color);
    phone_battery_layer_update(state);
  }else{
	text_layer_set_text_color(phone_batterytext_layer, background_color);
  }
}

static void init(void) {
		
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
	if (persist_exists(KEY_SHOW_PHONE_BATT) == false) {
		persist_write_bool(KEY_SHOW_PHONE_BATT, true);
	}
	if (persist_exists(KEY_HOURLY_VIBE) == false) {
		persist_write_bool(KEY_HOURLY_VIBE, false);
	}
	
	#if defined(PBL_COLOR)
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
	
	#ifdef PBL_SDK_2
	APP_LOG(APP_LOG_LEVEL_DEBUG, "SDK2");
	bluetooth_connection_service_subscribe(handle_bluetooth);
	#elif PBL_SDK_3
	APP_LOG(APP_LOG_LEVEL_DEBUG, "SDK3");
	connection_service_subscribe((ConnectionHandlers) {
	  .pebble_app_connection_handler = handle_bluetooth
	});
	UnobstructedAreaHandlers handler = {
    .did_change = prv_unobstructed_did_change
  	};
  	unobstructed_area_service_subscribe(handler, NULL);
	#endif
	
	// init AppSync
    init_sync();
	
    app_focus_service_subscribe(&handle_appfocus);
	
	// Initially show the correct settings
	update_settings();
	
}

static void deinit(void) {
	tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
	
	#ifdef PBL_SDK_2
	bluetooth_connection_service_unsubscribe();
	#elif PBL_SDK_3
	connection_service_unsubscribe();
	unobstructed_area_service_unsubscribe();
	#endif
	
	deinit_sync();
	
	app_focus_service_unsubscribe();
    window_stack_remove(window, false);
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}