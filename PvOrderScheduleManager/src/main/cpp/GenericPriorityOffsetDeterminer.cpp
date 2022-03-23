#include "StdAfx.h"

#include "GenericPriorityOffsetDeterminer.h"

#include <pvorderobj.h>
#include <PriorityDefs.h>
#include <pvorderdataexp.h>

CString CGenericPriorityOffsetDeterminer::DetermineGenericPriorityOffset(PvOrderObj& orderObj)
{
	const double dCatalogTypeCd = orderObj.GetCatalogTypeCd();

	if (!CDF::CatalogType::IsLaboratory(dCatalogTypeCd) && !CDF::CatalogType::IsPharmacy(dCatalogTypeCd))
	{
		const double dGenericPriorityCd = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailPriority);

		if (dGenericPriorityCd > 0.0)
		{
			SGenericPriorities	genericPriority;

			if (GetGenPriority(dGenericPriorityCd, orderObj.GetOeFormatId(), genericPriority))
			{
				return genericPriority.defaultStartDtTm;
			}
		}
	}

	return "";
}