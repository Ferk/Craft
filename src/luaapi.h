#ifndef _item_h_
#define _item_h_

#include "world.h"

void luaapi_init();
void luaapi_deinit();
int luaapi_load(char* script);
void luaapi_run(double delta);

void luaapi_world_generate(int p, int q, world_func func, void *arg);

#endif
