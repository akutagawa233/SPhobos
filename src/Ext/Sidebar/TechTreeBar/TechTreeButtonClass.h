#pragma once
#include <ControlClass.h>

class TechTreeButtonClass : public ControlClass
{
    public:
		TechTreeButtonClass() = default;
		TechTreeButtonClass(unsigned int id, int superIdx, int x, int y, int width, int height);

		~TechTreeButtonClass() = default;

		virtual bool Draw(bool forced) override;
		virtual void OnMouseEnter() override;
		virtual void OnMouseLeave() override;
		virtual bool Action(GadgetFlag fags, DWORD* pKey, KeyModifier modifier) override;

		bool LaunchSuper() const;
		void SetColumn(int column);
	public:
		static constexpr int StartID = 3000;

		int SuperIndex { -1 };
		bool IsHovering { false };
		int ColumnIndex { -1 };
};
