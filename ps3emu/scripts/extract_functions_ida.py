from idaapi import *

with open("c:\\output.txt", "w") as f:
    for i in range(0, get_func_qty()):
        func = getn_func(i)
        f.write("%x\n" % func.startEA)
