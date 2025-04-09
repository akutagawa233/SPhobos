#pragma once
#include <ControlClass.h>
class ToggleTechTreeButtonClass : public ControlClass
{
    public:
		ToggleTechTreeButtonClass() = default;
		ToggleTechTreeButtonClass(unsigned int id, int x, int y, int width, int height);

		~ToggleTechTreeButtonClass() = default;

		void UpdatePosition();

	public:
		bool IsHovering { false };
		bool IsPressed { false };
};
