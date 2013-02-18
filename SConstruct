import excons
from excons.tools import maya
import os, sys, glob

Field3D_prefix = ARGUMENTS.get("with-field3d", None)
Field3D_build  = False
HDF5_prefix    = ARGUMENTS.get("with-hdf5", None)
OpenEXR_prefix = ARGUMENTS.get("with-openexr", None)

mayaver = ARGUMENTS.get("maya-ver", None)

if sys.platform in ["win32", "darwin"]:
  libdir = "lib"
else:
  libdir = ("lib64" if excons.arch_dir == "x64" else "lib")

if not Field3D_prefix:
  Field3D_build = True
  Field3D_prefix = "./Field3D/install/%s/%s/release" % (sys.platform, "m64" if excons.arch_dir == "x64" else "m32")
  print("=== Using Field3D from %s, override using with-field3d=" % Field3D_prefix)

if not HDF5_prefix:
  if sys.platform == "win32":
    raise Exception("Please provide HDF5 prefix using with-hdf5=")
  elif sys.platform == "darwin":
    print("=== Using HDF5 from /opt/local, override using with-hdf5=")
    HDF5_prefix = "/opt/local"
  else:
    print("=== Using HDF5 from /usr, override using with-hdf5=")
    HDF5_prefix = "/usr"

if not OpenEXR_prefix:
  if sys.platform == "win32":
    raise Exception("Please provide OpenEXR prefix using with-openexr=")
  elif sys.platform == "darwin":
    print("=== Using OpenEXR from /opt/local, override using with-hdf5=")
    OpenEXR_prefix = "/opt/local"
  else:
    print("=== Using OpenEXR from /usr, override using with-hdf5=")
    OpenEXR_prefix = "/usr"

if not mayaver:
  print("=== Using maya 2011, override using maya-ver= or with-maya=")
  ARGUMENTS["maya-ver"] = "2011"

Field3D_prefix = os.path.abspath(os.path.expanduser(Field3D_prefix))
HDF5_prefix    = os.path.abspath(os.path.expanduser(HDF5_prefix))
OpenEXR_prefix = os.path.abspath(os.path.expanduser(OpenEXR_prefix))

if Field3D_build:
  # Generate Site.py for Field3D
  print("=== Generating Field3D Site.py...")
  sitepy = open("Field3D/Site.py", "w")
  sitepy.write("incPaths = ['%s/include', '%s/include', '%s/include/OpenEXR']\n" % (HDF5_prefix, OpenEXR_prefix, OpenEXR_prefix))
  sitepy.write("libPaths = ['%s/%s', '%s/%s']\n" % (HDF5_prefix, libdir, OpenEXR_prefix, libdir))
  sitepy.write("\n")
  sitepy.close()
  
  env = Environment()
  Export("env")
  SConscript("Field3D/SConscript")

targets = [
  {"name"    : "Field3DMayaPlugin",
   "type"    : "dynamicmodule",
   "ext"     : maya.PluginExt(),
   "srcs"    : glob.glob("src/*.cpp"),
   "incdirs" : ["%s/include" % OpenEXR_prefix,
                "%s/include/OpenEXR" % OpenEXR_prefix,
                "%s/include" % HDF5_prefix,
                "%s/include" % Field3D_prefix],
   "libdirs" : ["%s/%s" % (OpenEXR_prefix, libdir),
                "%s/%s" % (HDF5_prefix, libdir),
                "%s/%s" % (Field3D_prefix, "lib" if Field3D_build else libdir)],
   "libs"    : ["hdf5", "IlmThread", "Iex", "Imath", "Half", "Field3D"],
   "custom"  : [maya.Require]}
]

env = excons.MakeBaseEnv()
env.Append(CCFLAGS = ["-Wno-deprecated"])
excons.DeclareTargets(env, targets)

Default(["Field3DMayaPlugin"])
