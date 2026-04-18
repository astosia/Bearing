#include <pebble.h>
#include "Bearing.h"
#include "utils/weekday.h"
#include "utils/month.h"
#include "utils/MathUtils.h"
#include "utils/Weather.h"
#include "utils/UIConfig.h"
#ifndef PBL_PLATFORM_APLITE  //avoiding FCTX on OG and Steel
  #include <pebble-fctx/fctx.h>
  #include <pebble-fctx/fpath.h>
  #include <pebble-fctx/ffont.h>
#endif

#define SECONDS_TICK_INTERVAL_MS 1000
//#define LOG
//#define DEBUG
#define WARNING
//#define BACKLIGHTON   ///Use this for ShareX screencapture GIFs

// Layers
static Window *s_window;
static Layer *s_canvas_layer;
static Layer *s_bg_layer;
// static Layer *s_canvas_second_hand;
static Layer *s_fg_layer;

//#if defined (PBL_PLATFORM_EMERY) || defined (PBL_PLATFORM_GABBRO) || defined (PBL_PLATFORM_DIORITE) || defined (PBL_PLATFORM_FLINT)
#ifndef PBL_PLATFORM_APLITE
//static Layer *s_weather_layer_1;
//static Layer *s_weather_layer_2;
#endif

 static GRect bounds;

// Fonts
static GFont FontBTQTIcons;
static GFont FontWeatherIcons;
static GFont FontWeatherIconsSmall;
static GFont small_font;
static GFont medium_font;
static GFont small_medium_font;

#ifndef PBL_PLATFORM_APLITE
  static FFont *FCTX_hour_Font;
  static FFont *FCTX_minute_Font;
#else
  static GFont hour_font_aplite;
  static GFont minute_font_aplite;
  static GFont ampm_font_aplite;
#endif

//#endif

// Time and date
static struct tm prv_tick_time;
static int s_battery_level;
static int s_connected;
static char s_time_text[12], s_ampm_text[6], s_date_text[20]; //, s_batt_text[8], s_logo_text[16];
static int s_month;
static int s_day;
static int s_weekday;
static int s_hours;
static int minutes;
static int hours;
static ClaySettings settings;
static bool showSeconds;
static AppTimer *s_timeout_timer;
static AppTimer *s_weather_timeout_timer;

static int s_countdown = 30;


// ---------------------------------------------------------------------------
// gravity mode setup — not yet on Aplite
// ---------------------------------------------------------------------------
#ifndef PBL_PLATFORM_APLITE
  static int64_t s_last_tap_time = 0;  // ms relative to app start; 0 = no first tap recorded
  static int64_t s_app_start_ms = 0;   // set in prv_init, used to keep tap timestamps small
  static bool s_gravity_mode = false;
  static int sensitivty;

  #define DOUBLE_TAP_MIN_MS  100   // ignore if taps arrive faster than this (single-shake noise)
  #define DOUBLE_TAP_MAX_MS  800   // second tap must arrive within this window

  typedef struct {
    int32_t angle;     // 0 to TRIG_MAX_ANGLE
    int32_t velocity;  // Angular velocity
    int16_t radius;    // radius of the track
  } TrackBall;

  static TrackBall s_hour_ball, s_minute_ball;
  static AppTimer *s_physics_timer;
  static AppTimer *s_gravity_timeout_timer;
  static AppTimer *s_return_timer;
  static bool s_returning = false;
  #define TICK_MS             33        // 33ms = ~30fps for all animations
  #define RETURN_STEPS        3         // move smoothly back to the time
#else
  #define TICK_MS             33        // 33ms = ~30fps for startup animation (Aplite)
#endif // PBL_PLATFORM_APLITE

// ---------------------------------------------------------------------------
// Ball animation on startup — all angles in degrees
// ---------------------------------------------------------------------------
#define ANIM_TIMER_MS       TICK_MS     // 33m = ~30fps
#define ANIM_TOTAL_DEG      720         // 2 full rotations =720
#define ANIM_ACCEL_DEG      60          // ease-in over first quarter rotation
#define ANIM_DECEL_DEG      60         // ease-out over final full rotation
// Full speed: 1 lap (360) per 1.5s, at 50ms per tick = 12 degrees per tick
#define ANIM_FULL_DEG_TICK  12  //was 12 when 50ms

static bool s_anim_active = false;
static int32_t s_anim_angle_offset = 0;   // degrees travelled so far
static AppTimer *s_anim_timer = NULL;
static int s_loop = 0;
static int showWeather = 0;
static char  citistring[15];
static int s_uvmax_level __attribute__((unused)), s_uvnow_level __attribute__((unused));

#ifdef PBL_PLATFORM_EMERY
const UIConfig __attribute__((section(".rodata"))) config = {

  .BTQTRectWidth = 26 ,
  .BTQTRectHeight = 16,
  .BTIconXOffset = 0,
  .QTIconXOffset = -1,
 
  .hour_font_size = 44,
  .minute_font_size = 18,
  .other_text_font_size = 14,

  .tick_inset_inner = 15,
  .tick_inset_outer = 2,
  .minor_tick_inset = 15,
  .minute_text_radius = 64,

  .middle_ring_radius = 64,
  .outer_ring_radius = 104,
  .outer_ring_radius_fg = 123,  
  .bg_shadow_radius = 35,

  .bg_shadow_ring_width = 25, 
  .fg_shadow_radius = 34,   
  .fg_shadow_offset = 2,
  .fg_ring_width = 20,
  .fg_ring_width_outer = 58,
  .ball_radius_outer = 13,
  .time_marker_shadow_offset = 3,
  .ball_offset3=0.5,
  .ball_offset2=1,
  .ball_offset1=2,
  .ball_radius_3=12,
  .ball_radius_2=10,
  .ball_radius_1=8,
  .hour_ball_track_radius = 45,
  .minute_ball_track_radius = 84,

  .hourXoffset = 1,
  .hourYoffset = -2,
  .battery_line = 3

  //.testRect = {{{0,0},{200,228}}},  //reminder of format for including rect co-orindinates in this struct, this would be a full screen emery rect

};
#elif defined(PBL_PLATFORM_GABBRO)
const UIConfig __attribute__((section(".rodata"))) config = {
  .BTQTRectWidth = 26 ,
  .BTQTRectHeight = 16,
  .BTIconXOffset = 0,
  .QTIconXOffset = -2,

  .tick_inset_inner = 15,
  .tick_inset_outer = 6,
  .minor_tick_inset = 7,
  
  .hour_font_size = 48,
  .minute_font_size = 24-2,
  .other_text_font_size = 14,
  .minute_text_radius = 72,

  .middle_ring_radius = 72,
  .outer_ring_radius = 120,
  .outer_ring_radius_fg = 120,
  .bg_shadow_radius = 37,

  .bg_shadow_ring_width = 30, 
  .fg_shadow_radius = 36,       
  .fg_shadow_offset = 2,
  .fg_ring_width = 24,
  .fg_ring_width_outer = 24,
  .ball_radius_outer = 16,
  .time_marker_shadow_offset = 3,
  .ball_offset3=0.5,
  .ball_offset2=1,
  .ball_offset1=2,
  .ball_radius_3=14,
  .ball_radius_2=12,
  .ball_radius_1=10,
  .hour_ball_track_radius = 49,
  .minute_ball_track_radius = 96,

  .hourXoffset = 1,
  .hourYoffset = -2,
  .battery_line = 3
  
};

#elif defined(PBL_ROUND)
const UIConfig __attribute__((section(".rodata"))) config = {
  .BTQTRectWidth = 30,
  .BTQTRectHeight = 12,
  .BTIconXOffset = 0,
  .QTIconXOffset = 0,

  .tick_inset_inner = 10,
  .tick_inset_outer = 3,
  .minor_tick_inset = 4,

  .hour_font_size = 34, //was 46
  .minute_font_size = 14,
  .other_text_font_size = 10,
  .minute_text_radius = 50,

  .middle_ring_radius = 50,
  .outer_ring_radius = 82,  //was82
  .outer_ring_radius_fg = 84,  //was 82
  .bg_shadow_radius = 27,

  .bg_shadow_ring_width = 20, 
  .fg_shadow_radius = 26,       
  .fg_shadow_offset = 1,
  .fg_ring_width = 16,
  .fg_ring_width_outer = 20,
  .ball_radius_outer = 10,
  .time_marker_shadow_offset = 2,
  .ball_offset3=0,
  .ball_offset2=0,
  .ball_offset1=1,
  .ball_radius_3=9,
  .ball_radius_2=8,
  .ball_radius_1=6,
  .hour_ball_track_radius = 34,
  .minute_ball_track_radius = 66,

  .hourXoffset = 1,
  .hourYoffset = -1,
  .battery_line = 2
};


#elif defined(PBL_BW) // Aplite, Diorite, Flint
const UIConfig __attribute__((section(".rodata"))) config = {
  .BTQTRectWidth = 30,
  .BTQTRectHeight = 12,
  .BTIconXOffset = 0,
  .QTIconXOffset = 0,

  .hour_font_size = 28, //38
  .minute_font_size = 14,
  .other_text_font_size = 10,

  .tick_inset_inner = 9, //was 14
  .tick_inset_outer = 2,
  .minor_tick_inset = 9,
  .minute_text_radius = 46,

  .middle_ring_radius = 46,
  .outer_ring_radius = 78,
  .outer_ring_radius_fg = 91,
  .bg_shadow_radius = 23,

  .bg_shadow_ring_width = 20, 
  .fg_shadow_radius = 22,       
  .fg_shadow_offset = 1,
  .fg_ring_width = 16,
  .fg_ring_width_outer = 42,
  .ball_radius_outer = 10,
  .time_marker_shadow_offset = 1,
  .ball_offset3=0,
  .ball_offset2=0,
  .ball_offset1=1,
  .ball_radius_3=9,
  .ball_radius_2=8,
  .ball_radius_1=6,
  .hour_ball_track_radius = 30,
  .minute_ball_track_radius = 62,

  .hourXoffset = 1,
  .hourYoffset = -1,
  .battery_line = 2
};
#else // Basalt
const UIConfig __attribute__((section(".rodata"))) config = {
  .BTQTRectWidth = 30,
  .BTQTRectHeight = 12,
  .BTIconXOffset = 0,
  .QTIconXOffset = 0,
 
  .hour_font_size = 28, //38
  .minute_font_size = 16,
  .other_text_font_size = 10,

  .tick_inset_inner = 9, //was 14
  .tick_inset_outer = 2,
  .minor_tick_inset = 9,
  .minute_text_radius = 46,

  .middle_ring_radius = 46,
  .outer_ring_radius = 78,
  .outer_ring_radius_fg = 91,
  .bg_shadow_radius = 23,

  .bg_shadow_ring_width = 20, 
  .fg_shadow_radius = 22,       
  .fg_shadow_offset = 1,
  .fg_ring_width = 16,
  .fg_ring_width_outer = 42,
  .ball_radius_outer = 10,
  .time_marker_shadow_offset = 2,
  .ball_offset3=0,
  .ball_offset2=0,
  .ball_offset1=1,
  .ball_radius_3=9,
  .ball_radius_2=8,
  .ball_radius_1=6,
  .hour_ball_track_radius = 30,
  .minute_ball_track_radius = 62,

  .hourXoffset = 1,
  .hourYoffset = 0,
  .battery_line = 2
};
#endif

// ---------------------------------------------------------------------------
// Efficiency: Subscribe & Cache 
// ---------------------------------------------------------------------------
static void update_cached_strings() {

    minutes   = prv_tick_time.tm_min;
    hours     = prv_tick_time.tm_hour % 12;
    s_hours   = prv_tick_time.tm_hour;
    s_day     = prv_tick_time.tm_mday;
    s_weekday = prv_tick_time.tm_wday;
    s_month   = prv_tick_time.tm_mon;
   
    // 1. Time string
    int hourdraw = prv_tick_time.tm_hour;
    if (!clock_is_24h_style()) {
        hourdraw = (hourdraw == 0 || hourdraw == 12) ? 12 : hourdraw % 12;
        snprintf(s_time_text, sizeof(s_time_text), settings.AddZero12h ? "%02d:%02d" : "%d:%02d", hourdraw, prv_tick_time.tm_min);
        strftime(s_ampm_text, sizeof(s_ampm_text), "%p", &prv_tick_time);
    } else {
        snprintf(s_time_text, sizeof(s_time_text), settings.RemoveZero24h ? "%d:%02d" : "%02d:%02d", hourdraw, prv_tick_time.tm_min);
        s_ampm_text[0] = '\0';
    }
    // 2. Weekday string, TO DO
    if (settings.EnableDate) {
        char weekdaydraw[10];
        fetchwday(prv_tick_time.tm_wday, i18n_get_system_locale(), weekdaydraw);
        snprintf(s_date_text, sizeof(s_date_text), "%s %d", weekdaydraw, prv_tick_time.tm_mday);
    }
    
}


static void bluetooth_callback(bool connected) {
    bool was_connected = s_connected;
    s_connected = connected;
    if (was_connected && !connected && (!quiet_time_is_active() || settings.VibeOn)) {
        vibes_double_pulse();
    }
    layer_mark_dirty(s_fg_layer);
    //layer_mark_dirty(s_weather_layer_1);
    
    #ifndef PBL_PLATFORM_APLITE
    //layer_mark_dirty(s_weather_layer_2);
    #endif
}

static void battery_callback(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
}

// ---------------------------------------------------------------------------
// Tick & time balls drawing
// ---------------------------------------------------------------------------

static void draw_major_tick(GContext *ctx, int angle, int length, GColor fill_color, GColor border_color) {
  GPoint origin = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  GPoint p1;
  GPoint p2;

  //uses MathUtils, major tick is a line between two points
   p1 = polar_to_point_offset(origin, angle, bounds.size.h / 2 - config.tick_inset_inner);
   p2 = polar_to_point_offset(origin, angle, bounds.size.h / 2 - config.tick_inset_outer);
  
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, border_color);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, p1, p2);
}


static void draw_minor_tick(GContext *ctx, GPoint center, GColor border_color) {
  //minor tick is a circle
  GPoint origin = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, border_color);
  graphics_fill_circle(ctx, center, 1);
}


static void draw_hour_minute_balls(GContext *ctx, GPoint center) {
  
  graphics_context_set_antialiased(ctx, true);
  
  ////draw shadow on time markers
    graphics_context_set_fill_color(ctx, settings.ShadowColor);
    GPoint center_offset = GPoint(center.x + PBL_IF_BW_ELSE (settings.ShadowOffset,settings.ShadowOffset - 3), center.y + PBL_IF_BW_ELSE (settings.ShadowOffset,settings.ShadowOffset - 3));
    graphics_fill_circle(ctx, center_offset, config.ball_radius_outer);

  ///draw back layer of balls
  graphics_context_set_fill_color(ctx, settings.BallColor0);
  graphics_fill_circle(ctx, center, config.ball_radius_outer);

  //draw outer layer of balls
  graphics_context_set_fill_color(ctx, settings.BallColor3);
  GPoint center_less_ball_offset_3 = GPoint(center.x - config.ball_offset3, center.y - config.ball_offset3);
  graphics_fill_circle(ctx, center_less_ball_offset_3, config.ball_radius_3);

  //draw middle layer of balls
  graphics_context_set_fill_color(ctx, settings.BallColor2);
  GPoint center_less_ball_offset_2 = GPoint(center.x - config.ball_offset2, center.y - config.ball_offset2);
  graphics_fill_circle(ctx, center_less_ball_offset_2, config.ball_radius_2);

  //draw front layer of balls
  graphics_context_set_fill_color(ctx, settings.BallColor1);
  GPoint center_less_ball_offset_1 = GPoint(center.x - config.ball_offset1, center.y - config.ball_offset1);
  graphics_fill_circle(ctx, center_less_ball_offset_1, config.ball_radius_1);
  
}

#ifndef PBL_PLATFORM_APLITE
  static inline FPoint clockToCartesian(FPoint center, fixed_t radius, int32_t angle) {
      FPoint pt;
      int32_t c = cos_lookup(angle);
      int32_t s = sin_lookup(angle);
      pt.x = center.x + s * radius / TRIG_MAX_RATIO;
      pt.y = center.y - c * radius / TRIG_MAX_RATIO;
      return pt;
  }
#endif

/// ---------------------------------------------------------------------------
// Settings
/// ---------------------------------------------------------------------------

static void prv_save_settings(void) {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_default_settings(void) {
  
  settings.EnableDate = true;
  settings.EnableBattery = true;
  settings.BatteryArc = false;
  settings.HoursCentre = true;
  settings.AnimOn = true;
  settings.GravityModeOn = true;
  settings.GravModeTimeout = 10;  // seconds before balls return to clock
  settings.FrictionVal = 82;      // damping per tick (82 = retain 82% velocity)
  settings.SensitivityVal = 10;   // sensitivity multiplier (lower = more reactive)
  #if PBL_COLOR
      settings.ShadowOn = true;
      settings.FGColor = GColorCobaltBlue;
      settings.BackgroundColor1 = GColorDukeBlue;
      settings.ShadowColor = GColorBlack;
      settings.ShadowColor2 = GColorDukeBlue;
      settings.BallColor3 = GColorDarkGray;
      settings.BallColor2 = GColorLightGray;
      settings.BallColor1 = GColorWhite;
      settings.BallColor0 = GColorBlack;
      settings.MajorTickColor = GColorIcterine;
      settings.MinorTickColor = GColorIcterine;
      settings.TextColor1 = GColorIcterine;
      settings.BTQTColor = GColorDarkGray;
      settings.BatteryColor = GColorCobaltBlue;
        #if defined (PBL_PLATFORM_GABBRO) 
          settings.ShadowOffset = 7;
        #elif defined (PBL_PLATFORM_EMERY) 
          settings.ShadowOffset = 6;
        #else
          settings.ShadowOffset = 3;
        #endif
      snprintf(settings.ThemeSelect, sizeof(settings.ThemeSelect), "%s", "gr");
  #else  //defaults for BW screens
      settings.ShadowOn = false;
      settings.FGColor = GColorWhite;
      settings.BackgroundColor1 = GColorBlack;
      settings.ShadowColor = GColorWhite;
      settings.ShadowColor2 = GColorBlack;
      settings.BallColor3 = GColorBlack;
      settings.BallColor2 = GColorLightGray;
      settings.BallColor1 = GColorWhite;
      settings.BallColor0 = GColorBlack;
      settings.MajorTickColor = GColorBlack;
      settings.MinorTickColor = GColorBlack;
      settings.TextColor1 = GColorBlack;
      settings.ShadowOffset = 1;
      settings.BTQTColor = GColorBlack;
      settings.BatteryColor = GColorWhite;
      snprintf(settings.BWThemeSelect, sizeof(settings.BWThemeSelect), "%s", "wh");
  #endif
 
  settings.VibeOn = false;
  settings.RemoveZero24h = false;
  settings.AddZero12h = false;
  settings.showlocalAMPM = true;
  settings.ShowTime = true;

  settings.EnableMinorTick = true;
  settings.EnableMajorTick = true;

  // to do, weather
  settings.WeatherUnit = 0;  
  settings.UpSlider = 30;
  settings.UseWeather = false;
}

static void prv_load_settings(void) {
  prv_default_settings();
  int stored_size = persist_get_size(SETTINGS_KEY);
  if (stored_size == (int)sizeof(settings)) {
    persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Settings loaded: size=%d", stored_size);
    #endif
  } else {
    #ifdef WARNING
      APP_LOG(APP_LOG_LEVEL_WARNING, "Settings size mismatch (stored=%d expected=%d), using defaults",
            stored_size, (int)sizeof(settings));
    #endif
    prv_save_settings();
  }
     #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "sizeof(ClaySettings)=%d SETTINGS_KEY=%d", 
            (int)sizeof(ClaySettings), (int)SETTINGS_KEY);
     #endif
}

// ---------------------------------------------------------------------------
// Event handlers: tick handler, timers, animation, accel_tap
// ---------------------------------------------------------------------------

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_tick_time = *tick_time;

    update_cached_strings();
    
  if (units_changed & MINUTE_UNIT) {
    if (s_countdown == 0){
     //Reset weather update countdown
      s_countdown = settings.UpSlider;
    } else{
      s_countdown = s_countdown - 1;
    }
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Countdown to update %d loop is %d", s_countdown, s_loop);
    #endif
    minutes = tick_time->tm_min;
    hours = tick_time->tm_hour % 12;
    s_hours = tick_time->tm_hour;
    layer_mark_dirty(s_canvas_layer);  //analogue hands
    layer_mark_dirty(s_fg_layer);      //digital time 
    //layer_mark_dirty(s_weather_layer_1);  //to do
  
    if (settings.EnableDate && tick_time->tm_mday != s_day) {
      s_day = tick_time->tm_mday;
      s_weekday = tick_time->tm_wday;
      s_month = tick_time->tm_mon;
    }
  }

  if (s_countdown == 0 || s_countdown == 5){
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "countdown is 0, updated weather at %d", tick_time -> tm_min);
    #endif
      s_loop = 0;
      // Begin dictionary
      DictionaryIterator * iter;
      app_message_outbox_begin( & iter);
      // Add a key-value pair
      dict_write_uint8(iter, 0, 0);
      // Send the message!
      app_message_outbox_send();
  }

}

///gravity mode handlers
#ifndef PBL_PLATFORM_APLITE
static void update_ball_physics(TrackBall *ball, int16_t ax, int16_t ay, int32_t sensitivity) {
    // Gavity vectors. Pebble accel is ~ -1000 to 1000.
    int32_t gx = ax; 
    int32_t gy = -ay; // Invert Y for screen coordinates

    // 2. Calculate acceleration using Pebble's lookup tables
    // Formula: acc = (-gx * sin(theta) + gy * cos(theta))
    int32_t sin_th = sin_lookup(ball->angle);
    int32_t cos_th = cos_lookup(ball->angle);

    // Divide by a large number (e.g., 5000) to "tame" the speed
    int32_t acceleration = (-gx * sin_th + gy * cos_th) / sensitivity;  //was 8000

    // Update velocity with friction
    ball->velocity = ((ball->velocity + acceleration) * settings.FrictionVal) / 100;

    // Update angle (Wrap-around is handled automatically by uint16 casting if needed, 
    // but here we just let it accumulate)
    ball->angle += ball->velocity;
}


static void physics_timer_callback(void *context); 

// Stop grav mode and return balls to their clock positions
static void stop_gravity_mode() {
    s_gravity_mode = false;
    s_returning    = false;

    if (s_physics_timer) {
        app_timer_cancel(s_physics_timer);
        s_physics_timer = NULL;
    }
    if (s_gravity_timeout_timer) {
        app_timer_cancel(s_gravity_timeout_timer);
        s_gravity_timeout_timer = NULL;
    }
    if (s_return_timer) {
        app_timer_cancel(s_return_timer);
        s_return_timer = NULL;
    }

    layer_mark_dirty(s_canvas_layer);
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Gravity mode stopped");
    #endif
}

// Shortest distance in TRIG_MAX_ANGLE space
static int32_t angle_diff(int32_t from, int32_t to) {
    int32_t d = (to - from) % TRIG_MAX_ANGLE;
    if (d >  TRIG_MAX_ANGLE / 2) d -= TRIG_MAX_ANGLE;
    if (d < -TRIG_MAX_ANGLE / 2) d += TRIG_MAX_ANGLE;
    return d;
}

static int32_t current_hour_target() {
    int32_t a = (((prv_tick_time.tm_hour % 12) * TRIG_MAX_ANGLE) / 12)
              + ((prv_tick_time.tm_min * TRIG_MAX_ANGLE) / (12 * 60))
              - TRIG_MAX_ANGLE / 4;
    return ((a % TRIG_MAX_ANGLE) + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;
}
static int32_t current_minute_target() {
    int32_t a = (prv_tick_time.tm_min * TRIG_MAX_ANGLE) / 60
              - TRIG_MAX_ANGLE / 4;
    return ((a % TRIG_MAX_ANGLE) + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;
}

// Called at RETURN_TICK_MS — shifts balls toward their clock targets
static void return_tick_handler(void *context) {
    s_return_timer = NULL;

    int32_t hour_target   = current_hour_target();
    int32_t minute_target = current_minute_target();

    int32_t hour_delta   = angle_diff(s_hour_ball.angle,   hour_target);
    int32_t minute_delta = angle_diff(s_minute_ball.angle, minute_target);

    // Move 1/8th of remaining distance each tick — exponential ease-out
    s_hour_ball.angle   += hour_delta   >> RETURN_STEPS;
    s_minute_ball.angle += minute_delta >> RETURN_STEPS;

    layer_mark_dirty(s_canvas_layer);

    // Threshold: ~0.5° in TRIG_MAX_ANGLE units (65536 / 720 ≈ 91)
    bool hour_done   = (hour_delta   > -91 && hour_delta   < 91);
    bool minute_done = (minute_delta > -91 && minute_delta < 91);

    if (hour_done && minute_done) {
        // Snap exactly onto target then fully exit gravity mode
        s_hour_ball.angle   = hour_target;
        s_minute_ball.angle = minute_target;
        layer_mark_dirty(s_canvas_layer);
        stop_gravity_mode();
    } else {
        s_return_timer = app_timer_register(TICK_MS, return_tick_handler, NULL);
    }
}

// Fired after timeout — stop physics and begin rolling balls back to the clock
static void gravity_timeout_handler(void *context) {
    s_gravity_timeout_timer = NULL;
    s_returning = true;

    // Stop physics loop — lerp takes over from here
    if (s_physics_timer) {
        app_timer_cancel(s_physics_timer);
        s_physics_timer = NULL;
    }
    s_hour_ball.velocity   = 0;
    s_minute_ball.velocity = 0;

    s_return_timer = app_timer_register(TICK_MS, return_tick_handler, NULL);
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Gravity timeout: rolling back to clock");
    #endif
}

static void start_gravity_mode() {
    time_t temp = time(NULL);
    struct tm *t = localtime(&temp);

    // Convert time to Pebble's TRIG_MAX_ANGLE space, with the -TRIG_MAX_ANGLE/4
    // offset equivalent to the -90° used in the degree-based clock drawing.
    int32_t ha = (((t->tm_hour % 12) * TRIG_MAX_ANGLE) / 12)
               + ((t->tm_min        * TRIG_MAX_ANGLE) / (12 * 60))
               - TRIG_MAX_ANGLE / 4;
    s_hour_ball.angle = ((ha % TRIG_MAX_ANGLE) + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;

    int32_t ma = (t->tm_min * TRIG_MAX_ANGLE) / 60
               - TRIG_MAX_ANGLE / 4;
    s_minute_ball.angle = ((ma % TRIG_MAX_ANGLE) + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;

    s_hour_ball.velocity = 0;
    s_minute_ball.velocity = 0;
    
    // Set track radii based on your Bearing design
    s_hour_ball.radius = config.hour_ball_track_radius; 
    s_minute_ball.radius = config.minute_ball_track_radius;

    // Start the physics animation loop
    if (s_physics_timer) {
        app_timer_cancel(s_physics_timer);
    }
    s_physics_timer = app_timer_register(TICK_MS, physics_timer_callback, NULL);

    // Arm the timeout — return to clock after user-configured duration
    if (s_gravity_timeout_timer) {
        app_timer_cancel(s_gravity_timeout_timer);
    }
    uint32_t timeout_ms = (uint32_t)(settings.GravModeTimeout > 0 ? settings.GravModeTimeout : 10) * 1000;
    s_gravity_timeout_timer = app_timer_register(timeout_ms, gravity_timeout_handler, NULL);
}


// Physics timer: reads accelerometer, steps physics, redraws
static void physics_timer_callback(void *context) {
    s_physics_timer = NULL;

    if (!s_gravity_mode || !settings.GravityModeOn) {
        return;
    }

    // Sample accelerometer
    AccelData accel;
    accel_service_peek(&accel);

    // SensitivityVal scales the divisor: slider range 6-12 maps to 0.6x-1.2x of base values.
    // At default of 10 behaviour is identical to hardcoded 6000/10000.
    update_ball_physics(&s_hour_ball,   accel.x, accel.y, 6000  * settings.SensitivityVal / 10);
    update_ball_physics(&s_minute_ball, accel.x, accel.y, 10000 * settings.SensitivityVal / 10);

    layer_mark_dirty(s_canvas_layer);

    // Reschedule
    s_physics_timer = app_timer_register(TICK_MS, physics_timer_callback, NULL);
}
#endif // PBL_PLATFORM_APLITE

static void update_weather_view_visibility() {
 // If UseWeather was just turned off, force the view back to main (0)
  if (!settings.UseWeather) {
    showWeather = 0;
  }
  
  // #ifndef PBL_PLATFORM_APLITE
  // if (showWeather == 0) {
  //   layer_set_hidden(s_fg_layer, false);
  //   layer_set_hidden(s_weather_layer_1, true);
  //   layer_set_hidden(s_weather_layer_2, true);
  // } else if (showWeather == 1) {
  //   layer_set_hidden(s_fg_layer, true);
  //   layer_set_hidden(s_weather_layer_1, false);
  //   layer_set_hidden(s_weather_layer_2, true);
  // } else { // showWeather == 2
  //   layer_set_hidden(s_fg_layer, true);
  //   layer_set_hidden(s_weather_layer_1, true);
  //   layer_set_hidden(s_weather_layer_2, false);
  // }
  // #else
  // if (showWeather == 0) {
  //   layer_set_hidden(s_fg_layer, false);
  //   layer_set_hidden(s_weather_layer_1, true);
  // } else {
  //   layer_set_hidden(s_fg_layer, true);
  //   layer_set_hidden(s_weather_layer_1, false);
  // } 
  // #endif
}


/// Weather timer timeout
static void weather_timeout_handler(void *context) {
  s_weather_timeout_timer = NULL;
  showWeather = 0;
  update_weather_view_visibility();
}


/// Startup ball race animation timer
static void anim_timer_callback(void *context) {
  s_anim_timer = NULL;

  int32_t remaining = ANIM_TOTAL_DEG - s_anim_angle_offset;

  if (remaining <= 0) {
    s_anim_active = false;
    s_anim_angle_offset = 0;
    layer_mark_dirty(s_canvas_layer);
    return;
  }

  int32_t delta;

  if (s_anim_angle_offset < ANIM_ACCEL_DEG) {
    // Ease-in: linear ramp from 1 up to full speed over ANIM_ACCEL_DEG
    int32_t frac = (s_anim_angle_offset * 1000) / ANIM_ACCEL_DEG;  // 0..1000
    delta = (ANIM_FULL_DEG_TICK * frac / 1000) + 1;
  } else if (remaining > ANIM_DECEL_DEG) {
    // Full speed
    delta = ANIM_FULL_DEG_TICK;
  } else {
    // Ease-out:  wind-down over final ANIM_DECEL_DEG
    int32_t frac = (remaining * 1000) / ANIM_DECEL_DEG;  // 1000..0
    delta = (ANIM_FULL_DEG_TICK * frac * frac / (1000 * 1000)) + 1;
  }

  if (delta > remaining) delta = remaining;

  s_anim_angle_offset += delta;
  layer_mark_dirty(s_canvas_layer);
  s_anim_timer = app_timer_register(ANIM_TIMER_MS, anim_timer_callback, NULL);
}


//// Shake/tap events, relate only to weather, TO DO
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {

  #ifndef PBL_PLATFORM_APLITE
  if(settings.GravityModeOn){
   
    // ms since app launch — subtract start offset so values stay small and readable
    time_t tap_s;
    uint16_t tap_ms;
    tap_ms = (uint16_t)time_ms(&tap_s, NULL);
    int64_t now = (int64_t)tap_s * 1000 + tap_ms - s_app_start_ms;
    int64_t gap = now - s_last_tap_time;

    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "tap: gap=%dms", (int)gap);
    #endif

    if (gap >= DOUBLE_TAP_MIN_MS && gap <= DOUBLE_TAP_MAX_MS) {
      // Valid double-tap — gap long enough to be intentional, short enough to be deliberate
      #ifdef DEBUG
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Gravity: double tap registered (gap=%dms)", (int)gap);
      #endif
      if (s_gravity_mode) {
        stop_gravity_mode();
      } else {
        s_gravity_mode = true;
        start_gravity_mode();
      }
      s_last_tap_time = 0; // Reset so a 3rd tap doesn't re-trigger
    } else if (gap > DOUBLE_TAP_MAX_MS) {
      // Too slow — treat as first tap of a new sequence
      s_last_tap_time = now;
    }
    // gap < DOUBLE_TAP_MIN_MS: arrived too fast, almost certainly noise — ignore entirely

  }
  #endif // PBL_PLATFORM_APLITE


  #ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "accel tap registered");
  #endif
  if (settings.UseWeather) {
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "showWeather v1 = %d", showWeather);
    #endif
    if (showWeather >= 2) {
      showWeather = 0;
      #ifdef DEBUG
        APP_LOG(APP_LOG_LEVEL_DEBUG, "showWeather v2= %d", showWeather);
      #endif
    } else {
      showWeather = showWeather + 1;
      #ifdef DEBUG
        APP_LOG(APP_LOG_LEVEL_DEBUG, "showWeather v3= %d", showWeather);
      #endif
    }
  } else {
    showWeather = 0;
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "showWeather v4= %d", showWeather);
    #endif
  }

  //Manage weather screen timeout
  if (s_weather_timeout_timer) {
    app_timer_cancel(s_weather_timeout_timer);
    s_weather_timeout_timer = NULL;
  }
  if (showWeather != 0) {
    s_weather_timeout_timer = app_timer_register(31000, weather_timeout_handler, NULL);
  }

  update_weather_view_visibility();
}

// ---------------------------------------------------------------------------
// Health handlers, TO DO maybe
// ---------------------------------------------------------------------------

// static char s_current_steps_buffer[12];
// static int s_step_count = 0;

// // Is step data available?
// bool step_data_is_available(){
//     return HealthServiceAccessibilityMaskAvailable &
//       health_service_metric_accessible(HealthMetricStepCount,
//         time_start_of_today(), time(NULL));
//       }

// char get_thousands_separator() {
//   const char* sys_locale = i18n_get_system_locale();
  
//   // UK/US use a comma
//   if (strncmp(sys_locale, "en", 2) == 0) {
//       return ',';
//   } else if (strncmp(sys_locale, "fr", 2) == 0 || strncmp(sys_locale, "ru", 2) == 0) {
//       return ' '; //French or Russian, show a space
//   } else {
//       return '.';// Default (Dot)
//   }
// }

// // Todays current step count
//   static void get_step_count() {
//       s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
//   }

// static void display_step_count() {

//     int thousands = s_step_count / 1000;
//     //int hundreds = (s_step_count % 1000)/100;
//     int hundreds2 = (s_step_count % 1000);
//     char sep = get_thousands_separator();

//     snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
//     "%s", "");

//     if(thousands > 0) {
//         snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
//     //   "%d.%d%s", thousands, hundreds, "k");
//           "%d%c%03d", thousands, sep, hundreds2);
//     }
//     else {
//       snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
//         "%d", hundreds2);
//     }
//     #ifdef DEBUG
//      APP_LOG(APP_LOG_LEVEL_DEBUG, "step buffer is %s",s_current_steps_buffer);
//     #endif

//   }


// static void health_handler(HealthEventType event, void *context) {
//     if(event != HealthEventSleepUpdate) {
//       get_step_count();
//       display_step_count();
//       if (s_fg_layer) {  // guard against firing before layer is created
//       layer_mark_dirty(s_fg_layer);
//       } 
//     }
//   }

// ---------------------------------------------------------------------------
// AppMessage inbox handler
// ---------------------------------------------------------------------------

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
#ifdef LOG
  APP_LOG(APP_LOG_LEVEL_INFO, "Received message");
#endif

  bool settings_changed = false;
  bool theme_settings_changed = false;
  // (void)settings_changed;  //debug
  // (void)theme_settings_changed;  //debug

  Tuple *vibe_t               = dict_find(iter, MESSAGE_KEY_VibeOn);
  Tuple *enable_battery_t     = dict_find(iter, MESSAGE_KEY_EnableBattery);
  Tuple *battery_arc_t        = dict_find(iter, MESSAGE_KEY_BatteryArc);
  Tuple *hourcentre_t         = dict_find(iter, MESSAGE_KEY_HoursCentre);
  Tuple *anim_t               = dict_find(iter, MESSAGE_KEY_AnimOn);
  #ifndef PBL_PLATFORM_APLITE
  Tuple *grav_t               = dict_find(iter, MESSAGE_KEY_GravityModeOn);
  Tuple *gravtimeout_t        = dict_find(iter, MESSAGE_KEY_GravModeTimeout);
  Tuple *friction_t           = dict_find(iter, MESSAGE_KEY_FrictionVal);
  Tuple *sensitivity_t        = dict_find(iter, MESSAGE_KEY_SensitivityVal);
  #endif
  Tuple *shadowon_t           = dict_find(iter, MESSAGE_KEY_ShadowOn);
  Tuple *shadowoffset_t       = dict_find(iter, MESSAGE_KEY_ShadowOffset);

  Tuple *bwthemeselect_t      = dict_find(iter, MESSAGE_KEY_BWThemeSelect);
  Tuple *themeselect_t        = dict_find(iter, MESSAGE_KEY_ThemeSelect);
  Tuple *fg_color_t           = dict_find(iter, MESSAGE_KEY_FGColor);
  Tuple *bg_color1_t          = dict_find(iter, MESSAGE_KEY_BackgroundColor1);
  Tuple *bg_color2_t          = dict_find(iter, MESSAGE_KEY_ShadowColor);
  Tuple *fg_color2_t          = dict_find(iter, MESSAGE_KEY_ShadowColor2);
  Tuple *ball_col0_t          = dict_find(iter, MESSAGE_KEY_BallColor0);
  Tuple *ball_col1_t          = dict_find(iter, MESSAGE_KEY_BallColor1);
  Tuple *ball_col2_t          = dict_find(iter, MESSAGE_KEY_BallColor2);
  Tuple *ball_col3_t          = dict_find(iter, MESSAGE_KEY_BallColor3);
  Tuple *text_color1_t        = dict_find(iter, MESSAGE_KEY_TextColor1);
  Tuple *btqt_color_t         = dict_find(iter, MESSAGE_KEY_BTQTColor);
  Tuple *major_tick_color_t   = dict_find(iter, MESSAGE_KEY_MajorTickColor);
  Tuple *minor_tick_color_t   = dict_find(iter, MESSAGE_KEY_MinorTickColor);
  Tuple *batt_color_t         = dict_find(iter, MESSAGE_KEY_BatteryColor);

  Tuple *addzero12_t          = dict_find(iter, MESSAGE_KEY_AddZero12h);
  Tuple *remzero24_t          = dict_find(iter, MESSAGE_KEY_RemoveZero24h);
  Tuple *localampm_t          = dict_find(iter, MESSAGE_KEY_showlocalAMPM);
  Tuple *enable_time_t        = dict_find(iter, MESSAGE_KEY_ShowTime);
  Tuple *enable_date_t        = dict_find(iter, MESSAGE_KEY_EnableDate);

  Tuple *enable_minor_t       = dict_find(iter, MESSAGE_KEY_EnableMinorTick);
  Tuple *enable_major_t       = dict_find(iter, MESSAGE_KEY_EnableMajorTick);


  ///weather data

  Tuple * wtemp_t = dict_find(iter, MESSAGE_KEY_WeatherTemp);
  Tuple * iconnow_tuple = dict_find(iter, MESSAGE_KEY_IconNow);
  Tuple * iconfore_tuple = dict_find(iter, MESSAGE_KEY_IconFore);
  Tuple * wforetemp_t = dict_find(iter, MESSAGE_KEY_TempFore);
 
  Tuple * weather_t = dict_find(iter, MESSAGE_KEY_Weathertime);
  Tuple * neigh_t = dict_find(iter, MESSAGE_KEY_NameLocation);
  Tuple * frequpdate = dict_find(iter, MESSAGE_KEY_UpSlider);
  Tuple * useweather_t = dict_find(iter, MESSAGE_KEY_UseWeather);


  if (shadowoffset_t){
    settings.ShadowOffset = (int) shadowoffset_t -> value -> int32;
    settings_changed = true;
  }

  if (useweather_t) {
    settings.UseWeather = useweather_t->value->int32 != 0;
    settings_changed = true;
  }

  #ifndef PBL_PLATFORM_APLITE
  if (grav_t) {
    settings.GravityModeOn = grav_t->value->int32 == 1;
    // If gravity mode was just turned off, stop any running physics
    if (!settings.GravityModeOn && s_gravity_mode) {
      stop_gravity_mode();
    }
    settings_changed = true;
  }

  if (gravtimeout_t) {
    settings.GravModeTimeout = (int)gravtimeout_t->value->int32;
    settings_changed = true;
  }

  if (friction_t) {
    settings.FrictionVal = (int)friction_t->value->int32;
    settings_changed = true;
  }

  if (sensitivity_t) {
    settings.SensitivityVal = (int)sensitivity_t->value->int32;
    settings_changed = true;
  }
  #endif // PBL_PLATFORM_APLITE

  // check accel sub when weather or gravity settings change
  if (settings.UseWeather
    #ifndef PBL_PLATFORM_APLITE
    || settings.GravityModeOn
    #endif
  ) {
    accel_tap_service_subscribe(accel_tap_handler);
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "accel subscribed (weather=%d, gravity=%d)",
              (int)settings.UseWeather, (int)settings.GravityModeOn);
    #endif
  } else {
    accel_tap_service_unsubscribe();
  }

  if (addzero12_t) {
    settings.AddZero12h = addzero12_t->value->int32 != 0;
    settings_changed = true;
  }

  if (remzero24_t) {
    settings.RemoveZero24h = remzero24_t->value->int32 != 0;
    settings_changed = true;
  }

  if (localampm_t) {
    settings.showlocalAMPM = localampm_t->value->int32 == 1;
    settings_changed = true;
  }

  if (vibe_t) {
    settings.VibeOn = vibe_t->value->int32 != 0;
    settings_changed = true;
  }

  if (enable_date_t) {
    settings.EnableDate = enable_date_t->value->int32 == 1;
    settings_changed = true;
  }

 if (enable_minor_t) {
    settings.EnableMinorTick = enable_minor_t->value->int32 == 1;
    settings_changed = true;
  }
  
  if (enable_major_t) {
    settings.EnableMajorTick = enable_major_t->value->int32 == 1;
    settings_changed = true;
  }

  if (enable_time_t) {
    settings.ShowTime = enable_time_t->value->int32 == 1;
    settings_changed = true;
  }

  if (enable_battery_t) {
    settings.EnableBattery = enable_battery_t->value->int32 == 1;
    settings_changed = true;
  }

  if (battery_arc_t) {
    settings.BatteryArc = battery_arc_t->value->int32 == 1;
    settings_changed = true;
  }

  if (hourcentre_t) {
    settings.HoursCentre = hourcentre_t->value->int32 == 1;
    settings_changed = true;
  }

  if (anim_t) {
    settings.AnimOn = anim_t->value->int32 == 1;
    settings_changed = true;
  }



  if (wtemp_t){
  snprintf(settings.tempstring, sizeof(settings.tempstring), "%s", wtemp_t -> value -> cstring);
   settings_changed = true;
  }

 if (iconnow_tuple){
      snprintf(settings.iconnowstring,sizeof(settings.iconnowstring),"%s",safe_weather_condition((int)iconnow_tuple->value->int32));
     settings_changed = true;
  }

 if (iconfore_tuple){
    snprintf(settings.iconforestring,sizeof(settings.iconforestring),"%s",safe_weather_condition((int)iconfore_tuple->value->int32));
     settings_changed = true;
  }

 if (wforetemp_t){
    snprintf(settings.temphistring, sizeof(settings.temphistring), "%s", wforetemp_t -> value -> cstring);
     settings_changed = true;
  }

  if (weather_t){
    settings.Weathertimecapture = (int) weather_t -> value -> int32;
  //   snprintf(settings.weathertimecapture, sizeof(settings.weathertimecapture), "%s", weather_t -> value -> cstring);
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather data captured at time %d",settings.Weathertimecapture);
    #endif
     settings_changed = true;
  }

  if (neigh_t){
    snprintf(citistring, sizeof(citistring), "%s", neigh_t -> value -> cstring);
     settings_changed = true;
  }


  //Control of data gathered for http
  #ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Location Timezone is %s", citistring);
  #endif
  if (strcmp(citistring, "") == 0 || strcmp(citistring,"NotSet") == 0 ){
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Missing info at loop %d, GPS false, citistring is %s, 1 is %d, 2 is %d", s_loop, citistring, strcmp(citistring, ""),strcmp(citistring,"NotSet"));
    #endif
     settings_changed = true;
  } else{
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "GPS on, loop %d, citistring is %s, 1 is %d, 2 is %d", s_loop, citistring, strcmp(citistring, ""),strcmp(citistring,"NotSet"));
    #endif
     settings_changed = true;
  }

  if (frequpdate){
    settings.UpSlider = (int) frequpdate -> value -> int32;
    //Restart the counter
    s_countdown = settings.UpSlider;
     settings_changed = true;
  }

  ///health data settings, if I decide to put this in
  // #ifndef PBL_PLATFORM_APLITE
  // if (health_t) {
  //     settings.HealthOn = health_t->value->int32 == 1;
  //     if (settings.HealthOn) {
  //       #ifdef DEBUG
  //         APP_LOG(APP_LOG_LEVEL_DEBUG, "Health on");
  //       #endif
  //       health_service_events_unsubscribe();              // avoid double-subscribing
  //       health_service_events_subscribe(health_handler, NULL);  // ← ADD THIS
  //       get_step_count();
  //       display_step_count();
  //       settings_changed = true;
  //     } else {
  //       #ifdef DEBUG
  //         APP_LOG(APP_LOG_LEVEL_DEBUG, "Health off");
  //       #endif
  //       health_service_events_unsubscribe();              // ← ADD THIS
  //       snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer), "%s", "");
  //       settings_changed = true;
  //     }
  //   }
  // #endif
////////////


  if (bwthemeselect_t) {
    if (strcmp(bwthemeselect_t->value->cstring, "bl") == 0) { ////black foreground, white tracks
      settings.FGColor = GColorBlack;
      settings.BackgroundColor1 = GColorWhite;
      if (shadowon_t) { settings.ShadowOn = shadowon_t->value->int32 == 1;}
      settings.ShadowColor = settings.ShadowOn ? GColorBlack : GColorWhite;
      settings.ShadowColor2 = settings.ShadowOn ? GColorWhite : GColorBlack;
      settings.BallColor0 = GColorBlack;
      settings.BallColor3 = GColorBlack;
      settings.BallColor2 = GColorLightGray;
      settings.BallColor1 = GColorWhite;
      settings.TextColor1 = GColorWhite;
      settings.BTQTColor = GColorBlack;
      settings.MajorTickColor = GColorWhite;
      settings.MinorTickColor = GColorWhite;
      settings.BatteryColor = GColorBlack;
      theme_settings_changed = true;
    } else if (strcmp(bwthemeselect_t->value->cstring, "wh") == 0) {  ///white foreground, black tracks
      settings.FGColor = GColorWhite;
      settings.BackgroundColor1 = GColorBlack;
      if (shadowon_t) { settings.ShadowOn = shadowon_t->value->int32 == 1;}
      settings.ShadowColor = settings.ShadowOn ? GColorWhite : GColorBlack;
      settings.ShadowColor2 = settings.ShadowOn ? GColorBlack : GColorWhite;
      settings.BallColor0 = GColorBlack;
      settings.BallColor3 = GColorBlack;
      settings.BallColor2 = GColorLightGray;
      settings.BallColor1 = GColorWhite;
      settings.TextColor1 = GColorBlack;
      settings.BTQTColor = GColorBlack;
      settings.MajorTickColor = GColorBlack;
      settings.MinorTickColor = GColorBlack;
      settings.BatteryColor = GColorWhite;
      theme_settings_changed = true;
    } else if (strcmp(bwthemeselect_t->value->cstring, "cu") == 0) {
      if (fg_color_t)     { settings.FGColor = GColorFromHEX(fg_color_t->value->int32);settings_changed = true; }
      if (bg_color1_t)    { settings.BackgroundColor1 = GColorFromHEX(bg_color1_t->value->int32);settings_changed = true; }
      if (shadowon_t) {
        settings.ShadowOn = shadowon_t->value->int32 == 1;
        if (settings.ShadowOn) {
          if (bg_color2_t) { settings.ShadowColor = GColorFromHEX(bg_color2_t->value->int32); settings_changed = true; }
          if (fg_color2_t) { settings.ShadowColor2 = GColorFromHEX(fg_color2_t->value->int32); settings_changed = true;}
        } else {
          settings.ShadowColor = settings.BackgroundColor1; settings_changed = true;
          settings.ShadowColor2 = settings.FGColor; settings_changed = true;
        }
      }
      if (text_color1_t)  { settings.TextColor1 = GColorFromHEX(text_color1_t->value->int32); settings_changed = true; }
      if (btqt_color_t)  { settings.BTQTColor = GColorFromHEX(btqt_color_t->value->int32); settings_changed = true; }
      if (ball_col0_t)  { settings.BallColor0 = GColorFromHEX(ball_col0_t->value->int32);  settings_changed = true;}
      if (ball_col1_t)  { settings.BallColor1 = GColorFromHEX(ball_col1_t->value->int32); settings_changed = true; }
      if (ball_col2_t)  { settings.BallColor2 = GColorFromHEX(ball_col2_t->value->int32); settings_changed = true; }
      if (ball_col3_t)  { settings.BallColor3 = GColorFromHEX(ball_col3_t->value->int32); settings_changed = true; }
      if (major_tick_color_t)  { settings.MajorTickColor = GColorFromHEX(major_tick_color_t->value->int32); settings_changed = true;}
      if (minor_tick_color_t)  { settings.MinorTickColor = GColorFromHEX(minor_tick_color_t->value->int32); settings_changed = true;}
      if (batt_color_t)  { settings.BatteryColor = GColorFromHEX(batt_color_t->value->int32); settings_changed = true; }

 
      theme_settings_changed = true;
    }
  }

  if (themeselect_t) {
    if (strcmp(themeselect_t->value->cstring, "bl") == 0) { ////black foreground, white tracks
      settings.FGColor = GColorBlack;
      settings.BackgroundColor1 = GColorWhite;
      if (shadowon_t) { settings.ShadowOn = shadowon_t->value->int32 == 1;}
      settings.ShadowColor = settings.ShadowOn ? GColorLightGray : GColorWhite;
      settings.ShadowColor2 = settings.ShadowOn ? GColorDarkGray : GColorBlack;
      settings.BallColor3 = GColorBulgarianRose;
      settings.BallColor2 = GColorOrange;
      settings.BallColor1 = GColorChromeYellow;
      settings.BallColor0 = GColorBlack;
      settings.TextColor1 = GColorChromeYellow;
      settings.BTQTColor = GColorRed;
      settings.MajorTickColor = GColorRed;
      settings.MinorTickColor = GColorChromeYellow;
      settings.BatteryColor = GColorBlack;

      theme_settings_changed = true;
    } else if (strcmp(themeselect_t->value->cstring, "wh") == 0) {   ///white foreground, black tracks
      settings.FGColor = GColorWhite;
      settings.BackgroundColor1 = GColorBlack;
      if (shadowon_t) { settings.ShadowOn = shadowon_t->value->int32 == 1; }
      settings.ShadowColor = settings.ShadowOn ? GColorOxfordBlue : GColorBlack;
      settings.ShadowColor2 = settings.ShadowOn ? GColorLightGray : GColorWhite;
      settings.BallColor3 = GColorDarkGray;
      settings.BallColor2 = GColorLightGray;
      settings.BallColor1 = GColorWhite;
      settings.BallColor0 = GColorBlack;
      settings.TextColor1 = GColorBlack;
      settings.BTQTColor = GColorDarkGray;
      settings.MajorTickColor = GColorRed;
      settings.MinorTickColor = GColorOrange;
      settings.BatteryColor = GColorWhite;

      theme_settings_changed = true;
    } else if (strcmp(themeselect_t->value->cstring, "bu") == 0) {  ///paler blue foreground, dark blue tracks
      settings.FGColor = GColorCobaltBlue;
      settings.BackgroundColor1 = GColorDukeBlue;
      if (shadowon_t) { settings.ShadowOn = shadowon_t->value->int32 == 1; }
      settings.ShadowColor = settings.ShadowOn ? GColorBlack : GColorDukeBlue;
      settings.ShadowColor2 = settings.ShadowOn ? GColorDukeBlue : GColorCobaltBlue;
      settings.BallColor3 = GColorDarkGray;
      settings.BallColor2 = GColorLightGray;
      settings.BallColor1 = GColorWhite;
      settings.BallColor0 = GColorBlack;
      settings.TextColor1 = GColorIcterine;
      settings.BTQTColor = GColorDukeBlue;
      settings.MajorTickColor = GColorIcterine;
      settings.MinorTickColor = GColorIcterine;
      settings.BatteryColor = GColorCobaltBlue;
      theme_settings_changed = true;
    } else if (strcmp(themeselect_t->value->cstring, "pl") == 0) {  ///darker purple foreground, pink tracks
      settings.FGColor = GColorPurple;
      settings.BackgroundColor1 = GColorBrilliantRose;
      if (shadowon_t) { settings.ShadowOn = shadowon_t->value->int32 == 1; }
      settings.ShadowColor = settings.ShadowOn ? GColorMagenta : GColorBrilliantRose;
      settings.ShadowColor2 = settings.ShadowOn ? GColorMagenta : GColorPurple;
      settings.BallColor3 = GColorDarkGray;
      settings.BallColor2 = GColorLightGray;
      settings.BallColor1 = GColorWhite;
      settings.BallColor0 = GColorBlack;
      settings.TextColor1 = GColorBlack;
      settings.BTQTColor = GColorPurple;
      settings.MajorTickColor = GColorBlack;
      settings.MinorTickColor = GColorBlack;
      settings.BatteryColor = GColorPurple;
      theme_settings_changed = true;
    } else if (strcmp(themeselect_t->value->cstring, "gr") == 0) {  //black foreground, green tracks
      settings.FGColor = GColorBlack;
      settings.BackgroundColor1 = GColorBrightGreen;
      if (shadowon_t) { settings.ShadowOn = shadowon_t->value->int32 == 1; }
      settings.ShadowColor = settings.ShadowOn ? GColorKellyGreen : GColorInchworm;
      settings.ShadowColor2 = settings.ShadowOn ? GColorDarkGreen : GColorBlack;
      settings.BallColor3 = GColorDarkGray;
      settings.BallColor2 = GColorLightGray;
      settings.BallColor1 = GColorWhite;
      settings.BallColor0 = GColorBlack;
      settings.TextColor1 = GColorWhite;
      settings.BTQTColor = GColorDarkGreen;
      settings.MajorTickColor = GColorWhite;
      settings.MinorTickColor = GColorBrightGreen;
      settings.BatteryColor = GColorBlack;
      theme_settings_changed = true;
    } else if (strcmp(themeselect_t->value->cstring, "cu") == 0) {
      if (fg_color_t)     { settings.FGColor = GColorFromHEX(fg_color_t->value->int32); settings_changed = true; }
      if (bg_color1_t) { settings.BackgroundColor1 = GColorFromHEX(bg_color1_t->value->int32); settings_changed = true; }
      if (shadowon_t) {
        settings.ShadowOn = shadowon_t->value->int32 == 1;
        if (settings.ShadowOn) {
          if (bg_color2_t) { settings.ShadowColor = GColorFromHEX(bg_color2_t->value->int32); settings_changed = true; }
          if (fg_color2_t) { settings.ShadowColor2 = GColorFromHEX(fg_color2_t->value->int32); settings_changed = true; }
        } else {
          settings.ShadowColor = settings.BackgroundColor1; settings_changed = true;
          settings.ShadowColor2 = settings.FGColor; settings_changed = true;
        }
      }
      if (text_color1_t)  { settings.TextColor1 = GColorFromHEX(text_color1_t->value->int32); settings_changed = true; }
      if (btqt_color_t)  { settings.BTQTColor = GColorFromHEX(btqt_color_t->value->int32); settings_changed = true; }
      if (ball_col0_t)  { settings.BallColor0 = GColorFromHEX(ball_col0_t->value->int32); settings_changed = true; }
      if (ball_col1_t)  { settings.BallColor1 = GColorFromHEX(ball_col1_t->value->int32); settings_changed = true; }
      if (ball_col2_t)  { settings.BallColor2 = GColorFromHEX(ball_col2_t->value->int32);  settings_changed = true;}
      if (ball_col3_t)  { settings.BallColor3 = GColorFromHEX(ball_col3_t->value->int32); settings_changed = true; }
      if (major_tick_color_t)  { settings.MajorTickColor = GColorFromHEX(major_tick_color_t->value->int32);  settings_changed = true;}
      if (minor_tick_color_t)  { settings.MinorTickColor = GColorFromHEX(minor_tick_color_t->value->int32);  settings_changed = true;}
      if (batt_color_t)  { settings.BatteryColor = GColorFromHEX(batt_color_t->value->int32); settings_changed = true; }
      theme_settings_changed = true;
    }
  }


  /// update watchface for changes to settigns
  if (settings_changed || theme_settings_changed) {
    layer_mark_dirty(s_bg_layer);
    layer_mark_dirty(s_canvas_layer);
    layer_mark_dirty(s_fg_layer);

  /// set the startup animation going on save, if user switches this option to on in settings
    if(settings.AnimOn){
      s_anim_active = true;
      s_anim_angle_offset = 0;
      s_anim_timer = app_timer_register(ANIM_TIMER_MS, anim_timer_callback, NULL);
    }
  }

  prv_save_settings();
  update_weather_view_visibility();
}


// ---------------------------------------------------------------------------
// Layer update procedures
// ---------------------------------------------------------------------------

static void bg_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  
  ////fill background colour
    graphics_context_set_fill_color(ctx, settings.BackgroundColor1);
    graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h), 0, GCornersAll);

    GPoint origin = GPoint(bounds.size.w / 2, bounds.size.h / 2);

    GPoint offset_origin = GPoint((bounds.size.w / 2) + settings.ShadowOffset, (bounds.size.h / 2) + settings.ShadowOffset);

  ////draw background shadow for battery arc

     if (settings.EnableBattery && settings.BatteryArc) {

      int battery_angle = (s_battery_level * 360)/100;          

      graphics_context_set_stroke_width(ctx, config.battery_line);
      graphics_context_set_stroke_color(ctx, settings.ShadowColor);
      GRect battery_rect = GRect(
        offset_origin.x  - config.minute_ball_track_radius - (config.fg_ring_width/2) +config.battery_line+1,
        offset_origin.y  - config.minute_ball_track_radius - (config.fg_ring_width/2) +config.battery_line+1,
        (config.minute_ball_track_radius*2) + config.fg_ring_width - ((config.battery_line+1)*2),
        (config.minute_ball_track_radius*2) + config.fg_ring_width - ((config.battery_line+1)*2)
      );
      graphics_draw_arc(ctx, battery_rect, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(battery_angle));
      }

    ////draw background shadow for battery bar

     if (settings.EnableBattery && !settings.BatteryArc) {
      int battery_angle = (s_battery_level * 360)/100 - 90;
      //int battery_angle = (75 * 360)/100 - 90;
      GPoint p1;
      GPoint p2;
         
      //graphics_context_set_stroke_width(ctx, config.battery_ring_width);
      graphics_context_set_stroke_width(ctx, config.battery_line + 2);
      graphics_context_set_stroke_color(ctx, settings.ShadowColor);

      p1 = polar_to_point_offset(offset_origin, battery_angle, config.hour_ball_track_radius - (config.fg_ring_width/2));
      p2 = polar_to_point_offset(offset_origin, battery_angle, config.hour_ball_track_radius + (config.fg_ring_width/2));
      graphics_draw_line(ctx, p1, p2);
      
      }
    
    
    graphics_context_set_antialiased(ctx, true);
    graphics_context_set_fill_color(ctx, settings.ShadowColor);

    ///draw shadow for centre circle
    graphics_fill_circle(ctx, GPoint(origin.x + settings.ShadowOffset, origin.y + settings.ShadowOffset), config.bg_shadow_radius);

    ///draw shadow for inner ring
    graphics_context_set_stroke_width(ctx, config.bg_shadow_ring_width);
    graphics_context_set_stroke_color(ctx, settings.ShadowColor);
    GRect arc_rect_bg_shadow_middle = GRect(
        origin.x + settings.ShadowOffset - config.middle_ring_radius,
        origin.y + settings.ShadowOffset - config.middle_ring_radius,
        config.middle_ring_radius * 2,
        config.middle_ring_radius * 2
    );
    graphics_draw_arc(ctx, arc_rect_bg_shadow_middle, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    
    ///draw shadow for outer ring
    GRect arc_rect_bg_shadow_outer = GRect(
        origin.x + settings.ShadowOffset - config.outer_ring_radius,
        origin.y + settings.ShadowOffset - config.outer_ring_radius,
        config.outer_ring_radius * 2,
        config.outer_ring_radius * 2
    );
    graphics_draw_arc(ctx, arc_rect_bg_shadow_outer, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

  }

  static void fg_update_proc(Layer *layer, GContext *ctx) {
     GRect bounds = layer_get_bounds(layer);
     GPoint origin = GPoint(bounds.size.w / 2, bounds.size.h / 2);
     graphics_context_set_antialiased(ctx, true);


    /////draw foreground shadows/"edge" on the rings above the balls

    graphics_context_set_fill_color(ctx, settings.ShadowColor2);

    graphics_fill_circle(ctx, GPoint(origin.x + config.fg_shadow_offset, origin.y + config.fg_shadow_offset), config.fg_shadow_radius);

    graphics_context_set_stroke_width(ctx, config.fg_ring_width);
    graphics_context_set_stroke_color(ctx, settings.ShadowColor2);
    GRect arc_rect_fg_shadow_middle = GRect(
        origin.x + config.fg_shadow_offset - config.middle_ring_radius,
        origin.y + config.fg_shadow_offset - config.middle_ring_radius,
        config.middle_ring_radius * 2,
        config.middle_ring_radius * 2
    );
    graphics_draw_arc(ctx, arc_rect_fg_shadow_middle, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

    GRect arc_rect_fg_shadow_outer = GRect(
        origin.x + config.fg_shadow_offset - config.outer_ring_radius,
        origin.y + config.fg_shadow_offset - config.outer_ring_radius,
        config.outer_ring_radius * 2,
        config.outer_ring_radius * 2
    );
    graphics_draw_arc(ctx, arc_rect_fg_shadow_outer, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

    /////draw foreground coloured rings

    ///draw centre circle
    graphics_context_set_fill_color(ctx, settings.FGColor);
    graphics_fill_circle(ctx, GPoint(origin.x , origin.y ), config.fg_shadow_radius);

    ///draw innter ring (NO OFFSET!)
    graphics_context_set_stroke_width(ctx, config.fg_ring_width);
    graphics_context_set_stroke_color(ctx, settings.FGColor);
    GRect arc_rect_fg_middle = GRect(
        origin.x  - config.middle_ring_radius,
        origin.y  - config.middle_ring_radius,
        config.middle_ring_radius * 2,
        config.middle_ring_radius * 2
    );

    graphics_draw_arc(ctx, arc_rect_fg_middle, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

    ///draw outer ring (big enough to cover corners on rect watches)
    graphics_context_set_stroke_width(ctx, config.fg_ring_width_outer);
    GRect arc_rect_fg_outer = GRect(
        origin.x  - config.outer_ring_radius_fg,
        origin.y  - config.outer_ring_radius_fg,
        config.outer_ring_radius_fg * 2,
        config.outer_ring_radius_fg * 2
    );
    graphics_draw_arc(ctx, arc_rect_fg_outer, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

// Minor ticks — 
  if (settings.EnableMinorTick) {
    for (int i = 0; i < 60; i++) {
      //if (i % 5 == 0) continue;  //this does every degree except on major-tick angles (multiples of 30)
      int angle = i * 6;

        GPoint origin = GPoint(bounds.size.w / 2, bounds.size.h / 2);
        GPoint center = polar_to_point_offset(origin, angle, bounds.size.h / 2 - config.minor_tick_inset);
        draw_minor_tick(ctx, center, settings.MinorTickColor);

    }
  }
    // Major ticks — 12 hour positions
    if(settings.EnableMajorTick){
      for (int i = 0; i < 12; i++) {
        int angle = i * 30 - 90;
        draw_major_tick(ctx, angle, 16, settings.BackgroundColor1, settings.MajorTickColor);
      }
    }

    //draw foreground battery arc or bar

    if (settings.EnableBattery && settings.BatteryArc) {
      //battery_callback(battery_state_service_peek().charge_percent);
      //int battery_level = s_battery_level;
      int battery_angle = (s_battery_level * 360)/100;

      graphics_context_set_stroke_width(ctx, config.battery_line);
      graphics_context_set_stroke_color(ctx, settings.BatteryColor);
      GRect battery_rect = GRect(
        origin.x  - config.minute_ball_track_radius - (config.fg_ring_width/2) +config.battery_line+1,
        origin.y  - config.minute_ball_track_radius - (config.fg_ring_width/2) +config.battery_line+1,
        (config.minute_ball_track_radius*2) + config.fg_ring_width - ((config.battery_line+1)*2),
        (config.minute_ball_track_radius*2) + config.fg_ring_width - ((config.battery_line+1)*2)
      );
      graphics_draw_arc(ctx, battery_rect, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(battery_angle));

  }

   if (settings.EnableBattery && !settings.BatteryArc) {
      int battery_angle = (s_battery_level * 360)/100 - 90;
      GPoint p1;
      GPoint p2;
         
      graphics_context_set_stroke_width(ctx, config.battery_line + 2);
      graphics_context_set_stroke_color(ctx, settings.BatteryColor);

      p1 = polar_to_point_offset(origin, battery_angle, config.hour_ball_track_radius - (config.fg_ring_width/2));
      p2 = polar_to_point_offset(origin, battery_angle, config.hour_ball_track_radius + (config.fg_ring_width/2));
      graphics_draw_line(ctx, p1, p2);
      
  }

  #ifdef PBL_PLATFORM_APLITE  //instead of FCTX, use system fonts and don't rotate the text on the ring
    if (settings.ShowTime) {
      graphics_context_set_text_color(ctx, settings.TextColor1);

      int hourdraw;
      char hournow[4];
      if (clock_is_24h_style()) {
        hourdraw = s_hours;
        snprintf(hournow, sizeof(hournow), settings.RemoveZero24h ? "%d" : "%02d", hourdraw);
      } else {
        if (s_hours == 0 || s_hours == 12) hourdraw = 12;
        else hourdraw = s_hours % 12;
        snprintf(hournow, sizeof(hournow), settings.AddZero12h ? "%02d" : "%d", hourdraw);
      }

      char mindraw[3];
      snprintf(mindraw, sizeof(mindraw), "%02d", minutes);

      GRect hour_rect = GRect(50, 74-9, 45, 21);
      if(settings.HoursCentre){
      graphics_draw_text(ctx, hournow, hour_font_aplite, hour_rect,
          GTextOverflowModeFill, GTextAlignmentCenter, NULL);}
          else{
      graphics_draw_text(ctx, mindraw, hour_font_aplite, hour_rect,
          GTextOverflowModeFill, GTextAlignmentCenter, NULL);      
          }

      if(settings.HoursCentre){
        int minutes_angle = (minutes * 360) / 60 - 90;
        GPoint minute_center = polar_to_point_offset(
            GPoint(bounds.size.w / 2, bounds.size.h / 2),
            minutes_angle,
            config.middle_ring_radius);
        GRect minute_rect = GRect(
            minute_center.x - (16 / 2) ,
            minute_center.y - (18 / 2) -4,
            16, 18+6);
        

        graphics_draw_text(ctx, mindraw, minute_font_aplite, minute_rect,
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      }
      else{
        int hours12 = (s_hours == 0 || s_hours == 12) ? 12 : s_hours % 12;
        int32_t hour_angle = (hours * 360) / 12 + (minutes * 360) / 720 - 90;
        GPoint hour_center = polar_to_point_offset(
          GPoint(bounds.size.w / 2, bounds.size.h / 2),
          hour_angle,
          config.middle_ring_radius);
        GRect hour_rect = GRect(
            hour_center.x - (16 / 2) ,
            hour_center.y - (18 / 2) -4,
            16, 18+6);

         graphics_draw_text(ctx, hournow, minute_font_aplite, hour_rect,
          GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      }

      char local_ampm_string[5];
      strftime(local_ampm_string, sizeof(local_ampm_string), "%p", &prv_tick_time);

      if (settings.showlocalAMPM && !clock_is_24h_style()) {
        bool is_am = (prv_tick_time.tm_hour < 12);
        int ampm_y = is_am
            ? 74 - config.other_text_font_size - 2 - 3 
            : 74 + 21 + 2 - 6;
        GRect ampm_rect = GRect(50, ampm_y, 45, config.other_text_font_size + 2);
        graphics_draw_text(ctx, local_ampm_string, ampm_font_aplite, ampm_rect,
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      }
    }

  //use FCTX for non-aplite watches, to get antialiased big fonts and rotation
  #else 
   FContext fctx;
    if (FCTX_minute_Font == NULL) {
      #ifdef WARNING
        APP_LOG(APP_LOG_LEVEL_ERROR, "Font not loaded yet, skipping draw!");
      #endif
        return; 
    }
    fctx_init_context(&fctx, ctx);
    fctx_set_scale(&fctx, FPointOne, FPointOne);
    fctx_set_offset(&fctx, FPointZero);
    fctx_set_color_bias(&fctx, 0);
    #ifdef PBL_COLOR
     fctx_enable_aa(true);   //no point anti-aliasing for BW watches
    #endif
    
    ///draw digital time if selected in settings
    if (settings.ShowTime) {
      fctx_set_fill_color(&fctx, settings.TextColor1);

      int hourdraw;
      char hournow[4];
      if (clock_is_24h_style()) {
        hourdraw = s_hours;
        snprintf(hournow, sizeof(hournow), settings.RemoveZero24h ? "%d" : "%02d", hourdraw);
      } else {
        if (s_hours == 0 || s_hours == 12) hourdraw = 12;
        else hourdraw = s_hours % 12;
        snprintf(hournow, sizeof(hournow), settings.AddZero12h ? "%02d" : "%d", hourdraw);
      }

      char mindraw[3];
      snprintf(mindraw, sizeof(mindraw), "%02d", minutes);
      FPoint center_minutes = FPointI(bounds.size.w /2, bounds.size.h / 2);
      fixed_t minute_text_radius = (minutes < 16 || minutes > 45)
        ? INT_TO_FIXED(config.minute_text_radius + 1)
        : INT_TO_FIXED(config.minute_text_radius - 1);

      char timedraw[10];
      if(settings.HoursCentre){
        snprintf(timedraw, sizeof(timedraw), "%s", hournow);
      }else{
        snprintf(timedraw, sizeof(timedraw), "%s", mindraw);
      }

     
      //fixed_t safe_radius = INT_TO_FIXED((bounds.size.w / 2 - BEZEL_INSET));
      fctx_set_color_bias(&fctx, 0);

      if(settings.HoursCentre){
      
      int32_t minute_angle = (minutes ) * TRIG_MAX_ANGLE / 60;
      int32_t text_rotation;
            FTextAnchor text_anchor;
            if(minutes < 16 || minutes > 45){
                text_rotation = minute_angle;//  TRIG_MAX_ANGLE / 2;
            }
            else{
              text_rotation = minute_angle + TRIG_MAX_ANGLE / 2;
            }
                text_anchor = FTextAnchorMiddle;
       fctx_begin_fill(&fctx);
       fctx_set_fill_color(&fctx, settings.TextColor1); 
       fctx_set_text_em_height(&fctx,FCTX_minute_Font, config.minute_font_size);
       FPoint p = clockToCartesian(center_minutes, minute_text_radius, minute_angle);
       fctx_set_rotation(&fctx, text_rotation);
            fctx_set_offset(&fctx, p);
            fctx_draw_string(&fctx, mindraw, FCTX_minute_Font, GTextAlignmentCenter, text_anchor);
            fctx_end_fill(&fctx);
      }
      else {
          // Always use 12h hour for position on the dial
          int hours12 = (s_hours == 0 || s_hours == 12) ? 12 : s_hours % 12;
          int32_t hour_angle = (hours * TRIG_MAX_ANGLE / 12) + (minutes * TRIG_MAX_ANGLE / 720);

          int32_t text_rotation;
          FTextAnchor text_anchor;
          if (hours12 <= 3 || hours12 >= 10) {
              text_rotation = hour_angle;
          } else {
              text_rotation = hour_angle + TRIG_MAX_ANGLE / 2;
          }
          text_anchor = FTextAnchorMiddle;

          fctx_begin_fill(&fctx);
          fctx_set_fill_color(&fctx, settings.TextColor1);
          fctx_set_text_em_height(&fctx, FCTX_minute_Font, config.minute_font_size);
          FPoint p = clockToCartesian(center_minutes, minute_text_radius, hour_angle);
          fctx_set_rotation(&fctx, text_rotation);
          fctx_set_offset(&fctx, p);
          fctx_draw_string(&fctx, hournow, FCTX_minute_Font, GTextAlignmentCenter, text_anchor);  
          fctx_end_fill(&fctx);
      }

      

      char local_ampm_string[5];
      strftime(local_ampm_string, sizeof(local_ampm_string), "%p", &prv_tick_time);

      
      fctx_set_text_em_height(&fctx, FCTX_hour_Font, config.hour_font_size);
      
      FPoint time_pos = { INT_TO_FIXED(bounds.size.w / 2) + INT_TO_FIXED(config.hourXoffset), INT_TO_FIXED(bounds.size.h / 2) + INT_TO_FIXED(config.hourYoffset)};
      
      fctx_begin_fill(&fctx);
      fctx_set_color_bias(&fctx, 0);
      fctx_set_offset(&fctx, time_pos);
      fctx_set_rotation(&fctx, 0);
      #ifndef PBL_PLATFORM_APLITE
      fctx_draw_string(&fctx, timedraw, FCTX_hour_Font, GTextAlignmentCenter, FTextAnchorMiddle);
      #else
      fctx_draw_string(&fctx, timedraw, FCTX_minute_Font, GTextAlignmentCenter, FTextAnchorMiddle);
      #endif
      fctx_end_fill(&fctx);

        // Draw AM above, PM below the time string, both centred
        if (settings.showlocalAMPM && !clock_is_24h_style()) {
          bool is_am = (prv_tick_time.tm_hour < 12);
          fixed_t ampm_offset = INT_TO_FIXED(config.hour_font_size / 2 + config.other_text_font_size /4 + (is_am ? -2: +2));
          FPoint ampm_pos = {
            INT_TO_FIXED(bounds.size.w / 2),
            INT_TO_FIXED(bounds.size.h / 2) - INT_TO_FIXED(2) + (is_am ? -ampm_offset : ampm_offset)
          };
        fctx_begin_fill(&fctx);
        #ifndef PBL_PLATFORM_APLITE
        fctx_set_text_em_height(&fctx, FCTX_hour_Font, config.other_text_font_size);
        #else
        fctx_set_text_em_height(&fctx, FCTX_minute_Font, config.other_text_font_size);
        #endif
        fctx_set_color_bias(&fctx, 0);
        fctx_set_offset(&fctx, ampm_pos);
        #ifndef PBL_PLATFORM_APLITE
        fctx_draw_string(&fctx, local_ampm_string, FCTX_hour_Font, GTextAlignmentCenter, FTextAnchorMiddle);
        #else
        fctx_draw_string(&fctx, local_ampm_string, FCTX_minute_Font, GTextAlignmentCenter, FTextAnchorMiddle);
        #endif
        fctx_end_fill(&fctx);
        }
    }
    fctx_deinit_context(&fctx);
  #endif
   
  }

  static void hour_min_hands_canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  (void)bounds;


  #ifndef PBL_PLATFORM_APLITE
  if(settings.GravityModeOn && (s_gravity_mode || s_returning)){
    GPoint origin = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    // Convert TRIG_MAX_ANGLE space back to degrees for polar_to_point_offset
    int hour_deg   = (int)(s_hour_ball.angle   * 360 / TRIG_MAX_ANGLE);
    int minute_deg = (int)(s_minute_ball.angle * 360 / TRIG_MAX_ANGLE);
    draw_hour_minute_balls(ctx, polar_to_point_offset(origin, hour_deg,   config.hour_ball_track_radius));
    draw_hour_minute_balls(ctx, polar_to_point_offset(origin, minute_deg, config.minute_ball_track_radius));
  }
  else
  #endif // PBL_PLATFORM_APLITE

  {

  ///Draw the hour and minute balls
  minutes = prv_tick_time.tm_min;
  hours = prv_tick_time.tm_hour % 12;

  #ifdef HOUR
  hours = HOUR;
  #endif
  #ifdef MINUTE
  minutes = MINUTE;
  #endif

  int hours_angle   = (hours * 360) / 12 + (minutes * 360) / (60 * 12) - 90;
  int minutes_angle = (minutes * 360) / 60 - 90;

  if (s_anim_active && settings.AnimOn) {
    hours_angle += s_anim_angle_offset;
    minutes_angle += s_anim_angle_offset;
  }

  GPoint origin = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    draw_hour_minute_balls(ctx, polar_to_point_offset(origin, hours_angle, config.hour_ball_track_radius));
    draw_hour_minute_balls(ctx, polar_to_point_offset(origin, minutes_angle, config.minute_ball_track_radius));

  // Draw BT icon centred on the hour ball
      if (!s_connected) {  //remember to change back to !s_connected after testing

        GPoint hour_ball_center = polar_to_point_offset(
            GPoint(bounds.size.w / 2 + config.BTIconXOffset, bounds.size.h / 2),
            hours_angle,
            config.hour_ball_track_radius);

        int icon_w = config.BTQTRectWidth;
        int icon_h = config.BTQTRectHeight;
        GRect bt_rect = GRect(
            hour_ball_center.x - icon_w / 2,
            hour_ball_center.y - icon_h / 2,
            icon_w, icon_h);
        graphics_context_set_text_color(ctx, settings.BTQTColor);
        graphics_draw_text(ctx, "z", FontBTQTIcons, bt_rect,
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      }

  // Draw QT icon centred on the minute ball 
      #ifndef PBL_PLATFORM_APLITE
      if (quiet_time_is_active()) {  //remember to change back to true after testing

        GPoint minute_ball_center = polar_to_point_offset(
            GPoint(bounds.size.w / 2 + config.QTIconXOffset, bounds.size.h / 2),
            minutes_angle,
            config.minute_ball_track_radius);

        int icon_w = config.BTQTRectWidth;
        int icon_h = config.BTQTRectHeight;
        GRect qt_rect = GRect(
            minute_ball_center.x - icon_w / 2,
            minute_ball_center.y - icon_h / 2,
            icon_w, icon_h);
        graphics_context_set_text_color(ctx, settings.BTQTColor);
        graphics_draw_text(ctx, "\U0000E061", FontBTQTIcons, qt_rect,
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      }
      #endif
    }

}


// TO DO,  code copied from Hybrid.  maybe put a second hand layer on

//   static void layer_update_proc_seconds_hand(Layer *layer, GContext *ctx) {
//   if (!showSeconds) return;

//   GRect bounds = layer_get_bounds(layer);

//   time_t now = time(NULL);
//   struct tm *t = localtime(&now);
//   int seconds = t->tm_sec;

//   int seconds_angle = ((double)seconds / 60 * 360) - 90;
//   #ifdef PBL_ROUND
//     draw_seconds_line_hand(ctx, seconds_angle,
//                           bounds.size.w / 2 - config.second_hand_a,
//                           bounds.size.w / 2 - config.second_hand_b,
//                           settings.SecondsHandColor);
//   #else
//     if(settings.ForegroundShape){
//       draw_seconds_line_hand(ctx, seconds_angle,
//                           bounds.size.w / 2 - config.second_hand_a_round,
//                           bounds.size.w / 2 - config.second_hand_b_round,
//                           settings.SecondsHandColor);
//     }
//     else{
//       draw_seconds_line_hand(ctx, seconds_angle,
//                           bounds.size.w / 2 - config.second_hand_a,
//                           bounds.size.w / 2 - config.second_hand_b,
//                           settings.SecondsHandColor);
//     }
//   #endif
// }


//TO DO, code copied from Hybrid.  Put weather data on the centre circle on accel_taps

// static void weather_update_proc(Layer *layer, GContext *ctx) {
//   // if(!settings.UseWeather){
//   //   return;
//   // }

//   GRect bounds = layer_get_bounds(layer);
//   #ifndef PBL_ROUND
//   if (settings.ForegroundShape) {
//       // Round foreground (Hybrid style)
//       //(ctx, settings.SecondsHandColor, settings.FGColor);
//     } else {
//       // Rect foreground (HybridToo style)
//       GRect fg_rect = GRect(config.foregroundrect_x, config.foregroundrect_y,
//                             config.foregroundrect_w, config.foregroundrect_h);
//       graphics_context_set_antialiased(ctx, true);
//       graphics_context_set_fill_color(ctx, settings.FGColor);
//       graphics_fill_rect(ctx, fg_rect, config.corner_radius_foreground, GCornersAll);
//       graphics_context_set_stroke_color(ctx, settings.SecondsHandColor);
//       graphics_context_set_stroke_width(ctx, 2);
//       graphics_draw_round_rect(ctx, fg_rect, config.corner_radius_foreground);
//     }
//   #else
//    // draw_center(ctx, settings.SecondsHandColor, settings.FGColor);
//   #endif

//   if (settings.EnableLines) {
//   graphics_context_set_antialiased(ctx, true);
//   graphics_context_set_stroke_width(ctx, 1);
//   graphics_context_set_stroke_color(ctx, settings.LineColor);

//   int cx = bounds.size.w / 2;
//   int cy = bounds.size.h / 2;
//   int half_time = config.hour_font_size / 2;

//   // Horizontal lines above and below the time display
//   graphics_draw_line(ctx, GPoint(bounds.size.w / 4, cy + half_time),
//                           GPoint(bounds.size.w / 4 * 3, cy + half_time));
//   graphics_draw_line(ctx, GPoint(bounds.size.w / 4, cy - half_time + 4),
//                           GPoint(bounds.size.w / 4 * 3, cy - half_time + 4));

//   // Vertical line below centre
//   graphics_draw_line(ctx, GPoint(cx, cy + half_time + config.Line3yOffset),
//                           GPoint(cx, cy + (config.fg_radius - 6)));

//   //Vertical line in centre section
//   // graphics_draw_line(ctx, GPoint(cx, cy - config.Line6yOffset ),
//   //                         GPoint(cx,  cy + config.Line7yOffset ));

//   // Vertical lines above centre (left and right of 12 o'clock)
//   graphics_draw_line(ctx, GPoint(bounds.size.w * 0.42, cy - half_time - config.Line45yOffset),
//                           GPoint(bounds.size.w * 0.42, cy - (config.fg_radius - 10)));
//   graphics_draw_line(ctx, GPoint(bounds.size.w * 0.58, cy - half_time - config.Line45yOffset),
//                           GPoint(bounds.size.w * 0.58, cy - (config.fg_radius - 10)));
//   }


// GRect IconNowRect = config.MiddleLeftRect[0]; 
// GRect IconForeRect = config.MiddleRightRect[0];
// GRect WindKtsRect = config.TopLeftRect[0];
// GRect WindForeKtsRect = config.TopRightRect[0];
// GRect TempRect = config.BottomLeftRect1[0];
// GRect TempForeRect = config.BottomRightRect1[0];
// GRect WindDirNowRect = config.TopTopLeftRect[0];
// GRect WindDirForeRect = config.TopTopRightRect[0];
// GRect MoonRect = config.TopMiddleRect[0];


//             char TempForeToDraw[10];
//             snprintf(TempForeToDraw, sizeof(TempForeToDraw), "%s",settings.temphistring);

 
//             char CondToDraw[4];
//             char CondForeToDraw[4];
//             char TempToDraw[8];

//             char SpeedToDraw[10];
//             char SpeedForeToDraw[10];
//             char DirectionToDraw[4];
//             char DirectionForeToDraw[4];

//             snprintf(SpeedToDraw,sizeof(SpeedToDraw),"%s",settings.windstring);
//             snprintf(SpeedForeToDraw,sizeof(SpeedForeToDraw),"%s",settings.windavestring);
//             snprintf(DirectionToDraw,sizeof(DirectionToDraw),"%s",settings.windiconnowstring);
//             snprintf(DirectionForeToDraw,sizeof(DirectionForeToDraw),"%s", settings.windiconavestring);
//             snprintf(CondToDraw, sizeof(CondToDraw), "%s",settings.iconnowstring);
//             snprintf(TempToDraw, sizeof(TempToDraw), "%s",settings.tempstring);
//             snprintf(CondForeToDraw, sizeof(CondForeToDraw), "%s",settings.iconforestring);

//             char MoonToDraw[4];
//             snprintf(MoonToDraw, sizeof(MoonToDraw), "%s",settings.moonstring); 

//             //Wind speed
//             graphics_context_set_text_color(ctx,settings.TextColor1);
//             graphics_draw_text(ctx, SpeedToDraw, small_font, WindKtsRect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
//             graphics_draw_text(ctx, SpeedForeToDraw, small_font, WindForeKtsRect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
            
//             //Wind Direction
//             graphics_draw_text(ctx, DirectionToDraw, FontWeatherIcons, WindDirNowRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//             graphics_draw_text(ctx, DirectionForeToDraw, FontWeatherIcons, WindDirForeRect, GTextOverflowModeFill,GTextAlignmentCenter, NULL);

//             //Weathericons
//             graphics_draw_text(ctx, CondToDraw, FontWeatherIcons, IconNowRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//             graphics_draw_text(ctx, CondForeToDraw, FontWeatherIcons, IconForeRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

//             //weather temps
//             graphics_draw_text(ctx, TempToDraw, medium_font, TempRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//             graphics_draw_text(ctx, TempForeToDraw, small_font, TempForeRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
            
//             //Moonphase
//             graphics_draw_text(ctx, MoonToDraw, FontWeatherIcons, MoonRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

// }

// //#if defined (PBL_PLATFORM_EMERY) || defined (PBL_PLATFORM_GABBRO) || defined (PBL_PLATFORM_DIORITE) || defined (PBL_PLATFORM_FLINT)
// #ifndef PBL_PLATFORM_APLITE
// static void weather2_update_proc(Layer *layer, GContext *ctx) {
  
//   GRect bounds = layer_get_bounds(layer);
//   #ifndef PBL_ROUND
//   if (settings.ForegroundShape) {
//       // Round foreground (Hybrid style)
//       //draw_center(ctx, settings.SecondsHandColor, settings.FGColor);
//     } else {
//       // Rect foreground (HybridToo style)
//       GRect fg_rect = GRect(config.foregroundrect_x, config.foregroundrect_y,
//                             config.foregroundrect_w, config.foregroundrect_h);
//       graphics_context_set_antialiased(ctx, true);
//       graphics_context_set_fill_color(ctx, settings.FGColor);
//       graphics_fill_rect(ctx, fg_rect, config.corner_radius_foreground, GCornersAll);
//       graphics_context_set_stroke_color(ctx, settings.SecondsHandColor);
//       graphics_context_set_stroke_width(ctx, 2);
//       graphics_draw_round_rect(ctx, fg_rect, config.corner_radius_foreground);
//     }
//   #else
//    // draw_center(ctx, settings.SecondsHandColor, settings.FGColor);
//   #endif

//   if (settings.EnableLines) {
//   graphics_context_set_antialiased(ctx, true);
//   graphics_context_set_stroke_width(ctx, 1);
//   graphics_context_set_stroke_color(ctx, settings.LineColor);

//   int cx = bounds.size.w / 2;
//   int cy = bounds.size.h / 2;
//   int half_time = config.hour_font_size / 2;

//   // Horizontal lines above and below the time display
//   graphics_draw_line(ctx, GPoint(bounds.size.w / 4, cy + half_time),
//                           GPoint(bounds.size.w / 4 * 3, cy + half_time));
//   graphics_draw_line(ctx, GPoint(bounds.size.w / 4, cy - half_time + 4),
//                           GPoint(bounds.size.w / 4 * 3, cy - half_time + 4));

//   // Vertical line below centre
//   graphics_draw_line(ctx, GPoint(cx, cy + half_time + config.Line3yOffset),
//                           GPoint(cx, cy + (config.fg_radius - 6)));

//   // Vertical lines above centre (left and right of 12 o'clock)
//   graphics_draw_line(ctx, GPoint(bounds.size.w * 0.42, cy - half_time - config.Line45yOffset),
//                           GPoint(bounds.size.w * 0.42, cy - (config.fg_radius - 10)));
//   graphics_draw_line(ctx, GPoint(bounds.size.w * 0.58, cy - half_time - config.Line45yOffset),
//                           GPoint(bounds.size.w * 0.58, cy - (config.fg_radius - 10)));
//   }


//   GRect SunsetIconRect = config.BottomBottomRightRect[0];
//   GRect SunriseIconRect = config.BottomBottomLeftRect1[0];
//   GRect SunsetRect = config.BottomRightRect[0];
//   GRect SunriseRect = config.BottomLeftRect[0];
//   GRect UVDayValueRect = config.UVDayValueRect[0];
//   GRect uv_arc_bounds = config.uv_arc_bounds[0];
//   GRect uv_arc_bounds_max = config.uv_arc_bounds_max[0];
//   GRect uv_arc_bounds_now = config.uv_arc_bounds_now[0];
//   GRect uv_icon = config.uv_icon[0];

//   GRect RainDayValueRect = config.RainDayValueRect[0];
//   GRect Rain1hValueRect = config.Rain1hValueRect[0];
//   GRect Rain_arc_bounds = config.Rain_arc_bounds[0];
//   GRect Rain_arc_bounds_max = config.Rain_arc_bounds_max[0];
//   GRect Rain_arc_bounds_now = config.Rain_arc_bounds_now[0];
//   GRect Rain_icon = config.Rain_icon[0];

//   GRect BarotrendRect = config.TopMiddleRect[0];
//   GRect PressureNowRect = config.TopLeftRect[0];
//   GRect Pressure1hRect = config.TopRightRect[0];

//   graphics_context_set_text_color(ctx,settings.TextColor1);


//             if(settings.barotrend == 2){  //falling
//               graphics_draw_text(ctx, "\U0000F088", FontWeatherIcons, BarotrendRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//             } else if(settings.barotrend == 1){  //rising
//               graphics_draw_text(ctx, "\U0000F057", FontWeatherIcons, BarotrendRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//             } else { //default or steady
//               graphics_draw_text(ctx, "\U0000F04D", FontWeatherIcons, BarotrendRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//             }

//             //"%d.%d", s_rainday_level / 10, s_rainday_level % 10


//             int s_pressurenow_level = settings.pressurenow;
//             char PressureNowToDraw[7];
//             if (settings.pressurenow<650000){
//               snprintf(PressureNowToDraw, sizeof(PressureNowToDraw), "%d.%02d", s_pressurenow_level /1000, (s_pressurenow_level % 1000 / 10));
//             }
//             else{
//               snprintf(PressureNowToDraw, sizeof(PressureNowToDraw), "%d", s_pressurenow_level /1000);
//             }
//             //APP_LOG(APP_LOG_LEVEL_DEBUG, "s_pressurenow_level is %d, settings.pressurenow is %d, PressureNowToDraw is %s", s_pressurenow_level, settings.pressurenow, PressureNowToDraw);

//             graphics_draw_text(ctx, PressureNowToDraw, small_font, PressureNowRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
           
           
//             int s_pressure1h_level = settings.pressure1h;
//             char Pressure1hToDraw[7];
//             if (settings.pressure1h<650000){
//               snprintf(Pressure1hToDraw, sizeof(Pressure1hToDraw), "%d.%02d", s_pressure1h_level /1000, (s_pressure1h_level % 1000 / 10));
//             } else {
//               snprintf(Pressure1hToDraw, sizeof(Pressure1hToDraw), "%d", s_pressure1h_level / 1000);
//             }
//             //APP_LOG(APP_LOG_LEVEL_DEBUG, "s_pressure1h_level is %d, settings.pressuren1h is %d, Pressure1hToDraw is %s", s_pressure1h_level, settings.pressure1h, Pressure1hToDraw);

//             graphics_draw_text(ctx, Pressure1hToDraw, small_font, Pressure1hRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

//             char SunsetIconToShow[4];

//             snprintf(SunsetIconToShow, sizeof(SunsetIconToShow),  "%s", "\U0000F052");

//             char SunriseIconToShow[4];

//             snprintf(SunriseIconToShow, sizeof(SunriseIconToShow),  "%s",  "\U0000F051");

//             //sunsettime variable by clock setting
//             char SunsetToDraw[8];
//             if (clock_is_24h_style()){
//               snprintf(SunsetToDraw, sizeof(SunsetToDraw), "%s",settings.sunsetstring);
//             }
//             else {
//               snprintf(SunsetToDraw, sizeof(SunsetToDraw), "%s",settings.sunsetstring12);
//             }

//             char SunriseToDraw[8];
//             if (clock_is_24h_style()){
//                snprintf(SunriseToDraw, sizeof(SunriseToDraw), "%s",settings.sunrisestring);
//              }
//             else {
//                snprintf(SunriseToDraw, sizeof(SunriseToDraw), "%s",settings.sunrisestring12);
//              }

//              graphics_draw_text(ctx, SunriseIconToShow, FontWeatherIconsSmall, SunriseIconRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//              graphics_draw_text(ctx, SunsetIconToShow, FontWeatherIconsSmall, SunsetIconRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//              graphics_draw_text(ctx, SunriseToDraw, small_font, SunriseRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//              graphics_draw_text(ctx, SunsetToDraw, small_font, SunsetRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

//              graphics_draw_text(ctx, "\U0000F00D", FontWeatherIconsSmall, uv_icon, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//              graphics_draw_text(ctx, "\U0000F084", FontWeatherIconsSmall, Rain_icon, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

//              int s_uvmax_level = settings.UVIndexMax;  //forecast max uv for the day, limited to max of 10 for dial
//              int s_uvnow_level = settings.UVIndexNow;  //mark on the circle, also limited to nmax of 10
//              int s_uvday_level = settings.UVIndexDay;  //forecast max uv for the day - value in the circle, not limited to a max of 10
//             //  int s_uvmax_level = 8;
//             //  int s_uvnow_level = 5;
//             //  int s_uvday_level = 15;
//             //(void)s_uvmax_level; (void)s_uvnow_level; (void)s_uvday_level;
                    
//                     graphics_context_set_fill_color(ctx, settings.UVArcColor);
//                     int32_t angle_start = DEG_TO_TRIGANGLE(180+30);
//                     int32_t angle_end = DEG_TO_TRIGANGLE(360+180-30);
//                     uint16_t inset_thickness = 2;
//                     graphics_fill_radial(ctx,uv_arc_bounds,GOvalScaleModeFitCircle,inset_thickness,angle_start,angle_end);

//                     graphics_context_set_fill_color(ctx, settings.UVMaxColor);// GColorBlack);
//                     //graphics_fill_rect(ctx, UVMaxRect, 0, GCornerNone);
                                    
//                     int32_t angle_start_max = DEG_TO_TRIGANGLE(180+30);
//                     int32_t angle_end_max = DEG_TO_TRIGANGLE((180+30)+ ((360-60)*s_uvmax_level/10));
//                     uint16_t inset_thickness_max = 4;
//                     graphics_fill_radial(ctx,uv_arc_bounds_max,GOvalScaleModeFitCircle,inset_thickness_max,angle_start_max,angle_end_max);

//                     graphics_context_set_fill_color(ctx, settings.UVNowColor);
//                     //graphics_fill_rect(ctx,UVNowRect, 3, GCornersAll);
                                    
//                         int32_t angle_start_now = DEG_TO_TRIGANGLE((180+30)+((360-60)*s_uvnow_level/10)-3);
//                         int32_t angle_end_now = DEG_TO_TRIGANGLE((180+30)+((360-60)*s_uvnow_level/10)+3);

//                     uint16_t inset_thickness_now = 8;
//                     graphics_fill_radial(ctx,uv_arc_bounds_now,GOvalScaleModeFitCircle,inset_thickness_now,angle_start_now,angle_end_now);

//                     char UVValueToDraw[4];
//                     snprintf(UVValueToDraw, sizeof(UVValueToDraw), "%d",s_uvday_level);
//                     //snprintf(UVValueToDraw, sizeof(UVValueToDraw), "%d", 10 );
//                     graphics_context_set_text_color(ctx,settings.TextColor1);
//                     if (s_uvday_level<20){
//                       graphics_draw_text(ctx, UVValueToDraw, small_medium_font, UVDayValueRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//                     }
//                     else{
//                       graphics_draw_text(ctx, UVValueToDraw, small_medium_font, UVDayValueRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//                     }

//              int s_rainmax_level = settings.popmax; //change to be forecast probability or max prob of rain
//              int s_rainmin_level = settings.popmin;
//              int s_rainnow_level = settings.popstring;  //prob rain in next hour
//              int s_rainday_level = settings.rainfore;  //raistring is rain in the next hour, rainfore is the amount in next day, value in the circle 
//              int s_rain1h_level = settings.rainstring;

                    
//                     graphics_context_set_fill_color(ctx, settings.UVArcColor);
//                     int32_t angle_start_r = DEG_TO_TRIGANGLE(180+30);
//                     int32_t angle_end_r = DEG_TO_TRIGANGLE(360+180-30);
//                     uint16_t inset_thickness_r = 2;
//                     graphics_fill_radial(ctx,Rain_arc_bounds,GOvalScaleModeFitCircle,inset_thickness_r,angle_start_r,angle_end_r);

//                     graphics_context_set_fill_color(ctx, settings.UVMaxColor);// GColorBlack);
//                     //graphics_fill_rect(ctx, UVMaxRect, 0, GCornerNone);
                                    
//                     int32_t angle_start_max_r = DEG_TO_TRIGANGLE((180+30)+ ((360-60)*s_rainmin_level/100));
//                     int32_t angle_end_max_r = DEG_TO_TRIGANGLE((180+30)+ ((360-60)*s_rainmax_level/100));
//                     uint16_t inset_thickness_max_r = 4;
//                     graphics_fill_radial(ctx,Rain_arc_bounds_max,GOvalScaleModeFitCircle,inset_thickness_max_r,angle_start_max_r,angle_end_max_r);

//                     graphics_context_set_fill_color(ctx, settings.UVNowColor);
                                    
//                         int32_t angle_start_now_r = DEG_TO_TRIGANGLE((180+30)+((360-60)*s_rainnow_level/100)-3);
//                         int32_t angle_end_now_r = DEG_TO_TRIGANGLE((180+30)+((360-60)*s_rainnow_level/100)+3);

//                     uint16_t inset_thickness_now_r = 8;
//                     graphics_fill_radial(ctx,Rain_arc_bounds_now,GOvalScaleModeFitCircle,inset_thickness_now_r,angle_start_now_r,angle_end_now_r);

//                     char RainValueToDraw [9];
//                     if (s_rainday_level<10){
//                     snprintf(RainValueToDraw, sizeof(RainValueToDraw), "%d.%d", s_rainday_level / 10, (s_rainday_level % 10/10));
//                     }
//                     else{
//                      snprintf(RainValueToDraw, sizeof(RainValueToDraw), "%d", s_rainday_level / 10);  
//                     }

//                     char Rain1hToDraw [9];
//                     if (s_rain1h_level<10){
//                     snprintf(Rain1hToDraw, sizeof(Rain1hToDraw), "%d.%d", s_rain1h_level / 10, (s_rainday_level % 10/10));
//                     }
//                     else{
//                      snprintf(Rain1hToDraw, sizeof(Rain1hToDraw), "%d", s_rain1h_level / 10);  
//                     }

//                       graphics_context_set_text_color(ctx,settings.TextColor1);

//                       graphics_draw_text(ctx, RainValueToDraw, small_font, RainDayValueRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//                       graphics_draw_text(ctx, Rain1hToDraw, small_font, Rain1hValueRect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);             

// }
// #endif



// ---------------------------------------------------------------------------
// Window lifecycle
// ---------------------------------------------------------------------------

static void prv_window_load(Window *window) {
  
  #ifdef BACKLIGHTON
    light_enable(true);  ///for ShareX screencapture gifs.  Must comment out declaration on line 18 before publishing, otherwise the backlight will stay on!
  #endif

  ///FONTS
  
  #ifndef PBL_PLATFORM_APLITE
    FCTX_hour_Font = ffont_create_from_resource(RESOURCE_ID_DIN_CONDENSED_FFONT);   
    APP_LOG(APP_LOG_LEVEL_ERROR, "Fonts: hour=%p", (void*)FCTX_hour_Font);
    FCTX_minute_Font = ffont_create_from_resource(RESOURCE_ID_CONTHRAX_FFONT);  
    APP_LOG(APP_LOG_LEVEL_ERROR, "Fonts: minute=%p", (void*)FCTX_minute_Font);

  #else
    minute_font_aplite = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    hour_font_aplite = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
    ampm_font_aplite = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  #endif
  
 
  #if defined PBL_BW || defined PBL_PLATFORM_BASALT || defined PBL_PLATFORM_CHALK
   small_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
   small_medium_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
   medium_font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
   FontWeatherIcons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHERICONS_20));
   FontWeatherIconsSmall = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHERICONS_10));
  #elif defined (PBL_PLATFORM_EMERY) || defined (PBL_PLATFORM_GABBRO)
  //  FCTX_Font = ffont_create_from_resource(RESOURCE_ID_DIN_CONDENSED_FFONT);
   small_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
   small_medium_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
   medium_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
   FontWeatherIcons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHERICONS_31));
   FontWeatherIconsSmall = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHERICONS_12));
  #endif

  #if defined (PBL_PLATFORM_EMERY) || defined (PBL_PLATFORM_GABBRO)
    FontBTQTIcons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DRIPICONS_14));
  #else
    FontBTQTIcons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DRIPICONS_12));
  #endif

  ///Create layers and set which update procedures are used for each
  
  Layer *window_layer = window_get_root_layer(s_window);
    bounds = layer_get_bounds(window_layer);

  // Initial State Fetch
    time_t now = time(NULL);
    prv_tick_time = *localtime(&now);
    s_connected = connection_service_peek_pebble_app_connection();
    s_battery_level = battery_state_service_peek().charge_percent;
    update_cached_strings();
 
  
  // #ifdef DEBUG
  //   APP_LOG(APP_LOG_LEVEL_DEBUG, "prv_window_load: HealthOn=%d", (int)settings.HealthOn);
  // #endif
  // if (settings.HealthOn) {
  //     health_service_events_subscribe(health_handler, NULL);
  //       get_step_count();
  //     display_step_count();
  // }
    
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_callback);
    connection_service_subscribe((ConnectionHandlers){ .pebble_app_connection_handler = bluetooth_callback });

  if (settings.UseWeather) {
    s_timeout_timer = app_timer_register(1000, weather_timeout_handler, NULL);
  }
  if (settings.UseWeather
    #ifndef PBL_PLATFORM_APLITE
    || settings.GravityModeOn
    #endif
  ) {
    accel_tap_service_subscribe(accel_tap_handler);
    #ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "accel subscribed (weather=%d, gravity=%d), prv_window_load",
              (int)settings.UseWeather, (int)settings.GravityModeOn);
    #endif
  }
  
  // if (settings.EnableSecondsHand) {
  //   tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  //   if (settings.SecondsVisibleTime != 135) {
  //     s_timeout_timer = app_timer_register(1000 * settings.SecondsVisibleTime, timeout_handler, NULL);
  //     accel_tap_service_subscribe(accel_tap_handler);
  //     #ifdef DEBUG
  //       APP_LOG(APP_LOG_LEVEL_DEBUG, "accel subscribed seconds on <135 seconds, prv_window_load");
  //     #endif
  //   }
  // } else {
   // tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // }
  // showSeconds = settings.EnableSecondsHand;

  

  if (settings.Weathertimecapture == 0 && settings.UseWeather) {
      strcpy(settings.iconnowstring, "\U0000F03D");
      s_loop = 0;
      #ifdef LOG
        APP_LOG(APP_LOG_LEVEL_INFO, "Weather loaded at startup (First Run)");
      #endif
      DictionaryIterator * iter;
      app_message_outbox_begin( & iter);
      dict_write_uint8(iter, 0, 0);
      app_message_outbox_send();
    }

  s_bg_layer = layer_create(bounds);
  // s_canvas_second_hand = layer_create(bounds);
  s_canvas_layer = layer_create(bounds);
  s_fg_layer = layer_create(bounds);
  //s_weather_layer_1 = layer_create(bounds);
  
  #ifndef PBL_PLATFORM_APLITE
  //s_weather_layer_2 = layer_create(bounds);
  #endif
  
  
  // Set update procs before adding to window so no draw fires before fonts are ready
  layer_set_update_proc(s_bg_layer, bg_update_proc);
  layer_set_update_proc(s_canvas_layer, hour_min_hands_canvas_update_proc);
  layer_set_update_proc(s_fg_layer, fg_update_proc);
 
  layer_add_child(window_layer, s_bg_layer);
  layer_add_child(window_layer, s_canvas_layer);
  layer_add_child(window_layer, s_fg_layer);
  
  
  showWeather = 0; 
  update_weather_view_visibility();

  // Start startup ball animation
  if(settings.AnimOn){
    s_anim_active = true;
    s_anim_angle_offset = 0;
    s_anim_timer = app_timer_register(ANIM_TIMER_MS, anim_timer_callback, NULL);
  }
}

static void prv_window_unload(Window *window) {
  #ifdef BACKLIGHTON
    light_enable(false);
  #endif

  //cancel call timers
  if (s_timeout_timer) {
    app_timer_cancel(s_timeout_timer);
    s_timeout_timer = NULL;
  }
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
    s_anim_timer = NULL;
  }
  #ifndef PBL_PLATFORM_APLITE
  if (s_physics_timer) {
    app_timer_cancel(s_physics_timer);
    s_physics_timer = NULL;
  }
  if (s_gravity_timeout_timer) {
    app_timer_cancel(s_gravity_timeout_timer);
    s_gravity_timeout_timer = NULL;
  }
  if (s_return_timer) {
    app_timer_cancel(s_return_timer);
    s_return_timer = NULL;
  }
  #endif // PBL_PLATFORM_APLITE
  if (s_weather_timeout_timer) {
    app_timer_cancel(s_weather_timeout_timer);
    s_weather_timeout_timer = NULL;
  }

  accel_tap_service_unsubscribe();
  app_message_deregister_callbacks();
  connection_service_unsubscribe();
//  health_service_events_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  layer_destroy(s_canvas_layer);
  layer_destroy(s_bg_layer);
  layer_destroy(s_fg_layer);
  //layer_destroy(s_weather_layer_1);
  
  #ifndef PBL_PLATFORM_APLITE
  //layer_destroy(s_weather_layer_2);
  #endif
  // layer_destroy(s_canvas_second_hand);

  ///UNLOAD FONTS
  fonts_unload_custom_font(FontBTQTIcons);
  fonts_unload_custom_font(FontWeatherIcons);
  fonts_unload_custom_font(FontWeatherIconsSmall);
   #ifndef PBL_PLATFORM_APLITE
    ffont_destroy(FCTX_hour_Font);
    ffont_destroy(FCTX_minute_Font);
   #endif

}

// ---------------------------------------------------------------------------
// App lifecycle
// ---------------------------------------------------------------------------

static void prv_init(void) {
  #ifndef PBL_PLATFORM_APLITE
  // Capture app start time so tap timestamps are small relative values (ms since launch)
  time_t start_s;
  uint16_t start_ms;
  start_ms = (uint16_t)time_ms(&start_s, NULL);
  s_app_start_ms = (int64_t)start_s * 1000 + start_ms;
  #endif // PBL_PLATFORM_APLITE

  prv_load_settings();

  s_countdown = settings.UpSlider;

  app_message_register_inbox_received(prv_inbox_received_handler);
  #ifdef PBL_PLATFORM_APLITE
    app_message_open(512, 512); // Smaller buffers for older hardware
  #else
    app_message_open(2048, 1024);
  #endif

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}