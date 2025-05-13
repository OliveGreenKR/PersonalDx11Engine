#include "CameraOrbitControl.h"
#include "Camera.h"
#include "Debug.h"


bool FCameraOrbit::Initialize(UCamera* Camera)
{
    if (!Camera)
        return false;

    Vector3 Target = GetTargetPos(Camera);
    CalculateSphericalCoordinates(Camera, Target);
    return true;
}

void FCameraOrbit::CalculateSphericalCoordinates(UCamera* Camera, Vector3 Target)
{
    // Calculate camera's relative position to the target
    Vector3 CameraPosition = Camera->GetTransform().Position;
    Vector3 RelativePosition = CameraPosition - Target;

    // Calculate spherical coordinates
    // Radius (distance from target)
    Radius = RelativePosition.Length();

    // Handle case where camera is at the target's exact location
    if (Radius < KINDA_SMALLER) {
        LatitudeRad = 0.0f;
        LongitudeRad = 0.0f;
        // Depending on desired behavior, you might want a minimum radius here,
        // but 0 is mathematically correct for the point.
        Radius = 0.0f;
    }
    else {
        // Latitude (angle from XZ plane, in radians). asin range is [-PI/2, PI/2].
        // This corresponds to the angle between the relative vector and the XZ plane.
        LatitudeRad = asinf(RelativePosition.y / Radius);

        // Longitude (angle in XZ plane from positive X towards positive Z, in radians).
        // atan2f(y, x) calculates the angle for point (x, y) from positive X towards positive Y.
        // In our XZ plane context, we use atan2f(Z, X). Range is [-PI, PI].
        if (std::fabs(RelativePosition.x) < KINDA_SMALLER && std::fabs(RelativePosition.z) < KINDA_SMALLER) {
            LongitudeRad = 0.0f; // Default for position directly on the Y axis
        }
        else {
            LongitudeRad = atan2f(RelativePosition.z, RelativePosition.x);
        }
    }
}

void FCameraOrbit::UpdateCameraFromSpherical(UCamera* Camera, Vector3 Target,
                                             float Radius, float LatitudeRad, float LongitudeRad)
{
    // Check for invalid camera
    if (!Camera)
        return;

    // Convert spherical coordinates (radians) back to Cartesian coordinates relative to the target
    // Assuming Y is Up, X is Right, Z is Forward
    // Calculate the projected radius onto the XZ plane based on latitude
    double HorizontalRadius = Radius * cos(LatitudeRad);

    // Calculate the new camera position relative to the target
    Vector3 NewPosition;
    // Using the longitude (angle from +X towards +Z in XZ plane):
    NewPosition.x = Target.x + HorizontalRadius * cos(LongitudeRad); // Right component
    NewPosition.y = Target.y + Radius * sin(LatitudeRad);          // Up component
    NewPosition.z = Target.z + HorizontalRadius * sin(LongitudeRad); // Forward component

    // Set the camera's new world position
    Camera->SetPosition(NewPosition);

    // Make the camera look directly at the target
    Camera->LookAt(Target);
}

Vector3 FCameraOrbit::GetTargetPos(UCamera* Camera)
{
    Vector3 Target = Vector3::Zero();
    if (Camera->bLookAtObject && Camera->GetCurrentLookAt().lock())
    {
        Target = Camera->GetCurrentLookAt().lock()->GetTransform().Position;
    }
    return Target;
}

void FCameraOrbit::OrbitLongitude(UCamera* Camera, float DeltaRad)
{
    // Check for invalid camera
    if (!Camera)
        return;

    Vector3 Target = GetTargetPos(Camera);

    // Update longitude by the delta amount (in radians)
    LongitudeRad += DeltaRad;

    //normalization to [-PI, PI]:
    // const float PI = Math::PI; // Assuming Math::PI exists
     LongitudeRad = fmod(LongitudeRad, 2 * PI);
     if (LongitudeRad <= -PI) 
         LongitudeRad += 2 * PI;
     if (LongitudeRad > PI) 
         LongitudeRad -= 2 * PI;

    // Update camera position and orientation based on the new longitude
    UpdateCameraFromSpherical(Camera, Target, Radius, LatitudeRad, LongitudeRad);
}

 void FCameraOrbit::OrbitLatitude(UCamera* Camera, float DeltaRad)
 {
     // Check for invalid camera
     if (!Camera)
         return;

     Vector3 Target = GetTargetPos(Camera);

     // Update latitude by the delta amount (in radians)
     LatitudeRad += DeltaRad;

     // Clamp latitude to avoid issues at the poles (gimbal lock / flipping).
     // asin gives [-PI/2, PI/2]. Clamping slightly within this range is typical.
     constexpr float PI_HALF = PI / 2.0f; 
     LatitudeRad = std::clamp(LatitudeRad, -PI_HALF + 0.1f, PI_HALF - 0.1f);

     // Log post-update values (for debugging flow)
     LOG_FUNC_CALL("Post Latitude Rad : %.3f", LatitudeRad);

     // Update camera position and orientation based on the new latitude
     UpdateCameraFromSpherical(Camera, Target, Radius, LatitudeRad, LongitudeRad);
 }