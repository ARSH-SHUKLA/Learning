#include "StdAfx.h"

#include "TimeZeroHelper.h"

#include <pvcdf340.h>
#include <CPS_ImportPVCareCoordCom.h>

/////////////////////////////////////////////////////////////////////////////
/// \fn		Cerner::Foundations::Calendar CTimeZeroHelper::CalculateStartDateTimeFromTimeZeroInformation(IComponent& component, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime)
/// \brief		Calculates start date/time for time-zero components, and components with time-zero offsets.
///
/// \return		Cerner::Foundations::Calendar - The non-null start date/time of the component.
///
/// \param[in]	IComponent& component - The component to calculate the start date/time of.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The phase start date/time (cannot be null).
/// \param[in]	const Cerner::Foundations::Calendar& actionDateTime - The conversation/action date/time (cannot be null).
/// \param[in]	const Cerner::Foundations::Calendar& timeZeroDateTime - The possibly-null time-zero date/time of the parent phase.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
Cerner::Foundations::Calendar CTimeZeroHelper::CalculateStartDateTimeFromTimeZeroInformation(IComponent& component,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime)
{
	if (timeZeroDateTime.IsNull())
	{
		return phaseStartDateTime;
	}

	const CString sTimeZeroMeaning = (LPCTSTR)component.GetTZMean();

	if (sTimeZeroMeaning == "TIMEZERO")
	{
		Cerner::Foundations::Calendar timeZeroOrderDateTime = Cerner::Foundations::Calendar(timeZeroDateTime);
		return timeZeroOrderDateTime;
	}
	else if (sTimeZeroMeaning == "TIMEZEROLINK")
	{
		const double dOffsetValue = component.GetTZOffsetQty();
		const double dOffsetUnitCd = component.GetTZOffsetUnitCd();

		const bool bIsDays = CDF::AgeUnitCodeset::IsDays(dOffsetUnitCd);

		if (bIsDays)
		{
			const CString sTimeZero = timeZeroDateTime.GetDateString();

			Cerner::Foundations::Calendar timeZeroWithDays = timeZeroDateTime.AddDays((long)dOffsetValue);
			return timeZeroWithDays;
		}

		const bool bIsHours = CDF::AgeUnitCodeset::IsHours(dOffsetUnitCd);

		if (bIsHours)
		{
			// We intentionally support fractional offsets (.25, .5, .75) in hours
			const long lNumberOfMinutes = (long)(dOffsetValue * 60);
			Cerner::Foundations::Calendar timeZeroWithHours = timeZeroDateTime.AddMinutes(lNumberOfMinutes);

			return timeZeroWithHours;
		}

		const bool bIsMinutes = CDF::AgeUnitCodeset::IsMinutes(dOffsetUnitCd);

		if (bIsMinutes)
		{
			Cerner::Foundations::Calendar timeZeroWithMinutes = timeZeroDateTime.AddMinutes((long)dOffsetValue);

			return timeZeroWithMinutes;
		}
	}

	return timeZeroDateTime;
}

double CTimeZeroHelper::GetTimeZeroOffsetInMinutes(IComponent& component)
{
	const bool bTimeZeroLinked = component.IsTimeZeroLinked() == TRUE;

	if (bTimeZeroLinked)
	{
		const bool bTimeZeroActive = component.GetTZActiveInd() == TRUE;

		if (bTimeZeroActive)
		{
			const double dOffsetQty = component.GetTZOffsetQty();
			const double dOffsetUnitCd = component.GetTZOffsetUnitCd();

			if (CDF::AgeUnitCodeset::IsDays(dOffsetUnitCd))
			{
				return (dOffsetQty * 24 * 60);
			}
			else if (CDF::AgeUnitCodeset::IsHours(dOffsetUnitCd))
			{
				return (dOffsetQty * 60);
			}
			else if (CDF::AgeUnitCodeset::IsMinutes(dOffsetUnitCd))
			{
				return dOffsetQty;
			}
		}
	}

	return 0.0;
}