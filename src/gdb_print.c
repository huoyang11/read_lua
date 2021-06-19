/*
    用于写gdb脚本,方便调试
*/
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
  static char buf[BUFFSIZE] = {0};
  memset(buf,0,sizeof(buf));
  int len = 0;

  StackValue tem[2] = {0};
  setnilvalue(s2v(tem));
  while(luaH_next(L,t,tem)) {
    len += snprintf(buf + len,BUFFSIZE - len,"%s",print_value(L,s2v(tem)));
    len += snprintf(buf + len,BUFFSIZE - len,"\t");
    len += snprintf(buf + len,BUFFSIZE - len,"%s",print_value(L,s2v(tem + 1)));
    len += snprintf(buf + len,BUFFSIZE - len,"\n");
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
 len += snprintf(buf + len,BUFFSIZE-len,"\"");
 for (i=0; i<n; i++)
 {
  int c=(int)(unsigned char)s[i];
  switch (c)
  {
   case '"':
	len += snprintf(buf + len,BUFFSIZE-len,"\\\"");
	break;
   case '\\':
	len += snprintf(buf + len,BUFFSIZE-len,"\\\\");
	break;
   case '\a':
	len += snprintf(buf + len,BUFFSIZE-len,"\\a");
	break;
   case '\b':
	len += snprintf(buf + len,BUFFSIZE-len,"\\b");
	break;
   case '\f':
	len += snprintf(buf + len,BUFFSIZE-len,"\\f");
	break;
   case '\n':
	len += snprintf(buf + len,BUFFSIZE-len,"\\n");
	break;
   case '\r':
	len += snprintf(buf + len,BUFFSIZE-len,"\\r");
	break;
   case '\t':
	len += snprintf(buf + len,BUFFSIZE-len,"\\t");
	break;
   case '\v':
	len += snprintf(buf + len,BUFFSIZE-len,"\\v");
	break;
   default:
	if (isprint(c)) len += snprintf(buf + len,BUFFSIZE-len,"%c",c); else len += snprintf(buf + len,BUFFSIZE-len,"\\%03d",c);
	break;
  }
 }
 len += snprintf(buf + len,BUFFSIZE-len,"\"");

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
	len += snprintf(buf + len,BUFFSIZE-len,"nil");
	break;
  case LUA_VFALSE:
	len += snprintf(buf + len,BUFFSIZE-len,"false");
	break;
  case LUA_VTRUE:
	len += snprintf(buf + len,BUFFSIZE-len,"true");
	break;
  case LUA_VNUMFLT:
	{
	char buff[100];
	sprintf(buff,LUA_NUMBER_FMT,fltvalue(o));
	len += snprintf(buf + len,BUFFSIZE-len,"%s",buff);
	if (buff[strspn(buff,"-0123456789")]=='\0') len += snprintf(buf + len,BUFFSIZE-len,".0");
	break;
	}
  case LUA_VNUMINT:
	len += snprintf(buf + len,BUFFSIZE-len,LUA_INTEGER_FMT,ivalue(o));
	break;
  case LUA_VSHRSTR:
  case LUA_VLNGSTR:
	len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintString(tsvalue(o)));
	break;
  default:				/* cannot happen */
	len += snprintf(buf + len,BUFFSIZE-len,"?%d",ttypetag(o));
	break;
 }

 return buf;
}
const char *PrintCode(lua_State* L,const Proto* f)
{
 int len = 0;
 tmname=G(L)->tmname;
 static char buf[BUFFSIZE] = {0};
 memset(buf,0,BUFFSIZE);
 const Instruction* code=f->code;
 int pc,n=f->sizecode;
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
  len += snprintf(buf + len,BUFFSIZE-len,"\t%d\t",pc+1);
  if (line>0) len += snprintf(buf + len,BUFFSIZE-len,"[%d]\t",line); else len += snprintf(buf + len,BUFFSIZE-len,"[-]\t");
  len += snprintf(buf + len,BUFFSIZE-len,"%-9s\t",opnames[o]);
  switch (o)
  {
   case OP_MOVE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	break;
   case OP_LOADI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,sbx);
	break;
   case OP_LOADF:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,sbx);
	break;
   case OP_LOADK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,bx);
	len += snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,bx));
	break;
   case OP_LOADKX:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",a);
	len += snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,EXTRAARG));
	break;
   case OP_LOADFALSE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",a);
	break;
   case OP_LFALSESKIP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",a);
	break;
   case OP_LOADTRUE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",a);
	break;
   case OP_LOADNIL:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	len += snprintf(buf + len,BUFFSIZE-len,COMMENT "%d out",b+1);
	break;
   case OP_GETUPVAL:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	len += snprintf(buf + len,BUFFSIZE-len,COMMENT "%s",UPVALNAME(b));
	break;
   case OP_SETUPVAL:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%s",UPVALNAME(b));
	break;
   case OP_GETTABUP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%s",UPVALNAME(b));
	len += snprintf(buf + len,BUFFSIZE-len," "); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_GETTABLE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_GETI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_GETFIELD:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_SETTABUP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d%s",a,b,c,ISK);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%s",UPVALNAME(a));
	len += snprintf(buf + len,BUFFSIZE-len," "); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,b));
	if (isk) { len += snprintf(buf + len,BUFFSIZE-len," "); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c)); }
	break;
   case OP_SETTABLE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d%s",a,b,c,ISK);
	if (isk) { len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c)); }
	break;
   case OP_SETI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d%s",a,b,c,ISK);
	if (isk) { len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c)); }
	break;
   case OP_SETFIELD:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d%s",a,b,c,ISK);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,b));
	if (isk) { len += snprintf(buf + len,BUFFSIZE-len," "); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c)); }
	break;
   case OP_NEWTABLE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%d",c+EXTRAARGC);
	break;
   case OP_SELF:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d%s",a,b,c,ISK);
	if (isk) { len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c)); }
	break;
   case OP_ADDI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,sc);
	break;
   case OP_ADDK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_SUBK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_MULK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_MODK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_POWK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_DIVK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_IDIVK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_BANDK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_BORK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_BXORK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,c));
	break;
   case OP_SHRI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,sc);
	break;
   case OP_SHLI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,sc);
	break;
   case OP_ADD:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_SUB:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_MUL:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_MOD:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_POW:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_DIV:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_IDIV:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_BAND:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_BOR:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_BXOR:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_SHL:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_SHR:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	break;
   case OP_MMBIN:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%s",eventname(c));
	break;
   case OP_MMBINI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d %d",a,sb,c,isk);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%s",eventname(c));
	if (isk) len += snprintf(buf + len,BUFFSIZE-len," flip");
	break;
   case OP_MMBINK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d %d",a,b,c,isk);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%s ",eventname(c)); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,b));
	if (isk) len += snprintf(buf + len,BUFFSIZE-len," flip");
	break;
   case OP_UNM:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	break;
   case OP_BNOT:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	break;
   case OP_NOT:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	break;
   case OP_LEN:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	break;
   case OP_CONCAT:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,b);
	break;
   case OP_CLOSE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",a);
	break;
   case OP_TBC:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",a);
	break;
   case OP_JMP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",GETARG_sJ(i));
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "to %d",GETARG_sJ(i)+pc+2);
	break;
   case OP_EQ:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,isk);
	break;
   case OP_LT:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,isk);
	break;
   case OP_LE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,isk);
	break;
   case OP_EQK:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,isk);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT); len += snprintf(buf + len,BUFFSIZE-len,"%s",PrintConstant(f,b));
	break;
   case OP_EQI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,sb,isk);
	break;
   case OP_LTI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,sb,isk);
	break;
   case OP_LEI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,sb,isk);
	break;
   case OP_GTI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,sb,isk);
	break;
   case OP_GEI:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,sb,isk);
	break;
   case OP_TEST:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,isk);
	break;
   case OP_TESTSET:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,isk);
	break;
   case OP_CALL:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT);
	if (b==0) len += snprintf(buf + len,BUFFSIZE-len,"all in "); else len += snprintf(buf + len,BUFFSIZE-len,"%d in ",b-1);
	if (c==0) len += snprintf(buf + len,BUFFSIZE-len,"all out"); else len += snprintf(buf + len,BUFFSIZE-len,"%d out",c-1);
	break;
   case OP_TAILCALL:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%d in",b-1);
	break;
   case OP_RETURN:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT);
	if (b==0) len += snprintf(buf + len,BUFFSIZE-len,"all out"); else len += snprintf(buf + len,BUFFSIZE-len,"%d out",b-1);
	break;
   case OP_RETURN0:
	break;
   case OP_RETURN1:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",a);
	break;
   case OP_FORLOOP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,bx);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "to %d",pc-bx+2);
	break;
   case OP_FORPREP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,bx);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "to %d",pc+bx+2);
	break;
   case OP_TFORPREP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,bx);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "to %d",pc+bx+2);
	break;
   case OP_TFORCALL:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,c);
	break;
   case OP_TFORLOOP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,bx);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "to %d",pc-bx+2);
	break;
   case OP_SETLIST:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	if (isk) len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%d",c+EXTRAARGC);
	break;
   case OP_CLOSURE:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,bx);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT "%p",VOID(f->p[bx]));
	break;
   case OP_VARARG:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d",a,c);
	len +=snprintf(buf + len,BUFFSIZE-len,COMMENT);
	if (c==0) len += snprintf(buf + len,BUFFSIZE-len,"all out"); else len += snprintf(buf + len,BUFFSIZE-len,"%d out",c-1);
	break;
   case OP_VARARGPREP:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",a);
	break;
   case OP_EXTRAARG:
	len += snprintf(buf + len,BUFFSIZE-len,"%d",ax);
	break;
#if 0
   default:
	len += snprintf(buf + len,BUFFSIZE-len,"%d %d %d",a,b,c);
	len +=snprintf(buf + len,BUFFSIZE-nCOMMENT "not handled");
	break;
#endif
  }
  len += snprintf(buf + len,BUFFSIZE-len,"\n");
 }

 return buf;
}