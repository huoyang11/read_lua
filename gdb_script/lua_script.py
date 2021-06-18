import gdb
 
class plua_Tval(gdb.Command):
 
    def __init__(self):
        super(self.__class__, self).__init__("plua_Tval", gdb.COMMAND_USER)
 
    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        if len(argv) != 2:
            raise gdb.GdbError('输入参数数目不对')
        gdb.execute('p print_value(' + argv[0] + "," + argv[1] + ")")
 
plua_Tval()
