#pragma once

#include <list>
#include <CalculateNewOrderScheduleCriteria.h>
#include <CalculateModifyOrderScheduleCriteria.h>
#include <CalculateRenewOrderScheduleCriteria.h>
#include <CalculateActivateOrderScheduleCriteria.h>
#include <CalculateResumeOrderScheduleCriteria.h>

// Forward Declarations
class PvOrderObj;

class CInpatientOrderScheduleServiceCaller
{
public:
	CInpatientOrderScheduleServiceCaller(const HPATCON hPatCon);

	bool CalculateNewOrderSchedule(std::list<PvOrderObj*>& orders,
								   const CalculateNewOrderScheduleRequest::ETriggeringActionFlag triggeringActionFlag);
	bool CalculateNewOrderSchedule(std::list<CCalculateNewOrderScheduleCriteria>& criteria);

	bool CalculateModifyOrderSchedule(std::list<PvOrderObj*>& orders,
									  const CalculateModifyOrderScheduleRequest::ETriggeringActionFlag triggeringActionFlag);
	bool CalculateModifyOrderSchedule(std::list<CCalculateModifyOrderScheduleCriteria>& criteria);

	bool CalculateRenewOrderSchedule(std::list<PvOrderObj*>& orders,
									 const CalculateRenewOrderScheduleRequest::ETriggeringActionFlag triggeringActionFlag);
	bool CalculateRenewOrderSchedule(std::list<CCalculateRenewOrderScheduleCriteria>& criteria);

	bool CalculateActivateOrderSchedule(std::list<PvOrderObj*>& orders,
										const CalculateActivateOrderScheduleRequest::ETriggeringActionFlag triggeringActionFlag);
	bool CalculateActivateOrderSchedule(std::list<CCalculateActivateOrderScheduleCriteria>& criteria);

	bool CalculateResumeOrderSchedule(std::list<PvOrderObj*>& orders,
									  const CalculateResumeOrderScheduleRequest::ETriggeringActionFlag triggeringActionFlag);
	bool CalculateResumeOrderSchedule(std::list<CCalculateResumeOrderScheduleCriteria>& criteria);
private:
	void HandleSuccessfulScheduleServiceCall(std::list<PvOrderObj*>& orders);

private:
	const HPATCON m_hPatCon;
};