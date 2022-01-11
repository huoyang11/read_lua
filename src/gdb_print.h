#ifndef _GDB_PRINT_
#define _GDB_PRINT_

#include "lobject.h"

const char *print_value(lua_State *L,const TValue *o);

#endif