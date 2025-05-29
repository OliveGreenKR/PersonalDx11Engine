#include "ResourceInterface.h"

inline std::string IResource::GetTypeString() const
{

    EResourceType Type = GetType();
    std::string TytpeString = "NONE";
    switch (Type)
    {
        case EResourceType::Shader:
            TytpeString = "Shader";
            break;
        case EResourceType::Texture:
            TytpeString = "Texture";
            break;
        case EResourceType::Material:
            TytpeString = "Material";
            break;
        case EResourceType::Model:
            TytpeString = "Model";
            break;
    }
}
