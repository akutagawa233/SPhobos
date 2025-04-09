#pragma once
#include "TechTreeButtonClass.h"
#include "TechTreeColumnClass.h"
#include "ToggleTechTreeButtonClass.h"
#include <Ext/Scenario/Body.h>
#include <CommandClass.h>

class TechTreeSidebarClass 
{
	public:
		static TechTreeSidebarClass Instance;
		bool AddColumn();
		bool RemoveColumn();

		void InitClear();
		void InitIO();

		static bool IsEnabled();
	public:
		std::vector<TechTreeColumnClass*> Columns {};
		TechTreeColumnClass* CurrentColumn { nullptr };
		TechTreeButtonClass* CurrentButton { nullptr };
		ToggleTechTreeButtonClass* ToggleButton { nullptr };

		static CommandClass* Commands[10];
};
