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
#include "lstring.h"
#include "lualib.h"
#include "ltable.h"

#define BUFFSIZE 2048

//去gc操作
LUA_API const char *dbg_pushfstring (lua_State *L, const char *fmt, ...) {
  const char *ret;
  va_list argp;
  lua_lock(L);
  va_start(argp, fmt);
  ret = luaO_pushvfstring(L, fmt, argp);
  va_end(argp);
  //luaC_checkGC(L);
  lua_unlock(L);
  return ret;
}

//去gc操作
LUA_API const char *dbg_pushstring (lua_State *L, const char *s) {
  lua_lock(L);
  if (s == NULL)
    setnilvalue(s2v(L->top));
  else {
    TString *ts;
    ts = luaS_new(L, s);
    setsvalue2s(L, L->top, ts);
    s = getstr(ts);  /* internal copy's address */
  }
  api_incr_top(L);
  //luaC_checkGC(L);
  lua_unlock(L);
  return s;
}

//去gc操作
LUALIB_API const char *dbg_tolstring (lua_State *L, int idx, size_t *len) {
  if (luaL_callmeta(L, idx, "__tostring")) {  /* metafield? */
    if (!lua_isstring(L, -1))
      luaL_error(L, "'__tostring' must return a string");
  }
  else {
    switch (lua_type(L, idx)) {
      case LUA_TNUMBER: {
        if (lua_isinteger(L, idx))
          dbg_pushfstring(L, "%I", (LUAI_UACINT)lua_tointeger(L, idx));
        else
          dbg_pushfstring(L, "%f", (LUAI_UACNUMBER)lua_tonumber(L, idx));
        break;
      }
      case LUA_TSTRING:
        lua_pushvalue(L, idx);
        break;
      case LUA_TBOOLEAN:
        dbg_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
        break;
      case LUA_TNIL:
        lua_pushliteral(L, "nil");
        break;
      default: {
        int tt = luaL_getmetafield(L, idx, "__name");  /* try name */
        const char *kind = (tt == LUA_TSTRING) ? lua_tostring(L, -1) :
                                                 luaL_typename(L, idx);
        dbg_pushfstring(L, "%s: %p", kind, lua_topointer(L, idx));
        if (tt != LUA_TNIL)
          lua_remove(L, -2);  /* remove '__name' */
        break;
      }
    }
  }
  return lua_tolstring(L, -1, len);
}

static int dbg_print (lua_State *L) {

  size_t l;
  dbg_tolstring(L, 1, &l);

  return 1;
}

//通用类型输出
const char *print_value(lua_State *L,const TValue *o)
{
  setfvalue(s2v(L->top), dbg_print);
  api_incr_top(L);

  setobj(L,s2v(L->top), o);
  api_incr_top(L);

  lua_pcall(L,1,1,0);

  const char *s = lua_tostring(L,-1);

  lua_pop(L,1);

  return s;
}

//table输出
const char *print_table(lua_State *L,Table *t)
{
  static char buf[2048] = {0};
  memset(buf,0,sizeof(buf));
  int n = 0;

  StackValue tem[2] = {0};
  setnilvalue(s2v(tem));
  while(luaH_next(L,t,tem)) {
    n += snprintf(buf + n,BUFFSIZE - n,"%s",print_value(L,s2v(tem)));
    n += snprintf(buf + n,BUFFSIZE - n,"\t");
    n += snprintf(buf + n,BUFFSIZE - n,"%s",print_value(L,s2v(tem + 1)));
    n += snprintf(buf + n,BUFFSIZE - n,"\n");
    if (n >= BUFFSIZE) return buf;
  }

  return buf;
}