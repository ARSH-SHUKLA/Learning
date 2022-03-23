#include "StdAfx.h"

#include "InpatientOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "PlanScheduleManager.h"

#include <pvorderobj.h>
#include <pvorderpcobj.h>
#include <PvOrderProtocolObj.h>
#include <pvordutil.h>
#include <FutureOrderUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvScheduleTransactionAssistantExp.h>
#include <PvResumeScheduleControllerExp.h>
#include <PvResumeScheduleControllerFactoryExp.h>
#include <pvsingletonmgr.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>

CInpatientOrderScheduleUpdater::CInpatientOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnInitialAction(PvOrderObj& orderObj)
{
	const double dFmtActionCd = orderObj.GetFmtActionCd();

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		return UpdateOrderScheduleOnAddOrder(orderObj);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd))
	{
		return UpdateOrderScheduleOnInitialModify(orderObj);
	}
	else if (CDF::OrderAction::IsResume(dFmtActionCd))
	{
		return UpdateOrderScheduleOnInitialResume(orderObj);
	}
	else if (CDF::OrderAction::IsRenew(dFmtActionCd))
	{
		return UpdateOrderScheduleOnInitialRenew(orderObj);
	}
	else if (CDF::OrderAction::IsActivate(dFmtActionCd))
	{
		return UpdateOrderScheduleOnInitialActivate(orderObj);
	}

	return true;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAddOrder(PvOrderObj& orderObj)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (!bIsPlanOrder)
	{
		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				  CalculateNewOrderScheduleRequest::eNewOrderAdded);
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj)
{
	bool bResult = true;

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnInitialModify(orderObj);
	}
	else
	{
		CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::eInitialModify);
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnInitialActivate(PvOrderObj& orderObj)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (!bIsPlanOrder)
	{
		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		bResult = inpatientOrderScheduleServiceCaller.CalculateActivateOrderSchedule(orders,
				  CalculateActivateOrderScheduleRequest::eInitialActivate);
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnInitialRenew(PvOrderObj& orderObj)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	bResult = inpatientOrderScheduleServiceCaller.CalculateRenewOrderSchedule(orders,
			  CalculateRenewOrderScheduleRequest::eInitialRenew);

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnInitialResume(PvOrderObj& orderObj)
{
	bool bResult = false;

	CPvResumeScheduleControllerFactoryExp* pFactory = NULL;
	GetSingleInstance("RESUMESCHEDULEFACTORY_KEY", pFactory, AppCon_GetContext());

	if (NULL != pFactory)
	{
		// We need this pointer simply to sync the resume dt/tm with the schedule object.
		//pResumeController will be empty as we are removing the Resume Schedule Controller object when plan,sign is taken
		//passing boolean false to bypass the triggering action flag (CalculateResumeOrderScheduleRequest::eInitialResume)value before calling script 510904:CalculateResumeActionSchedule
		//the script is already called with CalculateResumeOrderScheduleRequest::eInitialResume triggering action flag when suspende order is resumed

		CPvResumeScheduleControllerExp* pResumeController = pFactory->GetResumeScheduleController(orderObj, false);

		if (NULL != pResumeController)
		{
			pResumeController->ApplyResumeScheduleToOrder();
			bResult = true;
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnRequestedStartChange(PvOrderObj& orderObj)
{
	bool bResult = false;
	bool bIsPlanOrder(false);
	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (pIComponent != nullptr)
	{
		bIsPlanOrder = true;

		if (PowerPlanUtil::IsOrderPartOfAnIVSequence((LONG_PTR)&orderObj))
		{
			bIsPlanOrder = false;
		}

		if (pIComponent->IsTaperComponent())
		{
			bIsPlanOrder = false;
		}
	}


	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnRequestedStartDateTimeChange(orderObj);
	}
	else
	{
		// The Requested Start Dt/Tm may be filled out with only a time so do not call schedule service.
		// If the service is called, it will reset the date time control to current.
		PvOrderFld* pStartDtTmFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

		if (NULL == pStartDtTmFld)
		{
			return false;
		}

		PvOrderFld::OeFieldValue fldVal;
		pStartDtTmFld->GetLastOeFieldValue(fldVal);

		if (fldVal.oeFieldDtTmValue.IsNull() && !fldVal.oeFieldTimeStr.IsEmpty())
		{
			return true;
		}

		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eRequestedStartDateTimeChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::eRequestedStartDateTimeChanged);
		}
		else if (CDF::OrderAction::IsActivate(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateActivateOrderSchedule(orders,
					  CalculateActivateOrderScheduleRequest::eRequestedStartDateTimeChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnReferenceStartChange(PvOrderObj& orderObj)
{
	bool bResult = false;
	bool bIsPlanOrder(false);
	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (pIComponent != nullptr)
	{
		bIsPlanOrder = true;

		if (PowerPlanUtil::IsOrderPartOfAnIVSequence((LONG_PTR)&orderObj))
		{
			bIsPlanOrder = false;
		}

		if (pIComponent->IsTaperComponent())
		{
			bIsPlanOrder = false;
		}
	}


	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnReferenceStartDateTimeChange(orderObj);
	}
	else
	{
		// The Reference Start Dt/Tm may be filled out with only a time so do not call schedule service.
		// If the service is called, it will reset the date time control to current.
		PvOrderFld* pRefStartDtTmFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReferenceStartDtTm);

		if (NULL == pRefStartDtTmFld)
		{
			return false;
		}

		PvOrderFld::OeFieldValue fldVal;
		pRefStartDtTmFld->GetLastOeFieldValue(fldVal);

		if (fldVal.oeFieldDtTmValue.IsNull() && !fldVal.oeFieldTimeStr.IsEmpty())
		{
			return true;
		}

		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::eReferenceStartDateTimeChanged);
		}
		else if (CDF::OrderAction::IsActivate(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateActivateOrderSchedule(orders,
					  CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnPRNChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnPRNChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::ePRNIndicatorChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::ePRNIndicatorChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnGenericPriorityChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnGenericPriorityChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::ePriorityChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::ePriorityChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnCollectionPriorityChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnCollectionPriorityChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::ePriorityChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::ePriorityChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnPharmacyPriorityChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnPharmacyPriorityChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::ePriorityChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::ePriorityChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnDurationChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnDurationChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eDurationChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::eDurationChanged);
		}
		else if (CDF::OrderAction::IsRenew(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateRenewOrderSchedule(orders,
					  CalculateRenewOrderScheduleRequest::eDurationChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnFrequencyChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnFrequencyChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eFrequencyChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::eFrequencyChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnInfuseOverChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnInfuseOverChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eInfuseOrderChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::eInfuseOrderChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnStopChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		// Do not call the schedule service for new-style Future Orders since stop change is not
		// allowed for one-time frequency.
		if (CFutureOrderUtil::IsFutureOrder(&orderObj))
		{
			return true;
		}

		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnStopDateTimeChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eStopDateTimeChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::eStopDateTimeChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnSchedulingInstructionsChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnSchedulingInstructionsChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eSchedulingInstructionsChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnFutureChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnFutureIndicatorChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eFutureIndicatorChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnConstantChange(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bIsPlanOrder = IsPlanOrder(orderObj);

	if (bIsPlanOrder)
	{
		CPlanScheduleManager planScheduleManager(m_hPatCon);
		bResult = planScheduleManager.UpdateOrderScheduleOnConstantIndicatorChange(orderObj);
	}
	else
	{
		const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

		if (!bCanScheduleBeCalculated)
		{
			return true;
		}

		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		const double dFmtActionCd = orderObj.GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eConstantIndicatorChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::eConstantIndicatorChanged);
		}
	}

	return bResult;
}

bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnMoveDose(PvOrderObj& orderObj)
{
	bool bResult = false;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	const bool bCanScheduleBeCalculated = CanScheduleBeCalculated(orderObj);

	if (!bCanScheduleBeCalculated)
	{
		return true;
	}

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		const short sNumberOfFollowingDosesToReturn = 3;

		CCalculateNewOrderScheduleCriteria orderCriteria(&orderObj, CalculateNewOrderScheduleRequest::eDoseMoved,
				sNumberOfFollowingDosesToReturn);

		std::list<CCalculateNewOrderScheduleCriteria> critieria;
		critieria.push_back(orderCriteria);

		PvScheduleTransactionAssistant::CalculateNewOrderSchedule(critieria);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd))
	{
		const short sNumberOfFollowingDosesToReturn = 3;

		CCalculateModifyOrderScheduleCriteria orderCriteria(&orderObj, CalculateModifyOrderScheduleRequest::eDoseMoved,
				sNumberOfFollowingDosesToReturn);

		std::list<CCalculateModifyOrderScheduleCriteria> critieria;
		critieria.push_back(orderCriteria);

		PvScheduleTransactionAssistant::CalculateModifyOrderSchedule(critieria);
	}
	else if (CDF::OrderAction::IsActivate(dFmtActionCd))
	{
		const short sNumberOfFollowingDosesToReturn = 3;

		CCalculateActivateOrderScheduleCriteria orderCriteria(&orderObj, CalculateActivateOrderScheduleRequest::eDoseMoved,
				sNumberOfFollowingDosesToReturn);

		std::list<CCalculateActivateOrderScheduleCriteria> critieria;
		critieria.push_back(orderCriteria);

		PvScheduleTransactionAssistant::CalculateActivateOrderSchedule(critieria);
	}

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalAction(PvOrderObj& orderObj)
/// \brief		Takes an order, determines the proposed order action being accepted,
///				and calls off to update the order schedule based on that proposed
///				order action.
///
/// \return		bool - If calling the schedule service was successful.
///
/// \param[out]	PvOrderObj& orderObj - The order having an action taken upon.
/// \owner		DJ021930
/////////////////////////////////////////////////////////////////////////////
bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalAction(PvOrderObj& orderObj)
{
	const double dFmtActionCd = orderObj.GetFmtActionCd();

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		return UpdateOrderScheduleOnAcceptProposalOrder(orderObj);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd))
	{
		return UpdateOrderScheduleOnAcceptProposalModify(orderObj);
	}
	else if (CDF::OrderAction::IsResume(dFmtActionCd))
	{
		return UpdateOrderScheduleOnAcceptProposalResume(orderObj);
	}
	else if (CDF::OrderAction::IsRenew(dFmtActionCd))
	{
		return UpdateOrderScheduleOnAcceptProposalRenew(orderObj);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalOrder(PvOrderObj& orderObj)
/// \brief		Calls the order schedule service to update an order schedule
///				for accepting a new order proposal. PowerPlan orders are not affected.
///
/// \return		bool - If calling the schedule service was successful.
///
/// \param[out]	PvOrderObj& orderObj - The order being acted upon.
/// \owner		DJ021930
/////////////////////////////////////////////////////////////////////////////
bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalOrder(PvOrderObj& orderObj)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
			  CalculateNewOrderScheduleRequest::eAcceptProposal);

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalModify(PvOrderObj& orderObj)
/// \brief		Calls the order schedule service to update an order schedule
///				for accepting a proposed modify. PowerPlan orders are not sent to
///				this service, and are rather handled by the PlanScheduleManager.
///
/// \return		bool - If calling the schedule service was successful.
///
/// \param[out]	PvOrderObj& orderObj - The order being acted upon.
/// \owner		DJ021930
/////////////////////////////////////////////////////////////////////////////
bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalModify(PvOrderObj& orderObj)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
			  CalculateModifyOrderScheduleRequest::eAcceptProposal);

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalResume(PvOrderObj& orderObj)
/// \brief		Calls the order schedule service to update an order schedule
///				for accepting a proposed renew.
///
/// \return		bool - If calling the schedule service was successful.
///
/// \param[out]	PvOrderObj& orderObj - The order being acted upon.
/// \owner		DJ021930
/////////////////////////////////////////////////////////////////////////////
bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalResume(PvOrderObj& orderObj)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	bResult = inpatientOrderScheduleServiceCaller.CalculateResumeOrderSchedule(orders,
			  CalculateResumeOrderScheduleRequest::eAcceptProposal);

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalRenew(PvOrderObj& orderObj)
/// \brief		Calls the order schedule service to update an order schedule
///				for accepting a proposed renew.
///
/// \return		bool - If calling the schedule service was successful.
///
/// \param[out]	PvOrderObj& orderObj - The order being acted upon.
/// \owner		DJ021930
/////////////////////////////////////////////////////////////////////////////
bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnAcceptProposalRenew(PvOrderObj& orderObj)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	bResult = inpatientOrderScheduleServiceCaller.CalculateRenewOrderSchedule(orders,
			  CalculateRenewOrderScheduleRequest::eAcceptProposal);

	return bResult;
}

bool CInpatientOrderScheduleUpdater::IsPlanOrder(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = GetPlanComponent(orderObj);

	if (NULL != pIComponent)
	{
		if (PowerPlanUtil::IsOrderPartOfAnIVSequence((LONG_PTR)&orderObj))
		{
			return false;
		}

		if (pIComponent->IsTaperComponent())
		{
			return false;
		}

		return true;
	}

	return false;
}

IComponent* CInpatientOrderScheduleUpdater::GetPlanComponent(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (pIComponent != NULL)
	{
		return pIComponent;
	}

	const double dProtocolOrderId = orderObj.GetProtocolOrderId();

	if (dProtocolOrderId > 0.0)
	{
		PvOrderObj* pParentProtocolOrderObj = dynamic_cast<PvOrderObj*>(CGenLoader().FindProfileOrderById(m_hPatCon,
											  dProtocolOrderId));

		if (NULL != pParentProtocolOrderObj)
		{
			if (pParentProtocolOrderObj->IsFutureRecurringOrder())
			{
				IComponentPtr pIParentProtocolComponent = TransferDispatch(pParentProtocolOrderObj->GetPlanComponent());
				return pIParentProtocolComponent;
			}
		}
	}

	return NULL;
}

bool CInpatientOrderScheduleUpdater::CanScheduleBeCalculated(PvOrderObj& orderObj)
{
	// Ignore Protocol Orders
	if (orderObj.IsProtocolOrder())
	{
		return false;
	}

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	// Ignore New-Style Future Orders
	if (CFutureOrderUtil::IsFutureOrder(&orderObj))
	{
		if (!CDF::OrderAction::IsActivate(dFmtActionCd))
		{
			return false;
		}
	}

	PvOrderPCObj* pOrderPCObj = dynamic_cast<PvOrderPCObj*>(&orderObj);

	if (NULL != pOrderPCObj)
	{
		if (pOrderPCObj->GetDummyParentFlag())
		{
			return false;
		}
	}

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		return true;
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd))
	{
		return true;
	}
	else if (CDF::OrderAction::IsActivate(dFmtActionCd))
	{
		return true;
	}
	else if (CDF::OrderAction::IsRenew(dFmtActionCd))
	{
		return true;
	}
	else if (CDF::OrderAction::IsResume(dFmtActionCd))
	{
		return true;
	}
	else if (CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnDiscernModify(PvOrderObj& orderObj)
/// \brief		Calls the order schedule service to update an order schedule
///				for the details modified from the custom discern alert
///
/// \return		bool - If calling the schedule service was successful.
///
/// \param[out]	PvOrderObj& orderObj - The order being acted upon.
/// \owner		TG069402
/////////////////////////////////////////////////////////////////////////////
bool CInpatientOrderScheduleUpdater::UpdateOrderScheduleOnDiscernModify(PvOrderObj& orderObj)
{
	double dFmtActionCd = orderObj.GetFmtActionCd();

	EOrigOrdAsFlag OrigOrdAsFlag = orderObj.GetOrigOrdAsFlag();

	if (OrigOrdAsFlag == eRegular || OrigOrdAsFlag == eSatellite)
	{
		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			return UpdateOrderScheduleOnAddOrder(orderObj);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd))
		{
			if (PvOrdUtil::IsModifyPriorToStart(&orderObj))
			{
				return UpdateOrderScheduleOnAddOrder(orderObj);
			}
			else
			{
				return UpdateOrderScheduleOnInitialModify(orderObj);
			}
		}
	}
	
	return false;
}