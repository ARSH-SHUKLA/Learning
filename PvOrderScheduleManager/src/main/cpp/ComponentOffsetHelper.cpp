#include "StdAfx.h"

#include "ComponentOffsetHelper.h"

#include <CPS_ImportPVCareCoordCom.h>
#include <pvcdf340.h>

bool CComponentOffsetHelper::DoesComponentHaveOffsetInHoursOrMinutes(IComponent& component)
{
	const bool bHasComponentOffset = component.HasComponentOffset() == TRUE;

	if (bHasComponentOffset)
	{
		const double dOffsetUnitCd = component.GetStartOffsetUnitCd();

		if (dOffsetUnitCd > 0.0)
		{
			if (CDF::AgeUnitCodeset::IsHours(dOffsetUnitCd))
			{
				return true;
			}

			if (CDF::AgeUnitCodeset::IsMinutes(dOffsetUnitCd))
			{
				return true;
			}
		}
	}

	return false;
}

bool CComponentOffsetHelper::DoesComponentHaveOffsetInDays(IComponent& component)
{
	const bool bHasComponentOffset = component.HasComponentOffset() == TRUE;

	if (bHasComponentOffset)
	{
		const double dOffsetUnitCd = component.GetStartOffsetUnitCd();

		if (dOffsetUnitCd > 0.0)
		{
			if (CDF::AgeUnitCodeset::IsDays(dOffsetUnitCd))
			{
				return true;
			}
		}
	}

	return false;
}

void CComponentOffsetHelper::ClearComponentOffset(IComponent& component)
{
	component.PutStartOffsetQty(0);
	component.PutStartOffsetUnitCd(0.0);
	component.PutStartOffsetUnitDisp((_bstr_t)"");
	component.PutStartOffsetUnitMean((_bstr_t)"");
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		Cerner::Foundations::Calendar CComponentOffsetHelper::AddComponentOffset(IComponent& component, const Cerner::Foundations::Calendar& dateTime)
/// \brief		Calculates a new date/time by adding the given component's start offset to the given date/time.
///
/// \return		Cerner::Foundations::Calendar - The non-null dateTime with the component offset added.
///
/// \param[in]	IComponent& component - The component containing the offset to add.
/// \param[in]	const Cerner::Foundations::Calendar& dateTime - The date/time to add the component's offset to (cannot be null).
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
Cerner::Foundations::Calendar CComponentOffsetHelper::AddComponentOffset(IComponent& component,
		const Cerner::Foundations::Calendar& dateTime)
{
	const double dOffsetValue = component.GetStartOffsetQty();
	const double dOffsetUnitCd = component.GetStartOffsetUnitCd();

	const bool bIsDays = CDF::AgeUnitCodeset::IsDays(dOffsetUnitCd);

	if (bIsDays)
	{
		return dateTime.AddDays((long) dOffsetValue);
	}

	const bool bIsHours = CDF::AgeUnitCodeset::IsHours(dOffsetUnitCd);

	if (bIsHours)
	{
		const long lNumberOfMinutes = (long)(dOffsetValue * 60);
		return dateTime.AddMinutes(lNumberOfMinutes);
	}

	const bool bIsMinutes = CDF::AgeUnitCodeset::IsMinutes(dOffsetUnitCd);

	if (bIsMinutes)
	{
		return dateTime.AddMinutes((long) dOffsetValue);
	}

	return dateTime;
}