#include "StdAfx.h"

#include "FrequencyChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"
#include "PlannedComponentDeterminer.h"
#include "PvFrequencyScheduleManager.h"
#include "UpdateStopTypeFieldHelper.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <FutureOrderUtil.h>
#include <pvcdf2054.h>
#include <pvorderdataexp.h>
#include <PriorityDefs.h>
#include <PvOrderScheduleManagerDefs.h>
#include <dcp_genericloader.h>

CFrequencyChangePlanOrderScheduleUpdater::CFrequencyChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CFrequencyChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnFrequencyChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its frequency is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose frequency changed.
/// \param[in]	PvOrderObj& orderObj - The order whose frequency changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CFrequencyChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnFrequencyChange(IComponent& component,
		PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnFrequencyChange(component, orderObj);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnFrequencyChange(component, orderObj);
	}
}

bool CFrequencyChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnFrequencyChange(IComponent& component,
		PvOrderObj& orderObj)
{
	const double dFrequencyCd = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailOrdFrequency);

	if (dFrequencyCd > 0.0)
	{
		PvOrderFld* pConstantFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailConstantInd);

		if (NULL != pConstantFld)
		{
			pConstantFld->AddOeFieldValue(0, "");
			pConstantFld->SetValuedSourceInd(eVSUser);
		}

		PvOrderScheduleManager::FrequencyInfoStruct frequencyInfo;
		CPvFrequencyScheduleManager::GetInstance().GetFreqSchedInfoByIDFromOrder(orderObj, frequencyInfo);

		const bool bPrnIndicator = frequencyInfo.nPrnDefaultInd == 1;

		if (bPrnIndicator)
		{
			PvOrderFld* pPRNFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailSchPrn);

			if (NULL != pPRNFld)
			{
				pPRNFld->AddOeFieldValue(1.0, "");
			}
		}

		const EFreqTypeFlag frequencyTypeFlag = (EFreqTypeFlag)orderObj.GetFreqTypeFlag();

		if (frequencyTypeFlag == eFTFOneTime)
		{
			COrderDetailHelper::ClearFields(orderObj, eDetailOrdDuration, eDetailOrdDurationUnit);

			if (component.IsDoTComponent() == FALSE)
			{
				CUpdateStopTypeFieldHelper::UpdateStopTypeFldToPhysicianStop(orderObj);
			}
		}

		SetFrequencyRelatedFieldsOnFrequencyChangeForPlannedOrder(orderObj);

	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CFrequencyChangePlanOrderScheduleUpdater::SetFrequencyRelatedFieldsOnFrequencyChangeForPlannedOrder(PvOrderObj& orderObj)
/// \brief		Handles updating the collection priority and reporting priority of the given plan order when its frequency is manually changed.
///
///
/// \param[in]	PvOrderObj& orderObj - The order whose frequency changed.
/// \owner		MS021481
/////////////////////////////////////////////////////////////////////////////
void CFrequencyChangePlanOrderScheduleUpdater::SetFrequencyRelatedFieldsOnFrequencyChangeForPlannedOrder(
	PvOrderObj& orderObj)
{
	// Frequency\Collection priority related field logic only done for lab orders.
	if (!CDF::CatalogType::IsLaboratory(orderObj.GetCatalogTypeCd()))
	{
		return;
	}

	PvOrderFld* collectionPriorityFld = NULL;
	PvOrderFld::OeFieldValue collectionPriorityValue;

	// Check existence of collection priority field
	collectionPriorityFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailCollPriority);

	if (collectionPriorityFld == nullptr)
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CFrequencySchedulePlanOrderScheduleUpdater"),
				  eMsgLvl_Audit,
				  _T("UpdatePlannedOrderScheduleOnFrequencyChange() Collection Priority field doesn't exist on format"));
		return;
	}

	// Check collection priority value.
	collectionPriorityFld->GetLastOeFieldValue(collectionPriorityValue);

	if (collectionPriorityValue.IsEmpty())
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CFrequencySchedulePlanOrderScheduleUpdater"),
				  eMsgLvl_Debug,
				  _T("UpdatePlannedOrderScheduleOnFrequencyChange() Collection priority has no value"));
		return;
	}

	// Proceed only if the collection priority is Routine or Stat.
	if (!CDF::CollectionPriority::IsRoutine(collectionPriorityValue.oeFieldValue)
		&& !CDF::CollectionPriority::IsStat(collectionPriorityValue.oeFieldValue))
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CFrequencySchedulePlanOrderScheduleUpdater"),
				  eMsgLvl_Warning,
				  _T("UpdatePlannedOrderScheduleOnFrequencyChange() Collection priority is not Routine or Stat"));
		return;
	}

	// Execute script to retrieve all collection priorities
	SrvHandle hRep = LoadPriorityList();

	if (!hRep)
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CFrequencySchedulePlanOrderScheduleUpdater"),
				  eMsgLvl_Warning,
				  _T("UpdatePlannedOrderScheduleOnFrequencyChange() Unable to get collection priorities"));
		return;
	}

	// Traverse reply to detemine Time Study priority
	SCollectionPriorities currentCollectionPriority;
	SCollectionPriorities timestudyCollectionPriority;
	timestudyCollectionPriority.collectionPriorityCd = 0.0;
	timestudyCollectionPriority.defaultReportPriorityCd = 0.0;

	srv_size_t iCount = SrvGetItemCount(hRep, "qual");

	for (srv_size_t iIndex = 0; iIndex < iCount; ++iIndex)
	{
		SrvHandle hItem = SrvGetItem(hRep, "qual", iIndex);

		if (!hItem)
		{
			return;
		}

		currentCollectionPriority.collectionPriorityCd = SrvGetDouble(hItem, "collection_priority_cd");
		currentCollectionPriority.collectionPriorityDisp = SrvGetStringPtr(hItem, "colleciton_priority_disp");
		currentCollectionPriority.collectionPriorityDesc = SrvGetStringPtr(hItem, "collection_priority_desc");
		currentCollectionPriority.timeStudyInd = SrvGetShort(hItem, "time_study_ind");
		currentCollectionPriority.defaultReportPriorityCd = SrvGetDouble(hItem, "default_report_priority_cd");
		currentCollectionPriority.defaultReportPriorityDisp = SrvGetStringPtr(hItem, "default_report_priority_disp");
		currentCollectionPriority.defaultStartDtTm = SrvGetStringPtr(hItem, "default_start_dt_tm");
		currentCollectionPriority.collectionPriorityMean = SrvGetStringPtr(hItem, "collection_Priority_mean");

		// If the user has entered Routine, find the Timed Routine priority.
		if (CDF::CollectionPriority::IsRoutine(collectionPriorityValue.oeFieldValue))
		{
			if ((currentCollectionPriority.collectionPriorityMean == "TIMEDROUTINE")
				&& (currentCollectionPriority.timeStudyInd == 1))
			{
				timestudyCollectionPriority = currentCollectionPriority;
			}
		}
		// If the user has entered Stat, find the Timed Stat priority
		else if (CDF::CollectionPriority::IsStat(collectionPriorityValue.oeFieldValue))
		{
			if ((currentCollectionPriority.collectionPriorityMean == "TIMEDSTAT") && (currentCollectionPriority.timeStudyInd == 1))
			{
				timestudyCollectionPriority = currentCollectionPriority;
			}
		}
	}

	// Set collection priority and related fields to appropriate time study priority
	if (timestudyCollectionPriority.collectionPriorityCd > 0.0)
	{
		PvOrderFld* reportingPriority = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailRptPriority);

		collectionPriorityFld->AddOeFieldValue(timestudyCollectionPriority.collectionPriorityCd,
											   timestudyCollectionPriority.collectionPriorityDesc);

		// Set reporting priority
		if (reportingPriority != nullptr && timestudyCollectionPriority.defaultReportPriorityCd > 0.0)
		{
			reportingPriority->AddOeFieldValue(timestudyCollectionPriority.defaultReportPriorityCd,
											   timestudyCollectionPriority.defaultReportPriorityDisp);
		}
	}
	else
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CFrequencySchedulePlanOrderScheduleUpdater"),
				  eMsgLvl_Audit,
				  _T("UpdatePlannedOrderScheduleOnFrequencyChange() No time study priority found on codeset 2054"));
	}

	return;
}

bool CFrequencyChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnFrequencyChange(IComponent& component,
		PvOrderObj& orderObj)
{
	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);
	const double dFmtActionCd = orderObj.GetFmtActionCd();

	std::list<PvOrderObj*> orders;

	if (NULL != pOrderProtocolObj)
	{
		std::list<PvOrderObj*> dotOrders;
		CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, *pOrderProtocolObj, dotOrders);

		for (auto orderIter = dotOrders.cbegin(); orderIter != dotOrders.cend(); orderIter++)
		{
			PvOrderObj* pDoTOrderObj = *orderIter;

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
				  CalculateNewOrderScheduleRequest::eFrequencyChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::eFrequencyChanged);
	}

	if (NULL != pOrderProtocolObj)
	{
		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(*pOrderProtocolObj);
	}

	return bResult;
}