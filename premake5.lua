-- premake5.lua
--
-- *** Please fix this script for your environment. ***
--
sources = {
  "./src/**.cpp",
}

workspace "GlslRenderWorkspace"
  configurations { "release", "debug" }
  language "C++"
  basedir "build"

  includedirs { "/usr/local/include" }
  libdirs { "/usr/local/lib" }
  buildoptions { "-fpermissive" }

  -- Links
  configuration { "macosx", "gmake" }
    links { "GLEW", "glfw3" }
    links { "pthread" }
    linkoptions { '-framework OpenGL' }
  configuration { "linux", "gmake" }
    -- for Arch Linux
--     links { "GLEW", "glfw", "GLU", "GL" }
    -- for Other linux
    links { "GLEW", "glfw3", "GLU", "GL" }
    links { "X11", "Xrandr", "Xi", "Xxf86vm", "Xcursor", "Xinerama" }
    links { "pthread", "dl" }

  -- Configuration
  configuration "debug"
    defines { "DEBUG" }
    flags { "Symbols" }

  configuration "release"
    defines { "NDEBUG" }
    flags { "Optimize" }

  project( "renderer" )
    kind "ConsoleApp"
    files { sources }
