#ifndef _util_h_
#define _util_h_

#include "config.h"
#include "libretro.h"

#define PI 3.14159265359
#define DEGREES(radians) ((radians) * 180 / PI)
#define RADIANS(degrees) ((degrees) * PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))


static retro_log_printf_t log_cb;

#if DEBUG
    #define LOGD(...) ((log_cb)?log_cb(RETRO_LOG_DEBUG, __VA_ARGS__):fprintf(stderr,__VA_ARGS__))
#else
    #define LOGD(...)
#endif
#define LOG(...) ((log_cb)?log_cb(RETRO_LOG_INFO, __VA_ARGS__):fprintf(stderr,__VA_ARGS__))
#define LOGW(...) ((log_cb)?log_cb(RETRO_LOG_DEBUG, __VA_ARGS__):fprintf(stderr,__VA_ARGS__))
#define LOGE(...) ((log_cb)?log_cb(RETRO_LOG_ERROR, __VA_ARGS__):fprintf(stderr,__VA_ARGS__))

int main_load_graphics(void);

int main_unload_graphics(void);

int main_load_game(int argc, char **argv);

void main_unload_game(void);

int main_init(void);

void main_deinit(void);

int main_run(double frame_time);

#endif
