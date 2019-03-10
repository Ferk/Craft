
#include "../libretro/libretro.h"
// LuaUTF8
#include "luaapi.h"

#ifdef HAVE_JIT
#include "luajit.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "util.h"
#include "renderer.h"
#include "noise.h"

//#include "deps/luautf8/lutf8lib.h"

// LuaSocket
#ifdef WANT_LUASOCKET
#include "deps/luasocket/luasocket.h"
#endif

#define LEVELS1	12	/* size of the first part of the stack */
#define LEVELS2	10	/* size of the second part of the stack */

static lua_State *L;

static char* stackDump (lua_State *L) {
  int i;
  static char str[1024];
  str[0] = '\0';
  int top = lua_gettop(L);
  for (i = 1; i <= top; i++) {  /* repeat for each level */
    int t = lua_type(L, i);
    switch (t) {

      case LUA_TSTRING:  /* strings */
        sprintf(str,"%s \"%s\"", str, lua_tostring(L, i));
        break;

      case LUA_TBOOLEAN:  /* booleans */
        sprintf(str,"%s %s", str, lua_toboolean(L, i) ? "true" : "false");
        break;

      case LUA_TNUMBER:  /* numbers */
        sprintf(str,"%s %g", str, lua_tonumber(L, i));
        break;

      default:  /* other values */
        sprintf(str,"%s <%s>", str, lua_typename(L, t));
        break;

    }
  }
  sprintf(str,"%s ", str);
  //sprintf(str,"L{ %s }",str);  /* end the listing */
  return str;
}

int luaapi_preload(lua_State *L, lua_CFunction f, const char *name)
{
   lua_getglobal(L, "package");
   lua_getfield(L, -1, "preload");
   lua_pushcfunction(L, f);
   lua_setfield(L, -2, name);
   lua_pop(L, 2);

   return 0;
}

static int db_errorfb (lua_State *L) {
  int level;
  int firstpart = 1;  /* still before eventual `...' */
  int arg = 0;
  lua_State *L1 = L; /*getthread(L, &arg);*/
  lua_Debug ar;
  if (lua_isnumber(L, arg+2)) {
    level = (int)lua_tointeger(L, arg+2);
    lua_pop(L, 1);
  }
  else
    level = (L == L1) ? 1 : 0;  /* level 0 may be this own function */
  if (lua_gettop(L) == arg)
    lua_pushliteral(L, "");
  else if (!lua_isstring(L, arg+1)) return 1;  /* message is not a string */
  else lua_pushliteral(L, "\n");
  lua_pushliteral(L, "stack traceback:");
  while (lua_getstack(L1, level++, &ar)) {
    if (level > LEVELS1 && firstpart) {
      /* no more than `LEVELS2' more levels? */
      if (!lua_getstack(L1, level+LEVELS2, &ar))
        level--;  /* keep going */
      else {
        lua_pushliteral(L, "\n\t...");  /* too many levels */
        while (lua_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
          level++;
      }
      firstpart = 0;
      continue;
    }
    lua_pushliteral(L, "\n\t");
    lua_getinfo(L1, "Snl", &ar);
    lua_pushfstring(L, "%s:", ar.short_src);
    if (ar.currentline > 0)
      lua_pushfstring(L, "%d:", ar.currentline);
    if (*ar.namewhat != '\0')  /* is there a name? */
        lua_pushfstring(L, " in function " LUA_QS, ar.name);
    else {
      if (*ar.what == 'm')  /* main? */
        lua_pushfstring(L, " in main chunk");
      else if (*ar.what == 'C' || *ar.what == 't')
        lua_pushliteral(L, " ?");  /* C function or tail call */
      else
        lua_pushfstring(L, " in function <%s:%d>",
                           ar.short_src, ar.linedefined);
    }
    lua_concat(L, lua_gettop(L) - arg);
  }
  lua_concat(L, lua_gettop(L) - arg);
  return 1;
}

const struct luaL_reg playerlib [] = {
  //{"new", newarray},
  //{"getname", lua_player_getname},
  {NULL, NULL}
};

extern Player *find_player(int id);

int lua_getplayer(lua_State *L)
{
  /*
    //First, we create a userdata instance, that will hold pointer to our singleton
    Player **pmPtr = (Player**)lua_newuserdata(L, sizeof(Player*));

    *pmPtr = find_player(0);

    //Now we create metatable for that object
    luaL_newmetatable(L, "PersonManagerMetaTable");
    //You should normally check, if the table is newly created or not, but
    //since it's a singleton, I won't bother.

    //The table is now on the top of the stack.
    //Since we want Lua to look for methods of PersonManager within the metatable,
    //we must pass reference to it as "__index" metamethod

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    //lua_setfield pops the value off the top of the stack and assigns it to our
    //field. Hence lua_pushvalue, which simply copies our table again on top of the stack.
    //When we invoke lua_setfield, Lua pops our first reference to the table and
    //stores it as "__index" field in our table, which is also on the second
    //topmost position of the stack.
    //This part is crucial, as without the "__index" field, Lua won't know where
    //to look for methods of PersonManager

    luaL_Reg personManagerFunctions[] = {
         'get', lua_PersonManager_getPerson,
          nullptr, nullptr
    };

    luaL_register(L, 0, personManagerFunctions);

    lua_setmetatable(L, -2);

    lua_setglobal(L, "PersonManager");
    */
}

/*** ***/

world_func last_mset_func;
void *last_block_map;

int lua_map_set(lua_State *L)
{
  if(!last_mset_func || !last_block_map) return 0;
  unsigned int x = luaL_checkint(L, 1);
  unsigned int y = luaL_checkint(L, 1);
  unsigned int z = luaL_checkint(L, 1);
  unsigned int w = luaL_checkint(L, 1);

  last_mset_func(x, y, z, w, last_block_map);
  return 0;
}

static const struct luaL_reg lua_maplib [] = {
  {"set", lua_map_set},
  {NULL, NULL}  /* sentinel */
};

/*** noise ***/

int lua_noise_seed(lua_State *L)
{
  unsigned int x = luaL_checkint(L, 1);
  seed(x);
  return 0;
}

int lua_noise_simplex2(lua_State *L)
{
  float x = luaL_checknumber(L, 1);
  float y = luaL_checknumber(L, 1);
  int octaves = luaL_checkint(L, 1);
  float persistence = luaL_checknumber(L, 1);
  float lacunarity = luaL_checknumber(L, 1);

  if(x == 0 || y == 0 || octaves == 0 || persistence == 0 || lacunarity == 0) {
    lua_pushnumber(L, 0); return 1;
  }

  lua_pushnumber(L, simplex2(x, y, octaves, persistence, lacunarity));
  return 1;
}

int lua_noise_simplex3(lua_State *L)
{
  float x = luaL_checknumber(L, 1);
  float y = luaL_checknumber(L, 1);
  float z = luaL_checknumber(L, 1);
  int octaves = luaL_checkint(L, 1);
  float persistence = luaL_checknumber(L, 1);
  float lacunarity = luaL_checknumber(L, 1);

  if(x == 0 || y == 0 || z == 0 || octaves == 0 || persistence == 0 || lacunarity == 0) {
    lua_pushnumber(L, 0); return 1;
  }

  lua_pushnumber(L, simplex3(x, y, z, octaves, persistence, lacunarity));
  return 1;
}

static const struct luaL_reg lua_noiselib [] = {
  {"simplex2", lua_noise_simplex2},
  {"simplex3", lua_noise_simplex3},
  {NULL, NULL}  /* sentinel */
};


void luaapi_init()
{
  L = luaL_newstate();
setbuf(stdout, NULL);
setbuf(stderr, NULL);
  luaL_openlibs(L);
  luaL_openlib(L, "noise", lua_noiselib, 0);
  luaL_openlib(L, "map", lua_maplib, 0);

  #ifdef HAVE_JIT
     luaJIT_setmode(L, -1, LUAJIT_MODE_WRAPCFUNC|LUAJIT_MODE_ON);
  #endif

  luaapi_preload(L, lua_getplayer, "getPlayer");
}

void luaapi_deinit()
{
  lua_close(L);
}


int luaapi_load(char* script)
{
  int status, i;
  /* Load the file containing the script we are going to run */
  status = luaL_loadfile(L, script);
  if (status) {
    /* If something went wrong, error message is at the top of */
    /* the stack */
    LOGE("Couldn't load file: %s\n", lua_tostring(L, -1));
    return status;
  }

  /*
   * Ok, now here we go: We pass data to the lua script on the stack.
   * That is, we first have to prepare Lua's virtual stack the way we
   * want the script to receive it, then ask Lua to run it.
   * /
  lua_newtable(L);    /* We will pass a table */
  /*
   * To put values into the table, we first push the index, then the
   * value, and then call lua_rawset() with the index of the table in the
   * stack. Let's see why it's -3: In Lua, the value -1 always refers to
   * the top of the stack. When you create the table with lua_newtable(),
   * the table gets pushed into the top of the stack. When you push the
   * index and then the cell value, the stack looks like:
   *
   * <- [stack bottom] -- table, index, value [top]
   *
   * So the -1 will refer to the cell value, thus -3 is used to refer to
   * the table itself. Note that lua_rawset() pops the two last elements
   * of the stack, so that after it has been called, the table is at the
   * top of the stack.
   * /
  for (i = 1; i <= 5; i++) {
    lua_pushnumber(L, i);   /* Push the table index * /
    lua_pushnumber(L, i*2); /* Push the cell value * /
    lua_rawset(L, -3);      /* Stores the pair in the table * /
  }
  /* By what name is the script going to reference our table? * /
  lua_setglobal(L, "foo");
  */

  // insert error handler
  int hpos = lua_gettop( L );
  lua_pushcfunction(L, db_errorfb);
  lua_insert( L, hpos );

  /* Ask Lua to run our little script */
  status = lua_pcall(L, 0, LUA_MULTRET, 0);
  if (status) {
    LOGE("Failed to run script: %s\n", lua_tostring(L, -1));
    return status;
  }
  else {
    LOG("Script successfully loaded: %s\n", script);
  }

    /* remove error handler from stack */
    lua_remove( L, hpos );
  return status;
}

void luaapi_run(double delta)
{
  int err = 0;
  //lua_pushcfunction(L, db_errorfb);

  lua_getglobal(L, "update");
  if (lua_isfunction(L, -1)) {
    lua_pushnumber(L, delta);
    if(err = lua_pcall(L, 1, 0, -4)) {
      LOGE("error: %s\n", lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }

  lua_gc(L, LUA_GCSTEP, 0);
}

void luaapi_world_generate(int p, int q, world_func mset_func, void *block_map)
{
  lua_State *genL = lua_newthread (L);
  int errorhandler = lua_gettop( genL ); // - nargs;

  if(errorhandler !=  0) {
    LOGE("skipping %dx%d due to unexpected stack!! %d {%s}\n", p,q,errorhandler, stackDump(genL));
    return;
  }

  lua_pushcfunction(genL, db_errorfb);
  lua_insert(genL, errorhandler);

  if(block_map != last_block_map) {
    last_block_map = block_map;
    last_mset_func = mset_func;
  }
  LOG("calling mapgen: %d,%d {%s}\n",p,q,stackDump(genL));

  lua_getglobal(genL, "mapgen");
  if (lua_isfunction(genL, -1)) {
    lua_pushnumber(genL, p);
    lua_pushnumber(genL, q);
    if(lua_pcall(genL, 2, 0, errorhandler)) {
      LOGE("error: %s\n", lua_tostring(genL, -1));
      lua_pop(genL, 1);
    }
  } else {
    lua_pop(genL, 1);
  }

  /* remove custom error message handler from stack */
  lua_remove( genL, errorhandler );

  //lua_gc(genL, LUA_GCSTEP, 0);

}
