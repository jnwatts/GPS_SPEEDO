#pragma once

#include "common.h"

typedef void (*mode_func_t)(void);

void display_test(void);
void show_error(int err);
void show_speed(void);
void show_odom(void);
void show_sats(void);
void show_dop(void);
void show_noop(void);
void show_overlay(const char *msg, float delay = 0.5);
void hide_overlay(void);
int load_odom(void);
int save_odom(void);
void update_position(void);
void update_dop(void);
void handle_key_event(key_event_t event);
void check_for_gps_ready(void);
void enter_sleep(void);
void exit_sleep(void);