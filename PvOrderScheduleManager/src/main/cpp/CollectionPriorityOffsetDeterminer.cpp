#include "StdAfx.h"

#include "CollectionPriorityOffsetDeterminer.h"

#include <pvorderobj.h>
#include <PriorityDefs.h>
#include <pvorderdataexp.h>

CString CCollectionPriorityOffsetDeterminer::DetermineCollectionPriorityOffset(PvOrderObj& orderObj)
{
	const double dCatalogTypeCd = orderObj.GetCatalogTypeCd();

	if (CDF::CatalogType::IsLaboratory(dCatalogTypeCd))
	{
		const double dCollectionPriorityCd = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailCollPriority);

		if (dCollectionPriorityCd > 0.0)
		{
			SCollectionPriorities collectionPriority;

			if (GetPriorities(dCollectionPriorityCd, collectionPriority))
			{
				return collectionPriority.defaultStartDtTm;
			}
		}
	}

	return "";
}