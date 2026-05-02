#pragma once
#include <pebble.h>

// ---------------------------------------------
// Per-platform rendering config
// ---------------------------------------------
typedef struct {
  int BTQTRectWidth;        // Width of BT & QT icon GRects
  int BTQTRectHeight;
  int BTIconXOffset;        // BT icon offset left or right from centre of time marker ball
  int QTIconXOffset;        // QT icon offset left or right from centre of ball
 
  int tick_inset_inner;     // Major tick inner end inset from screen edge (round only)
  int tick_inset_outer;     // Major tick outer end inset from screen edge
  int minor_tick_inset;     // Minor tick inset from screen edge (round only)
  int hour_font_size;
  int minute_font_size;
  int other_text_font_size;
  int minute_text_radius;
 
  int middle_ring_radius;
  int outer_ring_radius;
  int outer_ring_radius_fg;
  int bg_shadow_radius;

  int bg_shadow_ring_width;
  int fg_shadow_radius;
  int fg_shadow_offset;
  int fg_ring_width;
  int fg_ring_width_outer;
  int ball_radius_outer;
  int time_marker_shadow_offset;
  int ball_offset3;
  int ball_offset2;
  int ball_offset1;
  int ball_radius_3;
  int ball_radius_2;
  int ball_radius_1;
  int hour_ball_track_radius;
  int minute_ball_track_radius;

  int hourYoffset;
  int hourXoffset;
  int battery_line;

  int16_t hour_ball_track_rect_w;
  int16_t hour_ball_track_rect_h;
  int16_t minute_ball_track_rect_w;
  int16_t minute_ball_track_rect_h;
  int16_t inner_ball_track_rect_corner_r;
  int16_t outer_ball_track_rect_corner_r;

  // Rect ring geometry  (≈ ball_track ± fg_ring_width/2)
  int16_t inner_ring_rect_w;
  int16_t inner_ring_rect_h;
  int16_t outer_ring_rect_w;
  int16_t outer_ring_rect_h;
  int16_t inner_ring_rect_corner_r;
  int16_t outer_ring_rect_corner_r;
  int16_t inner_ring_rect_stroke;   // stroke width for inner ring in rect mode
  int16_t outer_ring_rect_stroke;   // stroke width for outer ring in rect mode

  int16_t time_ring_rect_corner_r;

  // Rect centre geometry  (≈ fg_shadow_radius)
  int16_t centre_rect_w;
  int16_t centre_rect_h;
  int16_t centre_rect_corner_r;

  // Rect tick geometry  (same style as HybridToo)
  int16_t majortickrect_w;
  int16_t majortickrect_h;
  int16_t corner_radius_majortickrect;
  int16_t minortickrect_w;
  int16_t minortickrect_h;
  int16_t corner_radius_minortickrect;
  int16_t tick_inset_outer_rect;

  GRect BatterySideBarRect[1];
  
} UIConfig;

extern const UIConfig config;