addf = package.loadlib("../lib/testlualib.so","lua_add")
sum = addf(60,34)
print(sum)
