#pragma once

#include <list>

// Forward Declarations
struct IPhase;

class CSubPhaseRetriever
{
public:
	static void GetIncludedSubPhases(IPhase& phase, std::list<IPhase*>& subPhases);
};