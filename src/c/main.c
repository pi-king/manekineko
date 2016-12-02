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

#define BASE_HEIGHT 168
#define BASE_WIDTH 144
#define ROUND_BASE_HEIGHT 180
#define ROUND_BASE_WIDTH 180

static Window *window;

static Layer *baseLayer;

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
static bool s_battery_api_supported = false;
static void init_app_message();
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
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
	
	#if defined(PBL_BW)
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
	layer_mark_dirty( text_layer_get_layer(batterytext_layer));
}

static void handle_bluetooth(bool connected) {
	
	if (!connected && persist_read_bool(KEY_VIBE_BT)){
		if (!quiet_time_is_active()){
			vibes_long_pulse();
		}
	}
	if (!persist_read_bool(KEY_SHOW_BT)){
		if (bitmap_layer_get_bitmap(bluetoothLayer) == nothing){
			return;
		}
	}
	
	//Layer *rootLayer = window_get_root_layer(window);
	layer_remove_from_parent(bitmap_layer_get_layer(bluetoothLayer));
	bitmap_layer_destroy(bluetoothLayer);
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
	
	bitmap_layer_set_compositing_mode(bluetoothLayer, GCompOpSet);
	
	
	if (persist_read_bool(KEY_SHOW_BT)) {
		if (connected) {
			bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
		} else{ 
			bitmap_layer_set_bitmap(bluetoothLayer, disconnect);
		}
	}else {
		bitmap_layer_set_bitmap(bluetoothLayer, nothing);
	}
	
	layer_add_child(baseLayer, bitmap_layer_get_layer(bluetoothLayer));
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
		handle_bluetooth (connection_service_peek_pebble_app_connection());
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
		if (!quiet_time_is_active()){
			vibes_double_pulse();
		}
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
		neko_rect = GRect(19 + PBL_IF_ROUND_ELSE(18,0), 0, 105, 130);
		mouth_from_rect = GRect(66 + PBL_IF_ROUND_ELSE(18,0), 52, 12, 8);
		mouth_to_rect = GRect(66 + PBL_IF_ROUND_ELSE(18,0), 59, 12, 8);
		hand_from_rect = GRect(97 + PBL_IF_ROUND_ELSE(18,0), 30, 37, 27);
		hand_to_rect = GRect(97 + PBL_IF_ROUND_ELSE(18,0), 45, 37, 27);
		lip_rect = GRect(64 + PBL_IF_ROUND_ELSE(18,0), 47, 16, 13);  //mouth x-2 y-5
		
		battery_rect = GRect(PBL_IF_ROUND_ELSE(15,8),PBL_IF_ROUND_ELSE(75,1),20,20);
		bluetooth_rect = GRect(PBL_IF_ROUND_ELSE(151,121),PBL_IF_ROUND_ELSE(82,2),20,20);
    	batterytext_rect = GRect(PBL_IF_ROUND_ELSE(10,3),PBL_IF_ROUND_ELSE(90,18),30,20);
    	phone_batterytext_rect = GRect(PBL_IF_ROUND_ELSE(145,119),PBL_IF_ROUND_ELSE(90,18),25,20);
		
		text_date_rect = GRect(0, 145, PBL_IF_ROUND_ELSE(ROUND_BASE_WIDTH,BASE_WIDTH), 35);
    	text_time_rect = GRect(0, 115, PBL_IF_ROUND_ELSE(ROUND_BASE_WIDTH,BASE_WIDTH), 53);
	}else{
	    neko_rect = GRect(19 + PBL_IF_ROUND_ELSE(18,0), 0 + quickViewHeightDiff, 105, 130);
		mouth_from_rect = GRect(66 + PBL_IF_ROUND_ELSE(18,0), 52 + quickViewHeightDiff, 12, 8);
		mouth_to_rect = GRect(66 + PBL_IF_ROUND_ELSE(18,0), 59 + quickViewHeightDiff, 12, 8);
		hand_from_rect = GRect(97 + PBL_IF_ROUND_ELSE(18,0), 30 + quickViewHeightDiff, 37, 27);
		hand_to_rect = GRect(97 + PBL_IF_ROUND_ELSE(18,0), 45 + quickViewHeightDiff, 37, 27);
		lip_rect = GRect(64 + PBL_IF_ROUND_ELSE(18,0), 47 + quickViewHeightDiff, 16, 13);  //mouth x-2 y-5
		
		battery_rect = GRect(PBL_IF_ROUND_ELSE(15,8),PBL_IF_ROUND_ELSE(75,1 + quickViewHeightDiff),20,20);
		bluetooth_rect = GRect(PBL_IF_ROUND_ELSE(150,121),PBL_IF_ROUND_ELSE(82,2 + quickViewHeightDiff),20,20);
    	batterytext_rect = GRect(PBL_IF_ROUND_ELSE(10,3),PBL_IF_ROUND_ELSE(90,18 + quickViewHeightDiff),30,20);
    	phone_batterytext_rect = GRect(PBL_IF_ROUND_ELSE(145,119),PBL_IF_ROUND_ELSE(90,18 + quickViewHeightDiff),25,20);
		
		text_date_rect = GRect(0, 30, PBL_IF_ROUND_ELSE(ROUND_BASE_WIDTH,BASE_WIDTH), 35);
    	text_time_rect = GRect(0, 0, PBL_IF_ROUND_ELSE(ROUND_BASE_WIDTH,BASE_WIDTH), 53);
	}
}

static void prv_unobstructed_did_change(void *context) {
	calLayersPosition();
	updateLayers();
}

static void window_load(Window *window) {
	Layer *rootLayer = window_get_root_layer(window);
	GRect rootRect = layer_get_frame(rootLayer);
	int rootHeight = rootRect.size.h;
	int rootWidth = rootRect.size.w;
	int x = (rootHeight - PBL_IF_ROUND_ELSE(ROUND_BASE_HEIGHT, BASE_HEIGHT))/2;
	int y = (rootWidth - PBL_IF_ROUND_ELSE(ROUND_BASE_WIDTH,BASE_WIDTH))/2;
	baseLayer = layer_create(GRect(x, y, PBL_IF_ROUND_ELSE(ROUND_BASE_WIDTH, BASE_WIDTH), PBL_IF_ROUND_ELSE(ROUND_BASE_HEIGHT,BASE_HEIGHT)));
	
	layer_add_child(rootLayer, baseLayer);
	
	calLayersPosition();
	
	neko = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NEKO_SMALL);
    mouth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOUTH);
    lip = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LIP);
	
	bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_2);
	disconnect = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT_2);
	battery = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_2);
	nothing = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NOTHING_2);
	
	batteryLayer = bitmap_layer_create(battery_rect);
	
	bitmap_layer_set_compositing_mode(batteryLayer, GCompOpSet);
	
    bitmap_layer_set_bitmap(batteryLayer, battery);
	layer_add_child(baseLayer, bitmap_layer_get_layer(batteryLayer));
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
	
	bitmap_layer_set_compositing_mode(bluetoothLayer, GCompOpSet);
	
    bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
	layer_add_child(baseLayer, bitmap_layer_get_layer(bluetoothLayer));
	
	nekoLayer = bitmap_layer_create(neko_rect);
	
	bitmap_layer_set_compositing_mode(nekoLayer, GCompOpSet);
	
    bitmap_layer_set_bitmap(nekoLayer, neko);
    layer_add_child(baseLayer, bitmap_layer_get_layer(nekoLayer));
	
	mouthLayer = bitmap_layer_create(mouth_from_rect);
    bitmap_layer_set_bitmap(mouthLayer, mouth);
    layer_add_child(baseLayer, bitmap_layer_get_layer(mouthLayer));
	
	lipLayer = bitmap_layer_create(lip_rect);
    bitmap_layer_set_bitmap(lipLayer, lip);
	layer_add_child(baseLayer, bitmap_layer_get_layer(lipLayer));
	
	//make "transparent" effect by creating black & white layer with different compsiting modes
	hand1 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HAND_WHITE);
	hand2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HAND_BLACK);
	
	handLayer1 = bitmap_layer_create(hand_from_rect);
	bitmap_layer_set_compositing_mode(handLayer1, GCompOpOr);
    bitmap_layer_set_bitmap(handLayer1, hand1);
	
	handLayer2 = bitmap_layer_create(hand_from_rect);
	bitmap_layer_set_compositing_mode(handLayer2, GCompOpClear);
    bitmap_layer_set_bitmap(handLayer2, hand2);
	
	layer_add_child(baseLayer, bitmap_layer_get_layer(handLayer1));
	layer_add_child(baseLayer, bitmap_layer_get_layer(handLayer2));
	
	batterytext_layer = text_layer_create(batterytext_rect);
    phone_batterytext_layer = text_layer_create(phone_batterytext_rect);
		
	text_date_layer = text_layer_create(text_date_rect);
    text_time_layer = text_layer_create(text_time_rect);
	
	text_layer_set_text_color(text_date_layer, text_color);
    text_layer_set_background_color(text_date_layer, GColorClear);
    text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
    text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(baseLayer, text_layer_get_layer(text_date_layer));
	
	
	text_layer_set_text_color(text_time_layer, text_color);
    text_layer_set_background_color(text_time_layer, GColorClear);
    text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
    text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
    layer_add_child(baseLayer, text_layer_get_layer(text_time_layer));
	
	text_layer_set_text_color(batterytext_layer, text_color);
    text_layer_set_background_color(batterytext_layer, GColorClear);
    text_layer_set_font(batterytext_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(batterytext_layer, GTextAlignmentCenter);
	layer_add_child(baseLayer, text_layer_get_layer(batterytext_layer));
	
	
	text_layer_set_text_color(phone_batterytext_layer, text_color);
    text_layer_set_background_color(phone_batterytext_layer, GColorClear);
    text_layer_set_font(phone_batterytext_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(phone_batterytext_layer, GTextAlignmentCenter);
	layer_add_child(baseLayer, text_layer_get_layer(phone_batterytext_layer));
	
	handle_battery(battery_state_service_peek());
	
	handle_bluetooth(connection_service_peek_pebble_app_connection());

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
	
	layer_destroy(baseLayer);
	
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
		if (GColorBlackARGB8 == persist_read_int(KEY_TEXT_COLOR) || 
		   GColorWhiteARGB8 == persist_read_int(KEY_TEXT_COLOR)){
			GColor8 colorBW;
			colorBW.argb = persist_read_int(KEY_TEXT_COLOR);
			text_color = colorBW;
		}else{
			text_color = GColorWhite;
		}
		if (GColorBlackARGB8 == persist_read_int(KEY_BKGND_COLOR) || 
		   GColorWhiteARGB8 == persist_read_int(KEY_BKGND_COLOR)){
			GColor8 colorBW;
			colorBW.argb = persist_read_int(KEY_BKGND_COLOR);
			background_color = colorBW;
		}else{
			background_color = GColorBlack;
		}
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
	
	//Layer *rootLayer = window_get_root_layer(window);
	layer_remove_from_parent(bitmap_layer_get_layer(batteryLayer));
	bitmap_layer_destroy(batteryLayer);
	
	batteryLayer = bitmap_layer_create(battery_rect);
	
	bitmap_layer_set_compositing_mode(batteryLayer, GCompOpSet);
	
	if (persist_read_bool(KEY_SHOW_BATTERY)) {
		bitmap_layer_set_bitmap(batteryLayer, battery);
		text_layer_set_text_color(batterytext_layer, text_color);
	}else{
		bitmap_layer_set_bitmap(batteryLayer, nothing);
		text_layer_set_text_color(batterytext_layer, background_color);
	}
	layer_add_child(baseLayer, bitmap_layer_get_layer(batteryLayer));
		
	if (s_battery_api_supported && persist_read_bool(KEY_SHOW_PHONE_BATT)) {
		text_layer_set_text_color(phone_batterytext_layer, text_color);
	}else{
		text_layer_set_text_color(phone_batterytext_layer, background_color);
	}
	layer_mark_dirty( text_layer_get_layer(phone_batterytext_layer));
	
	handle_bluetooth(connection_service_peek_pebble_app_connection());
}


static void init_app_message() {
	
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
  
  // calculate size of buffer
  uint32_t size = dict_calc_buffer_size_from_tuplets(initial_values, ARRAY_LENGTH(initial_values));
  
	APP_LOG(APP_LOG_LEVEL_DEBUG, "size of buffer is %lu", size * sizeof(uint8_t));
	
	// Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
	
	app_message_open(size << 1, 0);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  // A message was received, but had to be dropped
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *show_battery_tuple = dict_find(iterator, MESSAGE_KEY_KEY_SHOW_BATTERY);
  if(show_battery_tuple != NULL) {
	  if (persist_read_bool(KEY_SHOW_BATTERY) != (bool)show_battery_tuple->value->int8){
	  		if (show_battery_tuple->value->int8 > 0) {
				persist_write_bool(KEY_SHOW_BATTERY, true);
			} else {
				persist_write_bool(KEY_SHOW_BATTERY, false);
			}
			update_settings();
	  }
  }
	
	Tuple *show_date_tuple = dict_find(iterator, MESSAGE_KEY_KEY_SHOW_DATE);
  if(show_date_tuple != NULL) {
	  if (persist_read_bool(KEY_SHOW_DATE) != (bool)show_date_tuple->value->int8){
	  		if (show_date_tuple->value->int8 > 0) {
				persist_write_bool(KEY_SHOW_DATE, true);
			} else {
				persist_write_bool(KEY_SHOW_DATE, false);
			}
			update_settings();
	  }
  }
	
	Tuple *show_bt_tuple = dict_find(iterator, MESSAGE_KEY_KEY_SHOW_BT);
  if(show_bt_tuple != NULL) {
	  if (persist_read_bool(KEY_SHOW_BT) != (bool)show_bt_tuple->value->int8){
	  		if (show_bt_tuple->value->int8 > 0) {
				persist_write_bool(KEY_SHOW_BT, true);
			} else {
				persist_write_bool(KEY_SHOW_BT, false);
			}
			update_settings();
	  }
  }
	
	Tuple *vibe_bt_tuple = dict_find(iterator, MESSAGE_KEY_KEY_VIBE_BT);
  if(vibe_bt_tuple != NULL) {
	  if (persist_read_bool(KEY_VIBE_BT) != (bool)vibe_bt_tuple->value->int8){
	  		if (vibe_bt_tuple->value->int8 > 0) {
				persist_write_bool(KEY_VIBE_BT, true);
			} else {
				persist_write_bool(KEY_VIBE_BT, false);
			}
			update_settings();
	  }
  }
	
	Tuple *show_phone_batt_tuple = dict_find(iterator, MESSAGE_KEY_KEY_SHOW_PHONE_BATT);
  if(show_phone_batt_tuple != NULL) {
	  if (persist_read_bool(KEY_SHOW_PHONE_BATT) != (bool)show_phone_batt_tuple->value->int8){
	  		if (show_phone_batt_tuple->value->int8 > 0) {
				persist_write_bool(KEY_SHOW_PHONE_BATT, true);
			} else {
				persist_write_bool(KEY_SHOW_PHONE_BATT, false);
			}
			update_settings();
	  }
  }
	
	Tuple *hourly_vibe_tuple = dict_find(iterator, MESSAGE_KEY_KEY_HOURLY_VIBE);
  if(hourly_vibe_tuple != NULL) {
	  if (persist_read_bool(KEY_HOURLY_VIBE) != (bool)hourly_vibe_tuple->value->int8){
	  		if (hourly_vibe_tuple->value->int8 > 0) {
				persist_write_bool(KEY_HOURLY_VIBE, true);
			} else {
				persist_write_bool(KEY_HOURLY_VIBE, false);
			}
			update_settings();
	  }
  }
	
	Tuple *phone_battery_tuple = dict_find(iterator, MESSAGE_KEY_KEY_PHONE_BATTERY);
  if(phone_battery_tuple != NULL) {
		update_phone_battery(phone_battery_tuple->value->int8);
  }
	  
	  Tuple *background_color_tuple = dict_find(iterator, MESSAGE_KEY_KEY_BKGND_COLOR);
  if(background_color_tuple != NULL) {
	  if (persist_read_int(KEY_BKGND_COLOR) != background_color_tuple->value->int32){
		  if (persist_read_int(KEY_BKGND_COLOR) != background_color_tuple->value->int32){
			persist_write_int(KEY_BKGND_COLOR, background_color_tuple->value->int32);
			update_settings();
		  }
	  }
  }
	  
	Tuple *text_color_tuple = dict_find(iterator, MESSAGE_KEY_KEY_TEXT_COLOR);
  if(text_color_tuple != NULL) {
	if (persist_read_int(KEY_TEXT_COLOR) != text_color_tuple->value->int32){
		persist_write_int(KEY_TEXT_COLOR, text_color_tuple->value->int32);
		update_settings();
	}
  }
	
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
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "SDK3");
	connection_service_subscribe((ConnectionHandlers) {
	  .pebble_app_connection_handler = handle_bluetooth
	});
	UnobstructedAreaHandlers handler = {
    .did_change = prv_unobstructed_did_change
  	};
  	unobstructed_area_service_subscribe(handler, NULL);
	
    init_app_message();
	
    app_focus_service_subscribe(&handle_appfocus);
	
	// Initially show the correct settings
	update_settings();
	
}

static void deinit(void) {
	tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
	
	connection_service_unsubscribe();
	unobstructed_area_service_unsubscribe();
	
	app_focus_service_unsubscribe();
    window_stack_remove(window, false);
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}