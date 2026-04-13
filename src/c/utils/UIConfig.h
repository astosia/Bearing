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
  
} UIConfig;

extern const UIConfig config;