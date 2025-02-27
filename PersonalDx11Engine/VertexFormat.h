#pragma once
#include <cstdint>
#include <memory>
#include "Math.h"
#include "D3D.h"

// 기본 정점 형식 인터페이스 (추상 클래스)
class IVertexFormat
{
public:
    virtual ~IVertexFormat() = default;

    // 정점 포맷에 대한 정보 제공
    virtual uint32_t GetStride() const = 0;
    virtual class D3D11_INPUT_ELEMENT_DESC* GetInputLayoutDesc() const = 0;
    virtual uint32_t GetInputLayoutElementCount() const = 0;

    // 다형성 복제 패턴
    virtual std::unique_ptr<IVertexFormat> Clone() const = 0;
};


// 템플릿 기반 정점 형식 구현 클래스
template<typename T>
class TVertexFormat : public IVertexFormat
{
public:
    using VertexType = T;

    TVertexFormat() = default;
    virtual ~TVertexFormat() = default;

    uint32_t GetStride() const override { return sizeof(T); }

    // 구체클래스 구현 메서드
    D3D11_INPUT_ELEMENT_DESC* GetInputLayoutDesc() const override = 0;
    uint32_t GetInputLayoutElementCount() const override = 0;

    virtual std::unique_ptr<IVertexFormat> Clone() const override = 0;
};


// 위치만 있는 기본 정점 형식
struct FVertexPosition
{
    Vector3 Position;
};

class FVertexFormatPosition : public TVertexFormat<FVertexPosition>
{
public:
    D3D11_INPUT_ELEMENT_DESC* GetInputLayoutDesc() const override {
        static D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        return layout;
    }

    uint32_t GetInputLayoutElementCount() const override { return 1; }

    std::unique_ptr<IVertexFormat> Clone() const override {
        return std::make_unique<FVertexFormatPosition>(*this);
    }
};

// 위치와 텍스처 좌표 정점 형식
struct FVertexPositionTexture
{
    Vector3 Position;
    Vector2 TexCoord;
};

class FVertexFormatPositionTexture : public TVertexFormat<FVertexPositionTexture>
{
public:
    D3D11_INPUT_ELEMENT_DESC* GetInputLayoutDesc() const override {
        static D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        return layout;
    }

    uint32_t GetInputLayoutElementCount() const override { return 2; }

    std::unique_ptr<IVertexFormat> Clone() const override {
        return std::make_unique<FVertexFormatPositionTexture>(*this);
    }
};