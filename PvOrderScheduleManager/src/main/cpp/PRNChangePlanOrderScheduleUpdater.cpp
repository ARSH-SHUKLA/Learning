#include "StdAfx.h"

#include "PRNChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"
#include "PlannedComponentDeterminer.h"
#include "ConstantIndicatorHelper.h"

#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>

CPRNChangePlanOrderScheduleUpdater::CPRNChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPRNChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnPRNChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its PRN indicator is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose PRN indicator changed.
/// \param[in]	PvOrderObj& orderObj - The order whose PRN indicator changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPRNChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnPRNChange(IComponent& component, PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnPRNChange(component, orderObj);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnPRNChange(component, orderObj);
	}
}

bool CPRNChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnPRNChange(IComponent& component,
		PvOrderObj& orderObj)
{
	const short nPRNInd = (short)orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailSchPrn);

	if (nPRNInd == 1)
	{
		if (!orderObj.IsIV())
		{
			PvOrderFld* pConstantFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailConstantInd);

			if (NULL != pConstantFld)
			{
				pConstantFld->AddOeFieldValue(0, "");
				pConstantFld->SetValuedSourceInd(eVSUser);

				CConstantIndicatorHelper::UpdateConstantIndicatorAcceptFlag(orderObj);
			}
		}
	}

	return true;
}

bool CPRNChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnPRNChange(IComponent& component,
		PvOrderObj& orderObj)
{
	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);
	const double dFmtActionCd = orderObj.GetFmtActionCd();

	std::list<PvOrderObj*> orders;

	if (NULL != pOrderProtocolObj)
	{
		std::list<PvOrderObj*> dotOrders;
		CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, *pOrderProtocolObj, dotOrders);

		for (auto dotIter = dotOrders.cbegin(); dotIter != dotOrders.cend(); dotIter++)
		{
			PvOrderObj* pDoTOrderObj = *dotIter;
			const double dDoTFmtActionCd = pDoTOrderObj->GetFmtActionCd();

			if (dDoTFmtActionCd == dFmtActionCd)
			{
				orders.push_back(pDoTOrderObj);
			}
		}
	}
	else
	{
		orders.push_back(&orderObj);
	}

	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				  CalculateNewOrderScheduleRequest::ePRNIndicatorChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::ePRNIndicatorChanged);
	}

	if (NULL != pOrderProtocolObj)
	{
		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(*pOrderProtocolObj);
	}

	return bResult;
}