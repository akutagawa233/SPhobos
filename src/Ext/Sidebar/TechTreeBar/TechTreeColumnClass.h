#pragma once
#include "TechTreeButtonClass.h"
#include <ControlClass.h>

#include <vector>

class TechTreeColumnClass :	public ControlClass
{
public:
	TechTreeColumnClass() = default;
	TechTreeColumnClass(unsigned int id, int maxButtons, int x, int y, int width, int height);
	~TechTreeColumnClass() = default;

private:

};

