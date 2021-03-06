#ifndef OCTREENODE_H
#define OCTREENODE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeNode
    {
    public:
        OctreeNode (const std::vector<Vertex> &vertices, const Vector3 &center, const float &size, const int &depth);
        ~OctreeNode();

        std::vector<OctreeNodeVertex> GetVertices(const Vector3 &localCameraPosition, const float &splatSize);
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(const int &level);
        bool IsLeafNode();

        OctreeNode *children[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        OctreeNodeVertex nodeVertex;
    };
}

#endif