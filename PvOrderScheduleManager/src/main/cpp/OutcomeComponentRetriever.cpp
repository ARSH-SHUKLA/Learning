#include "StdAfx.h"

#include "OutcomeComponentRetriever.h"

#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>

void COutcomeComponentRetriever::GetIncludedOutcomeComponents(IPhase& phase, std::list<IComponent*>& outcomeComponents)
{
	IProVideObjectCollectionPtr pCompCollection = phase.GetComponentCollection();

	if (pCompCollection == NULL)
	{
		return;
	}

	long compTotal(0);
	pCompCollection->get_Count(&compTotal);

	for (long compCnt(compTotal); compCnt > 0; --compCnt)
	{
		_variant_t vComponent = pCompCollection->Value(CComVariant(compCnt));
		IComponentPtr pIComponent = vComponent.pdispVal;

		if (NULL != pIComponent)
		{
			if (TRUE == pIComponent->GetIncluded())
			{
				const CString sComponentTypeMeaning = (LPCTSTR)pIComponent->GetComponentTypeMean();

				if (sComponentTypeMeaning == "RESULT OUTCO")
				{
					outcomeComponents.push_back(pIComponent);
				}
			}
		}

		VariantClear(&vComponent);
	}
}