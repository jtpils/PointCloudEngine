#include "PointCloudLODRenderer.h"

std::vector<PointCloudLODRenderer*> PointCloudLODRenderer::sharedPointCloudLODRenderers;

PointCloudLODRenderer* PointCloudLODRenderer::CreateShared(std::wstring plyfile)
{
    PointCloudLODRenderer *pointCloudLODRenderer = new PointCloudLODRenderer(plyfile);
    pointCloudLODRenderer->shared = true;

    sharedPointCloudLODRenderers.push_back(pointCloudLODRenderer);
    return pointCloudLODRenderer;
}

void PointCloudLODRenderer::ReleaseAllSharedPointCloudLODRenderers()
{
    for (auto it = sharedPointCloudLODRenderers.begin(); it != sharedPointCloudLODRenderers.end(); it++)
    {
        PointCloudLODRenderer *pointCloudLODRenderer = *it;
        pointCloudLODRenderer->Release();
        delete pointCloudLODRenderer;
    }

    sharedPointCloudLODRenderers.clear();
}

PointCloudLODRenderer::PointCloudLODRenderer(std::wstring plyfile)
{
    std::vector<PointCloudVertex> vertices = LoadPlyFile(plyfile);

    // Create the octree
    octree = new Octree(vertices);

    // Text for showing properties
    SceneObject *pointCloudLODText = Hierarchy::Create(L"PointCloudLODText");
    text = pointCloudLODText->AddComponent(new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false));

    pointCloudLODText->transform->position = Vector3(-1, -0.95, 0);
    pointCloudLODText->transform->scale = 0.3f * Vector3::One;
}

PointCloudEngine::PointCloudLODRenderer::~PointCloudLODRenderer()
{
    SafeDelete(octree);
}

void PointCloudLODRenderer::Initialize(SceneObject *sceneObject)
{
    // Create the constant buffer for WVP
    D3D11_BUFFER_DESC cbDescWVP;
    ZeroMemory(&cbDescWVP, sizeof(cbDescWVP));
    cbDescWVP.Usage = D3D11_USAGE_DEFAULT;
    cbDescWVP.ByteWidth = sizeof(PointCloudLODConstantBuffer);
    cbDescWVP.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDescWVP.CPUAccessFlags = 0;
    cbDescWVP.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&cbDescWVP, NULL, &pointCloudLODConstantBuffer);
    ErrorMessage(L"CreateBuffer failed for the constant buffer matrices.", L"Initialize", __FILEW__, __LINE__, hr);
}

void PointCloudLODRenderer::Update(SceneObject *sceneObject)
{
    // Handle input
    if (Input::GetKeyDown(Keyboard::Left) && level > 0)
    {
        level--;
    }
    else if (Input::GetKeyDown(Keyboard::Right) && octreeVertices.size() > 0)
    {
        level++;
    }

    // Create new buffer from the current octree traversal
    octreeVertices = octree->GetOctreeVerticesAtLevel(level);

    // Set the text
    text->text = L"Octree Level: " + std::to_wstring(level) + L", Bounding Cubes: " + std::to_wstring(octreeVertices.size());
}

void PointCloudLODRenderer::Draw(SceneObject *sceneObject)
{
    int octreeVerticesSize = octreeVertices.size();

    if (octreeVerticesSize > 0)
    {
        // The vertex buffer should be recreated once the octree vertex count is larger than the current buffer
        // It might be good to recreate it as well when the octree vertex count is much smaller to save gpu memory
        if (octreeVerticesSize > vertexBufferSize)
        {
            // Release vertex buffer
            SafeRelease(vertexBuffer);

            // Create a vertex buffer description with dynamic write access
            D3D11_BUFFER_DESC vertexBufferDesc;
            ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
            vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            vertexBufferDesc.ByteWidth = sizeof(OctreeVertex) * octreeVerticesSize;
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
            D3D11_SUBRESOURCE_DATA vertexBufferData;
            ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
            vertexBufferData.pSysMem = &octreeVertices[0];

            // Create the buffer
            hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);
            ErrorMessage(L"CreateBuffer failed for the vertex buffer.", L"Initialize", __FILEW__, __LINE__, hr);

            vertexBufferSize = octreeVerticesSize;
        }
        else
        {
            // Just update the dynamic buffer
            D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
            ZeroMemory(&mappedVertexBuffer, sizeof(D3D11_MAPPED_SUBRESOURCE));

            // Disable GPU access to the vertex buffer data
            d3d11DevCon->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);

            // Update vertex buffer data
            memcpy(mappedVertexBuffer.pData, &octreeVertices[0], octreeVerticesSize * sizeof(OctreeVertex));

            // Reenable GPU access
            d3d11DevCon->Unmap(vertexBuffer, 0);
        }

        // Set the shaders
        d3d11DevCon->VSSetShader(pointCloudLODShader->vertexShader, 0, 0);
        d3d11DevCon->GSSetShader(pointCloudLODShader->geometryShader, 0, 0);
        d3d11DevCon->PSSetShader(pointCloudLODShader->pixelShader, 0, 0);

        // Set the Input (Vertex) Layout
        d3d11DevCon->IASetInputLayout(pointCloudLODShader->inputLayout);

        // Bind the vertex buffer and index buffer to the input assembler (IA)
        UINT offset = 0;
        UINT stride = sizeof(OctreeVertex);
        d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

        // Set primitive topology
        d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

        // Set shader constant buffer variables
        pointCloudLODConstantBufferData.World = sceneObject->transform->worldMatrix.Transpose();
        pointCloudLODConstantBufferData.WorldInverseTranspose = pointCloudLODConstantBufferData.World.Invert().Transpose();
        pointCloudLODConstantBufferData.View = camera.view.Transpose();
        pointCloudLODConstantBufferData.Projection = camera.projection.Transpose();

        // Update effect file buffer, set shader buffer to our created buffer
        d3d11DevCon->UpdateSubresource(pointCloudLODConstantBuffer, 0, NULL, &pointCloudLODConstantBufferData, 0, 0);
        d3d11DevCon->VSSetConstantBuffers(0, 1, &pointCloudLODConstantBuffer);
        d3d11DevCon->GSSetConstantBuffers(0, 1, &pointCloudLODConstantBuffer);

        d3d11DevCon->Draw(octreeVerticesSize, 0);
    }
}

void PointCloudLODRenderer::Release()
{
    SafeRelease(vertexBuffer);
    SafeRelease(pointCloudLODConstantBuffer);
}
