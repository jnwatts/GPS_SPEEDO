#pragma once

typedef void (*mode_func_t)(void);

void display_test(void);
void show_error(int err);
void show_speed(void);
void show_odom(void);
int load_odom(void);
int save_odom(void);
void update_position(void);
