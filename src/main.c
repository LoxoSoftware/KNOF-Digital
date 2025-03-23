#include <pebble.h>

#define FACE_XOFF       5
#define FACE_YOFF       5
#define FACE_ROWS       5
#define FACE_COLUMNS    5
#define FACE_DIGITW     26
#define FACE_DIGITH     31
#define FACE_DIGITM     1   //Margin

static Window* win_face;

static GBitmap* bmp_digit= NULL; //Digit background
static GBitmap* bmp_font= NULL;

//Palette:
//  0: Font color
//  1: Mask color
//  2: Background dither color 1
//  3: Background dither color 2
#define PAL_FONT_COLOR  0
#define PAL_BG_COLOR1   1
#define PAL_BG_COLOR2   2
GColor palette[FACE_ROWS][3];
GColor mask_color;

static char     time_str[8];
static char     date_str[8];
static char     day_str[8];
static char     batt_str[8];
static uint8_t  battery_perc= 254;

const uint8_t chmap[64]= //Starts at 32 in the ASCII table
{
    0x0F,0x2D,0x0F,0x2B,0x2C,0x0C,0x0F,0x0F, // !"#$%&'
    0x0F,0x0F,0x0D,0x2E,0x0F,0x0E,0x2F,0x0B, //()*+,-./
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, //01234567
    0x08,0x09,0x0A,0x0F,0x0F,0x0F,0x0F,0x2A, //89:;<=>?
    0x1E,0x10,0x11,0x12,0x13,0x14,0x15,0x16, //@ABCDEFG
    0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E, //HIJKLMNO
    0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26, //PQRSTUVW
    0x27,0x28,0x29,0x0F,0x0F,0x0F,0x0F,0x2F, //XYZ[\]^_
    //NOTE: - '°' takes place of '*'
    //      - <heart> takes place of '#'
    //      - <bolt> takes place of '$'
};

const char* weekdays_en[7]= { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
//const char* weekdays_it[7]= { "DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB" };

void face_draw_row(GContext* ctx, uint8_t row, char* text)
{
    graphics_context_set_compositing_mode(ctx, GCompOpSet);

    GColor dpal[]= { mask_color, palette[row][PAL_BG_COLOR1], palette[row][PAL_BG_COLOR2] };
    GColor fpal[]= { GColorClear, palette[row][PAL_FONT_COLOR] };

    gbitmap_set_palette(bmp_digit, dpal, false);
    gbitmap_set_palette(bmp_font, fpal, false);

    for (int ix=0; ix<FACE_COLUMNS; ix++)
    {
        uint8_t ch= chmap[text[ix]-32];

        GRect drect= (GRect){
            .origin= (GPoint){ FACE_XOFF+ix*(FACE_DIGITW+FACE_DIGITM), FACE_YOFF+row*(FACE_DIGITH+FACE_DIGITM) },
            .size=    (GSize){ FACE_DIGITW, FACE_DIGITH },
        };
        GRect sub_rect= (GRect){
            .origin= (GPoint){ (ch%8)*FACE_DIGITW, (ch/8)*FACE_DIGITH },
            .size=    (GSize){ FACE_DIGITW, FACE_DIGITH },
        };

        graphics_draw_bitmap_in_rect(ctx, bmp_digit, drect);

        GBitmap* bmp_char= gbitmap_create_as_sub_bitmap(bmp_font, sub_rect);
        graphics_draw_bitmap_in_rect(ctx, bmp_char, drect);
        gbitmap_destroy(bmp_char);
    }
}

static void update_time()
{
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Write the current hours and minutes into a buffer
    strftime(time_str, sizeof(time_str), clock_is_24h_style() ?
                                            "%H:%M" : "%I:%M", tick_time);
    strftime(date_str, sizeof(date_str), "%d/%m", tick_time);
    strftime(day_str, sizeof(day_str), "%w", tick_time);
    snprintf(day_str, sizeof(day_str), "  %s", weekdays_en[day_str[0]-'0']);

    layer_mark_dirty(window_get_root_layer(win_face));
}

static void win_face_gfx(Layer* layer, GContext* ctx)
{
    graphics_context_set_fill_color(ctx, mask_color);
    //graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, 0);

    face_draw_row(ctx, 0, batt_str);
    face_draw_row(ctx, 1, time_str);
    face_draw_row(ctx, 2, day_str);
    face_draw_row(ctx, 3, date_str);
    face_draw_row(ctx, 4, "#KNOF");
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    update_time();
}

static void battery_callback(BatteryChargeState state)
{
    // Record the new battery level
    battery_perc = state.charge_percent;
    snprintf(batt_str, sizeof(batt_str), "% 4d%%", battery_perc);

    layer_mark_dirty(window_get_root_layer(win_face));
}


static void win_face_load(Window *window)
{
    bmp_digit = gbitmap_create_with_resource(RESOURCE_ID_BMP_DIGIT);
    bmp_font = gbitmap_create_with_resource(RESOURCE_ID_BMP_FONT);

    mask_color= GColorWhite;
    palette[0][PAL_FONT_COLOR]= GColorCobaltBlue;
    palette[0][PAL_BG_COLOR1]=  GColorCeleste;
    palette[0][PAL_BG_COLOR2]=  GColorPictonBlue;
    palette[1][PAL_FONT_COLOR]= GColorDarkCandyAppleRed;
    palette[1][PAL_BG_COLOR1]=  GColorMelon;
    palette[1][PAL_BG_COLOR2]=  GColorRajah;
    palette[2][PAL_FONT_COLOR]= GColorCobaltBlue;
    palette[2][PAL_BG_COLOR1]=  GColorCeleste;
    palette[2][PAL_BG_COLOR2]=  GColorPictonBlue;
    palette[3][PAL_FONT_COLOR]= GColorCobaltBlue;
    palette[3][PAL_BG_COLOR1]=  GColorCeleste;
    palette[3][PAL_BG_COLOR2]=  GColorPictonBlue;
    palette[4][PAL_FONT_COLOR]= GColorCobaltBlue;
    palette[4][PAL_BG_COLOR1]=  GColorCeleste;
    palette[4][PAL_BG_COLOR2]=  GColorPictonBlue;

    layer_set_update_proc(window_get_root_layer(window), (LayerUpdateProc)win_face_gfx);
    update_time();
    battery_callback(battery_state_service_peek());
}

static void win_face_unload(Window *window)
{
    gbitmap_destroy(bmp_digit);
    gbitmap_destroy(bmp_font);
}

static void app_init(void)
{
    win_face = window_create();
    window_set_window_handlers(win_face, (WindowHandlers) {
        .load = win_face_load,
        .unload = win_face_unload,
    });

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_callback);

    window_stack_push(win_face, true);
}

static void app_deinit(void)
{
    window_destroy(win_face);
}

int main(void)
{
    app_init();
    app_event_loop();
    app_deinit();
}
