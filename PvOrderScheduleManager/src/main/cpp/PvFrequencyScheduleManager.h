#pragma once

#include <PvOrderScheduleManagerDefs.h>
#include <pvorderfld.h>

// Forward Declarations
class PvOrderObj;

class CPvFrequencyScheduleManager
{
public:
	static CPvFrequencyScheduleManager& GetInstance();

	bool GetFreqSchedInfoByIDFromOrder(PvOrderObj& orderObj, PvOrderScheduleManager::FrequencyInfoStruct& freqStruct);
	bool GetFreqSchedInfoByCD(PvOrderObj& orderObj, PvOrderScheduleManager::FrequencyInfoStruct& freqStruct,
							  PvOrderFld::OeFieldValue& freqCDVal, double dEncounterId, bool& bIsCustomFreq);
	void AddFreqSchedInfo(PvOrderScheduleManager::FrequencyInfoStruct& freqStruct);

private:
	CPvFrequencyScheduleManager();

private:
	CMap<double, double, PvOrderScheduleManager::FrequencyInfoStruct, PvOrderScheduleManager::FrequencyInfoStruct>
	m_freqInfoMap;
};