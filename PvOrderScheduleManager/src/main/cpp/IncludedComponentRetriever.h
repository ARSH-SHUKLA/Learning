#pragma once

#include <list>

// Forward Declarations
struct IPhase;
struct IComponent;

class CIncludedComponentRetriever
{
public:
	static void GetIncludedComponents(IPhase& phase, std::list<IComponent*>& components);
};