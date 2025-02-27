#include "Model.h"
#include "D3DShader.h"

bool UModel::InitializeAsCube() {
    auto manager = UModelBufferManager::Get();
    DataHash = manager->GetCubeHash();
    bIsInitialized = (DataHash != 0);
    return bIsInitialized;
}

bool UModel::InitializeAsSphere() {
    auto manager = UModelBufferManager::Get();
    DataHash = manager->GetSphereHash();
    bIsInitialized = (DataHash != 0);
    return bIsInitialized;
}

bool UModel::InitializeAsPlane() {
    auto manager = UModelBufferManager::Get();
    DataHash = manager->GetPlaneHash();
    bIsInitialized = (DataHash != 0);
    return bIsInitialized;
}

FBufferResource* UModel::GetBufferResource() {
    if (!bIsInitialized) return nullptr;
    return UModelBufferManager::Get()->GetBufferByHash(DataHash);
}