#pragma once

// Forward Declarations
struct IPhase;

class CPlanPhaseDateTimesModifiableDeterminer
{
public:
	bool ArePhaseDateTimesModifiable(IPhase& phase) const;
};