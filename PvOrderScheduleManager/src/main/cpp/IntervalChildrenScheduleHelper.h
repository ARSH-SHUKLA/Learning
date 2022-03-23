#pragma once

#include <RelatedFldsStatus.h>

// Forward Declarations
class PvOrderPCObj;

class CIntervalChildrenScheduleHelper
{
public:
	CIntervalChildrenScheduleHelper(const HPATCON hPatCon);

	ERelatedFldsStatus SetReqStartDtTmForIntervalChildren(PvOrderPCObj& orderPCObj);

private:
	PvOrderPCObj* FindIntervalChildById(PvOrderPCObj& parentPCObj, const double dOrderId) const;
	ERelatedFldsStatus SetReqStartDtTmForIntervalChild(PvOrderPCObj& orderPCObj, PvOrderPCObj& intervalChildPCObj);

private:
	const HPATCON m_hPatCon;
};