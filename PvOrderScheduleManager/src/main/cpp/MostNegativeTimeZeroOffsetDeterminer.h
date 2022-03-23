#pragma once

// Forward Declarations
struct IComponent;

class CMostNegativeTimeZeroOffsetDeterminer
{
public:
	bool DoesComponentHaveMostNegativeTimeZeroOffset(IComponent& component) const;
};