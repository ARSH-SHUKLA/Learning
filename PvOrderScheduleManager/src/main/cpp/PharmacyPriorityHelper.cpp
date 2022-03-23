#include "StdAfx.h"

#include "PharmacyPriorityHelper.h"

#include <pvorderobj.h>
#include <pvorderfld.h>
#include <pvcdf4010.h>
#include <pvcdf6000.h>

bool CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(PvOrderObj& orderObj)
{
	const double dCatalogTypeCd = orderObj.GetCatalogTypeCd();

	if (CDF::CatalogType::IsPharmacy(dCatalogTypeCd))
	{
		const double dPharmacyPriorityCd = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailRxPriority);

		if (CDF::PharmOrdPriority::IsStat(dPharmacyPriorityCd))
		{
			return true;
		}

		if (CDF::PharmOrdPriority::IsNow(dPharmacyPriorityCd))
		{
			return true;
		}
	}

	return false;
}

bool CPharmacyPriorityHelper::WasPharmacyPriorityChangedFromStatOrNowToRoutine(PvOrderObj& orderObj)
{
	const double dCatalogTypeCd = orderObj.GetCatalogTypeCd();

	if (CDF::CatalogType::IsPharmacy(dCatalogTypeCd))
	{
		PvOrderFld* pRxPriorityFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailRxPriority);

		if (NULL != pRxPriorityFld)
		{
			const double dPharmacyPriorityCd = pRxPriorityFld->GetLastOeFldValue();

			if (dPharmacyPriorityCd == 0.0 || CDF::PharmOrdPriority::IsRoutine(dPharmacyPriorityCd))
			{
				PvOrderFld::OeFieldValue previousFieldValue;
				const bool bHasPreviousValue = pRxPriorityFld->GetPrevOeFieldValue(previousFieldValue);

				if (bHasPreviousValue)
				{
					const double dPreviousPharmacyPriorityCd = previousFieldValue.oeFieldValue;

					if (CDF::PharmOrdPriority::IsStat(dPreviousPharmacyPriorityCd))
					{
						return true;
					}

					if (CDF::PharmOrdPriority::IsNow(dPreviousPharmacyPriorityCd))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool CPharmacyPriorityHelper::SetPharmacyPriorityToRoutineIfStatOrNow(PvOrderObj& orderObj)
{
	const bool bHasStatOrNowPharmacyPriority = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(
				orderObj);

	if (bHasStatOrNowPharmacyPriority)
	{
		PvOrderFld* pRxPriorityFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailRxPriority);

		if (NULL != pRxPriorityFld)
		{
			pRxPriorityFld->Clear();
			pRxPriorityFld->AddOeFieldValue(CDF::PharmOrdPriority::GetRoutineCd(), CDF::PharmOrdPriority::GetRoutineDisplay());
			return true;
		}
	}

	return false;
}