import os
from SCons.Script import Import

Import("env")

home = os.getenv("HOME")
var = os.getenv("LOCAL_CONFIG")
print(var.replace("~", home))
if var and os.path.exists(var.replace("~", home)):
    name = var.replace("~", home)
    env.Append(CCFLAGS=["-include", f"{name}"])
