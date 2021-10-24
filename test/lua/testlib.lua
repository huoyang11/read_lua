package.cpath = "../lib/?.so"
local test = require "mytestlib"

local stu = test.new()

stu:setAge(55)

print(stu:getAge())
