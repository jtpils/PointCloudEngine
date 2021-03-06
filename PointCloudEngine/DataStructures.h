#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    struct Color16
    {
        // 6 bits: red, 6 bits green, 4 bits blue
        unsigned short data;

        Color16()
        {
            data = 0;
        }

        Color16(byte red, byte green, byte blue)
        {
            byte r = round(63 * (red / 255.0f));
            byte g = round(63 * (green / 255.0f));
            byte b = round(15 * (blue / 255.0f));

            data = r << 10;
            data = data | g << 4;
            data = data | b;
        }
    };

    struct PolarNormal
    {
        // Compact representation of a normal with polar coordinates using inclination theta and azimuth phi
        // When theta=0 and phi=0 this represents an empty normal (0, 0, 0)
        // [0, pi] therefore 0=0, 255=pi
        byte theta;
        // [-pi, pi] therefore 0=-pi, 255=pi
        byte phi;

        PolarNormal()
        {
            theta = 0;
            phi = 0;
        }

        PolarNormal(Vector3 normal)
        {
            normal.Normalize();

            theta = 255.0f * (acos(normal.z) / XM_PI);
            phi = 127.5f + 127.5f * (atan2f(normal.y, normal.x) / XM_PI);

            if (theta == 0 && phi == 0)
            {
                // Set phi to another value than 0 to avoid representing the empty normal (0, 0, 0)
                // This is okay since phi has no inpact on the saved normal anyways
                phi = 128;
            }
        }

        Vector3 ToVector3()
        {
            if (theta == 0 && phi == 0)
            {
                return Vector3(0, 0, 0);
            }

            float t = XM_PI * (theta / 255.0f);
            float p = XM_PI * ((phi / 127.5f) - 1.0f);

            return Vector3(sin(t) * cos(p), sin(t) * sin(p), cos(t));
        }
    };

    struct Vertex
    {
        // Stores the .ply file vertices
        Vector3 position;
        Vector3 normal;
        byte color[3];
    };

    struct OctreeNodeVertex
    {
        // Bounding volume cube position
        Vector3 position;

        // The different cluster mean normals and colors in object space calculated by k-means algorithm with k=6
        // Each weights is the percentage of points assigned to this cluster (0=0%, 255=100%)
        // TODO: One weight can be omitted because the sum of all weights is always 1
        PolarNormal normals[6];
        Color16 colors[6];
        byte weights[6];

        // Width of the whole cube
        float size;
    };
}

#endif