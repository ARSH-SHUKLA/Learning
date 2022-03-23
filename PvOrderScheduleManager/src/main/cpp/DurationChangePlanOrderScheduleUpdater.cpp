#include "StdAfx.h"

#include "DurationChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"
#include "PlannedComponentDeterminer.h"
#include "OrderDurationHelper.h"
#include "ConstantIndicatorHelper.h"
#include "OrderDetailHelper.h"
#include "UpdateStopTypeFieldHelper.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>

CDurationChangePlanOrderScheduleUpdater::CDurationChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CDurationChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnDurationChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its duration is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose duration changed.
/// \param[in]	PvOrderObj& orderObj - The order whose duration changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CDurationChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnDurationChange(IComponent& component,
		PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnDurationChange(component, orderObj);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnDurationChange(component, orderObj);
	}
}

bool CDurationChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnDurationChange(IComponent& component,
		PvOrderObj& orderObj)
{
	const bool bIsDurationInDoses = COrderDurationHelper::IsDurationInDoses(orderObj);

	if (bIsDurationInDoses)
	{
		if (!orderObj.IsIV())
		{
			COrderDetailHelper::SetAcceptFlag(orderObj, eDetailOrdFrequency, eOeAcceptRequired);

			PvOrderFld* pConstantFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailConstantInd);

			if (NULL != pConstantFld)
			{
				pConstantFld->AddOeFieldValue(0, "");
				pConstantFld->SetValuedSourceInd(eVSUser);

				CConstantIndicatorHelper::UpdateConstantIndicatorAcceptFlag(orderObj);
			}
		}
	}
	else if (!orderObj.IsIntermittent())
	{
		COrderDetailHelper::ResetAcceptFlag(orderObj, eDetailOrdFrequency);
	}

	if (component.IsDoTComponent() == FALSE)
	{
		CUpdateStopTypeFieldHelper::UpdateStopTypeFldToPhysicianStop(orderObj);
	}

	return true;
}

bool CDurationChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnDurationChange(IComponent& component,
		PvOrderObj& orderObj)
{
	component.PutLinkToPhase(FALSE);

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);

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
				  CalculateNewOrderScheduleRequest::eDurationChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::eDurationChanged);
	}
	else if (CDF::OrderAction::IsRenew(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateRenewOrderSchedule(orders,
				  CalculateRenewOrderScheduleRequest::eDurationChanged);
	}

	if (NULL != pOrderProtocolObj)
	{
		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(*pOrderProtocolObj);
	}

	return bResult;
}