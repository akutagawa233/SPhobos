#include "TechTreeButtonClass.h"
#include "TechTreeSidebarClass.h"
#include <EventClass.h>
#include <CCToolTip.h>
#include <CommandClass.h>
#include <UI.h>

#include <Ext/SWType/Body.h>
#include <Utilities/AresFunctions.h>

TechTreeButtonClass::TechTreeButtonClass(unsigned int id, int superIdx, int x, int y, int width, int height)
	: ControlClass(id, x, y, width, height, (GadgetFlag::LeftPress | GadgetFlag::RightPress), false)
	, SuperIndex(superIdx)
{
	// 将当前按钮添加到侧边栏最后一列的按钮集合
	//if (const auto backColumn = TechTreeSidebarClass::Instance.Columns.back())
		//backColumn->Buttons.emplace_back(this);

	// 根据侧边栏全局启用状态设置按钮禁用状态
	this->Disabled = !TechTreeSidebarClass::IsEnabled();
}

bool TechTreeButtonClass::Draw(bool forced)
{
	// 非强制绘制时直接返回
	if (!forced)
		return false;
	// 获取绘制表面和基础坐标信息
	const auto pSurface = DSurface::Composite;
	auto bounds = pSurface->GetRect();
	Point2D location = { this->X, this->Y };
	RectangleStruct destRect = { location.X, location.Y, this->Width, this->Height };

	// 获取当前玩家和超级武器相关数据
	const auto pCurrent = HouseClass::CurrentPlayer;
	const auto pSuper = pCurrent->Supers[this->SuperIndex];
	const auto pSWExt = SWTypeExt::ExtMap.Find(pSuper->Type);
	// 处理不同类型的图标绘制
	// 优先使用PCX格式的新式图标
	// support for pcx cameos
	if (const auto pPCXCameo = pSWExt->SidebarPCX.GetSurface())
	{
		PCX::Instance.BlitToSurface(&destRect, pSurface, pPCXCameo);
	}
	// 回退到SHP格式的旧式图标
	else if (const auto pCameo = pSuper->Type->SidebarImage) // old shp cameos, fixed palette
	{
		// 图标格式转换和后备处理逻辑
		const auto pCameoRef = pCameo->AsReference();
		char pFilename[0x20];
		strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
		_strlwr_s(pFilename);

		if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
		{
			PCX::Instance.LoadFile(pFilename);

			if (const auto CameoPCX = PCX::Instance.GetSurface(pFilename))
				PCX::Instance.BlitToSurface(&destRect, pSurface, CameoPCX);
		}
		else
		{
			const auto pConvert = pSWExt->SidebarPal.Convert ? pSWExt->SidebarPal.GetConvert() : FileSystem::CAMEO_PAL;
			pSurface->DrawSHP(pConvert, pCameo, 0, &location, &bounds, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
		}
	}
	// 绘制悬停状态边框
	if (this->IsHovering)
	{
		// 绘制高亮矩形框
		RectangleStruct cameoRect = { location.X, location.Y, this->Width, this->Height };
		const COLORREF tooltipColor = Drawing::RGB_To_Int(Drawing::TooltipColor);
		pSurface->DrawRect(&cameoRect, tooltipColor);
	}
	// 检查禁用条件并绘制暗化效果
	if (pSuper->IsReady && !pCurrent->CanTransactMoney(pSWExt->Money_Amount) ||
		(pSWExt->SW_UseAITargeting && AresFunctions::IsTargetConstraintsEligible && !AresFunctions::IsTargetConstraintsEligible(AresFunctions::SWTypeExtMap_Find(pSuper->Type), HouseClass::CurrentPlayer, true)))
	{
		// 绘制禁用状态的半透明遮罩
		RectangleStruct darkenBounds { 0, 0, location.X + this->Width, location.Y + this->Height };
		pSurface->DrawSHP(FileSystem::SIDEBAR_PAL, FileSystem::DARKEN_SHP, 0, &location, &darkenBounds, BlitterFlags::bf_400 | BlitterFlags::Darken, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
	}
	// 判断准备状态并处理显示逻辑
	const bool ready = !pSuper->IsSuspended && (pSuper->Type->UseChargeDrain ? pSuper->ChargeDrainState == ChargeDrainState::Ready : pSuper->IsReady);
	bool drawReadiness = true;
	// 绘制准备状态文本
	if (drawReadiness)
	{
		// 绘制"Ready"等状态文本
		if (const auto buffer = pSuper->NameReadiness())
		{
			Point2D textLoc = { location.X + this->Width / 2, location.Y };
			const COLORREF foreColor = Drawing::RGB_To_Int(Drawing::TooltipColor);
			constexpr TextPrintType printType = TextPrintType::FullShadow | TextPrintType::Point8 | TextPrintType::Background | TextPrintType::Center;

			pSurface->DrawTextA(buffer, &bounds, &textLoc, foreColor, 0, printType);
		}
	}

	// 绘制进度动画指示
	if (pSuper->ShouldDrawProgress())
	{
		// 绘制进度时钟动画
		Point2D loc = { location.X, location.Y };
		pSurface->DrawSHP(FileSystem::SIDEBAR_PAL, FileSystem::GCLOCK2_SHP, pSuper->AnimStage() + 1, &loc, &bounds, BlitterFlags::bf_400 | BlitterFlags::TransLucent50, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
	}
	return true;
}

void TechTreeButtonClass::OnMouseEnter()
{
	// 检查侧边栏是否处于可用状态
	if (!TechTreeSidebarClass::IsEnabled())
		return;

	// 更新按钮悬停状态和侧边栏当前按钮记录
	this->IsHovering = true;
	TechTreeSidebarClass::Instance.CurrentButton = this;
	// 触发对应列的鼠标进入动画效果（根据按钮所在的列索引）
	TechTreeSidebarClass::Instance.Columns[this->ColumnIndex]->OnMouseEnter();
	// 立即显示工具提示（通过保存原始延迟时间并设置0延迟实现）
	CCToolTip::Instance->SaveTimerDelay();
	CCToolTip::Instance->SetTimerDelay(0);
}

void TechTreeButtonClass::OnMouseLeave()
{
	// 更新按钮悬停状态
	this->IsHovering = false;
	// 清除侧边栏记录的当前悬停按钮
	TechTreeSidebarClass::Instance.CurrentButton = nullptr;
	// 通知所属列处理鼠标离开事件（列级别的悬停状态更新等）
	TechTreeSidebarClass::Instance.Columns[this->ColumnIndex]->OnMouseLeave();
	// 恢复工具提示控件的默认显示延迟（取消可能的临时延迟设置）
	CCToolTip::Instance->RestoreTimeDelay();
}

bool TechTreeButtonClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	// 有效性检查：侧边栏未启用时直接返回
	if (!TechTreeSidebarClass::IsEnabled())
		return false;

	// 右键按下处理：清除当前选择的超级武器类型
	if (flags & GadgetFlag::RightPress)
		DisplayClass::Instance.CurrentSWTypeIndex = -1;

	// 左键按下处理：执行标准操作流程
	if (flags & GadgetFlag::LeftPress)
	{
		MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
		VocClass::PlayGlobal(RulesClass::Instance->GUIBuildSound, 0x2000, 1.0);
		this->LaunchSuper();
	}

	// 通过函数指针调用基类ControlClass的Action方法
	// this->ControlClass::Action(flags, pKey, KeyModifier::None);
	reinterpret_cast<bool(__thiscall*)(ControlClass*, GadgetFlag, DWORD*, KeyModifier)>(0x48E5A0)(this, flags, pKey, KeyModifier::None);
	return true;
}

void TechTreeButtonClass::SetColumn(int column)
{
	this->ColumnIndex = column;
}

bool TechTreeButtonClass::LaunchSuper() const
{
	// 获取当前玩家及超级武器数据
	const auto pCurrent = HouseClass::CurrentPlayer;
	const auto pSuper = pCurrent->Supers[this->SuperIndex];
	const auto pSWExt = SWTypeExt::ExtMap.Find(pSuper->Type);
	// 判断特殊发射模式
	const bool manual = !pSWExt->SW_ManualFire && pSWExt->SW_AutoFire;
	const bool unstoppable = pSuper->Type->UseChargeDrain && pSuper->ChargeDrainState == ChargeDrainState::Draining && pSWExt->SW_Unstoppable;

	// 基础发射条件检查
	if (!pSuper->CanFire() && !manual)
	{
		VoxClass::PlayIndex(pSWExt->EVA_Impatient);
		return false;
	}

	// 资金检查处理
	if (!pCurrent->CanTransactMoney(pSWExt->Money_Amount))
	{
		VoxClass::PlayIndex(pSWExt->EVA_InsufficientFunds);// 资金不足语音提示
		pSWExt->PrintMessage(pSWExt->Message_InsufficientFunds, pCurrent);
	}
	// 目标约束条件检查
	else if (!pSWExt->SW_UseAITargeting || (AresFunctions::IsTargetConstraintsEligible && AresFunctions::IsTargetConstraintsEligible(AresFunctions::SWTypeExtMap_Find(pSuper->Type), HouseClass::CurrentPlayer, true)))
	{
		// 常规发射流程
		if (!manual && !unstoppable)
		{
			const auto swIndex = pSuper->Type->ArrayIndex;

			// AI自动选择目标模式
			if (pSuper->Type->Action == Action::None || pSWExt->SW_UseAITargeting)
			{
				// 创建并提交特殊事件
				EventClass Event = EventClass(pCurrent->ArrayIndex, EventType::SpecialPlace, swIndex, CellStruct::Empty);
				EventClass::AddEvent(Event);
			}
			// 手动选择目标模式
			else
			{
				// 重置UI状态
				DisplayClass::Instance.CurrentBuilding = nullptr;
				DisplayClass::Instance.CurrentBuildingType = nullptr;
				DisplayClass::Instance.CurrentBuildingOwnerArrayIndex = -1;
				DisplayClass::Instance.SetActiveFoundation(nullptr);
				MapClass::Instance.SetRepairMode(0);
				MapClass::Instance.SetSellMode(0);
				DisplayClass::Instance.PowerToggleMode = false;
				DisplayClass::Instance.PlanningMode = false;
				DisplayClass::Instance.PlaceBeaconMode = false;
				// 设置超级武器选择状态
				DisplayClass::Instance.CurrentSWTypeIndex = swIndex;
				MapClass::Instance.UnselectAll();
				VoxClass::PlayIndex(pSWExt->EVA_SelectTarget);// 播放目标选择提示
			}

			return true;
		}
	}
	else
	{
		// 目标约束条件不满足提示
		pSWExt->PrintMessage(pSWExt->Message_CannotFire, pCurrent);
	}

	return false;
}
