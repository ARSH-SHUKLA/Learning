#pragma once

#include <dcp_orddefs.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CModifiableOrderRetriever
{
public:
	CModifiableOrderRetriever(const HPATCON hPatCon);

	PvOrderObj* GetModifiableOrderFromComponent(IComponent& orderComponent) const;
	PvOrderObj* GetModifiableSchedulableOrderFromComponent(IComponent& schedulableComponent) const;
	PvOrderObj* GetModifiableOrderFromComponentForStopDateTimeChange(IComponent& orderComponent) const;
private:
	bool PrepareComponentIfNeeded(IComponent& orderComponent, const double dOrderId) const;
	ESpType GetScratchpadType() const;

private:
	const HPATCON m_hPatCon;
};