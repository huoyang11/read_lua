# gdb脚本

lua_script.py   python版本
lua_script      gdb本身支持的版本

## 加载
```shell
打开
vim ~/.gdbinit

在文件中添加
source 脚本路径
```

## 命令

```
lua_Tval  输出TValue类型
参数1: lua_State *
参数2: TValue *


plua_table  输出Table类型
参数1: lua_State *
参数2: Table *

plua_Ttable  输出Table类型
参数1: lua_State *
参数2: TValue *

plua_str 输出TString类型
参数1: TString *
```
