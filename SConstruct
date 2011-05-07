VariantDir("build", "src")

name = "simgui"
#files = Glob("build/*.cpp")
files = ["build/gui.cpp", "build/gui_draw.cpp", "build/main.cpp"]
libs = ["GL", "GLU", "sfml-window", "sfml-system", "sfml-graphics"]

Program(name, files, LIBS=libs, CCFLAGS="-g")

