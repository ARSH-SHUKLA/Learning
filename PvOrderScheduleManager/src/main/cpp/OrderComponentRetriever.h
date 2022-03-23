#pragma once

#include <list>

// Forward Declarations
struct IPhase;
struct IComponent;

class COrderComponentRetriever
{
public:
	static void GetIncludedOrderComponents(IPhase& phase, std::list<IComponent*>& orderComponents);
	static void GetIncludedTimeZeroLinkedOrderComponents(IPhase& phase,
			std::list<IComponent*>& timeZeroLinkedOrderComponents);
};