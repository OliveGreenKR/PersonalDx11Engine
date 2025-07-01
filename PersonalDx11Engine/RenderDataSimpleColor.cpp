#include "RenderDataSimpleColor.h"
#include "Debug.h"

void FRenderDataSimpleColor::AddVSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    if (!Buffer)
    {
        LOG_WARNING("Wrong ConstantBuffer");
        return;
    }
    //null도 전달
    VSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

void FRenderDataSimpleColor::GetVSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const
{
    //Out 초기화
    OutSlot = -1;
    OutDataSize = 0;
    OutBuffer = nullptr;
    OutData = nullptr;

    if (Index >= VSConstantBuffers.size())
        return;


    OutSlot = VSConstantBuffers[Index].Slot;
    OutBuffer = VSConstantBuffers[Index].Buffer;
    OutData = VSConstantBuffers[Index].Data;
    OutDataSize = VSConstantBuffers[Index].DataSize;
    return;
}
