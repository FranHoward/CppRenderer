# OpenGL dependencies

GLFW source is vendored without changes from the official 3.4 release:
https://github.com/glfw/glfw/tree/3.4
Its zlib/libpng license is preserved in glfw/LICENSE.md.

The generated header-only GLAD OpenGL 3.3 loader is the file distributed by GLFW
3.4 at deps/glad/gl.h, with its generation metadata and embedded Khronos license
preserved. source/Glad.cpp supplies its single implementation translation unit.
The application requests a core profile and only uses core 3.3 entry points.

GLAD generator license: https://github.com/Dav1dde/glad/blob/glad2/LICENSE
GLAD-generated code is public domain (WTFPL/CC0); Khronos-derived portions remain
under the Khronos MIT license embedded in the generated header.
