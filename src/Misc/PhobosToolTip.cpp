#include <Helpers/Macro.h>

#include "PhobosToolTip.h"

#include <AircraftClass.h>
#include <BuildingClass.h>
#include <UnitClass.h>
#include <InfantryClass.h>

#include <GameOptionsClass.h>
#include <CCToolTip.h>
#include <BitFont.h>
#include <BitText.h>
#include <FPSCounter.h>
#include <Phobos.h>

#include <Ext/Side/Body.h>
#include <Ext/Surface/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/Sidebar/SWSidebar/SWSidebarClass.h>
#include <Ext/Sidebar/UniqueButton/UniqueTechnoColumnClass.h>
#include <Ext/Sidebar/SelectedButton/SelectedInfoClass.h>

#include <sstream>
#include <iomanip>

PhobosToolTip PhobosToolTip::Instance;

inline bool PhobosToolTip::IsEnabled() const
{
	return Phobos::UI::ExtendedToolTips;
}

inline const wchar_t* PhobosToolTip::GetUIDescription(TechnoTypeExt::ExtData* pData) const
{
	return Phobos::Config::ToolTipDescriptions && !pData->UIDescription.Get().empty()
		? pData->UIDescription.Get().Text
		: nullptr;
}

inline const wchar_t* PhobosToolTip::GetUnbuildableUIDescription(TechnoTypeExt::ExtData* pData) const
{
	return Phobos::Config::ToolTipDescriptions && !pData->UIDescription_Unbuildable.Get().empty()
		? pData->UIDescription_Unbuildable.Get().Text
		: nullptr;
}

inline const wchar_t* PhobosToolTip::GetUIDescription(SWTypeExt::ExtData* pData) const
{
	return Phobos::Config::ToolTipDescriptions && !pData->UIDescription.Get().empty()
		? pData->UIDescription.Get().Text
		: nullptr;
}

inline int PhobosToolTip::GetBuildTime(TechnoTypeClass* pType) const
{
	static char pTrick[0x6C8]; // Just big enough to hold all types
	switch (pType->WhatAmI())
	{
	case AbstractType::BuildingType:
		VTable::Set(pTrick, BuildingClass::AbsVTable);
		reinterpret_cast<BuildingClass*>(pTrick)->Type = (BuildingTypeClass*)pType;
		break;
	case AbstractType::AircraftType:
		VTable::Set(pTrick, AircraftClass::AbsVTable);
		reinterpret_cast<AircraftClass*>(pTrick)->Type = (AircraftTypeClass*)pType;
		break;
	case AbstractType::InfantryType:
		VTable::Set(pTrick, InfantryClass::AbsVTable);
		reinterpret_cast<InfantryClass*>(pTrick)->Type = (InfantryTypeClass*)pType;
		break;
	case AbstractType::UnitType:
		VTable::Set(pTrick, UnitClass::AbsVTable);
		reinterpret_cast<UnitClass*>(pTrick)->Type = (UnitTypeClass*)pType;
		break;
	}

	// TechnoTypeClass only has 4 final classes :
	// BuildingTypeClass, AircraftTypeClass, InfantryTypeClass and UnitTypeClass
	// It has to be these four classes, otherwise pType will just be nullptr
	reinterpret_cast<TechnoClass*>(pTrick)->Owner = HouseClass::CurrentPlayer;
	int nTimeToBuild = reinterpret_cast<TechnoClass*>(pTrick)->TimeToBuild();
	// 54 frames at least
	return std::max(54, nTimeToBuild);
}

inline int PhobosToolTip::GetPower(TechnoTypeClass* pType) const
{
	switch (pType->WhatAmI())
	{
	case AbstractType::AircraftType:
	case AbstractType::InfantryType:
	case AbstractType::UnitType:
		{
			if (!Phobos::Config::UnitPowerDrain)
				return 0;

			const auto pExt = TechnoTypeExt::ExtMap.Find(pType);
			return pExt->Power;
		}
	case AbstractType::BuildingType:
		{
			auto pBldType = (BuildingTypeClass*)pType;
			return pBldType->PowerBonus - pBldType->PowerDrain;
		}
	default:
		return 0;
	}
}

inline const wchar_t* PhobosToolTip::GetBuffer() const
{
	return this->TextBuffer.c_str();
}

void PhobosToolTip::HelpText(BuildType& cameo)
{
	if (cameo.ItemType == AbstractType::Special)
		this->HelpText_Super(cameo.ItemIndex);
	else
		this->HelpText_Techno(ObjectTypeClass::GetTechnoType(cameo.ItemType, cameo.ItemIndex));
}

inline static int TickTimeToSeconds(int tickTime)
{
	if (!Phobos::Config::RealTimeTimers)
		return tickTime / 15;

	if (Phobos::Config::RealTimeTimers_Adaptive
		|| GameOptionsClass::Instance.GameSpeed == 0
		|| (Phobos::Misc::CustomGS && !SessionClass::IsMultiplayer()))
	{
		return tickTime / std::max((int)FPSCounter::CurrentFrameRate, 1);
	}

	return tickTime / (60 / GameOptionsClass::Instance.GameSpeed);
}

void PhobosToolTip::HelpText_Techno(TechnoTypeClass* pType)
{
	if (!pType)
		return;

	auto const pData = TechnoTypeExt::ExtMap.Find(pType);

	int nBuildTime = TickTimeToSeconds(this->GetBuildTime(pType));
	int nSec = nBuildTime % 60;
	int nMin = nBuildTime / 60;

	int cost = pType->GetActualCost(HouseClass::CurrentPlayer);

	std::wostringstream oss;
	oss << pType->UIName << L"\n"
		<< (cost < 0 ? L"+" : L"")
		<< Phobos::UI::CostLabel << std::abs(cost) << L" "
		<< Phobos::UI::TimeLabel
		<< std::setw(2) << std::setfill(L'0') << nMin << L":"
		<< std::setw(2) << std::setfill(L'0') << nSec;

	if (auto const nPower = this->GetPower(pType))
	{
		oss << L" " << Phobos::UI::PowerLabel;
		if (nPower > 0)
			oss << L"+";
		oss << std::setw(1) << nPower;
	}

	if (auto pDesc = this->GetUIDescription(pData))
		oss << L"\n" << pDesc;

	if (pData->IsGreyCameoForCurrentPlayer)
	{
		if (auto pExDesc = this->GetUnbuildableUIDescription(pData))
			oss << L"\n" << pExDesc;
	}

	this->TextBuffer = oss.str();
}

void PhobosToolTip::HelpText_Super(int swidx)
{
	auto pSuper = HouseClass::CurrentPlayer->Supers.Items[swidx];
	auto const pData = SWTypeExt::ExtMap.Find(pSuper->Type);

	std::wostringstream oss;
	oss << pSuper->Type->UIName;
	bool showSth = false;

	if (int nCost = std::abs(pData->Money_Amount))
	{
		oss << L"\n";

		if (pData->Money_Amount > 0)
			oss << '+';

		oss << Phobos::UI::CostLabel << nCost;
		showSth = true;
	}

	int rechargeTime = TickTimeToSeconds(pSuper->GetRechargeTime());
	if (rechargeTime > 0)
	{
		if (!showSth)
			oss << L"\n";

		int nSec = rechargeTime % 60;
		int nMin = rechargeTime / 60;

		oss << (showSth ? L" " : L"") << Phobos::UI::TimeLabel
			<< std::setw(2) << std::setfill(L'0') << nMin << L":"
			<< std::setw(2) << std::setfill(L'0') << nSec;
		showSth = true;
	}

	auto const& sw_ext = HouseExt::ExtMap.Find(HouseClass::CurrentPlayer)->SuperExts[swidx];
	int sw_shots = pData->SW_Shots;
	int remain_shots = pData->SW_Shots - sw_ext.ShotCount;
	if (sw_shots > 0)
	{
		if (!showSth)
			oss << L"\n";

		wchar_t buffer[64];
		swprintf_s(buffer, Phobos::UI::SWShotsFormat, remain_shots, sw_shots);
		oss << (showSth ? L" " : L"") << buffer;
	}

	if (auto pDesc = this->GetUIDescription(pData))
		oss << L"\n" << pDesc;

	this->TextBuffer = oss.str();
}

// Hooks
//侧边栏_工具栏_帮助说明
DEFINE_HOOK(0x6A9316, SidebarClass_StripClass_HelpText, 0x6)
{
	PhobosToolTip::Instance.IsCameo = true;

	if (!PhobosToolTip::Instance.IsEnabled())//检测工具描述是否启用，不启用则直接继续执行源代码
		return 0;

	GET(StripClass*, pThis, EAX);
	PhobosToolTip::Instance.HelpText(pThis->Cameos[0]); // pStrip->Cameos[nID] in fact
	R->EAX(L"X");
	return 0x6A93DE;
}

//显示工具提醒的钩子,任何需要显示工具提示的地方都可以调用这个钩子
DEFINE_HOOK(0x4AE51E, DisplayClass_GetToolTip_HelpText, 0x6)
{
	enum { ApplyToolTip = 0x4AE69D };
	// 检查超武侧边栏是否已启用
	if (SWSidebarClass::IsEnabled())
	{
		// 如果当前有按钮被选中
		if (const auto button = SWSidebarClass::Instance.CurrentButton)
		{
			// 设置工具提示实例的标志为 cameo
			PhobosToolTip::Instance.IsCameo = true;

			// 检查 Phobos 工具提示是否已启用
			if (PhobosToolTip::Instance.IsEnabled())
			{
				// 调用方法设置帮助文本，并获取缓冲区地址
				PhobosToolTip::Instance.HelpText_Super(button->SuperIndex);
				R->EAX(PhobosToolTip::Instance.GetBuffer());
			}
			else
			{
				// 获取当前玩家的超级武器信息，并设置工具提示为超级武器的 UI 名称
				const auto pSuper = HouseClass::CurrentPlayer->Supers[button->SuperIndex];
				R->EAX(pSuper->Type->UIName);
			}

			return ApplyToolTip;
		}
		// 如果当前有列被选中或者切换按钮处于悬停状态
		else if (SWSidebarClass::Instance.CurrentColumn
			// 设置工具提示为空，并跳转到特定地址应用工具提示
			|| SWSidebarClass::Instance.ToggleButton && SWSidebarClass::Instance.ToggleButton->IsHovering)
		{
			R->EAX(0);
			return ApplyToolTip;
		}
	}

	// 检查独特单位栏是否有单位被悬停
	const auto uniqueIndex = UniqueTechnoColumnClass::Instance.Hovering;

	if (uniqueIndex >= 0)
	{
		// 获取全局拥有的独特单位向量
		auto& vec = ScenarioExt::Global()->OwnedUniqueTechnos;

		// 如果独特单位索引在向量范围内
		if (uniqueIndex < static_cast<int>(vec.size()))
		{
			// 设置工具提示为独特单位的 UI 名称，并跳转到特定地址应用工具提示
			R->EAX(vec[uniqueIndex]->TypeExtData->OwnerObject()->UIName);
			return ApplyToolTip;
		}
	}

	// 检查选定信息是否处于悬停状态
	if (SelectedInfoClass::Instance.IsHovering)
	{
		// 设置工具提示为空，并跳转到特定地址应用工具提示
		R->EAX(0);
		return ApplyToolTip;
	}

	// 如果以上条件都不满足，继续执行原代码
	return 0;
}

// TODO: reimplement CCToolTip::Draw2 completely

DEFINE_HOOK(0x478EE1, CCToolTip_Draw2_SetBuffer, 0x6)
{
	// 检查Phobos工具提示是否已启用，并且当前提示是否为Cameo类型
	if (PhobosToolTip::Instance.IsEnabled() && PhobosToolTip::Instance.IsCameo)
		// 如果是，则将EDI寄存器的值设置为Phobos工具提示的缓冲区地址
		R->EDI(PhobosToolTip::Instance.GetBuffer());
	return 0;
}

DEFINE_HOOK(0x478E10, CCToolTip_Draw1, 0x0)
{
	// 获取CCToolTip对象的指针，以便后续操作
	GET(CCToolTip*, pThis, ECX);
	// 从堆栈中获取是否进行完整重绘的标志
	GET_STACK(bool, bFullRedraw, 0x4);

	// !onSidebar or (onSidebar && ExtToolTip::IsCameo)
	// 根据条件判断是否需要重新创建CCToolTip对象
	if (!bFullRedraw || PhobosToolTip::Instance.IsCameo)
	{
		PhobosToolTip::Instance.IsCameo = false;
		PhobosToolTip::Instance.SlaveDraw = false;

		// 调用Process方法重新创建CCToolTip对象
		pThis->ToolTipManager::Process();	//this function re-create CCToolTip
	}

	// 如果当前存在ToolTip，则根据条件进行绘制
	if (pThis->CurrentToolTip)
	{
		// 如果不进行完整重绘，则根据IsCameo状态设置SlaveDraw标志
		if (!bFullRedraw)
			PhobosToolTip::Instance.SlaveDraw = PhobosToolTip::Instance.IsCameo;

		// 设置CCToolTip对象的FullRedraw标志
		pThis->FullRedraw = bFullRedraw;
		// 调用DrawText方法绘制当前的ToolTip
		pThis->DrawText(pThis->CurrentToolTipData);
	}
	return 0x478E25;
}

//设置外观
DEFINE_HOOK(0x478E4A, CCToolTip_Draw2_SetSurface, 0x6)
{
	// 检查是否需要使用从属绘制方式
	if (PhobosToolTip::Instance.SlaveDraw)
	{
		//设置绘制表面为复合表面
		R->ESI(DSurface::Composite);
		// 跳转到新的代码位置，以完成从属绘制
		return 0x478ED3;
	}
	return 0;
}

DEFINE_HOOK(0x478EF8, CCToolTip_Draw2_SetMaxWidth, 0x5)
{
	if (PhobosToolTip::Instance.IsCameo)
	{
		if (Phobos::UI::MaxToolTipWidth > 0)
			R->EAX(Phobos::UI::MaxToolTipWidth);
		else
			R->EAX(DSurface::ViewBounds.Width);

	}
	return 0;
}

DEFINE_HOOK(0x478F52, CCToolTip_Draw2_SetX, 0x8)
{
	if (PhobosToolTip::Instance.SlaveDraw)
		R->EAX(R->EAX() + DSurface::Sidebar->GetWidth());

	return 0;
}

DEFINE_HOOK(0x478F77, CCToolTip_Draw2_SetY, 0x6)
{
	if (PhobosToolTip::Instance.IsCameo)
	{
		LEA_STACK(RectangleStruct*, Rect, STACK_OFFSET(0x3C, -0x20));

		int const maxHeight = DSurface::ViewBounds.Height - 32;

		if (Rect->Height > maxHeight)
			Rect->Y += maxHeight - Rect->Height;

		if (Rect->Y < 0)
			Rect->Y = 0;
	}
	return 0;
}

// If tooltip rectangle width is constrained, make sure
// there is a padding zone so text isn't drawn into border
DEFINE_HOOK(0x479029, CCToolTip_Draw2_SetPadding, 0x5)
{
	if (PhobosToolTip::Instance.IsCameo)
	{
		if (Phobos::UI::MaxToolTipWidth > 0)
			R->EDX(R->EDX() - 5);
	}

	return 0;
}

void __declspec(naked) _CCToolTip_Draw2_FillRect_RET()
{
	ADD_ESP(8); // We need to handle origin two push here...
	JMP(0x478FE1);
}
DEFINE_HOOK(0x478FDC, CCToolTip_Draw2_FillRect, 0x5)
{
	GET(SurfaceExt*, pThis, ESI);
	LEA_STACK(RectangleStruct*, pRect, STACK_OFFSET(0x44, -0x10));

	const bool isCameo = PhobosToolTip::Instance.IsCameo;

	if (isCameo && Phobos::UI::AnchoredToolTips
		&& PhobosToolTip::Instance.IsEnabled()
		&& Phobos::Config::ToolTipDescriptions
		&& !SWSidebarClass::Instance.CurrentButton)
	{
		LEA_STACK(LTRBStruct*, a2, STACK_OFFSET(0x44, -0x20));
		auto x = DSurface::SidebarBounds.X - pRect->Width - 2;
		pRect->X = x;
		a2->Left = x;
		pRect->Y -= 40;
		a2->Top -= 40;
	}

	// Should we make some SideExt items as static to improve the effeciency?
	// Though it might not be a big improvement... - secsome
	const int nPlayerSideIndex = ScenarioClass::Instance->PlayerSideIndex;
	if (auto const pSide = SideClass::Array.GetItemOrDefault(nPlayerSideIndex))
	{
		if (auto const pData = SideExt::ExtMap.Find(pSide))
		{
			// Could this flag be lazy?
			if (isCameo)
				SidebarClass::Instance.SidebarBackgroundNeedsRedraw = true;

			pThis->FillRectTrans(pRect,
				pData->ToolTip_Background_Color.GetEx(&RulesExt::Global()->ToolTip_Background_Color),
				pData->ToolTip_Background_Opacity.Get(RulesExt::Global()->ToolTip_Background_Opacity)
			);

			if (Phobos::Config::ToolTipBlur)
				pThis->BlurRect(*pRect, pData->ToolTip_Background_BlurSize.Get(RulesExt::Global()->ToolTip_Background_BlurSize));

			return (int)_CCToolTip_Draw2_FillRect_RET;
		}
	}

	return 0;
}

// TODO in the future
//
//DEFINE_HOOK(0x478E30, CCToolTip_Draw2, 0x7)
//{
//	GET(CCToolTip*, pThis, ECX);
//	GET_STACK(ToolTipManagerData*, pManagerData, 0x4);
//
//	DSurface* pSurface = nullptr;
//
//	RectangleStruct bounds = pManagerData->Dimension;
//
//	if (GameOptionsClass::Instance.SidebarSide == 1)
//	{
//		int nR = DSurface::ViewBounds.X + DSurface::ViewBounds.Width;
//		if (bounds.X + pManagerData->Dimension.Width <= nR)
//			pSurface = DSurface::Composite;
//		else
//		{
//			if (!pThis->FullRedraw || bounds.X < nR)
//				return 0x479048;
//			pSurface = DSurface::Sidebar;
//			bounds.X -= nR;
//			*reinterpret_cast<bool*>(0xB0B518) = true;
//		}
//	}
//	else
//	{
//		int nR = DSurface::SidebarBounds.X + DSurface::SidebarBounds.Width;
//		if (bounds.X < nR)
//		{
//			if (!pThis->FullRedraw || bounds.X + pManagerData->Dimension.Width >= nR)
//				return 0x479048;
//			pSurface = DSurface::Sidebar;
//			*reinterpret_cast<bool*>(0xB0B518) = true;
//		}
//		else
//		{
//			pSurface = DSurface::Composite;
//			bounds.X -= nR;
//		}
//	}
//
//	if (pSurface)
//	{
//		BitFont::Instance->GetTextDimension(
//			PhobosToolTip::Instance.GetBuffer(), bounds.Width, bounds.Height,
//			Phobos::UI::MaxToolTipWidth > 0 ? Phobos::UI::MaxToolTipWidth : DSurface::WindowBounds.Width);
//
//		if (pManagerData->Dimension.Width + bounds.X > pSurface->GetWidth())
//			bounds.X = pSurface->GetWidth() - pManagerData->Dimension.Width;
//
//		bounds.Width = pManagerData->Dimension.Width;
//		bounds.Height += 4;
//
//		BitFont::Instance->field_41 = 1;
//		BitFont::Instance->SetBounds(&bounds);
//		BitFont::Instance->Color = static_cast<WORD>(Drawing::RGB2DWORD(191, 98, 10));
//
//		BitText::Instance->DrawText(BitFont::Instance, pSurface, PhobosToolTip::Instance.GetBuffer(),
//			bounds.X + 4, bounds.Y + 2, bounds.Width, bounds.Height, 0, 0, 0);
//	}
//
//	return 0x479048;
//}
