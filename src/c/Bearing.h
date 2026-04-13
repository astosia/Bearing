#pragma once
#include <pebble.h>

#define SETTINGS_KEY 124

typedef struct ClaySettings {
  bool EnableDate;
  bool EnableBattery;
  bool VibeOn;
  int Font;
  char BWThemeSelect[4];
  char ThemeSelect[4];
  GColor FGColor;
  GColor BackgroundColor1;
  GColor ShadowColor;
  GColor ShadowColor2;
  GColor BallColor0;
  GColor BallColor1;
  GColor BallColor2;
  GColor BallColor3;
  GColor TextColor1;
  GColor MajorTickColor;
  GColor MinorTickColor;
  GColor BTQTColor;
  GColor BatteryColor;
  bool BatteryArc;
  bool HoursCentre;
  bool AnimOn;
  bool ShadowOn;
  bool RemoveZero24h;
  bool AddZero12h;
  bool showlocalAMPM;
  bool ShowTime;
  bool EnableMinorTick;
  bool EnableMajorTick;
  bool UseWeather;
  int UpSlider;
  int ShadowOffset;
  char WeatherTemp [8];
  char TempFore [10];
  char tempstring[8];
  char iconnowstring[4];
  char iconforestring[4];
  
  char temphistring[10];
  int WeatherUnit;
  char WindUnit [8];
  int Weathertimecapture;
 
} __attribute__((__packed__)) ClaySettings;

_Static_assert(sizeof(ClaySettings) <= 256, "ClaySettings exceeds Pebble 256-byte persist limit!");


