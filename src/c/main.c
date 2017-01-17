#include "pebble.h"

#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ICONS 3
#define NUM_FIRST_MENU_ITEMS 3
#define NUM_SECOND_MENU_ITEMS 1

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static GBitmap *s_menu_icon_trigger;
static bool s_js_ready;
const uint32_t inbox_size = 64;
const uint32_t outbox_size = 256;

static char* menu_title[3] = {"Come home", "Leaving", "Go to bed"};
static char* menu_subtitle[3] = {"mobile.come_home", "mobile.leaving", "mobile.go_to_bed"};

/*
* Communication listener and implementation
*/
bool comm_is_js_ready() {
  return s_js_ready;
}

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Receive handler");
  Tuple *js_tuple = dict_find(iter, MESSAGE_KEY_JSREADY);
  if(js_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Receive js-ready message");
    s_js_ready = true;
    menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_menu_layer, GColorPictonBlue, GColorBlack);    
    menu_layer_reload_data(s_menu_layer);
  }
  Tuple *events_tuple = dict_find(iter, MESSAGE_KEY_EVENTS);
  if(events_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Get event message -> quit app");
    window_stack_pop_all(true);
  }  
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  // A message was received, but had to be dropped
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iter, void *context) {
  // The message just sent has been successfully delivered
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Successfully send message to phone!");
}

static void outbox_failed_callback(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  // The message just sent failed to be delivered
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int)reason);
}

static void send_trigger(char *trigger) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if(result == APP_MSG_OK) {
    dict_write_cstring(iter, MESSAGE_KEY_TRIGGER, trigger);
    result = app_message_outbox_send();
    if(result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Start sending trigger: %s", trigger);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
  }
}

/*
* UI specific implementation
*/
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return NUM_FIRST_MENU_ITEMS;
    case 1:
      return NUM_SECOND_MENU_ITEMS;
    default:
      return 0;
  }
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Determine which section we're working with
  switch (section_index) {
    case 0:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, "Trigger");
      break;
    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "Switches");
      break;
  }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  // Determine which section we're going to draw in
  switch (cell_index->section) {
    case 0:
      // Use the row to specify which item we'll draw
      switch (cell_index->row) {
        case 0: case 1: case 2: case 3:
          menu_cell_basic_draw(ctx, cell_layer, menu_title[cell_index->row], menu_subtitle[cell_index->row], s_menu_icon_trigger);
          break;
      }
      break;
    case 1:
      switch (cell_index->row) {
        case 0:
          // There is title draw for something more simple than a basic menu item
          menu_cell_title_draw(ctx, cell_layer, "Switch");
          break;
      }
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  send_trigger(menu_subtitle[cell_index->row]);
  // Use the row to specify which item will receive the select action
  switch (cell_index->row) {
    // This is the menu item with the cycling icon
    case 1:
      // After changing the icon, mark the layer to have it updated
      layer_mark_dirty(menu_layer_get_layer(menu_layer));
      break;
  }

}

static void main_window_load(Window *window) {
  // Here we load the bitmap assets
  s_menu_icon_trigger = gbitmap_create_with_resource(RESOURCE_ID_CC_BLACK);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Load trigger icon: %s", ((s_menu_icon_trigger == NULL)?"error":"success"));

  // Now we prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorDarkGray);
  menu_layer_set_highlight_colors(s_menu_layer, GColorPictonBlue, GColorDarkGray);

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  app_message_open(inbox_size, outbox_size);
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  
  // Send dummy message to wakeup companion app
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if(result == APP_MSG_OK) {
    dict_write_cstring(iter, MESSAGE_KEY_TRIGGER, "");
    result = app_message_outbox_send();
  }
}

static void main_window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(s_menu_layer);

  // Cleanup the menu icons
  gbitmap_destroy(s_menu_icon_trigger);  
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}