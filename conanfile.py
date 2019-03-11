from conans import ConanFile, CMake


class CairoConan(ConanFile):
  name = "smart_pointer"
  version = "0.1.0"
  description = "Draw on screen"
  url = "https://github.com/chreniuc/smart_pointer"
  homepage = "https://github.com/chreniuc/smart_pointer"
  author = "Hreniuc Cristian-Alexandru <cristi@hreniuc.pw>"
  license = "GNU LGPL 3"
  settings = "os", "arch", "compiler", "build_type"
  default_options = "*:shared=False", "cairo:xcb=True"
  generators = "cmake"
  chanel = "stable"
  author = "chreniuc"

  requires = ('cairo/1.15.14@chreniuc/stable')
  # We still have two librries linked dynamically
  # Dependencies of: http://www.linuxfromscratch.org/blfs/view/svn/x/libxcb.html
  # libXau and libXdmcp : Should I link them statically?

  def build(self):
    cmake = CMake(self)
    cmake.verbose = True
    cmake.configure()
    cmake.build()
