import excons
from excons.tools import maya
import os, glob

Field3D_prefix = ARGUMENTS.get("with-field3d", None)
HDF5_prefix    = ARGUMENTS.get("with-hdf5", None)
OpenEXR_prefix = ARGUMENTS.get("with-openexr", None)

if not Field3D_prefix:
  raise Exception("Please provide Field3D prefix using with-field3d= flag")

if not HDF5_prefix:
  raise Exception("Please provide HDF5 prefix using with-hdf5= flag")

if not OpenEXR_prefix:
  raise Exception("Please provide OpenEXR prefix using with-openexr= flag")

Field3D_prefix = os.path.abspath(os.path.expanduser(Field3D_prefix))
HDF5_prefix    = os.path.abspath(os.path.expanduser(HDF5_prefix))
OpenEXR_prefix = os.path.abspath(os.path.expanduser(OpenEXR_prefix))

targets = [
  {"name"    : "Field3DMayaPlugin",
   "type"    : "dynamicmodule",
   "ext"     : maya.PluginExt(),
   "srcs"    : glob.glob("src/*.cpp"),
   "incdirs" : ["%s/include" % OpenEXR_prefix,
                "%s/include/OpenEXR" % OpenEXR_prefix,
                "%s/include" % HDF5_prefix,
                "%s/include" % Field3D_prefix],
   "libdirs" : ["%s/lib" % OpenEXR_prefix,
                "%s/lib" % HDF5_prefix,
                "%s/lib" % Field3D_prefix],
   "libs"    : ["hdf5", "IlmThread", "Iex", "Imath", "Half", "Field3D"],
   "custom"  : [maya.Require]}
]

env = excons.MakeBaseEnv()
env.Append(CCFLAGS = ["-Wno-deprecated"])
excons.DeclareTargets(env, targets)
