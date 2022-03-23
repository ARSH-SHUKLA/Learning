#include "StdAfx.h"

#include "MostNegativeTimeZeroOffsetDeterminer.h"
#include "IncludedComponentRetriever.h"
#include "TimeZeroHelper.h"

#include <CPS_ImportPVCareCoordCom.h>

bool CMostNegativeTimeZeroOffsetDeterminer::DoesComponentHaveMostNegativeTimeZeroOffset(IComponent& component) const
{
	IPhase* pIPhase = PowerPlanUtil::GetPhase(&component);

	if (NULL == pIPhase)
	{
		return false;
	}

	std::list<IComponent*> components;
	CIncludedComponentRetriever::GetIncludedComponents(*pIPhase, components);

	const double dComponentId = component.GetActCompId();
	const double dOffsetInMinutes = CTimeZeroHelper::GetTimeZeroOffsetInMinutes(component);

	for (auto componentIter = components.cbegin(); componentIter != components.cend(); componentIter++)
	{
		IComponent* pComponent = *componentIter;
		const double dTempComponentId = pComponent->GetActCompId();

		if (dTempComponentId == dComponentId)
		{
			continue;
		}

		const double dTempOffsetInMinutes = CTimeZeroHelper::GetTimeZeroOffsetInMinutes(*pComponent);

		if (dTempOffsetInMinutes <= dOffsetInMinutes)
		{
			return false;
		}
	}

	return true;
}