#ifndef SCENE_H
#define SCENE_H

#pragma once
#include "PointCloudEngine.h"

class Scene
{
public:
    void Initialize();
    void Update(Timer &timer);
    void Draw();
    void Release();

private:
    SceneObject *fpsText;
    SceneObject *debugText;

    float cameraPitch, cameraYaw, cameraDistance;
    Vector2 input;
};

#endif