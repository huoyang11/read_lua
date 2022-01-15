/*
    用于写gdb脚本,方便调试
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <ctype.h>

#include "lstate.h"
#include "lua.h"
#include "lauxlib.h"
#include "lapi.h"
#include "ltable.h"
#include "lstring.h"
#include "llex.h"
#include "lauxlib.h"
#include "lualib.h"
#include "ldo.h"

#include "ldebug.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lopnames.h"
#include "lundump.h"

#define BUFFSIZE 4096

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
const char *print_value(const TValue *o)
{
  if (!o)
  {
    return "";
  }

  lua_State *L = luaL_newstate();

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
  static char buf[BUFFSIZE] = {0};
  memset(buf,0,sizeof(buf));
  int len = 0;

  StackValue tem[2] = {0};
  setnilvalue(s2v(tem));
  while(luaH_next(L,t,tem)) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",print_value(L,s2v(tem)));
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\t");
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",print_value(L,s2v(tem + 1)));
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\n");
    if (len >= BUFFSIZE) return buf;
  }

  return buf;
}

static TString **tmname;
#define UPVALNAME(x) ((f->upvalues[x].name) ? getstr(f->upvalues[x].name) : "-")
#define VOID(p) ((const void*)(p))
#define eventname(i) (getstr(tmname[i]))

#define COMMENT		"\t; "
#define EXTRAARG	GETARG_Ax(code[pc+1])
#define EXTRAARGC	(EXTRAARG*(MAXARG_C+1))
#define ISK		(isk ? "k" : "")

const char *PrintString(const TString* ts)
{
 int len = 0;
 static char buf[BUFFSIZE] = {0};
 memset(buf,0,BUFFSIZE);
 const char* s=getstr(ts);
 size_t i,n=tsslen(ts);
 len += snprintf(buf + len,BUFFSIZE - len - 1,"\"");
 for (i=0; i<n; i++)
 {
  int c=(int)(unsigned char)s[i];
  switch (c)
  {
   case '"':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\\"");
    break;
   case '\\':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\\\");
    break;
   case '\a':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\a");
    break;
   case '\b':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\b");
    break;
   case '\f':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\f");
    break;
   case '\n':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\n");
    break;
   case '\r':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\r");
    break;
   case '\t':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\t");
    break;
   case '\v':
    len += snprintf(buf + len,BUFFSIZE - len - 1,"\\v");
    break;
   default:
    if (isprint(c)) len += snprintf(buf + len,BUFFSIZE - len - 1,"%c",c); else len += snprintf(buf + len,BUFFSIZE - len - 1,"\\%03d",c);
    break;
  }
 }
 len += snprintf(buf + len,BUFFSIZE - len - 1,"\"");

 return buf;
}

const char *PrintConstant(const Proto* f, int i)
{
 int len = 0;
 static char buf[BUFFSIZE] = {0};
 memset(buf,0,BUFFSIZE);
 const TValue* o=&f->k[i];
 switch (ttypetag(o))
 {
  case LUA_VNIL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"nil");
    break;
  case LUA_VFALSE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"false");
    break;
  case LUA_VTRUE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"true");
    break;
  case LUA_VNUMFLT:
    {
    char buff[100];
    sprintf(buff,LUA_NUMBER_FMT,fltvalue(o));
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",buff);
    if (buff[strspn(buff,"-0123456789")]=='\0') len += snprintf(buf + len,BUFFSIZE - len - 1,".0");
    break;
    }
  case LUA_VNUMINT:
    len += snprintf(buf + len,BUFFSIZE - len - 1,LUA_INTEGER_FMT,ivalue(o));
    break;
  case LUA_VSHRSTR:
  case LUA_VLNGSTR:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintString(tsvalue(o)));
    break;
  default:				/* cannot happen */
    len += snprintf(buf + len,BUFFSIZE - len - 1,"?%d",ttypetag(o));
    break;
 }

 return buf;
}

//输出编译成的指令集
const char *PrintCode(lua_State* L,const Proto* f,int n)
{
 int len = 0;
 tmname=G(L)->tmname;
 static char buf[BUFFSIZE] = {0};
 memset(buf,0,BUFFSIZE);
 const Instruction* code=f->code;
 int pc;
 for (pc=0; pc<n; pc++)
 {
  Instruction i=code[pc];
  OpCode o=GET_OPCODE(i);
  int a=GETARG_A(i);
  int b=GETARG_B(i);
  int c=GETARG_C(i);
  int ax=GETARG_Ax(i);
  int bx=GETARG_Bx(i);
  int sb=GETARG_sB(i);
  int sc=GETARG_sC(i);
  int sbx=GETARG_sBx(i);
  int isk=GETARG_k(i);
  int line=luaG_getfuncline(f,pc);
  len += snprintf(buf + len,BUFFSIZE - len - 1,"\t%d\t",pc+1);
  if (line>0) len += snprintf(buf + len,BUFFSIZE - len - 1,"[%d]\t",line); else len += snprintf(buf + len,BUFFSIZE - len - 1,"[-]\t");
  len += snprintf(buf + len,BUFFSIZE - len - 1,"%-9s\t",opnames[o]);
  switch (o)
  {
   case OP_MOVE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    break;
   case OP_LOADI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,sbx);
    break;
   case OP_LOADF:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,sbx);
    break;
   case OP_LOADK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,bx);
    len += snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,bx));
    break;
   case OP_LOADKX:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",a);
    len += snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,EXTRAARG));
    break;
   case OP_LOADFALSE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",a);
    break;
   case OP_LFALSESKIP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",a);
    break;
   case OP_LOADTRUE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",a);
    break;
   case OP_LOADNIL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    len += snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%d out",b+1);
    break;
   case OP_GETUPVAL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    len += snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%s",UPVALNAME(b));
    break;
   case OP_SETUPVAL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%s",UPVALNAME(b));
    break;
   case OP_GETTABUP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%s",UPVALNAME(b));
    len += snprintf(buf + len,BUFFSIZE - len - 1," "); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_GETTABLE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_GETI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_GETFIELD:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_SETTABUP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d%s",a,b,c,ISK);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%s",UPVALNAME(a));
    len += snprintf(buf + len,BUFFSIZE - len - 1," "); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,b));
    if (isk) { len += snprintf(buf + len,BUFFSIZE - len - 1," "); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c)); }
    break;
   case OP_SETTABLE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d%s",a,b,c,ISK);
    if (isk) { len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c)); }
    break;
   case OP_SETI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d%s",a,b,c,ISK);
    if (isk) { len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c)); }
    break;
   case OP_SETFIELD:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d%s",a,b,c,ISK);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,b));
    if (isk) { len += snprintf(buf + len,BUFFSIZE - len - 1," "); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c)); }
    break;
   case OP_NEWTABLE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%d",c+EXTRAARGC);
    break;
   case OP_SELF:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d%s",a,b,c,ISK);
    if (isk) { len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c)); }
    break;
   case OP_ADDI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,sc);
    break;
   case OP_ADDK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_SUBK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_MULK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_MODK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_POWK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_DIVK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_IDIVK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_BANDK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_BORK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_BXORK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,c));
    break;
   case OP_SHRI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,sc);
    break;
   case OP_SHLI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,sc);
    break;
   case OP_ADD:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_SUB:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_MUL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_MOD:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_POW:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_DIV:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_IDIV:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_BAND:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_BOR:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_BXOR:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_SHL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_SHR:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    break;
   case OP_MMBIN:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%s",eventname(c));
    break;
   case OP_MMBINI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d %d",a,sb,c,isk);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%s",eventname(c));
    if (isk) len += snprintf(buf + len,BUFFSIZE - len - 1," flip");
    break;
   case OP_MMBINK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d %d",a,b,c,isk);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%s ",eventname(c)); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,b));
    if (isk) len += snprintf(buf + len,BUFFSIZE - len - 1," flip");
    break;
   case OP_UNM:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    break;
   case OP_BNOT:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    break;
   case OP_NOT:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    break;
   case OP_LEN:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    break;
   case OP_CONCAT:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,b);
    break;
   case OP_CLOSE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",a);
    break;
   case OP_TBC:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",a);
    break;
   case OP_JMP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",GETARG_sJ(i));
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "to %d",GETARG_sJ(i)+pc+2);
    break;
   case OP_EQ:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,isk);
    break;
   case OP_LT:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,isk);
    break;
   case OP_LE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,isk);
    break;
   case OP_EQK:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,isk);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT); len += snprintf(buf + len,BUFFSIZE - len - 1,"%s",PrintConstant(f,b));
    break;
   case OP_EQI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,sb,isk);
    break;
   case OP_LTI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,sb,isk);
    break;
   case OP_LEI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,sb,isk);
    break;
   case OP_GTI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,sb,isk);
    break;
   case OP_GEI:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,sb,isk);
    break;
   case OP_TEST:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,isk);
    break;
   case OP_TESTSET:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,isk);
    break;
   case OP_CALL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT);
    if (b==0) len += snprintf(buf + len,BUFFSIZE - len - 1,"all in "); else len += snprintf(buf + len,BUFFSIZE - len - 1,"%d in ",b-1);
    if (c==0) len += snprintf(buf + len,BUFFSIZE - len - 1,"all out"); else len += snprintf(buf + len,BUFFSIZE - len - 1,"%d out",c-1);
    break;
   case OP_TAILCALL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%d in",b-1);
    break;
   case OP_RETURN:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT);
    if (b==0) len += snprintf(buf + len,BUFFSIZE - len - 1,"all out"); else len += snprintf(buf + len,BUFFSIZE - len - 1,"%d out",b-1);
    break;
   case OP_RETURN0:
    break;
   case OP_RETURN1:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",a);
    break;
   case OP_FORLOOP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,bx);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "to %d",pc-bx+2);
    break;
   case OP_FORPREP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,bx);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "to %d",pc+bx+2);
    break;
   case OP_TFORPREP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,bx);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "to %d",pc+bx+2);
    break;
   case OP_TFORCALL:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,c);
    break;
   case OP_TFORLOOP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,bx);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "to %d",pc-bx+2);
    break;
   case OP_SETLIST:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    if (isk) len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%d",c+EXTRAARGC);
    break;
   case OP_CLOSURE:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,bx);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT "%p",VOID(f->p[bx]));
    break;
   case OP_VARARG:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d",a,c);
    len +=snprintf(buf + len,BUFFSIZE - len - 1,COMMENT);
    if (c==0) len += snprintf(buf + len,BUFFSIZE - len - 1,"all out"); else len += snprintf(buf + len,BUFFSIZE - len - 1,"%d out",c-1);
    break;
   case OP_VARARGPREP:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",a);
    break;
   case OP_EXTRAARG:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d",ax);
    break;
#if 0
   default:
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%d %d %d",a,b,c);
    len +=snprintf(buf + len,BUFFSIZE-nCOMMENT "not handled");
    break;
#endif
  }
  len += snprintf(buf + len,BUFFSIZE - len - 1,"\n");
 }

 return buf;
}

#define FIRST_RESERVED	(UCHAR_MAX + 1)
static const char *const luaX_tokens [] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while",
    "//", "..", "...", "==", ">=", "<=", "~=",
    "<<", ">>", "::", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};

//输出当前解析的token
const char *print_token(int token)
{
  if (token >= FIRST_RESERVED) return luaX_tokens[token - FIRST_RESERVED];

  const char *p = (char *)&token;
  return p;
}

typedef struct LoadF {
  int n;  /* number of pre-read characters */
  FILE *f;  /* file being read */
  char buff[BUFSIZ];  /* area for reading file */
} LoadF;

static int skipBOM (LoadF *lf) {
  const char *p = "\xEF\xBB\xBF";  /* UTF-8 BOM mark */
  int c;
  lf->n = 0;
  do {
    c = getc(lf->f);
    if (c == EOF || c != *(const unsigned char *)p++) return c;
    lf->buff[lf->n++] = c;  /* to be read by the parser */
  } while (*p != '\0');
  lf->n = 0;  /* prefix matched; discard it */
  return getc(lf->f);  /* return next character */
}

static int skipcomment (LoadF *lf, int *cp) {
  int c = *cp = skipBOM(lf);
  if (c == '#') {  /* first line is a comment (Unix exec. file)? */
    do {  /* skip first line */
      c = getc(lf->f);
    } while (c != EOF && c != '\n');
    *cp = getc(lf->f);  /* skip end-of-line, if present */
    return 1;  /* there was a comment */
  }
  else return 0;  /* no comment */
}

static int loadF_init(const char *filename,LoadF *lf)
{
  if (!lf) return -1;

  int c;

  lf->f = fopen(filename, "r");
  if (!lf->f) {
    return -1;
  }

  if (skipcomment(lf, &c))  /* read initial portion */
    lf->buff[lf->n++] = '\n';  /* add line to correct line numbers */
  if (c == LUA_SIGNATURE[0] && filename) {  //lua的二进制文件
    lf->f = freopen(filename, "rb", lf->f);
    if (lf->f == NULL) return -1;
    skipcomment(lf, &c);
  }
  if (c != EOF)
    lf->buff[lf->n++] = c;

  return 0;
}

static int loadF_uninit(LoadF *lf)
{
  if (!lf) return -1;

  fclose(lf->f);

  return 0;
}

static const char *getF (lua_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;  /* not used */
  if (lf->n > 0) {  //有预读取的字符串
    *size = lf->n;  /* return them (chars already in buffer) */
    lf->n = 0;  /* no more pre-read characters */
  }
  else {  /* read a block from file */
    /* 'fread' can return > 0 *and* set the EOF flag. If next call to
       'getF' called 'fread', it might still wait for user input.
       The next check avoids this problem. */
    if (feof(lf->f)) return NULL;
    *size = fread(lf->buff, 1, sizeof(lf->buff), lf->f);  /* read block */
  }
  return lf->buff;
}

//输出当前lua文件token
const char *print_tokens(LexState *ls)
{
  LoadF lf;
  ZIO z;
  int len = 0;
  int nextline = 0;
  Mbuffer buff = {0};
  LexState lexstate;
  int numline = 1;
  lua_State *L = luaL_newstate();
  const char *filename = (char *)(ls->source->contents) + 1;
  static char buf[BUFFSIZE] = {0};
  memset(buf,0,BUFFSIZE);

  luaL_openlibs(L);
  loadF_init(filename,&lf);
  luaZ_init(L, &z, getF, &lf);
  int c = zgetc(&z);
  lexstate.buff = &buff;
  lexstate.h = luaH_new(L);
  sethvalue2s(L, L->top, lexstate.h);  /* anchor it */
  luaD_inctop(L);
  luaX_setinput(L, &lexstate, &z, ls->source, c);

  luaX_next(&lexstate);
  while (lexstate.t.token != TK_EOS) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s ",print_token(lexstate.t.token));
    if (numline != lexstate.linenumber) {
      int i = len - 2;
      nextline = lexstate.linenumber - numline;
      while (*(buf + i--) != ' ' && i > 0);
      if (i != 0) {
        if (lexstate.linenumber - nextline == ls->linenumber) {
          char tem[LUA_MINBUFFER] = {0};
          memcpy(tem,buf + i + 2,LUA_MINBUFFER);
          len = i + 2;
          len += snprintf(buf + len,BUFFSIZE - len - 1,"      %s%s%s%s","<-----------    ",print_token(ls->t.token),"\n",tem);
        } else {
          *(buf + i + 1) = '\n';
        }
      }

      numline = lexstate.linenumber;
    }
    luaX_next(&lexstate);
  }
  len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","\n");

  L->top--;
  //luaH_free(L,lexstate.h);  //加入gc不需要自己释放(坑了我一个多小时，大坑大坑)
  luaZ_freebuffer(L, lexstate.buff);
  loadF_uninit(&lf);
  lua_close(L);
  
  return buf;
}

const char *print_listcolor(GCObject *item)
{
  static char buf[BUFFSIZE] = {0};
  memset(buf,0,BUFFSIZE);
  int len = 0;

  if (isblack(item)) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","color:black1");
  } else if (iswhite(item)) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","color:white");
  } else if (isgray(item)) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","color:gray");
  } else {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","color:black2");
  }

  return buf;
}

const char *print_listtype(GCObject *item)
{
  static char buf[BUFFSIZE] = {0};
  memset(buf,0,BUFFSIZE);
  int len = 0;

  len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","type:");
  if (item->tt == LUA_VCCL) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","CClosure");
  } else if (item->tt == LUA_VLCF) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","LClosure");
  } else if (item->tt == LUA_VUPVAL) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","UpVal");
  } else if (item->tt == LUA_VPROTO) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","Proto");
  } else if (item->tt == LUA_VUSERDATA) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","Udata");
  } else if (item->tt == LUA_VTABLE) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","Table");
  } else if (item->tt == LUA_VTHREAD) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","Thread");
  } else if (item->tt == LUA_VSHRSTR) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","shrstr");
  } else if (item->tt == LUA_VLNGSTR) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","lngstr");
  }

  return buf;
}

static GCObject *getgclist (GCObject *o) {
  switch (o->tt) {
    case LUA_VTABLE: return gco2t(o)->gclist;
    case LUA_VLCL: return gco2lcl(o)->gclist;
    case LUA_VCCL: return gco2ccl(o)->gclist;
    case LUA_VTHREAD: return gco2th(o)->gclist;
    case LUA_VPROTO: return gco2p(o)->gclist;
    case LUA_VUSERDATA: {
      Udata *u = gco2u(o);
      lua_assert(u->nuvalue > 0);
      return u->gclist;
    }
    default: lua_assert(0); return 0;
  }
}

const char *print_lists(GCObject *list)
{
  static char buf[BUFFSIZE] = {0};
  memset(buf,0,BUFFSIZE);
  int len = 0;

  for (GCObject *it = list;it;it = getgclist(it)) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","{\n");

    len += snprintf(buf + len,BUFFSIZE - len - 1,"  %s\n",print_listcolor(it));
    len += snprintf(buf + len,BUFFSIZE - len - 1,"  %s\n",print_listtype(it));
    len += snprintf(buf + len,BUFFSIZE - len - 1,"  addr:%p\n",it);
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","}\n");
  }

  return buf;
}

const char *print_nextlists(GCObject *list)
{
  static char buf[BUFFSIZE] = {0};
  memset(buf,0,BUFFSIZE);
  int len = 0;

  for (GCObject *it = list;it;it = it->next) {
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","{\n");

    len += snprintf(buf + len,BUFFSIZE - len - 1,"  %s\n",print_listcolor(it));
    len += snprintf(buf + len,BUFFSIZE - len - 1,"  %s\n",print_listtype(it));
    len += snprintf(buf + len,BUFFSIZE - len - 1,"  addr:%p\n",it);
    len += snprintf(buf + len,BUFFSIZE - len - 1,"%s","}\n");
    if (len >= BUFFSIZE) break;
  }

  return buf;
}