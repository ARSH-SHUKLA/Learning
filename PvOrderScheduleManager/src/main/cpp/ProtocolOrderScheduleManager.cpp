#include "StdAfx.h"

#include "ProtocolOrderScheduleManager.h"

#include <PvOrderProtocolObj.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>
#include <PvOrdCalUtil.h>
#include <pvcdf6004.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>

CProtocolOrderScheduleManager::CProtocolOrderScheduleManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

void CProtocolOrderScheduleManager::UpdateProtocolSchedule(IPhase& phase) const
{
	IPhase* pIProtocolPhase = PowerPlanUtil::GetProtocolPhase(&phase);

	if (pIProtocolPhase != NULL)
	{
		IProVideObjectCollectionPtr pCompCollection(pIProtocolPhase->GetComponentCollection());

		if (pCompCollection != NULL)
		{
			long compTotal(0);
			pCompCollection->get_Count(&compTotal);

			for (long compCnt = 1; compCnt <= compTotal; ++compCnt)
			{
				_variant_t vComponent = pCompCollection->Value(CComVariant(compCnt));
				IComponentPtr pIComponent = vComponent.pdispVal;

				if (NULL != pIComponent)
				{
					if (TRUE == pIComponent->GetIncluded())
					{
						PvOrderObj* pOrderObj = CGenLoader().GetOrderObjFromComponent(m_hPatCon, pIComponent);

						PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(pOrderObj);

						if (NULL != pOrderProtocolObj)
						{
							UpdateProtocolSchedule(*pOrderProtocolObj);
						}
					}
				}

				VariantClear(&vComponent);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn      void CProtocolOrderScheduleManager::UpdateProtocolSchedule(PvOrderProtocolObj& protocolObj) const
/// \brief      This function sets the start date/time of the protocol order to the start date/time of its first
///                day of treatment order and sets the stop date/time of the protocol order to the stop date/time
///                of its last day of treatment order.
///
/// \return     void
///
/// \param[in]  PvOrderProtocolObj& protocolObj - The protocol order to update
/// \owner		MH015940
/////////////////////////////////////////////////////////////////////////////
void CProtocolOrderScheduleManager::UpdateProtocolSchedule(PvOrderProtocolObj& protocolObj) const
{
	std::list<PvOrderObj*> dotOrderList;
	CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, protocolObj, dotOrderList);

	if (!dotOrderList.empty())
	{
		for (std::list<PvOrderObj*>::iterator itr(dotOrderList.begin()), end(dotOrderList.end()); itr != end; ++itr)
		{
			PvOrderObj* pDOTOrderObj = *itr;

			if (NULL != pDOTOrderObj && !CDF::OrderStatus::IsCanceled(pDOTOrderObj->GetOrderStatusCd()))
			{
				PvOrderFld* pFirstProtocolFld = protocolObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);
				PvOrderFld* pFirstDOTOrderFld = pDOTOrderObj->m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

				if (NULL != pFirstProtocolFld && NULL != pFirstDOTOrderFld)
				{
					pFirstProtocolFld->CopyOeFieldValues(*pFirstDOTOrderFld);
					pFirstProtocolFld->SetVisualValueMask(pFirstDOTOrderFld->GetVisualValueMask());
					pFirstProtocolFld->SetValuedSourceInd(pFirstDOTOrderFld->GetValuedSourceInd());
				}

				PvOrderFld* pProtocolReferenceStartDateTimeFld = protocolObj.m_orderFldArr.GetFieldFromMeanId(
							eDetailReferenceStartDtTm);
				PvOrderFld* pFirstDOTReferenceStartDateTimeFld = pDOTOrderObj->m_orderFldArr.GetFieldFromMeanId(
							eDetailReferenceStartDtTm);

				if (NULL != pProtocolReferenceStartDateTimeFld && NULL != pFirstDOTReferenceStartDateTimeFld)
				{
					pProtocolReferenceStartDateTimeFld->CopyOeFieldValues(*pFirstDOTReferenceStartDateTimeFld);
					pProtocolReferenceStartDateTimeFld->SetVisualValueMask(pFirstDOTReferenceStartDateTimeFld->GetVisualValueMask());
					pProtocolReferenceStartDateTimeFld->SetValuedSourceInd(pFirstDOTReferenceStartDateTimeFld->GetValuedSourceInd());
				}

				PvOrderFld* pProtocolNextDoseDateTimeFld = protocolObj.m_orderFldArr.GetFieldFromMeanId(
							eDetailNextDoseDtTm);
				PvOrderFld* pFirstDOTNextDoseDateTimeFld = pDOTOrderObj->m_orderFldArr.GetFieldFromMeanId(
							eDetailNextDoseDtTm);

				if (NULL != pProtocolNextDoseDateTimeFld && NULL != pFirstDOTNextDoseDateTimeFld)
				{
					pProtocolNextDoseDateTimeFld->CopyOeFieldValues(*pFirstDOTNextDoseDateTimeFld);
					pProtocolNextDoseDateTimeFld->SetVisualValueMask(pFirstDOTNextDoseDateTimeFld->GetVisualValueMask());
					pProtocolNextDoseDateTimeFld->SetValuedSourceInd(pFirstDOTNextDoseDateTimeFld->GetValuedSourceInd());
				}

				break;
			}
		}

		for (std::list<PvOrderObj*>::reverse_iterator ritr(dotOrderList.rbegin()), rend(dotOrderList.rend()); ritr != rend;
			 ++ritr)
		{
			PvOrderObj* pDOTOrderObj = *ritr;

			if (NULL != pDOTOrderObj && !CDF::OrderStatus::IsCanceled(pDOTOrderObj->GetOrderStatusCd()))
			{
				PvOrderFld* pFirstProtocolFld = protocolObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);
				PvOrderFld* pFirstDOTOrderFld = pDOTOrderObj->m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);

				if (NULL != pFirstProtocolFld && NULL != pFirstDOTOrderFld)
				{
					pFirstProtocolFld->CopyOeFieldValues(*pFirstDOTOrderFld);
					pFirstProtocolFld->SetVisualValueMask(pFirstDOTOrderFld->GetVisualValueMask());
					pFirstProtocolFld->SetValuedSourceInd(pFirstDOTOrderFld->GetValuedSourceInd());
				}

				break;
			}
		}

		protocolObj.BuildDisplayLines();
	}
}