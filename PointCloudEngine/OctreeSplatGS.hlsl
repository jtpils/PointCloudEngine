#include "OctreeVSPS.hlsl"

// Higher factor reduces the spacing between tilted splats but reduces the color diversity (blend overlapping splats to avoid this)
// 1.0f = Orthogonal splats to the camera are as large as the pixel area they should fill and do not overlap
// 2.0f = Orthogonal splats to the camera are twice as large and overlap with all their surrounding splats
static const float overlapFactor = 1.75f;

[maxvertexcount(16)]
void GS(point VS_OUTPUT input[1], inout TriangleStream<GS_OUTPUT> output)
{
    /*

        1,4__5
        |\   |
        | \  |
        |  \ |
        |___\|
        3    2,6

    */

    float3 position = mul(float4(input[0].position, 1), World);
    float3 normal = normalize(mul(float4(input[0].normal, 0), WorldInverseTranspose));

    float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
    float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
    float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

    // Billboard should face in the same direction as the normal
    float distanceToCamera = distance(cameraPosition, position);
    float splatSizeWorld = overlapFactor * splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;
    float3 up = 0.5f * splatSizeWorld * normalize(cross(normal, cameraRight));
    float3 right = 0.5f * splatSizeWorld * normalize(cross(normal, up));

    float4x4 VP = mul(View, Projection);

    GS_OUTPUT element;
    element.color = input[0].color;
    element.normal = normal;

    element.position = mul(float4(position + up - right, 1), VP);
    output.Append(element);

    element.position = mul(float4(position - up + right, 1), VP);
    output.Append(element);

    element.position = mul(float4(position - up - right, 1), VP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(position + up - right, 1), VP);
    output.Append(element);

    element.position = mul(float4(position + up + right, 1), VP);
    output.Append(element);

    element.position = mul(float4(position - up + right, 1), VP);
    output.Append(element);
}