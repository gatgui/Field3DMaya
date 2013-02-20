import excons
from excons.tools import maya
import os, sys, glob


mayaver = ARGUMENTS.get("maya-ver", None)
if not mayaver:
  print("=== Using maya 2011, override using maya-ver= or with-maya=")
  mayaver = "2011"
  ARGUMENTS["maya-ver"] = mayaver

if not ARGUMENTS.get("with-field3d", None):
  Field3D_build = True
  prefix = os.path.abspath("./Field3D/install/%s/%s/release" % (sys.platform, "m64" if excons.Build64() else "m32"))
  Field3D_inc = "%s/include" % prefix
  Field3D_lib = "%s/lib" % prefix
  ARGUMENTS["with-field3d-inc"] = Field3D_inc
  ARGUMENTS["with-field3d-lib"] = Field3D_lib
else:
  Field3D_inc, Field3D_lib = excons.GetDirs("field3d")

HDF5_inc, HDF5_lib = excons.GetDirs("hdf5")

OpenEXR_inc, OpenEXR_lib = excons.GetDirs("openexr")

if Field3D_build:
  env = Environment()
  env.Append(CPPPATH = [HDF5_inc, OpenEXR_inc, OpenEXR_inc+"/OpenEXR"])
  env.Append(LIBPATH = [HDF5_lib, OpenEXR_lib])
  Export("env")
  SConscript("Field3D/SConscript")

targets = [
  {"name"    : "maya%s/plug-ins/field3dFluidCache" % mayaver,
   "alias"   : "mayaField3d",
   "type"    : "dynamicmodule",
   "ext"     : maya.PluginExt(),
   "srcs"    : glob.glob("src/*.cpp"),
   "incdirs" : [OpenEXR_inc, OpenEXR_inc+"/OpenEXR", HDF5_inc, Field3D_inc],
   "libdirs" : [Field3D_lib],
   "libs"    : ["Field3D"],
   "custom"  : [maya.Require, maya.Plugin]},
  {"name"    : "maya%s/plug-ins/exportF3d" % mayaver,
   "alias"   : "exportF3d",
   "type"    : "dynamicmodule",
   "ext"     : maya.PluginExt(),
   "srcs"    : ["Field3D/contrib/maya_plugin/exportF3d/exportF3d.cpp"],
   "incdirs" : [OpenEXR_inc, OpenEXR_inc+"/OpenEXR", HDF5_inc, Field3D_inc],
   "libdirs" : [Field3D_lib],
   "libs"    : ["Field3D"],
   "custom"  : [maya.Require, maya.Plugin]}
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, targets)

Default(["mayaField3d", "exportF3d"])
