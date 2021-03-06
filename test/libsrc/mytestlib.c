#include "lua.h"  
#include "lauxlib.h"  
#include "lualib.h"  

#include <stdio.h>  

struct StudentTag
{
	char *strName; // 学生姓名
	char *strNum; // 学号
	int iSex; // 学生性别
	int iAge; // 学生年龄
}T;

static int Student(lua_State *L)
{
	size_t iBytes = sizeof(struct StudentTag);
	struct StudentTag *pStudent;
	pStudent = (struct StudentTag *)lua_newuserdata(L, iBytes);

	 //设置元表
	luaL_getmetatable(L, "Student");
	lua_setmetatable(L, -2);

	//lua_pushnumber(L, 123);

	return 1; // 新的userdata已经在栈上了
}

static int GetName(lua_State *L)
{
	struct StudentTag *pStudent = (struct StudentTag *)luaL_checkudata(L, 1, "Student");
	lua_pushstring(L, pStudent->strName);
	return 1;
}

static int SetName(lua_State *L)
{
	// 第一个参数是userdata
	struct StudentTag *pStudent = (struct StudentTag *)luaL_checkudata(L, 1, "Student");

	// 第二个参数是一个字符串
	const char *pName = luaL_checkstring(L, 2);
	luaL_argcheck(L, pName != NULL && pName != "", 2, "Wrong Parameter");

	pStudent->strName =(char*) pName;
	return 0;
}

static int GetAge(lua_State *L)
{
	struct StudentTag *pStudent = (struct StudentTag *)luaL_checkudata(L, 1, "Student");
	lua_pushinteger(L, pStudent->iAge);
	return 1;
}

static int SetAge(lua_State *L)
{
	struct StudentTag *pStudent = (struct StudentTag *)luaL_checkudata(L, 1, "Student");

	int iAge = luaL_checkinteger(L, 2);
	luaL_argcheck(L, iAge >= 6 && iAge <= 100, 2, "Wrong Parameter");
	pStudent->iAge = iAge;
	return 0;
}

static int GetSex(lua_State *L)
{
	// 这里由你来补充
	return 1;
}

static int SetSex(lua_State *L)
{
	// 这里由你来补充
	return 0;
}

static int GetNum(lua_State *L)
{
	// 这里由你来补充
	return 1;
}

static int SetNum(lua_State *L)
{
	// 这里由你来补充
	return 0;
}

static  luaL_Reg arrayFunc_meta[] =
{
	{ "getName", GetName },
	{ "setName", SetName },
	{ "getAge", GetAge },
	{ "setAge", SetAge },
	{ "getSex", GetSex },
	{ "setSex", SetSex },
	{ "getNum", GetNum },
	{ "setNum", SetNum },
	{ NULL, NULL }
};

static luaL_Reg arrayFunc[] =
{
	{ "new", Student},
	{ NULL, NULL }
};

int luaopen_mytestlib(lua_State *L)
{
	// 创建一个新的元表
	luaL_newmetatable(L, "Student");
	// 元表.__index = 元表
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, arrayFunc_meta, 0);
	luaL_newlib(L, arrayFunc);
	lua_pushvalue(L, -1);
	lua_setglobal(L, "Sdudent"); /* the module name */
	return 1;
}
