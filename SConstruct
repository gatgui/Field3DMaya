import os
import re
import sys
import glob
import excons
from excons.tools import maya
from excons.tools import ilmbase
from excons.tools import boost
from excons.tools import hdf5
from excons.tools import dl

defs = []
incdirs = []
libdirs = []
libs = []

# Field3D configuration
field3d_static = (excons.GetArgument("field3d-static", 0, int) != 0)

field3d_inc, field3d_lib = excons.GetDirs("field3d")
if not field3d_inc and not field3d_lib:
  # Build Field3D as a static lib
  field3d_static = True
  excons.SetArgument("field3d-static", 1)
  
  SConscript("Field3D/SConstruct")

if field3d_inc:
  incdirs.append(field3d_inc)

if field3d_lib:
  libdirs.append(field3d_lib)

libs.append("Field3D")

if field3d_static:
  defs.append("FIELD3D_STATIC")

if sys.platform == "win32":
  defs.append("NO_TTY")

# Maya plugin
targets = [
  {"name"    : "maya%s/plug-ins/f3dTools" % maya.Version(),
   "alias"   : "f3dTools",
   "type"    : "dynamicmodule",
   "ext"     : maya.PluginExt(),
   "defs"    : defs,
   "srcs"    : glob.glob("src/*.cpp"),
   "incdirs" : incdirs,
   "libdirs" : libdirs,
   "libs"    : libs,
   "custom"  : [hdf5.Require(hl=False),
                ilmbase.Require(ilmthread=False, iexmath=False),
                boost.Require(libs=["system", "regex"]),
                maya.Require, maya.Plugin]}
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, targets)

Default(["f3dTools"])
