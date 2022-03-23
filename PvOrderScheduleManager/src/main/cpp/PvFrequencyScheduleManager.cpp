#include "StdAfx.h"

#include "PvFrequencyScheduleManager.h"
#include "AdhocFrequencyDeterminer.h"

#include <pvorderobj.h>
#include <pvordutil.h>
#include <pvorderdataexp.h>
#include <PvOrdersTransactionAssistantExp.h>
#include <Pvordencounterutil.h>
#include <pvprefs.h>
#include <pvcdf4001987.h>

CPvFrequencyScheduleManager::CPvFrequencyScheduleManager()
{

}

CPvFrequencyScheduleManager& CPvFrequencyScheduleManager::GetInstance()
{
	static CPvFrequencyScheduleManager frequencyScheduleManager;
	return frequencyScheduleManager;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn      bool CPvFrequencyScheduleManager::GetFreqSchedInfoByIDFromOrder(PvOrderObj& orderObj, PvOrderScheduleManager::FrequencyInfoStruct& freqStruct)
/// \brief      Returns frequency scheduling info for a given order object
///				based on the frequency ID field (eDetailFrequencyId) value stored in the order object.
///
/// \return     bool - true on success
///
/// \param[in]  PvOrderObj& orderObj - The order object
/// \param[in]  PvOrderScheduleManager::FrequencyInfoStruct& freqStruct - Struct containing frequency scheduling info
///
/// \pre Order details must be loaded.
/// \owner		AE017990
/////////////////////////////////////////////////////////////////////////////
bool CPvFrequencyScheduleManager::GetFreqSchedInfoByIDFromOrder(PvOrderObj& orderObj,
		PvOrderScheduleManager::FrequencyInfoStruct& freqStruct)
{
	PvOrderFld* pFreqIdFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailFrequencyId);

	if (NULL == pFreqIdFld)
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
				  _T("GetFreqSchedInfoByIDFromOrder() Failed to retrieve frequency ID order entry field."));
		return false;
	}

	double dFreqId = pFreqIdFld->GetLastOeFldValue();

	if (dFreqId != 0.0)
	{
		freqStruct.Reset();

		if (!m_freqInfoMap.Lookup(dFreqId, freqStruct))
		{
			LONG_PTR lFreqHandle = GetFreqSchedById_Init();

			if (lFreqHandle == 0)
			{
				MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
						  _T("GetFreqSchedInfoByIDFromOrder() Failed to initiate rx_get_freq_sched_by_id."));
				return false;
			}

			SrvHandle hReply = GetFreqSchedById(lFreqHandle, dFreqId);

			if (hReply == 0)
			{
				MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
						  _T("GetFreqSchedInfoByIDFromOrder() End - Could not execute rx_get_freq_sched_by_id."));
				GetFreqSchedById_Quit(lFreqHandle);
				return false;
			}

			int iItemCnt = SrvGetItemCount(hReply, "frequency_list");

			if (iItemCnt != 1)
			{
				MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
						  _T("GetFreqSchedInfoByIDFromOrder() Number of elements in frequency_list is not 1."));
				GetFreqSchedById_Quit(lFreqHandle);
				return false;
			}

			SrvHandle hFreq = SrvGetItem(hReply, "frequency_list", 0);

			if (hFreq == 0)
			{
				MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
						  _T("GetFreqSchedInfoByIDFromOrder() Cannot get the first element from frequency_list."));
				GetFreqSchedById_Quit(lFreqHandle);
				return false;
			}

			freqStruct.dFreqId = dFreqId;
			freqStruct.lFreqType = SrvGetLong(hFreq, "frequency_type");
			freqStruct.nPrnDefaultInd = (short)SrvGetLong(hFreq, "prn_default_ind");
			freqStruct.nHybridInd = (freqStruct.lFreqType == eFTFInterval) && (SrvGetItemCount(hFreq, "tod_list") > 0) ? 1 : 0;

			GetFreqSchedById_Quit(lFreqHandle);

			m_freqInfoMap.SetAt(dFreqId, freqStruct);
		}

		return true;
	}
	else
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
				  _T("GetFreqSchedInfoByIDFromOrder() Failed to retrieve frequency ID order entry field value."));
		return false;
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn      bool CPvFrequencyScheduleManager::GetFreqSchedInfoByCD(PvOrderObj& orderObj, PvOrderScheduleManager::FrequencyInfoStruct& freqStruct, 
///													PvOrderFld::OeFieldValue& freqCDVal, double dEncounterId, bool& bIsCustomFreq)
/// \brief      Returns frequency scheduling info for a given order object
///				based on a given frequency CD field (eDetailOrdFrequency) value.
///
/// \return     bool
///
/// \param[in]  PvOrderObj& orderObj - The order object
/// \param[in]  PvOrderScheduleManager::FrequencyInfoStruct& freqStruct - Struct containing frequency scheduling info
/// \param[in]  PvOrderFld::OeFieldValue& freqCDVal - Frequency CD OE Field Value (eDetailOrdFrequency).
/// \param[in]  double dEncounterId -  encounter Id
/// \param[in,out] bool& bIsCustomFreq - boolean variable gets updated to true only if entered frequency is custom frequency, otherwise false.
/// \owner		AE017990 and SK042435
/////////////////////////////////////////////////////////////////////////////
bool CPvFrequencyScheduleManager::GetFreqSchedInfoByCD(PvOrderObj& orderObj,
		PvOrderScheduleManager::FrequencyInfoStruct& freqStruct,
		PvOrderFld::OeFieldValue& freqCDVal, double dEncounterId, bool& bIsCustomFreq)
{
	PvOrderFld* pFreqIdFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailFrequencyId);

	if (pFreqIdFld == nullptr)
	{
		return false;
	}

	if (freqCDVal.IsEmpty())
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
				  _T("GetFreqSchedInfoByCD() Frequency CD OE Field Value is empty."));
		return false;
	}

	// If the frequency schedule is already cached in the database scratchpad,
	// we must clear the existing schedule.
	orderObj.m_adHocFreqList.currSchedList.RemoveAll();

	if (PrefCon_GetValueInt(eApp, "ALLOW_CUSTOM_FREQ") == 1 && pFreqIdFld->GetModifiedInd() == 0)
	{
		if (CDF::OrderRelationType::IsRepeat(orderObj.GetOrderRelationTypeCd()))
		{
			CAdhocFrequencyDeterminer adhocFrequencyDeterminerObj;
			bIsCustomFreq = adhocFrequencyDeterminerObj.LoadAdhocIndFromFreqId(orderObj.GetFrequencyId());

			if (bIsCustomFreq)
			{
				return false;
			}
		}
	}

	freqStruct.Reset();

	const double dProviderId = orderObj.GetProviderId();
	const double dCatalogCd = orderObj.GetCatalogCd();
	const double dPatientId = orderObj.GetPatientId();
	const double dActivityTypeCd = orderObj.GetActivityTypeCd();
	double dNurseUnitCd = PvOrdUtil::GetPatLocationCd(dPatientId, dEncounterId);
	double dFacilityCd = Pvordencounterutil::GetLocFacilityCd(dPatientId, dEncounterId);

	if (PvOrdUtil::IsFutureOrder(&orderObj))
	{
		if (CodeValue_GetCodeValue(CDF::OrderAction::CODESET, "MODIFY") == orderObj.GetFmtActionCd())
		{
			dNurseUnitCd = orderObj.GetFutureNursingUnitCd();
			dFacilityCd = orderObj.GetFutureFacilityCd();
		}
		else
		{
			double dEncounterIdFmPat = PvOrdUtil::GetEncounterId(orderObj.GetPatCon());
			dNurseUnitCd = Pvordencounterutil::GetNurseUnitCd(dPatientId, dEncounterIdFmPat);
			dFacilityCd = Pvordencounterutil::GetLocFacilityCd(dPatientId, dEncounterIdFmPat);
		}
	}

	LONG_PTR lRxGetFreqId = RxGetFreqId_Init(freqCDVal.oeFieldValue, 0, dProviderId, dCatalogCd, 0, dNurseUnitCd,
							dActivityTypeCd, dFacilityCd);

	if (0 == lRxGetFreqId)
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
				  _T("GetFreqSchedInfoByCD() Failed to initiate rx_get_freq_id."));
		return false;
	}

	const SrvHandle hRep = RxGetFreqId_Get(lRxGetFreqId);

	if (0 == hRep)
	{
		MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, _T("PvOrderPlanManagerCOM::CPvFrequencyScheduleManager"), eMsgLvl_Audit,
				  _T("GetFreqSchedInfoByCD() End - Could not execute rx_get_freq_id."));
		return false;
	}

	freqStruct.dFreqId = SrvGetDouble(hRep, "frequency_id");
	freqStruct.lFreqType = SrvGetLong(hRep, "frequency_type");
	freqStruct.nPrnDefaultInd = SrvGetShort(hRep, "prn_default_ind");
	freqStruct.nHybridInd = SrvGetShort(hRep, "hybrid_ind");

	RxGetFreqId_Quit(lRxGetFreqId);

	m_freqInfoMap.SetAt(freqStruct.dFreqId, freqStruct);

	return true;
}

void CPvFrequencyScheduleManager::AddFreqSchedInfo(PvOrderScheduleManager::FrequencyInfoStruct& freqStruct)
{
	m_freqInfoMap.SetAt(freqStruct.dFreqId, freqStruct);
}