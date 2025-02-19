#pragma once
#include "ActorComponent.h"
#include "Transform.h"

class USceneComponent : public UActorComponent
{
protected:
    FTransform ComponentTransform;

public:
    virtual const FTransform* GetTransform() const { return &ComponentTransform; }
};
