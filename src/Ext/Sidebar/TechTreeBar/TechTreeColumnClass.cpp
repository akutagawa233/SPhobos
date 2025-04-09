#include "TechTreeColumnClass.h"
#include "TechTreeSidebarClass.h"

#include <Ext/SWType/Body.h>
#include <Ext/Side/Body.h>

TechTreeColumnClass::TechTreeColumnClass(unsigned int id, int maxButtons, int x, int y, int width, int height)
	: ControlClass(id, x, y, width, height, static_cast<GadgetFlag>(0), false)
	, MaxButtons(maxButtons)
{
	//TechTreeColumnClass::Instance.Columns.emplace_back(this);
	//this->Disabled = !TechTreeColumnClass::IsEnabled();
}

void TechTreeColumnClass::ClearButtons(bool remove)
{
	auto& buttons = this->Buttons;

	if (remove)
	{
		for (const auto button : buttons)
			GScreenClass::Instance.RemoveButton(button);
	}

	buttons.clear();
}
