#pragma once
#include "Scene.h"
#include <vector>

// Backend-neutral upload data. All positions and normals are in world space.
struct GpuVertex
{
    Vector3f position;
    Vector3f normal;
    Color albedo;
};

enum class GpuLightType { Directional = 0, Point = 1, Spot = 2 };

struct GpuLight
{
    GpuLightType type = GpuLightType::Point;
    Vector3f position = Vector3f(0);
    Vector3f direction = Vector3f(0, -1, 0);
    Color color = Color(0);
    Vector3f attenuations = Vector3f(0, 0, 1);
    float cosInner = 1.0f;
    float cosOuter = 0.0f;
};

struct GpuSceneData
{
    std::vector<GpuVertex> vertices;
    std::vector<GpuLight> lights;
    Vector3f boundsMin = Vector3f(-1);
    Vector3f boundsMax = Vector3f(1);
};

// No OpenGL dependency: conversion can be tested without a window or GPU.
GpuSceneData BuildGpuScene(const Scene& scene, int sphereSlices = 64, int sphereStacks = 32,
    int diskSegments = 64);
