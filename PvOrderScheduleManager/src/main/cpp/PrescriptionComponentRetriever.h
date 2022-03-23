#pragma once

#include <list>

// Forward Declarations
struct IPhase;
struct IComponent;

class CPrescriptionComponentRetriever
{
public:
	static void GetIncludedPrescriptionComponents(IPhase& phase, std::list<IComponent*>& prescriptionComponents);
};