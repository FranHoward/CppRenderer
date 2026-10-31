# C++Renderer

This is a starter C++17 renderer project built from a user-provided template.

The project includes:

- GLM 1.0.3 (header-only mathematics library)
- MiniFB 0.10.1 (window and pixel-buffer display)
- CMake build configuration

## Building on Windows

Install CMake and ensure that `cmake`, `gcc`, and `g++` are available in your `PATH`. Then double-click `build.bat`, or run the following command in a terminal:

```powershell
.\build.bat
```

The generated executable is located at `build\C++Renderer.exe`.

## Building on Linux or macOS

```sh
./build.sh
```

The application entry point is `source/main.cpp`. The renderer uses GLM for camera and geometry calculations and MiniFB to display the ray-traced scene.

## XML scenes

Scene loading is implemented in `source/SceneLoader.cpp` through the existing
`Scene::LoadSceneFromXML(filepath, width, height)` factory. The caller owns the
returned Scene. `Renderer::Renderer` accepts a `const char* filepath` argument;
`main` passes `scene/Scene02.xml`, two spheres with red and blue Lambert materials
and a point light. `scene/CornellBox.xml` is an open-front Cornell box with five inward-facing
walls, two spheres, a rotated box and one point light. `scene/Scene01.xml`
retains the original camera, two-triangle square and sphere.
The renderer uses direct lighting with hard shadows and evaluates assigned material BRDFs.
Objects without a material keep the previous light-only response.

The XML format is case-sensitive:

- `Scene` contains exactly one `Camera`, objects and lights. Objects may be grouped
  in `SceneObjects`, and lights in `Lights`, or placed directly under `Scene`.
- `Camera` requires `position`, `target`, `up`, `fov`, `near` and `far` attributes.
- `SceneObject` accepts `position`, `rotation` and `scale`; defaults are `0 0 0`,
  `0 0 0` and `1`. Rotation uses Euler angles in degrees; scale is uniform
  and must be positive.
- `LambertMaterial` requires a unique, nonblank `name` and linear RGB `albedo`
  with each component in `[0, 1]`. Definitions may be grouped in `Materials` or
  placed directly under `Scene`. Assign one with `SceneObject material="RedLambert"`.
  Definitions may appear before or after objects; unknown references and duplicate
  names are errors. Materials are owned by the scene and may be shared by objects.
  Lambert evaluates `albedo / pi`.
- Each `SceneObject` contains at least one `Triangle` (`v0`, `v1`, `v2`),
  `Sphere` (`radius`), or `Disk` (`radius`). Vertices and radii use object
  space; vectors use three space-separated numbers. A Disk lies in local z = 0.
- `DirectionalLight` requires `direction` and RGB `radiance`.
- `PointLight` requires `position`, RGB `intensity` and `attenuations`.
- `SpotLight` additionally requires `direction`, `innerAngle` and `outerAngle`.
  Directions describe light travel and must be nonzero. Angles are cone half-angles
  in degrees, with `0 <= innerAngle < outerAngle < 180`.
- Light colors must be nonnegative. `attenuations` uses **quadratic, linear,
  constant** order: `1 / (x*r*r + y*r + z)`. Coefficients must be nonnegative
  and not all zero; for example `1 0 1` avoids a singularity at the source.

Invalid files and scene parameters throw an exception with the filename and,
for schema errors, the XML line number. The application prints the error and
exits with a nonzero status.

Run from the project directory or the build directory. CMake copies all three scene XML files
into `build/scene` and refreshes it on subsequent builds after edits.
For Visual Studio configurations, use the build directory as the working
directory rather than its Debug/Release subdirectory.

TinyXML-2 11.0.0 is vendored in `third_party/tinyxml2` under the zlib license.
Upstream: https://github.com/leethomason/tinyxml2/tree/11.0.0

Build and run the scene loader checks:

    cmake --build build
    ctest --test-dir build --output-on-failure
