#pragma once
#include "PhysicsStateInternalInterface.h"
#include <bitset>

using maksType = std::uint16_t;

enum class EPhysicsJob : uint8_t
{
	APPLY_FORCE = 0,
	APPLY_IMPULSE,
	SET_VELOCITY,
	ADD_VELOCITY,
	SET_ANGULARVELOCITY,
	ADD_ANGULARVELOCITY,
};

 struct alignas(16) FPhysicsJob
{
private:
	std::bitset<16> JobTypeMask;
protected:
	inline uint16_t GetRaw() { return JobTypeMask.to_ulong(); }
	inline uint16_t Count() { return JobTypeMask.count(); }

	inline void ResetAll() { JobTypeMask.reset(); }
	inline void Reset(EPhysicsJob JobType) { if(Check(JobType)) JobTypeMask.flip((maksType)JobType); }

	inline void Flip() { JobTypeMask.flip(); }
	inline void Flip(EPhysicsJob JobType) { JobTypeMask.flip((maksType)JobType); }

	inline void Set(EPhysicsJob JobType) { JobTypeMask.set((maksType)JobType); }
	inline bool Check(EPhysicsJob JobType) { return JobTypeMask.test((maksType)JobType); }

public:
	virtual void Execute(IPhysicsStateInternal* PhysicsObj) = 0;
};

struct FJobApplyForce : public FPhysicsJob
{
private:
	Vector3 Force;
	Vector3 WorldPosition;
	bool bToCOM : 1;

public:
	
	explicit  FJobApplyForce(Vector3 InForece, Vector3 InWorldPosition) : 
		Force(InForece), WorldPosition(WorldPosition), bToCOM(false)
	{
		Set(EPhysicsJob::APPLY_FORCE);
	};
	explicit  FJobApplyForce(Vector3 InForece) : 
		Force(InForece), WorldPosition(Vector3::Zero()), bToCOM(true)
	{
		Set(EPhysicsJob::APPLY_FORCE);
	};

	void Execute(IPhysicsStateInternal* PhysicsObj)
	{
		if (bToCOM)
		{
			PhysicsObj->P_ApplyForce(Force);
		}
		else
		{
			PhysicsObj->P_ApplyForce(Force, WorldPosition);
		}
		
	}
};

struct FJobApplyImpulse : public FPhysicsJob
{
private:
	Vector3 Force;
	Vector3 WorldPosition;
	bool bToCOM : 1;

public:

	explicit  FJobApplyImpulse(Vector3 InForece, Vector3 InWorldPosition) :
		Force(InForece), WorldPosition(WorldPosition), bToCOM(false)
	{
		Set(EPhysicsJob::APPLY_IMPULSE);
	};
	explicit  FJobApplyImpulse(Vector3 InForece) :
		Force(InForece), WorldPosition(Vector3::Zero()), bToCOM(true)
	{
		Set(EPhysicsJob::APPLY_IMPULSE);
	};

	void Execute(IPhysicsStateInternal* PhysicsObj)
	{
		if (bToCOM)
		{
			PhysicsObj->P_ApplyImpulse(Force);
		}
		else
		{
			PhysicsObj->P_ApplyImpulse(Force, WorldPosition);
		}

	}
};

struct FJobSetVelocity : public FPhysicsJob
{
private:
	Vector3 Velocity;

public:
	explicit  FJobSetVelocity(Vector3 InVelocity) 
	{
		Set(EPhysicsJob::SET_VELOCITY);
	};

	void Execute(IPhysicsStateInternal* PhysicsObj)
	{
		PhysicsObj->P_SetVelocity(Velocity);
	}
};

struct FJobAddVelocity : public FPhysicsJob
{
private:
	Vector3 Velocity;

public:
	explicit  FJobAddVelocity(Vector3 InVelocity)
	{
		Set(EPhysicsJob::ADD_VELOCITY);
	};

	void Execute(IPhysicsStateInternal* PhysicsObj)
	{
		PhysicsObj->P_AddVelocity(Velocity);
	}
};

struct FJobSetAngularVelocity : public FPhysicsJob
{
private:
	Vector3 AngularVelocity;

public:
	explicit  FJobSetAngularVelocity(Vector3 InAngularVelocity)
	{
		Set(EPhysicsJob::SET_ANGULARVELOCITY);
	};

	void Execute(IPhysicsStateInternal* PhysicsObj)
	{
		PhysicsObj->P_SetAngularVelocity(AngularVelocity);
	}
};

struct FJobAddAngularVelocity : public FPhysicsJob
{
private:
	Vector3 AngularVelocity;

public:
	explicit  FJobAddAngularVelocity(Vector3 InAngularVelocity)
	{
		Set(EPhysicsJob::ADD_ANGULARVELOCITY);
	};

	void Execute(IPhysicsStateInternal* PhysicsObj)
	{
		PhysicsObj->P_AddAngularVelocity(AngularVelocity);
	}
};