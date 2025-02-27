#include "ModelBufferManager.h"

const FVertexDataContainer* UModelBufferManager::GetVertexDataByHash(size_t hash)
{
    std::lock_guard<std::mutex> lock(Mutex);

    auto it = VertexDataCache.find(hash);
    if (it == VertexDataCache.end()) {
        return nullptr;
    }

    return it->second.get();
}

// ť�� �޽� ���� ����
std::unique_ptr<FVertexDataContainer> UModelBufferManager::CreateCubeMesh()
{
    // ��ġ, �븻, �ؽ�ó ��ǥ�� �ִ� ť�� ����
    struct FVertexPosNormTex {
        Vector3 Position;
        Vector3 Normal;
        Vector2 TexCoord;
    };

    FVertexPosNormTex vertices[] = {
        // �ո� (+Z)
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},

        // �޸� (-Z)
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},

        // ���� (+Y)
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},

        // �Ʒ��� (-Y)
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},

        // �����ʸ� (+X)
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},

        // ���ʸ� (-X)
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}
    };

    uint32_t indices[] = {
        0,  1,  2,  0,  2,  3,  // �ո�
        4,  5,  6,  4,  6,  7,  // �޸�
        8,  9,  10, 8,  10, 11, // ����
        12, 13, 14, 12, 14, 15, // �Ʒ���
        16, 17, 18, 16, 18, 19, // �����ʸ�
        20, 21, 22, 20, 22, 23  // ���ʸ�
    };

    // ���� ���� ����
    class FVertexFormatPosNormTex : public IVertexFormat {
    public:
        uint32_t GetStride() const override { return sizeof(FVertexPosNormTex); }

        D3D11_INPUT_ELEMENT_DESC* GetInputLayoutDesc() const override {
            static D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
            };
            return layout;
        }

        uint32_t GetInputLayoutElementCount() const override { return 3; }

        std::unique_ptr<IVertexFormat> Clone() const override {
            return std::make_unique<FVertexFormatPosNormTex>(*this);
        }
    };

    // ���� ������ �����̳� ����
    return std::make_unique<FVertexDataContainer>(
        vertices, 24, sizeof(FVertexPosNormTex),
        indices, 36,
        std::make_unique<FVertexFormatPosNormTex>()
    );
}

// �� �޽� ���� ����
std::unique_ptr<FVertexDataContainer> UModelBufferManager::CreateSphereMesh(uint32_t slices, uint32_t stacks)
{
    struct FVertexPosNormTex {
        Vector3 Position;
        Vector3 Normal;
        Vector2 TexCoord;
    };

    std::vector<FVertexPosNormTex> vertices;
    std::vector<uint32_t> indices;

    const float radius = 0.5f;

    // ���� ����
    vertices.reserve((slices + 1) * (stacks + 1));

    for (uint32_t stack = 0; stack <= stacks; stack++) {
        float phi = stack * PI / stacks;
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        for (uint32_t slice = 0; slice <= slices; slice++) {
            float theta = slice * 2.0f * PI / slices;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            // ��ġ ���
            float x = cosTheta * sinPhi;
            float y = cosPhi;
            float z = sinTheta * sinPhi;

            // �ؽ�ó ��ǥ
            float u = static_cast<float>(slice) / slices;
            float v = static_cast<float>(stack) / stacks;

            // ���� �߰�
            FVertexPosNormTex vertex;
            vertex.Position = Vector3(x * radius, y * radius, z * radius);
            vertex.Normal = Vector3(x, y, z); // ����ȭ�� ��ġ = �븻
            vertex.TexCoord = Vector2(u, v);

            vertices.push_back(vertex);
        }
    }

    // �ε��� ����
    indices.reserve(slices * stacks * 6);

    for (uint32_t stack = 0; stack < stacks; stack++) {
        for (uint32_t slice = 0; slice < slices; slice++) {
            uint32_t topLeft = stack * (slices + 1) + slice;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (stack + 1) * (slices + 1) + slice;
            uint32_t bottomRight = bottomLeft + 1;

            // �ﰢ�� �� ���� ���� ����
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // ���� ���� ����
    class FVertexFormatPosNormTex : public IVertexFormat {
    public:
        uint32_t GetStride() const override { return sizeof(FVertexPosNormTex); }

        D3D11_INPUT_ELEMENT_DESC* GetInputLayoutDesc() const override {
            static D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
            };
            return layout;
        }

        uint32_t GetInputLayoutElementCount() const override { return 3; }

        std::unique_ptr<IVertexFormat> Clone() const override {
            return std::make_unique<FVertexFormatPosNormTex>(*this);
        }
    };

    // ���� ������ �����̳� ����
    return std::make_unique<FVertexDataContainer>(
        vertices.data(), static_cast<uint32_t>(vertices.size()), sizeof(FVertexPosNormTex),
        indices.data(), static_cast<uint32_t>(indices.size()),
        std::make_unique<FVertexFormatPosNormTex>()
    );
}

// ��� �޽� ���� ����
std::unique_ptr<FVertexDataContainer> UModelBufferManager::CreatePlaneMesh()
{
    struct FVertexPosNormTex {
        Vector3 Position;
        Vector3 Normal;
        Vector2 TexCoord;
    };

    FVertexPosNormTex vertices[] = {
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
    };

    uint32_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    // ���� ���� ����
    class FVertexFormatPosNormTex : public IVertexFormat {
    public:
        uint32_t GetStride() const override { return sizeof(FVertexPosNormTex); }

        D3D11_INPUT_ELEMENT_DESC* GetInputLayoutDesc() const override {
            static D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
            };
            return layout;
        }

        uint32_t GetInputLayoutElementCount() const override { return 3; }

        std::unique_ptr<IVertexFormat> Clone() const override {
            return std::make_unique<FVertexFormatPosNormTex>(*this);
        }
    };

    // ���� ������ �����̳� ����
    return std::make_unique<FVertexDataContainer>(
        vertices, 4, sizeof(FVertexPosNormTex),
        indices, 6,
        std::make_unique<FVertexFormatPosNormTex>()
    );
}