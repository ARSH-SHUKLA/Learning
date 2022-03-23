#pragma once

#include <list>

// Forward Declarations
struct IPhase;
struct IComponent;

class COutcomeComponentRetriever
{
public:
	static void GetIncludedOutcomeComponents(IPhase& phase, std::list<IComponent*>& outcomeComponents);
};