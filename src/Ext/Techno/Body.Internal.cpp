#include "Body.h"

#include <AirstrikeClass.h>
#include <SpawnManagerClass.h>
#include <Utilities/EnumFunctions.h>

// Unsorted methods

void TechnoExt::ExtData::InitializeLaserTrails()
{
	if (this->LaserTrails.size())
		return;

	if (auto pTypeExt = this->TypeExtData)
	{
		for (auto const& entry : pTypeExt->LaserTrailData)
		{
			this->LaserTrails.emplace_back(entry.GetType(), this->OwnerObject()->Owner, entry.FLH, entry.IsOnTurret);
		}
	}
}

void TechnoExt::ObjectKilledBy(TechnoClass* pVictim, TechnoClass* pKiller)
{
	TechnoClass* pObjectKiller = ((pKiller->GetTechnoType()->Spawned || pKiller->GetTechnoType()->MissileSpawn) && pKiller->SpawnOwner) ?
		pKiller->SpawnOwner : pKiller;

	if (pObjectKiller && pObjectKiller->BelongsToATeam())
	{
		if (auto const pFootKiller = generic_cast<FootClass*>(pObjectKiller))
		{
			auto pKillerTechnoData = TechnoExt::ExtMap.Find(pObjectKiller);
			pKillerTechnoData->LastKillWasTeamTarget = pFootKiller->Team->Focus == pVictim;
		}
	}
}

// reversed from 6F3D60
CoordStruct TechnoExt::GetFLHAbsoluteCoords(TechnoClass* pThis, CoordStruct pCoord, bool isOnTurret, int turIdx)
{
	auto const pType = pThis->GetTechnoType();
	auto const pFoot = abstract_cast<FootClass*>(pThis);
	Matrix3D mtx;

	// Step 1: get body transform matrix
	if (pFoot && pFoot->Locomotor)
		mtx = pFoot->Locomotor->Draw_Matrix(nullptr);
	else // no locomotor means no rotation or transform of any kind (f.ex. buildings) - Kerbiter
		mtx.MakeIdentity();

	// Steps 2-3: turret offset and rotation
	if (isOnTurret && pThis->HasTurret())
	{
		TechnoTypeExt::ApplyTurretOffset(pType, &mtx, 1.0, turIdx);

		double turretRad = pThis->TurretFacing().GetRadian<32>();
		// For BuildingClass turret facing is equal to primary facing
		float angle = pFoot ? (float)(turretRad - pThis->PrimaryFacing.Current().GetRadian<32>()) : (float)(turretRad);

		mtx.RotateZ(angle);
	}

	// Step 4: apply FLH offset
	mtx.Translate((float)pCoord.X, (float)pCoord.Y, (float)pCoord.Z);

	auto result = mtx.GetTranslation();

	// Step 5: apply as an offset to global object coords
	// Resulting coords are mirrored along X axis, so we mirror it back
	auto location = pThis->GetRenderCoords() + CoordStruct { (int)result.X, -(int)result.Y, (int)result.Z };

	return location;
}

CoordStruct TechnoExt::GetBurstFLH(TechnoClass* pThis, int weaponIndex, bool& FLHFound)
{
	FLHFound = false;
	CoordStruct FLH = CoordStruct::Empty;

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	auto pInf = abstract_cast<InfantryClass*>(pThis);
	std::span<std::vector<CoordStruct>> pickedFLHs = pExt->WeaponBurstFLHs;

	if (pThis->Veterancy.IsElite())
	{
		if (pInf && pInf->IsDeployed())
			pickedFLHs = pExt->EliteDeployedWeaponBurstFLHs;
		else if (pInf && pInf->Crawling)
			pickedFLHs = pExt->EliteCrouchedWeaponBurstFLHs;
		else
			pickedFLHs = pExt->EliteWeaponBurstFLHs;
	}
	else
	{
		if (pInf && pInf->IsDeployed())
			pickedFLHs = pExt->DeployedWeaponBurstFLHs;
		else if (pInf && pInf->Crawling)
			pickedFLHs = pExt->CrouchedWeaponBurstFLHs;
	}
	if ((int)pickedFLHs[weaponIndex].size() > pThis->CurrentBurstIndex)
	{
		FLHFound = true;
		FLH = pickedFLHs[weaponIndex][pThis->CurrentBurstIndex];
	}

	return FLH;
}

CoordStruct TechnoExt::GetSimpleFLH(InfantryClass* pThis, int weaponIndex, bool& FLHFound)
{
	FLHFound = false;
	CoordStruct FLH = CoordStruct::Empty;

	if (auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type))
	{
		Nullable<CoordStruct> pickedFLH;

		if (pThis->IsDeployed())
		{
			if (weaponIndex == 0)
				pickedFLH = pTypeExt->DeployedPrimaryFireFLH;
			else if (weaponIndex == 1)
				pickedFLH = pTypeExt->DeployedSecondaryFireFLH;
		}
		else
		{
			if (pThis->Crawling)
			{
				if (weaponIndex == 0)
					pickedFLH = pTypeExt->PronePrimaryFireFLH;
				else if (weaponIndex == 1)
					pickedFLH = pTypeExt->ProneSecondaryFireFLH;
			}
		}

		if (pickedFLH.isset())
		{
			FLH = pickedFLH.Get();
			FLHFound = true;
		}
	}

	return FLH;
}

//初始化数显
void TechnoExt::ExtData::InitializeDisplayInfo()
{
	// 获取宿主对象并初始化主武器指针
	const auto pThis = this->OwnerObject();
	const auto pPrimary = pThis->GetWeapon(0)->WeaponType;

	/* 设置再装填时间逻辑：
	1. 当存在有效主武器且单位具备对地攻击能力时，使用主武器的射速
	2. 否则检查副武器有效性，使用副武器的射速 */
	if (pPrimary && pThis->GetTechnoType()->LandTargeting != LandTargetingType::Land_Not_OK)
		pThis->RearmTimer.TimeLeft = pPrimary->ROF;
	else if (const auto pSecondary = pThis->GetWeapon(1)->WeaponType)
		pThis->RearmTimer.TimeLeft = pSecondary->ROF;

	// 设置计时器起始时间，确保起始值不超过-2且不大于当前倒计时时间的负值
	pThis->RearmTimer.StartTime = Math::min(-2, -pThis->RearmTimer.TimeLeft);
}

//初始化AE
void TechnoExt::ExtData::InitializeAttachEffects()
{
	// 获取关联的类型扩展数据并进行有效性检查
	if (auto pTypeExt = this->TypeExtData)
	{
		// 检查附加效果类型列表是否为空，避免无效操作
		if (pTypeExt->AttachEffects.AttachTypes.size() < 1)
			return;

		// 获取宿主对象并执行额外属性的附加
		auto const pThis = this->OwnerObject();
		/* 调用额外属性附加系统：
		 * - 参数1: 效果宿主对象（被附加效果的目标）
		 * - 参数2: 效果来源对象（触发效果的主体）
		 * - 参数3&4: 此处重复使用宿主对象，可能表示持续时间和覆盖规则的控制方
		 * - 参数5: 包含附加效果配置数据的结构体 */
		AttachEffectClass::Attach(pThis, pThis->Owner, pThis, pThis, pTypeExt->AttachEffects);
	}
}

//初始化AE
void TechnoExt::ExtData::InitializeExtras()
{
	// 获取关联的类型扩展数据并进行有效性检查
	if (auto pTypeExt = this->TypeExtData)
	{
		// 检查额外属性类型列表是否为空，避免无效操作
		if (pTypeExt->ExtrasType.size() < 1)
			return;

		// 获取宿主对象并执行效果附加
		auto const pThis = this->OwnerObject();
		/* 调用核心附加效果系统：
		 * - 参数1: 效果宿主对象（被附加效果的目标）
		 * - 参数2: 效果来源对象（触发效果的主体）
		 * - 参数3&4: 此处重复使用宿主对象，可能表示持续时间和覆盖规则的控制方
		 * - 参数5: 包含附加效果配置数据的结构体 */
		//ExtrasClass::Attach(pThis, pThis->Owner, pThis, pThis, pTypeExt->ExtrasType);
	}
}

// Gets tint colors for invulnerability, airstrike laser target and berserk, depending on parameters.
int TechnoExt::GetTintColor(TechnoClass* pThis, bool invulnerability, bool airstrike, bool berserk)
{
	int tintColor = 0;

	if (pThis)
	{
		if (invulnerability && pThis->IsIronCurtained())
			tintColor |= GeneralUtils::GetColorFromColorAdd(pThis->ForceShielded ? RulesClass::Instance->ForceShieldColor : RulesClass::Instance->IronCurtainColor);
		if (airstrike && pThis->Airstrike && pThis->Airstrike->Target == pThis)
			tintColor |= GeneralUtils::GetColorFromColorAdd(RulesClass::Instance->LaserTargetColor);
		if (berserk && pThis->Berzerk)
			tintColor |= GeneralUtils::GetColorFromColorAdd(RulesClass::Instance->BerserkColor);
	}

	return tintColor;
}

// Gets custom tint color from TechnoTypes & Warheads.
int TechnoExt::GetCustomTintColor(TechnoClass* pThis)
{
	int dummy = 0;
	int color = 0;
	TechnoExt::ApplyCustomTintValues(pThis, color, dummy);
	return color;
}

// Gets custom tint intensity from TechnoTypes & Warheads.
int TechnoExt::GetCustomTintIntensity(TechnoClass* pThis)
{
	int dummy = 0;
	int intensity = 0;
	TechnoExt::ApplyCustomTintValues(pThis, dummy, intensity);
	return intensity;
}

// Applies custom tint color and intensity from TechnoTypes and any AttachEffects and shields it might have on provided values.
void TechnoExt::ApplyCustomTintValues(TechnoClass* pThis, int& color, int& intensity)
{
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	bool hasTechnoTint = pTypeExt->Tint_Color.isset() || pTypeExt->Tint_Intensity;
	bool hasShieldTint = pExt->Shield && pExt->Shield->IsActive() && pExt->Shield->GetType()->HasTint();

	// Bail out early if no custom tint is applied.
	if (!hasTechnoTint && !pExt->AE.HasTint && !hasShieldTint)
		return;

	if (hasTechnoTint && EnumFunctions::CanTargetHouse(pTypeExt->Tint_VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer))
	{
		color |= Drawing::RGB_To_Int(pTypeExt->Tint_Color);
		intensity += static_cast<int>(pTypeExt->Tint_Intensity * 1000);
	}

	if (pExt->AE.HasTint)
	{
		for (auto const& attachEffect : pExt->AttachedEffects)
		{
			auto const type = attachEffect->GetType();

			if (!attachEffect->IsActive() || !type->HasTint())
				continue;

			if (!EnumFunctions::CanTargetHouse(type->Tint_VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer))
				continue;

			color |= Drawing::RGB_To_Int(type->Tint_Color);
			intensity += static_cast<int>(type->Tint_Intensity * 1000);
		}
	}

	if (hasShieldTint)
	{
		auto const pShieldType = pExt->Shield->GetType();

		if (!EnumFunctions::CanTargetHouse(pShieldType->Tint_VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer))
			return;

		color |= Drawing::RGB_To_Int(pShieldType->Tint_Color);
		intensity += static_cast<int>(pShieldType->Tint_Intensity * 1000);
	}
}

// This is still not even correct, but let's see how far this can help us
void TechnoExt::ChangeOwnerMissionFix(FootClass* pThis)
{
	pThis->ShouldScanForTarget = false;
	pThis->ShouldEnterAbsorber = false;
	pThis->ShouldEnterOccupiable = false;
	pThis->ShouldGarrisonStructure = false;

	if (pThis->HasAnyLink() || pThis->GetTechnoType()->ResourceGatherer) // Don't want miners to stop
		return;

	switch (pThis->GetCurrentMission())
	{
	case Mission::Harvest:
	case Mission::Sleep:
	case Mission::Harmless:
	case Mission::Repair:
		return;
	}

	pThis->Override_Mission(Mission::Guard, nullptr, nullptr); // I don't even know what this is
	pThis->ShouldLoseTargetNow = TRUE;
	pThis->QueueMission(pThis->GetTechnoType()->DefaultToGuardArea ? Mission::Area_Guard : Mission::Guard, true);
}

// Updates layers of all animations attached to the given techno.
void TechnoExt::UpdateAttachedAnimLayers(TechnoClass* pThis)
{
	// Skip if has no attached animations.
	if (!pThis || !pThis->HasParachute)
		return;

	// Could possibly be faster to track the attached anims in TechnoExt but the profiler doesn't show this as a performance hog so whatever.
	for (auto pAnim : AnimClass::Array)
	{
		if (pAnim->OwnerObject != pThis)
			continue;

		DisplayClass::Instance.Submit(pAnim);
	}
}
