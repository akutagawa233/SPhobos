#include "UniqueTechnoButtonClass.h"
#include "UniqueTechnoColumnClass.h"

#include <FactoryClass.h>
#include <TacticalClass.h>

#include <Ext/Scenario/Body.h>

UniqueTechnoButtonClass::UniqueTechnoButtonClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 60, 48, GadgetFlag::LeftPress, false)
{
	this->Disabled = !UniqueTechnoColumnClass::Instance.Visible;
}

bool UniqueTechnoButtonClass::Draw(bool forced)
{
	//如果特殊单位列表不可见，则不绘制按钮
	if (!UniqueTechnoColumnClass::Instance.Visible)
		return false;

	// 获取拥有的独特科技列表
	auto& vec = ScenarioExt::Global()->OwnedUniqueTechnos;

	// 如果列表为空，则不绘制按钮
	if (vec.empty())
		return false;

	// 计算按钮对应的独特科技索引
	const int index = this->ID - UniqueTechnoColumnClass::StartID;

	// 如果索引超出范围，则不绘制按钮
	if (index >= static_cast<int>(vec.size()))
		return false;

	// 获取独特科技的扩展数据和相关对象
	const auto pExt = vec[index];
	const auto pTechno = pExt->OwnerObject();
	const auto pTypeExt = pExt->TypeExtData;
	const auto pType = pTypeExt->OwnerObject();

	// 设置按钮的位置和绘制区域
	Point2D position { this->X, this->Y };
	RectangleStruct drawRect { this->X, this->Y, 60, 48 };

	// 优先绘制科技的PCX图标
	if (const auto CameoPCX = pTypeExt->CameoPCX.GetSurface())
	{
		PCX::Instance.BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
	}
	// 如果没有PCX图标，则绘制SHP图标
	else if (const auto pSHP = pType->GetCameo())
	{
		// 定义一个获取缺失图标的方法
		auto getMissingCameo = [pSHP]() -> BSurface*
		{
			const auto pCameoRef = pSHP->AsReference();
			char pFilename[0x20];
			strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
			_strlwr_s(pFilename);

			if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
			{
				PCX::Instance.LoadFile(pFilename);

				if (const auto MissingCameoPCX = PCX::Instance.GetSurface(pFilename))
					return MissingCameoPCX;
			}

			return nullptr;
		};

		// 如果有缺失的PCX图标，则绘制它
		if (const auto MissingCameoPCX = getMissingCameo())
		{
			PCX::Instance.BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
		}
		else
		{
			// 否则绘制SHP图标
			RectangleStruct rect { 0, 0, position.X + 60, position.Y + 48 };
			DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
				BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}
	}

	// 如果科技对象不在 Limbo 状态，则根据其状态绘制相应的颜色覆盖
	if (!pTechno->InLimbo)
	{
		const auto pRules = RulesClass::Instance;
		auto ratio = pTechno->GetHealthPercentage();

		// 根据科技对象的不同状态，绘制不同的颜色覆盖
		if (pTechno->IsIronCurtained())
		{
			ColorStruct fillColor { 50, 50, 50 };
			DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20);
		}
		else
		{
			int time = Unsorted::CurrentFrame - pExt->LastHurtFrame;

			// 根据健康比例绘制不同颜色和透明度的覆盖
			if (ratio < pRules->ConditionRed)
			{
				ColorStruct fillColor { 255, 0, 0 };
				int trans = 40 - time;

				if (trans < 0)
				{
					const int round = time % 60;
					trans = ((round <= 20) ? 0 : ((round <= 40) ? (round - 20) : (60 - round)));
				}

				if (trans > 0)
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
			}
			else if (ratio < pRules->ConditionYellow)
			{
				ColorStruct fillColor { 255, 0, 0 };
				int trans = 30 - time;

				if (trans < 0)
				{
					const int round = time % 160;
					trans = ((round <= 140) ? 0 : ((round <= 150) ? (round - 140) : (160 - round)));
				}

				if (trans > 0)
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
			}
			else if (time < 20)
			{
				ColorStruct fillColor { 255, 0, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, (20 - time));
			}

			time = Unsorted::CurrentFrame - pTechno->LastFireBulletFrame;

			if (time < 20)
			{
				ColorStruct fillColor { 255, 255, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20 - time);
			}

			// 根据特殊状态绘制颜色覆盖
			if (pTechno->TemporalTargetingMe)
			{
				ColorStruct fillColor { 100, 100, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pTechno->AirstrikeTintStage)
			{
				ColorStruct fillColor { 255, 50, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pTechno->DrainingMe || pTechno->LocomotorSource)
			{
				ColorStruct fillColor { 200, 0, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pTechno->IsUnderEMP() || pTechno->Deactivated)
			{
				ColorStruct fillColor { 128, 128, 128 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
		}

		// 如果科技对象有关联的 bunkered 物品且不是建筑，则绘制相应的矩形
		if (pTechno->BunkerLinkedItem && pTechno->WhatAmI() != AbstractType::Building)
		{
			RectangleStruct rect { (position.X + 3), (position.Y + 1), 54, 7 };
			DSurface::Composite->DrawRect(&rect, 0x781F);
		}

		// 绘制健康条的背景和前景
		RectangleStruct rect { (position.X + 4), (position.Y + 2), 52, 5 };
		DSurface::Composite->FillRect(&rect, 0);

		++rect.X;
		++rect.Y;
		rect.Width = static_cast<int>(50 * ratio + 0.5);
		rect.Height = 3;

		const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
		DSurface::Composite->FillRect(&rect, color);

		// 如果有护盾，则绘制护盾条
		const auto pShield = pExt->Shield.get();

		if (pShield && !pShield->IsBrokenAndNonRespawning())
		{
			ratio = (static_cast<double>(pShield->GetHP()) / pShield->GetType()->Strength.Get());
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			ColorStruct fillColor { 153, 153, 255 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}

		// 如果处于铁幕状态，则绘制铁幕条
		if (pTechno->IsIronCurtained())
		{
			const auto& timer = pTechno->IronCurtainTimer;
			ratio = static_cast<double>(timer.GetTimeLeft()) / timer.TimeLeft;
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			ColorStruct fillColor { 200, 50, 50 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}
	}

	// 如果科技对象在 Limbo 状态，则绘制其选择的 transporter 的状态
	else if (auto pSelect = pTechno->Transporter)
	{
		// 遍历运输链以找到最后一个运输单位
		for (auto pTrans = pSelect; pTrans; pTrans = pTrans->Transporter)
			pSelect = pTrans;

		// 获取扩展数据和规则实例
		const auto pSelectExt = TechnoExt::ExtMap.Find(pSelect);
		const auto pRules = RulesClass::Instance;
		auto ratio = pTechno->GetHealthPercentage();

		// 根据选择的 transporter 的状态绘制颜色覆盖
		if (pSelect->IsIronCurtained())
		{
			ColorStruct fillColor { 50, 50, 50 };
			DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20);
		}
		else
		{
			int time = Unsorted::CurrentFrame - pSelectExt->LastHurtFrame;

			// 根据单位受伤状态绘制颜色
			if (ratio < pRules->ConditionRed)
			{
				ColorStruct fillColor { 255, 0, 0 };
				int trans = 40 - time;

				if (trans < 0)
				{
					const int round = time % 60;
					trans = ((round <= 20) ? 0 : ((round <= 40) ? (round - 20) : (60 - round)));
				}

				if (trans > 0)
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
			}
			else if (ratio < pRules->ConditionYellow)
			{
				ColorStruct fillColor { 255, 0, 0 };
				int trans = 30 - time;

				if (trans < 0)
				{
					const int round = time % 160;
					trans = ((round <= 140) ? 0 : ((round <= 150) ? (round - 140) : (160 - round)));
				}

				if (trans > 0)
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
			}
			
			else if (time < 20)
			{
				ColorStruct fillColor { 255, 0, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, (20 - time));
			}

			time = Unsorted::CurrentFrame - pSelect->LastFireBulletFrame;
			// 根据单位开火状态绘制颜色
			if (time < 20)
			{
				ColorStruct fillColor { 255, 255, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20 - time);
			}
			// 根据特殊状态绘制颜色
			if (pSelect->TemporalTargetingMe)
			{
				ColorStruct fillColor { 100, 100, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pSelect->LocomotorSource)
			{
				ColorStruct fillColor { 200, 0, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pSelect->IsUnderEMP() || pSelect->Deactivated)
			{
				ColorStruct fillColor { 128, 128, 128 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
		}

		// 绘制单位的生命条
		RectangleStruct rect { (position.X + 3), (position.Y + 1), 0, 7 };

		if (pSelect->BunkerLinkedItem && pSelect->WhatAmI() != AbstractType::Building)
		{
			rect.Width = 54;
			DSurface::Composite->DrawRect(&rect, 0x781F);
		}
		else
		{
			rect.Width = static_cast<int>(54 * pSelect->GetHealthPercentage() + 0.5);
			DSurface::Composite->DrawRect(&rect, 0xFB20);
		}

		++rect.X;
		++rect.Y;
		rect.Width = 52;
		rect.Height = 5;

		DSurface::Composite->FillRect(&rect, 0);

		++rect.X;
		++rect.Y;
		rect.Width = static_cast<int>(50 * ratio + 0.5);
		rect.Height = 3;

		const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
		DSurface::Composite->FillRect(&rect, color);

		// 绘制护盾状态
		const auto pShield = pSelectExt->Shield.get();

		if (pShield && !pShield->IsBrokenAndNonRespawning())
		{
			ratio = (static_cast<double>(pShield->GetHP()) / pShield->GetType()->Strength.Get());
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			ColorStruct fillColor { 153, 153, 255 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}

		// 绘制铁幕状态
		if (pSelect->IsIronCurtained())
		{
			const auto& timer = pTechno->IronCurtainTimer;
			ratio = static_cast<double>(timer.GetTimeLeft()) / timer.TimeLeft;
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			ColorStruct fillColor { 200, 50, 50 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}
	}
	else
	{
		// 处理非运输单位的绘制逻辑
		const auto absType = pTechno->WhatAmI();
		const auto buildCat = (absType == AbstractType::Building) ? static_cast<BuildingClass*>(pTechno)->Type->BuildCat : BuildCat::DontCare;
		const auto pFactory = pTechno->Owner->GetPrimaryFactory(absType, pType->Naval, buildCat);

		if (pFactory && pFactory->Object == pTechno)
		{
			ColorStruct fillColor { 0, 0, 0 };
			DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 30);

			RectangleStruct rect { (position.X + 4), (position.Y + 2), 52, 5 };
			DSurface::Composite->FillRect(&rect, 0);

			const auto ratio = static_cast<double>(pFactory->GetProgress()) / 54;
			rect = RectangleStruct { (position.X + 5), (position.Y + 3), static_cast<int>(50 * ratio), 3 };
			DSurface::Composite->FillRect(&rect, 0xFFFF);
		}
		else
		{
			RectangleStruct rect { (position.X + 3), (position.Y + 1), 54, 7 };
			DSurface::Composite->DrawRect(&rect, 0x781F);

			++rect.X;
			++rect.Y;
			rect.Width = 52;
			rect.Height = 5;

			DSurface::Composite->FillRect(&rect, 0);

			const auto ratio = pTechno->GetHealthPercentage();

			++rect.X;
			++rect.Y;
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			rect.Height = 3;

			const auto pRules = RulesClass::Instance;
			const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
			DSurface::Composite->FillRect(&rect, color);
		}
	}

	if (this->Hovering)
	{
		RectangleStruct rect { 0, 0, position.X + 60, position.Y + 48 };
		DSurface::Composite->DrawRectEx(&rect, &drawRect, Drawing::RGB_To_Int(Drawing::TooltipColor));
	}

	return true;
}

void UniqueTechnoButtonClass::OnMouseEnter()
{
	// 特殊单位列表不可见时不做处理
	if (!UniqueTechnoColumnClass::Instance.Visible)
		return;

	// 获取全局拥有的特殊单位容器
	auto& vec = ScenarioExt::Global()->OwnedUniqueTechnos;
	// 计算当前按钮在特殊单位列表中的索引位置
	const int index = this->ID - UniqueTechnoColumnClass::StartID;

	// 索引越界保护
	if (index >= static_cast<int>(vec.size()))
		return;

	// 更新悬停状态
	this->Hovering = true;
	// 设置全局悬停索引用于高亮显示
	UniqueTechnoColumnClass::Instance.Hovering = index;
	// 强制更新鼠标光标显示（保持默认样式）
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void UniqueTechnoButtonClass::OnMouseLeave()
{
	// 更新按钮自身及关联列的悬停状态
	this->Hovering = false;
	UniqueTechnoColumnClass::Instance.Hovering = -1;
	// 将鼠标光标恢复为系统默认样式
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

bool UniqueTechnoButtonClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	// 前置条件检查：列不可见或非左键点击事件时直接返回
	if (!UniqueTechnoColumnClass::Instance.Visible || !(flags & GadgetFlag::LeftPress))
		return false;

	// 获取全局拥有的特殊单位容器
	auto& vec = ScenarioExt::Global()->OwnedUniqueTechnos;
	// 计算当前按钮在特殊单位列表中的索引位置
	const int index = this->ID - UniqueTechnoColumnClass::StartID;

	// 索引有效性检查
	if (index >= static_cast<int>(vec.size()))
		return false;

	// 播放全局界面音效
	VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
	// 获取对应科技的对象指针
	auto pSelect = vec[index]->OwnerObject();

	// 遍历运输链获取最终运输工具（顶层运输载具）
	for (auto pTrans = pSelect->Transporter; pTrans; pTrans = pTrans->Transporter)
		pSelect = pTrans;

	// 选择逻辑处理：当没有选中或未选中当前单位时取消所有选择
	if (ObjectClass::CurrentObjects.Count != 1 || !pSelect->IsSelected)
		MapClass::UnselectAll();

	// 单位状态检查：不在待命状态且选中失败时聚焦到单位位置
	if (!pSelect->InLimbo && !pSelect->Select())
		TacticalClass::Instance->SetTacticalPosition(&pSelect->Location);

	// 调用基类ControlClass的默认处理逻辑（通过硬编码地址调用）
	reinterpret_cast<bool(__thiscall*)(ControlClass*, GadgetFlag, DWORD*, KeyModifier)>(0x48E5A0)(this, flags, pKey, KeyModifier::None);
	return true;
}
