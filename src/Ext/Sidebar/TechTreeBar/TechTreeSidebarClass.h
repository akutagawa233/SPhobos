#pragma once
#include "TechTreeButtonClass.h"
#include "TechTreeColumnClass.h"
//#include "ToggleSWButtonClass.h"
#include <Ext/Scenario/Body.h>
#include <CommandClass.h>

class TechTreeSidebarClass 
{
	public:
		static TechTreeSidebarClass Instance;

		static bool IsEnabled();
	public:
		std::vector<TechTreeColumnClass*> Columns {};
		TechTreeButtonClass* CurrentButton { nullptr };
};
