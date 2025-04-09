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

	void ClearButtons(bool remove = true);

	std::vector<TechTreeButtonClass*> Buttons {};
	int MaxButtons { 0 };
};

