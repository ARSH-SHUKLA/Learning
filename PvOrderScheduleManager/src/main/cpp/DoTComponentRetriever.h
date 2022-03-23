#pragma once

#include <list>

// Forward Declarations
struct IPlan;
struct IPhase;
struct IComponent;

class CDoTComponentRetriever
{
public:
	static void FindDoTComponentsFromProtocolComponent(IComponent& protocolComponent,
			std::list<IComponent*>& treatmentPeriodComponents);

private:
	static void FindDoTComponentsFromProtocolComponent(IPlan& plan, IPhase& protocolPhase, IComponent& protocolComponent,
			std::list<IComponent*>& treatmentPeriodComponents);
	static IComponent* FindDoTComponentFromProtocolComponentAndTreatmentPeriod(IComponent& protocolComponent,
			IPhase& treatmentPeriodPhase);
};