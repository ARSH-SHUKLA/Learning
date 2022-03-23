#include "StdAfx.h"

#include "PriorityFieldHelper.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>

void CPriorityFieldHelper::DisablePriorityFieldsForIncompatibleOrderTypes(PvOrderObj& orderObj)
{
	const double dCatalogTypeCd = orderObj.GetCatalogTypeCd();

	if (CDF::CatalogType::IsPharmacy(dCatalogTypeCd))
	{
		COrderDetailHelper::SetAcceptFlagsIfFieldIsDisplayed(orderObj, eDetailCollPriority, eOeAcceptDisplayOnly,
				eOeAcceptDisplayOnly);
		COrderDetailHelper::SetAcceptFlagsIfFieldIsDisplayed(orderObj, eDetailPriority, eOeAcceptDisplayOnly,
				eOeAcceptDisplayOnly);
	}
	else if (CDF::CatalogType::IsLaboratory(dCatalogTypeCd))
	{
		COrderDetailHelper::SetAcceptFlagsIfFieldIsDisplayed(orderObj, eDetailRxPriority, eOeAcceptDisplayOnly,
				eOeAcceptDisplayOnly);
		COrderDetailHelper::SetAcceptFlagsIfFieldIsDisplayed(orderObj, eDetailPriority, eOeAcceptDisplayOnly,
				eOeAcceptDisplayOnly);
	}
	else
	{
		COrderDetailHelper::SetAcceptFlagsIfFieldIsDisplayed(orderObj, eDetailRxPriority, eOeAcceptDisplayOnly,
				eOeAcceptDisplayOnly);
		COrderDetailHelper::SetAcceptFlagsIfFieldIsDisplayed(orderObj, eDetailCollPriority, eOeAcceptDisplayOnly,
				eOeAcceptDisplayOnly);
	}
}