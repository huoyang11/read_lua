#include <lua.hpp>
#include <iostream>
#include <vector>

using namespace std;

static int init(lua_State *L)
{
	vector<int> **arr = (vector<int> **)lua_newuserdata(L, sizeof(void *));
    *arr = new vector<int>;

    printf("addr = %p\n",arr);

	 //设置元表
	luaL_getmetatable(L, "array");
	lua_setmetatable(L, -2);

	//lua_pushnumber(L, 123);

	return 1; // 新的userdata已经在栈上了
}

static int arr_push(lua_State *L)
{
	vector<int> **arr = (vector<int> **)luaL_checkudata(L, 1, "array");
    int val = luaL_checkinteger(L, 2);
	(*arr)->push_back(val);
	return 1;
}

static int arr_pop(lua_State *L)
{
	// 第一个参数是userdata
	vector<int> **arr = (vector<int> **)luaL_checkudata(L, 1, "array");
    (*arr)->pop_back();
	return 0;
}

static int arr_length(lua_State *L)
{
	// 第一个参数是userdata
	vector<int> **arr = (vector<int> **)luaL_checkudata(L, 1, "array");
    int size = (*arr)->size();

	lua_pushinteger(L,size);

	return 1;
}

static int arr_foreach(lua_State *L)
{
	vector<int> **arr = (vector<int> **)luaL_checkudata(L, 1, "array");
	int size = (*arr)->size();

	for(auto it : **arr)
	{
		printf("%d\n",it);
	}

	return 0;
}

static  luaL_Reg arrayFunc_meta[] =
{
	{ "push", arr_push },
	{ "pop", arr_pop },
	{ "length", arr_length },
	{ "foreach", arr_foreach },
	{ NULL, NULL }
};

static luaL_Reg arrayFunc[] =
{
	{ "new", init},
	{ NULL, NULL }
};

extern "C" int luaopen_arrtest(lua_State *L)
{
	// 创建一个新的元表
	luaL_newmetatable(L, "array");
	// 元表.__index = 元表
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, arrayFunc_meta, 0);
	luaL_newlib(L, arrayFunc);
	lua_pushvalue(L, -1);
	lua_setglobal(L, "array"); /* the module name */
	return 1;
}