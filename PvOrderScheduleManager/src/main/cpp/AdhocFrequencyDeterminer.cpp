#include "stdafx.h"

#include "AdhocFrequencyDeterminer.h"

#include <pvorderobdataexp.h>

const long ADHOC_IND = 16;

/////////////////////////////////////////////////////////////////////////////
/// \fn      bool CPvActionManagerInpatient::LoadAdhocIndFromFreqId(double dFreqId, long& lAdhocInd)
/// \brief      This method will call the script 380100 to decide if the
///			  frequency was customized.
/// \return     bool
/// \param[in]  double dFreqId - Frequency Id
/// \param[in]  long& lAdhocInd - return parameter. Customized if value is 16.
/// owner     DP014848
/////////////////////////////////////////////////////////////////////////////
bool CAdhocFrequencyDeterminer::LoadAdhocIndFromFreqId(const double dFreqId) const
{
	if (0.0 < dFreqId)
	{
		long lAdhocInd(0);
		LONG_PTR lGetFreqSchedById = GetFreqSchedById_Init();

		if (0 < lGetFreqSchedById)
		{
			SrvHandle hReply = GetFreqSchedById(lGetFreqSchedById, dFreqId);

			if (NULL != hReply)
			{
				int iFreqCnt = SrvGetItemCount(hReply, "frequency_list");

				if (0 < iFreqCnt)
				{
					SrvHandle hFreq = SrvGetItem(hReply, "frequency_list", 0);

					if (NULL != hFreq)
					{
						lAdhocInd = SrvGetLong(hFreq, "freq_qualifier");

						if (ADHOC_IND == lAdhocInd)
						{
							GetFreqSchedById_Quit(lGetFreqSchedById);
							return true;
						}
					}
				}
			}
		}

		GetFreqSchedById_Quit(lGetFreqSchedById);
	}

	return false;
}