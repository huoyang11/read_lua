import gdb
 
class plua_Tval(gdb.Command):
 
    def __init__(self):
        super(self.__class__, self).__init__("plua_Tval", gdb.COMMAND_USER)
 
    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        if len(argv) != 2:
            raise gdb.GdbError('输入参数数目不对')
        gdb.execute("p print_value(" + argv[0] + "," + argv[1] + ")")

class plua_Ttable(gdb.Command):
 
    def __init__(self):
        super(self.__class__, self).__init__("plua_Ttable", gdb.COMMAND_USER)
 
    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        if len(argv) != 2:
            raise gdb.GdbError('输入参数数目不对')
        gdb.execute('printf "%s",print_table(' + argv[0] + "," + "hvalue(" + argv[1] + "))")

class plua_table(gdb.Command):
 
    def __init__(self):
        super(self.__class__, self).__init__("plua_table", gdb.COMMAND_USER)
 
    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        if len(argv) != 2:
            raise gdb.GdbError('输入参数数目不对')
        gdb.execute('printf "%s",print_table(' + argv[0] + "," + argv[1] + ")")

class plua_str(gdb.Command):
 
    def __init__(self):
        super(self.__class__, self).__init__("plua_str", gdb.COMMAND_USER)
 
    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        if len(argv) != 1:
            raise gdb.GdbError('输入参数数目不对')
        gdb.execute("p (char *)(((TString *)" + argv[0] + ")->contents)")

class plua_code(gdb.Command):
 
    def __init__(self):
        super(self.__class__, self).__init__("plua_code", gdb.COMMAND_USER)
 
    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        if len(argv) != 2:
            raise gdb.GdbError('输入参数数目不对')
        gdb.execute('printf "%s",PrintCode(' + argv[0] + "," + argv[1] + ")")

class plua_token(gdb.Command):
 
    def __init__(self):
        super(self.__class__, self).__init__("plua_token", gdb.COMMAND_USER)
 
    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        if len(argv) != 1:
            raise gdb.GdbError('输入参数数目不对')
        gdb.execute("p print_token(" + argv[0] + "->t.token)")

plua_Tval()
plua_Ttable()
plua_table()
plua_str()
plua_code()

gdb.execute("define plua_lscode\nplua_code $arg0->L $arg0->fs.f\nend")

plua_token()