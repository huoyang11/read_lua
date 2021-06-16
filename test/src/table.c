#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <ltable.h>
#include <lstring.h>
#include <lobject.h>


static void PrintString(const TString* ts)
{
    const char* s=getstr(ts);
    size_t i,n=tsslen(ts);
    printf("\"");
    for (i=0; i<n; i++)
    {
        int c=(int)(unsigned char)s[i];
        switch (c)
        {
            case '"':
                printf("\\\"");
                break;
            case '\\':
                printf("\\\\");
                break;
            case '\a':
                printf("\\a");
                break;
            case '\b':
                printf("\\b");
                break;
            case '\f':
                printf("\\f");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\t':
                printf("\\t");
                break;
            case '\v':
                printf("\\v");
                break;
            default:
                if (isprint(c)) printf("%c",c); else printf("\\%03d",c);
                break;
        }
    }
    printf("\"");
}

static void print_value(const TValue *o)
{
    switch (ttypetag(o))
    {
        case LUA_VNIL:
            printf("nil");
            break;
        case LUA_VFALSE:
            printf("false");
            break;
        case LUA_VTRUE:
            printf("true");
            break;
        case LUA_VNUMFLT:
            {
                char buff[100];
                sprintf(buff,LUA_NUMBER_FMT,fltvalue(o));
                printf("%s",buff);
                if (buff[strspn(buff,"-0123456789")]=='\0') printf(".0");
                break;
            }
        case LUA_VNUMINT:
            printf(LUA_INTEGER_FMT,ivalue(o));
            break;
        case LUA_VSHRSTR:
        case LUA_VLNGSTR:
            PrintString(tsvalue(o));
            break;
        default:				/* cannot happen */
            printf("?%d",ttypetag(o));
            break;
    }
}

struct kv_data
{
    char *key;
    int  value;
};

static struct kv_data data[] = {
    {"a",1},
    {"b",2},
    {"c",3},
    {"d",4},
    {"e",5},
    {"f",6},
    {"g",7},
    {"h",8},
    {"i",9},
    {"j",10},
    {"k",12},
    {"l",13},
    {"m",14},
    {"n",15},
    {"o",16},
    {"p",17},
    {NULL,-1},
};

int main(int argc,char *argv[])
{
    lua_State *L = luaL_newstate();

    Table *t = luaH_new(L);

    luaH_resize(L,t,3,4);

    TValue key,val;

    struct kv_data *it = data;
    for(;it->key;it++) {
        setsvalue(L, &key, luaS_new(L,it->key));
        setivalue(&val, it->value);
        luaH_set(L,t,&key,&val);
        //setivalue(p, it->value);
    }

    setnilvalue(s2v(L->top));
    while(luaH_next(L,t,L->top)) {
        //printf("%s   %f\n",getstr(tsvalue(s2v(L->top))),nvalue(s2v(L->top+1)));
        print_value(s2v(L->top));
        printf("\t");
        print_value(s2v(L->top+1));
        printf("\n");
    }

    printf("sizeof(node) = %ld\n",sizeof(Node));

    return 0;
}
