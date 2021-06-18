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

static int b_print (lua_State *L) {

  size_t l;
  luaL_tolstring(L, 1, &l);

  return 1;
}

const char *print_value(lua_State *L,const TValue *o)
{
  setfvalue(s2v(L->top), b_print);
  api_incr_top(L);

  setobj(L,s2v(L->top), o);
  api_incr_top(L);

  lua_pcall(L,1,1,0);

  const char *s = lua_tostring(L,-1);

  lua_pop(L,1);

  return s;
}

//TODO
const char *printf_table(lua_State *L,const TValue *o)
{
  static char buf[2048] = {0};
  memset(buf,0,sizeof(buf));

  Table *t = hvalue(o);

  setnilvalue(s2v(L->top));
  while(luaH_next(L,t,L->top)) {
      //printf("%s   %f\n",getstr(tsvalue(s2v(L->top))),nvalue(s2v(L->top+1)));
      print_value(L,s2v(L->top));
      printf("\t");
      print_value(L,s2v(L->top+1));
      printf("\n");
  }

  return buf;
}