#include "TechTreeSidebarClass.h"
#include <CommandClass.h>

#include <Ext/House/Body.h>
#include <Ext/Side/Body.h>
#include <Ext/SWType/Body.h>

TechTreeSidebarClass TechTreeSidebarClass::Instance;
CommandClass* TechTreeSidebarClass::Commands[10];

bool TechTreeSidebarClass::AddColumn() {
	auto& columns = this->Columns;
	// 检查列数限制：当现有列数超过配置的最大值时拒绝添加
	if (static_cast<int>(columns.size()) >= Phobos::UI::TechTreeSidebar_MaxColumns)
		return false;
	// 计算当前剩余可用按钮空间：总容量减去已有列数占用的空间
	const int maxButtons = Phobos::UI::TechTreeSidebar_Max - static_cast<int>(columns.size());
	if (maxButtons <= 0)
		return false;
	// 创建新的列对象：包含按钮ID分配、最大按钮容量、初始坐标位置和尺寸参数
	const int cameoWidth = 60;
	const auto column = GameCreate<TechTreeColumnClass>(TechTreeButtonClass::StartID + SuperWeaponTypeClass::Array.Count + 1 + static_cast<int>(columns.size()), maxButtons, 0, 0, cameoWidth + Phobos::UI::TechTreeSidebar_Interval, Phobos::UI::TechTreeSidebar_CameoHeight);

	if (!column)
		return false;
	// 初始化列状态并添加到界面
	column->Zap();// 执行列对象初始化
	GScreenClass::Instance.AddButton(column);// 将列添加到游戏界面
	return true;
}

bool TechTreeSidebarClass::RemoveColumn()
{
	auto& columns = this->Columns;

	// 检查列容器是否为空
	if (columns.empty())
		return false;

	// 获取最后一列的指针并验证有效性
	if (const auto backColumn = columns.back())
	{
		// 处理与当前活动列的关联关系
		AnnounceInvalidPointer(TechTreeSidebarClass::Instance.CurrentColumn, backColumn);
		// 从屏幕界面移除关联的按钮控件
		GScreenClass::Instance.RemoveButton(backColumn);

		// 从容器中移除最后一列
		columns.erase(columns.end() - 1);
		return true;
	}
	return false;
}

void TechTreeSidebarClass::InitClear()
{
	// 重置当前操作的列和按钮
	this->CurrentColumn = nullptr;
	this->CurrentButton = nullptr;

	// 处理切换按钮：解除关联并从屏幕移除
	if (const auto toggleButton = this->ToggleButton)
	{
		this->ToggleButton = nullptr;
		GScreenClass::Instance.RemoveButton(toggleButton);
	}

	auto& columns = this->Columns;

	// 遍历清理所有列对象：先清除按钮再从屏幕移除
	for (const auto column : columns)
	{
		column->ClearButtons();
		GScreenClass::Instance.RemoveButton(column);
	}

	// 清空列容器
	columns.clear();
}

void TechTreeSidebarClass::InitIO() {
	// 检查全局开关：侧边栏功能未启用或处于末日模式时直接返回
	if (!Phobos::UI::TechTreeSidebar || Unsorted::ArmageddonMode)
		return;
	// 获取当前玩家阵营的扩展数据
	if (const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.Items[ScenarioClass::Instance->PlayerSideIndex]))
	{
		// 从阵营扩展数据中加载按钮状态贴图资源
		const auto pOnPCX = pSideExt->SuperWeaponSidebar_OnPCX.GetSurface();
		const auto pOffPCX = pSideExt->SuperWeaponSidebar_OffPCX.GetSurface();
		int width = 0, height = 0;

		/* 计算按钮尺寸逻辑：
		   1. 同时存在启用/禁用贴图时取最大尺寸
		   2. 只有单一贴图时使用其实际尺寸
		   3. 没有有效贴图时保持默认尺寸 */
		if (pOnPCX)
		{
			if (pOffPCX)
			{
				width = std::max(pOnPCX->GetWidth(), pOffPCX->GetWidth());
				height = std::max(pOnPCX->GetHeight(), pOffPCX->GetHeight());
			}
			else
			{
				width = pOnPCX->GetWidth();
				height = pOnPCX->GetHeight();
			}
		}
		else if (pOffPCX)
		{
			width = pOffPCX->GetWidth();
			height = pOffPCX->GetHeight();
		}
		// 创建并配置切换按钮控件
		if (width > 0 && height > 0)
		{
			if (const auto toggleButton = GameCreate<ToggleTechTreeButtonClass>(TechTreeButtonClass::StartID + SuperWeaponTypeClass::Array.Count, 0, 0, width, height))
			{
				toggleButton->Zap(); // 重置按钮状态
				GScreenClass::Instance.AddButton(toggleButton);// 添加到游戏界面
				TechTreeSidebarClass::Instance.ToggleButton = toggleButton;// 存储实例引用
				toggleButton->UpdatePosition();// 更新屏幕位置
			}
		}
	}
}

bool TechTreeSidebarClass::IsEnabled()
{
	return ScenarioExt::Global()->TechTreeSidebar_Enable;
}


