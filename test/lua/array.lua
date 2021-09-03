package.cpath = "/home/yh/public/read_lua/test/lib/?.so"
local array = require "arrtest"

local arr = array.new()

arr:push(1)
arr:push(10)
arr:foreach()
print("------")
print("len = "..arr:length())

arr:pop()
arr:push(1231)
arr:foreach()
