#pragma once

// Forward Declarations
struct IComponent;

class CComponentOffsetChangeHandler
{
public:
	CComponentOffsetChangeHandler(const HPATCON hPatCon);

	bool UpdateScheduleOnComponentOffsetChange(IComponent& component);

private:
	bool UpdateScheduleOnComponentOffsetChangeForOrderComponent(IComponent& component);
	bool UpdateScheduleOnComponentOffsetChangeForOutcomeComponent(IComponent& component);
	bool UpdateScheduleOnComponentOffsetChangeForSubPhaseComponent(IComponent& component);

private:
	const HPATCON m_hPatCon;
};