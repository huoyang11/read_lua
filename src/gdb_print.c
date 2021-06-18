/*
    用于写gdb脚本,方便调试
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <ctype.h>

#include "lua.h"

#include "lapi.h"
#include "lobject.h"
#include "lstate.h"
#include "lauxlib.h"
#include "lualib.h"

//TODO
static int b_print (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */

  size_t l;
  const char *s = luaL_tolstring(L, 1, &l);
  lua_pop(L, 1);

  lua_pushstring(L,s);

  return 0;
}

//TODO
const char *print_value(lua_State *L,const TValue *o)
{
  setfvalue(L,s2v(L->top), b_print);
  api_incr_top(L);

  setobj(L,s2v(L->top), o);
  api_incr_top(L);

  lua_pcall(L,1,1,0);

  return NULL;
}
