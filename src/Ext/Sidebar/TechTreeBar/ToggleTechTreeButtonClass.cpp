#include "ToggleTechTreeButtonClass.h"
#include "TechTreeSidebarClass.h"
#include <CCToolTip.h>
#include <GameOptionsClass.h>
#include <ScenarioClass.h>

#include <Ext/Side/Body.h>

ToggleTechTreeButtonClass::ToggleTechTreeButtonClass(unsigned int id, int x, int y, int width, int height)
	: ControlClass(id, x, y, width, height, (GadgetFlag::LeftPress | GadgetFlag::LeftRelease), false)
{
	TechTreeSidebarClass::Instance.ToggleButton = this;
}

void ToggleTechTreeButtonClass::UpdatePosition()
{
	Point2D position = Point2D::Empty;
	auto& columns = TechTreeSidebarClass::Instance.Columns;

	if (!columns.empty())
	{
		const auto backColumn = columns.back();
		position.X = TechTreeSidebarClass::Instance.IsEnabled() ? backColumn->X + backColumn->Width : 0;
		position.Y = backColumn->Y + (backColumn->Height - this->Height) / 2;
	}
	else
	{
		position.X = 0;
		position.Y = (GameOptionsClass::Instance.ScreenHeight - this->Height) / 2;
	}

	this->SetPosition(position.X, position.Y);

}
