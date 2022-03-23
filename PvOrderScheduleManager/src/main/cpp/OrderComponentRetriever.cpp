#include "StdAfx.h"

#include "OrderComponentRetriever.h"

#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>

void COrderComponentRetriever::GetIncludedOrderComponents(IPhase& phase, std::list<IComponent*>& orderComponents)
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

				if (sComponentTypeMeaning == "ORDER CREATE")
				{
					orderComponents.push_back(pIComponent);
				}
			}
		}

		VariantClear(&vComponent);
	}
}

void COrderComponentRetriever::GetIncludedTimeZeroLinkedOrderComponents(IPhase& phase,
		std::list<IComponent*>& timeZeroLinkedOrderComponents)
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

				if (sComponentTypeMeaning == "ORDER CREATE")
				{
					const bool bTimeZeroActive = pIComponent->GetTZActiveInd() == TRUE;

					if (bTimeZeroActive)
					{
						timeZeroLinkedOrderComponents.push_back(pIComponent);
					}
				}
			}
		}

		VariantClear(&vComponent);
	}
}