// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2025 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef DY_SOLVER_CONSTRAINT_1D_H
#define DY_SOLVER_CONSTRAINT_1D_H

#include "foundation/PxVec3.h"
#include "PxvConfig.h"
#include "DySolverConstraintTypes.h"
#include "DySolverBody.h"
#include "PxConstraintDesc.h"
#include "DySolverConstraintDesc.h"
#include "DyCpuGpu1dConstraint.h"

namespace physx
{

namespace Dy
{

// dsequeira: we should probably fork these structures for constraints and extended constraints,
// since there's a few things that are used for one but not the other

struct SolverConstraint1DHeader
{
	PxU8	type;			// enum SolverConstraintType - must be first byte
	PxU8	count;			// count of following 1D constraints
	PxU8	dominance;
	PxU8	breakable;		// indicate whether this constraint is breakable or not						

	PxReal	linBreakImpulse;
	PxReal	angBreakImpulse;
	PxReal	invMass0D0;
	PxVec3	body0WorldOffset;
	PxReal	invMass1D1;
	PxReal	linearInvMassScale0;		// only used by articulations
	PxReal	angularInvMassScale0;		// only used by articulations
	PxReal	linearInvMassScale1;		// only used by articulations
	PxReal	angularInvMassScale1;		// only used by articulations
};

PX_COMPILE_TIME_ASSERT(sizeof(SolverConstraint1DHeader) == 48);

PX_ALIGN_PREFIX(16)
struct SolverConstraint1D 
{
public:
	PxVec3		lin0;					//!< linear velocity projection (body 0)	
	PxReal		constant;				//!< constraint constant term

	PxVec3		lin1;					//!< linear velocity projection (body 1)
	PxReal		unbiasedConstant;		//!< constraint constant term without bias

	PxVec3		ang0;					//!< angular velocity projection (body 0)
	PxReal		velMultiplier;			//!< constraint velocity multiplier

	PxVec3		ang1;					//!< angular velocity projection (body 1)
	PxReal		impulseMultiplier;		//!< constraint impulse multiplier

	PxVec3		ang0Writeback;			//!< unscaled angular velocity projection (body 0)
	PxReal		residualVelIter;

	PxReal		minImpulse;				//!< Lower bound on impulse magnitude	 
	PxReal		maxImpulse;				//!< Upper bound on impulse magnitude
	PxReal		appliedForce;			//!< applied force to correct velocity+bias
private:
	union 
	{		
		PxU32		flags; //Use only the most significant bit (which corresponds to the float's sign bit)
		PxReal		residualPosIter;
	};
public:	

	void setSolverConstants(const Constraint1dSolverConstantsPGS& solverConstants)
	{
		constant = solverConstants.constant;
		unbiasedConstant = solverConstants.unbiasedConstant;
		velMultiplier = solverConstants.velMultiplier;
		impulseMultiplier = solverConstants.impulseMultiplier;
	}

	PX_FORCE_INLINE PxU32 setBit(PxU32 value, PxU32 bitLocation, bool bitState)
	{
		if (bitState)
			return value | (1 << bitLocation);
		else
			return value & (~(1 << bitLocation));
	}

	PX_FORCE_INLINE void setOutputForceFlag(bool outputForce)
	{
		flags = setBit(flags, 31, outputForce);
	}
	
	PX_FORCE_INLINE bool getOutputForceFlag() const
	{
		return flags & 0x80000000;
	}

	PX_FORCE_INLINE void setPositionIterationResidual(PxReal residual)
	{
		bool flag = getOutputForceFlag();
		residualPosIter = residual;
		setOutputForceFlag(flag);
	}
	
	PX_FORCE_INLINE PxReal getPositionIterationResidual() const
	{
		return PxAbs(residualPosIter);
	}

	PX_FORCE_INLINE void setVelocityIterationResidual(PxReal r)
	{
		residualVelIter = r;
	}

} PX_ALIGN_SUFFIX(16); 	

PX_COMPILE_TIME_ASSERT(sizeof(SolverConstraint1D) == 96);


struct SolverConstraint1DExt : public SolverConstraint1D
{
public:
	Cm::SpatialVectorV deltaVA;
	Cm::SpatialVectorV deltaVB;
};

//PX_COMPILE_TIME_ASSERT(sizeof(SolverConstraint1DExt) == 160);


PX_FORCE_INLINE void init(SolverConstraint1DHeader& h, 
						  PxU8 count, 
						  bool isExtended,
						  const PxConstraintInvMassScale& ims)
{
	h.type			= PxU8(isExtended ? DY_SC_TYPE_EXT_1D : DY_SC_TYPE_RB_1D);
	h.count			= count;
	h.dominance		= 0;
	h.linearInvMassScale0	= ims.linear0;
	h.angularInvMassScale0	= ims.angular0;
	h.linearInvMassScale1	= -ims.linear1;
	h.angularInvMassScale1	= -ims.angular1;
}

PX_FORCE_INLINE void init(SolverConstraint1D& c,
						  const PxVec3& _linear0, const PxVec3& _linear1, 
						  const PxVec3& _angular0, const PxVec3& _angular1,
						  PxReal _minImpulse, PxReal _maxImpulse)
{
	PX_ASSERT(_linear0.isFinite());
	PX_ASSERT(_linear1.isFinite());
	c.lin0					= _linear0;
	c.lin1					= _linear1;
	c.ang0					= _angular0;
	c.ang1					= _angular1;
	c.minImpulse			= _minImpulse;
	c.maxImpulse			= _maxImpulse;
	c.setOutputForceFlag(false);
	c.appliedForce			= 0;
	c.residualVelIter		= 0;
	c.setPositionIterationResidual(0.0f);
}

PX_FORCE_INLINE bool needsNormalVel(const Px1DConstraint &c)
{
	return c.flags & Px1DConstraintFlag::eRESTITUTION
		|| (c.flags & Px1DConstraintFlag::eSPRING && c.flags & Px1DConstraintFlag::eACCELERATION_SPRING);
}

}
}

#endif
