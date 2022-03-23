#pragma once

// Forward Declarations
struct IPhase;
struct IComponent;
class PvOrderObj;
class PvOrderFld;

class CPlanOrderDateTimesModifiableDeterminer
{
public:
	bool IsOrderStartDateTimeModifiable(PvOrderObj& orderObj) const;
	
	bool IsOrderStopDateTimeModifiable(PvOrderObj& orderObj) const;

private:
	bool AreOrderComponentsDateTimesModifiable(IPhase& phase, IComponent& component, PvOrderObj& orderObj,
			PvOrderFld& startDateTimeFld) const;
};