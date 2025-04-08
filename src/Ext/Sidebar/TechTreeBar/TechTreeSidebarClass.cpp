#include "TechTreeSidebarClass.h"
#include <CommandClass.h>

#include <Ext/House/Body.h>
#include <Ext/Side/Body.h>
#include <Ext/SWType/Body.h>

TechTreeSidebarClass TechTreeSidebarClass::Instance;

bool TechTreeSidebarClass::IsEnabled()
{
	return ScenarioExt::Global()->TechTreeSidebar_Enable;
}
