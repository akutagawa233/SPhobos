#include "ExtrasClass.h"

#include <Ext/Anim/Body.h>
#include <Ext/Rules/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/WarheadType/Body.h>

#include <Utilities/GeneralUtils.h>
#include <AnimClass.h>
#include <HouseClass.h>
#include <RadarEventClass.h>
#include <TacticalClass.h>

std::vector<ExtrasClass*> ExtrasClass::Array;

ExtrasClass::ExtrasClass()
	: Techno { nullptr }
{
	// 将新创建的额外属性实例添加到全局管理数组
	ExtrasClass::Array.emplace_back(this);
}

ExtrasClass::ExtrasClass(TechnoClass* pTechno, bool isAttached)
	: Techno { pTechno }
{
	// 初始化额外属性的属性
	this->UpdateType(); // 从规则文件加载护盾类型配置
	this->TechnoID = this->Techno->GetTechnoType();// 记录关联技术单位的类型ID（用于后续状态同步）
	ExtrasClass::Array.emplace_back(this);
}

ExtrasClass::~ExtrasClass()
{
	auto it = std::find(ExtrasClass::Array.begin(), ExtrasClass::Array.end(), this);

	if (it != ExtrasClass::Array.end())
		ExtrasClass::Array.erase(it);
}

void ExtrasClass::UpdateType()
{
	this->Type = TechnoExt::ExtMap.Find(this->Techno)->CurrentExtrasType;
}

// =============================
// 加载 / 保存

template <typename T>
bool ExtrasClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->Techno)
		.Process(this->TechnoID)
		.Success();
}

bool ExtrasClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	return Serialize(Stm);
}

bool ExtrasClass::Save(PhobosStreamWriter& Stm) const
{
	return const_cast<ExtrasClass*>(this)->Serialize(Stm);
}


// =============================
//
// Is used for DeploysInto/UndeploysInto
//部署迁移额外属性
void ExtrasClass::SyncExtrasToAnother(TechnoClass* pFrom, TechnoClass* pTo)
{
	const auto pFromExt = TechnoExt::ExtMap.Find(pFrom);
	const auto pToExt = TechnoExt::ExtMap.Find(pTo);

	if (pFromExt->Extras)
	{
		//需要迁移的属性
		pToExt->CurrentExtrasType = pFromExt->CurrentExtrasType;
		pToExt->Extras = std::make_unique<ExtrasClass>(pTo);
		pToExt->Extras->TechnoID = pFromExt->Extras->TechnoID;
		pToExt->Extras->Available = pFromExt->Extras->Available;
	}

	if (pFrom->WhatAmI() == AbstractType::Building && pFromExt->Extras)
		pFromExt->Extras = nullptr;
}
