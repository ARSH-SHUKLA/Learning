#include "StdAfx.h"

#include "DoTComponentRetriever.h"
#include "IncludedComponentRetriever.h"

#include <RTZDefs.h>
#include <CPS_ImportPVCareCoordCom.h>

void CDoTComponentRetriever::FindDoTComponentsFromProtocolComponent(IComponent& protocolComponent,
		std::list<IComponent*>& treatmentPeriodComponents)
{
	IPhase* pIProtocolPhase = PowerPlanUtil::GetPhase(&protocolComponent);

	if (NULL != pIProtocolPhase)
	{
		IPlan* pIPlan = PowerPlanUtil::GetPlan(&protocolComponent);

		if (NULL != pIPlan)
		{
			FindDoTComponentsFromProtocolComponent(*pIPlan, *pIProtocolPhase, protocolComponent, treatmentPeriodComponents);
		}
	}
}

void CDoTComponentRetriever::FindDoTComponentsFromProtocolComponent(IPlan& plan, IPhase& protocolPhase,
		IComponent& protocolComponent, std::list<IComponent*>& treatmentPeriodComponents)
{
	TreatmentPeriodVector treatmentPeriodVector;
	protocolPhase.GetTreatmentPeriods((LONG_PTR)&treatmentPeriodVector);

	for (TreatmentPeriodVector::const_iterator treatmentPeriodIter = treatmentPeriodVector.begin();
		 treatmentPeriodIter != treatmentPeriodVector.end(); treatmentPeriodIter++)
	{
		const double dTreatmentPeriodPhaseId = treatmentPeriodIter->second;
		IPhasePtr pITreatmentPeriod = plan.GetPhaseByKey(dTreatmentPeriodPhaseId);

		if (NULL != pITreatmentPeriod)
		{
			IComponent* pITreatmentPeriodComponent = FindDoTComponentFromProtocolComponentAndTreatmentPeriod(protocolComponent,
					*pITreatmentPeriod);

			if (NULL != pITreatmentPeriodComponent)
			{
				treatmentPeriodComponents.push_back(pITreatmentPeriodComponent);
			}
		}
	}
}

IComponent* CDoTComponentRetriever::FindDoTComponentFromProtocolComponentAndTreatmentPeriod(
	IComponent& protocolComponent, IPhase& treatmentPeriodPhase)
{
	const double dProtocolComponentGroupNumber = protocolComponent.GetProtocolComponentGroupNbr();

	std::list<IComponent*> treatmentPeriodComponents;
	CIncludedComponentRetriever::GetIncludedComponents(treatmentPeriodPhase, treatmentPeriodComponents);

	for (auto componentIter = treatmentPeriodComponents.cbegin(); componentIter != treatmentPeriodComponents.cend();
		 componentIter++)
	{
		IComponent* pITreatmentPeriodComponent = *componentIter;

		const double dGroupNumber = pITreatmentPeriodComponent->GetProtocolComponentGroupNbr();

		if (dProtocolComponentGroupNumber == dGroupNumber)
		{
			return pITreatmentPeriodComponent;
		}
	}

	return NULL;
}