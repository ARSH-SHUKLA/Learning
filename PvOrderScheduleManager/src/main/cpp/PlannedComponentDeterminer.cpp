#include "StdAfx.h"

#include "PlannedComponentDeterminer.h"

#include <pvorderobj.h>
#include <CPS_ImportPVCareCoordCom.h>

bool CPlannedComponentDeterminer::IsPlannedOrderComponent(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL != pIComponent)
	{
		if (!PowerPlanUtil::IsOrderPartOfAnIVSequence((LONG_PTR)&orderObj))
		{
			if (pIComponent->IsTaperComponent())
			{
				return false;
			}
			else if (pIComponent->GetInitiated())
			{
				return false;
			}

			return true;
		}
	}

	return false;
}