#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0x1B, 0xCF, 0x9B, 0x64, 0x60, 0x1E, 0x47, 0xC3, 0x96, 0x87, 0xB4, 0x30, 0xE0, 0xB5, 0x4A, 0xC7 }
PBL_APP_INFO(MY_UUID,
			 "manekineko", "piking",
			 1, 1, /* App version */
			 RESOURCE_ID_IMAGE_MENU_ICON,
			 APP_INFO_WATCH_FACE);

Window window;

TextLayer text_time_layer;

BitmapLayer handLayer1;
BitmapLayer handLayer2;

BmpContainer neko;
BmpContainer mouth;
BmpContainer lip;
RotBmpPairContainer hand;

PropertyAnimation mouth_animation_beg;
PropertyAnimation mouth_animation_end;
PropertyAnimation hand_animation_beg;
PropertyAnimation hand_animation_beg2;
PropertyAnimation hand_animation_end;
PropertyAnimation hand_animation_end2;

GRect mouth_from_rect;
GRect mouth_to_rect;
GRect hand_from_rect;
GRect hand_to_rect;

static char time_text[] = "00:00";

void animation_started(Animation *animation, void *data) {
	(void)animation;
	(void)data;
}

void animation_stopped(Animation *animation, void *data) {
	(void)animation;
	(void)data;

	text_layer_set_text(&text_time_layer, time_text);

	property_animation_init_layer_frame(&mouth_animation_end, &mouth.layer.layer, &mouth_to_rect, &mouth_from_rect);
	property_animation_init_layer_frame(&hand_animation_end, (Layer *)&handLayer1, &hand_to_rect, &hand_from_rect);
	property_animation_init_layer_frame(&hand_animation_end2, (Layer *)&handLayer2, &hand_to_rect, &hand_from_rect);
	
	animation_set_duration(&mouth_animation_end.animation, 500);
	animation_set_duration(&hand_animation_end.animation, 1000);
	animation_set_duration(&hand_animation_end2.animation, 1000);
	
	animation_set_curve(&mouth_animation_end.animation,AnimationCurveEaseOut);
	animation_set_curve(&hand_animation_end.animation,AnimationCurveEaseOut);
	animation_set_curve(&hand_animation_end2.animation,AnimationCurveEaseOut);

	animation_schedule(&mouth_animation_end.animation);
	animation_schedule(&hand_animation_end.animation);
	animation_schedule(&hand_animation_end2.animation);
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
	(void)t;
	(void)ctx;

	property_animation_init_layer_frame(&mouth_animation_beg, &mouth.layer.layer, &mouth_from_rect, &mouth_to_rect);
	property_animation_init_layer_frame(&hand_animation_beg, (Layer *)&handLayer1, &hand_from_rect, &hand_to_rect);
	property_animation_init_layer_frame(&hand_animation_beg2, (Layer *)&handLayer2, &hand_from_rect, &hand_to_rect);
	
	animation_set_duration(&mouth_animation_beg.animation, 500);
	animation_set_duration(&hand_animation_beg.animation, 1000);
	animation_set_duration(&hand_animation_beg2.animation, 1000);
	
	animation_set_curve(&mouth_animation_beg.animation,AnimationCurveEaseOut);
	animation_set_curve(&hand_animation_beg.animation,AnimationCurveEaseOut);
	animation_set_curve(&hand_animation_beg2.animation,AnimationCurveEaseOut);

	animation_set_handlers(&mouth_animation_beg.animation, (AnimationHandlers) {
		.started = (AnimationStartedHandler) animation_started,
		.stopped = (AnimationStoppedHandler) animation_stopped
	}, &ctx);

	// section based on Simplicity by Pebble Team begins
	char *time_format;
	if (clock_is_24h_style()) {
		time_format = "%R";
	} else {
		time_format = "%I:%M";
	}

	string_format_time(time_text, sizeof(time_text), time_format, t->tick_time);

	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}
	// section ends

	animation_schedule(&mouth_animation_beg.animation);
	animation_schedule(&hand_animation_beg.animation);
	animation_schedule(&hand_animation_beg2.animation);
}

void handle_init(AppContextRef ctx) {
	(void)ctx;

	window_init(&window, "Manekineko");
	window_stack_push(&window, true /* Animated */);

	resource_init_current_app(&APP_RESOURCES);

	bmp_init_container(RESOURCE_ID_IMAGE_NEKO,  &neko);
	bmp_init_container(RESOURCE_ID_IMAGE_MOUTH, &mouth);
	bmp_init_container(RESOURCE_ID_IMAGE_LIP, &lip);
	
	layer_add_child(&window.layer, &neko.layer.layer);
	
	mouth_from_rect = GRect(62, 57, 12, 8);
	mouth_to_rect = GRect(62, 65, 12, 8);
	hand_from_rect = GRect(93, 35, 37, 27);
	hand_to_rect = GRect(93, 50, 37, 27);
	
	GRect frame;  //holding frame
	
	frame = layer_get_frame(&mouth.layer.layer);
	frame.origin.x = mouth_from_rect.origin.x;
	frame.origin.y = mouth_from_rect.origin.y;
	layer_set_frame(&mouth.layer.layer, frame);
	layer_add_child(&neko.layer.layer, &mouth.layer.layer);
	
	//add lip layer to cover mouth
	frame = layer_get_frame(&lip.layer.layer);
	frame.origin.x = mouth_from_rect.origin.x-2;
	frame.origin.y = mouth_from_rect.origin.y-5;
	layer_set_frame(&lip.layer.layer, frame);
	layer_add_child(&neko.layer.layer, &lip.layer.layer);
	
	text_layer_init(&text_time_layer, GRect(0, 120, 144, 50));
	text_layer_set_text_color(&text_time_layer, GColorBlack);
	text_layer_set_background_color(&text_time_layer, GColorClear);
	text_layer_set_text_alignment(&text_time_layer, GTextAlignmentCenter);
	text_layer_set_font(&text_time_layer, fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD));
	layer_add_child(&window.layer, &text_time_layer.layer);
	
	//make "transparent" effect by creating black & white layer with different compsiting modes
	rotbmp_pair_init_container(RESOURCE_ID_IMAGE_HAND_WHITE, RESOURCE_ID_IMAGE_HAND_BLACK, &hand);

	bitmap_layer_init(&handLayer1, hand_from_rect);
	bitmap_layer_init(&handLayer2, hand_from_rect);
	bitmap_layer_set_compositing_mode(&handLayer1, GCompOpOr);
	bitmap_layer_set_compositing_mode(&handLayer2, GCompOpClear);
	bitmap_layer_set_bitmap(&handLayer1, &hand.white_bmp);
	bitmap_layer_set_bitmap(&handLayer2, &hand.black_bmp);

	layer_add_child(&neko.layer.layer, &handLayer1.layer);
	layer_add_child(&neko.layer.layer, &handLayer2.layer);
}

void handle_deinit(AppContextRef ctx) {
	(void)ctx;

	bmp_deinit_container(&neko);
	bmp_deinit_container(&mouth);
	bmp_deinit_container(&lip);
	rotbmp_pair_deinit_container(&hand);

}


void pbl_main(void *params) {
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.deinit_handler = &handle_deinit,
		.tick_info = {
			.tick_handler = &handle_minute_tick,
			.tick_units = MINUTE_UNIT
		}
	};
	app_event_loop(params, &handlers);
}