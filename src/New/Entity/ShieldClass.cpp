#include "ShieldClass.h"

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

std::vector<ShieldClass*> ShieldClass::Array;

ShieldClass::ShieldClass()
	: Techno { nullptr }
	, HP { 0 }
	, Timers { }
	, AreAnimsHidden { false }
{
	ShieldClass::Array.emplace_back(this);
}

ShieldClass::ShieldClass(TechnoClass* pTechno, bool isAttached)
	: Techno { pTechno }
	, IdleAnim { nullptr }
	, Timers { }
	, Cloak { false }
	, Online { true }
	, Temporal { false }
	, Available { true }
	, AreAnimsHidden { false }
	, Attached { isAttached }
	, SelfHealing_Rate_Warhead { -1 }
	, Respawn_Rate_Warhead { -1 }
{
	this->UpdateType();
	this->SetHP(this->Type->InitialStrength.Get(this->Type->Strength));
	this->TechnoID = this->Techno->GetTechnoType();
	ShieldClass::Array.emplace_back(this);
}

/**
 * @brief ShieldClass 类析构函数
 *
 * 主要功能：当 ShieldClass 实例被销毁时，自动从静态成员 Array 中
 * 移除当前实例指针，维护容器有效性防止悬挂指针。
 *
 * 关键操作流程：
 * 1. 在静态数组 Array 中定位当前实例地址
 * 2. 若存在则安全擦除该元素，保持容器数据完整性
 */
ShieldClass::~ShieldClass()
{
	// 在静态数组中查找当前实例指针
	auto it = std::find(ShieldClass::Array.begin(), ShieldClass::Array.end(), this);

	// 安全移除元素：当且仅当指针存在于数组时才执行擦除操作
	if (it != ShieldClass::Array.end())
		ShieldClass::Array.erase(it);
}

void ShieldClass::UpdateType()
{
	this->Type = TechnoExt::ExtMap.Find(this->Techno)->CurrentShieldType;
}

/**
 * @brief 处理指针失效时的逻辑，清理关联的动画对象引用
 *
 * @param ptr 失效的指针，需要检查是否为 AnimClass 类型
 * @param removed 标识对象是否被移除（当前代码未使用该参数，可能是预留参数或遗留代码）
 */
void ShieldClass::PointerGotInvalid(void* ptr, bool removed)
{
	// 检查指针是否为 AnimClass 类型并进行类型转换
	if (auto const pAnim = abstract_cast<AnimClass*>(static_cast<AbstractClass*>(ptr)))
	{
		// 遍历所有护盾实例
		for (auto pShield : ShieldClass::Array)
		{
			// 清理已失效的闲置动画引用
			if (pAnim == pShield->IdleAnim)
				pShield->IdleAnim = nullptr;
		}
	}
}

// =============================
// load / save

template <typename T>
bool ShieldClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->Techno)
		.Process(this->TechnoID)
		.Process(this->IdleAnim)
		.Process(this->Timers.SelfHealing)
		.Process(this->Timers.SelfHealing_WHModifier)
		.Process(this->Timers.Respawn)
		.Process(this->Timers.Respawn_WHModifier)
		.Process(this->HP)
		.Process(this->Cloak)
		.Process(this->Online)
		.Process(this->Temporal)
		.Process(this->Available)
		.Process(this->Attached)
		.Process(this->AreAnimsHidden)
		.Process(this->Type)
		.Process(this->SelfHealing_Warhead)
		.Process(this->SelfHealing_Rate_Warhead)
		.Process(this->SelfHealing_RestartInCombat_Warhead)
		.Process(this->SelfHealing_RestartInCombatDelay_Warhead)
		.Process(this->Respawn_Warhead)
		.Process(this->Respawn_Rate_Warhead)
		.Process(this->LastBreakFrame)
		.Process(this->LastTechnoHealthRatio)
		.Success();
}

bool ShieldClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	return Serialize(Stm);
}

bool ShieldClass::Save(PhobosStreamWriter& Stm) const
{
	return const_cast<ShieldClass*>(this)->Serialize(Stm);
}

// =============================
//
// Is used for DeploysInto/UndeploysInto
void ShieldClass::SyncShieldToAnother(TechnoClass* pFrom, TechnoClass* pTo)
{
	const auto pFromExt = TechnoExt::ExtMap.Find(pFrom);
	const auto pToExt = TechnoExt::ExtMap.Find(pTo);

	if (pFromExt->Shield)
	{
		pToExt->CurrentShieldType = pFromExt->CurrentShieldType;
		pToExt->Shield = std::make_unique<ShieldClass>(pTo);
		pToExt->Shield->TechnoID = pFromExt->Shield->TechnoID;
		pToExt->Shield->Available = pFromExt->Shield->Available;
		pToExt->Shield->HP = pFromExt->Shield->HP;
	}

	if (pFrom->WhatAmI() == AbstractType::Building && pFromExt->Shield)
		pFromExt->Shield = nullptr;
}

/**
 * @brief 检查附加对象的护盾是否处于破损状态
 *
 * 本函数通过查询技术对象的扩展数据，判断其关联的护盾是否失效。
 * 当护盾不存在或护盾生命值小于等于0时视为破损状态。
 *
 * @param pAttached 指向要检查的附加对象的指针，应绑定到技术对象上
 * @return bool
 *   - true : 护盾不存在或已破损（HP <= 0）
 *   - false: 护盾存在且未破损，或对象不是有效技术对象
 */
bool ShieldClass::ShieldIsBrokenTEvent(ObjectClass* pAttached)
{
	// 尝试将附加对象转换为技术对象并查询扩展数据
	if (auto pTechno = abstract_cast<TechnoClass*>(pAttached))
	{
		if (auto pExt = TechnoExt::ExtMap.Find(pTechno))
		{
			// 获取护盾实例并判断其有效性及生命状态
			ShieldClass* pShield = pExt->Shield.get();
			return !pShield || pShield->HP <= 0;
		}
	}

	// 非技术对象或未找到扩展数据时默认返回护盾未破损
	return false;
}

/**
 * @brief 处理护盾接收伤害的逻辑，计算吸收和穿透的伤害量
 *
 * @param args 包含伤害相关参数的结构体指针，主要字段：
 *             - Damage: 输入输出参数，原始伤害值指针
 *             - WH: 使用的弹头类型
 *             - DistanceToEpicenter: 到爆炸中心的距离
 *             - Attacker: 攻击来源单位
 * @return int 实际穿透护盾的伤害值(可能包含正负值)
 */
int ShieldClass::ReceiveDamage(args_ReceiveDamage* args)
{
	// 前置条件检查：护盾不存在/处于时间冻结状态/0伤害时直接返回
	if (!this->HP || this->Temporal || *args->Damage == 0)
		return *args->Damage;

	/* 处理寄生虫特殊机制：
	   当负伤害(修复)发生时，清除寄生单位并重置抑制计时器 */
	// Handle a special case where parasite damages shield but not the unit and unit itself cannot be targeted by repair weapons.
	if (*args->Damage < 0)
	{
		if (auto const pFoot = abstract_cast<FootClass*>(this->Techno))
		{
			if (auto const pParasite = pFoot->ParasiteEatingMe)
			{
				// Remove parasite.
				pParasite->ParasiteImUsing->SuppressionTimer.Start(50);
				pParasite->ParasiteImUsing->ExitUnit();
			}
		}
	}

	// 获取弹头扩展数据并判断免疫状态
	auto const pWHExt = WarheadTypeExt::ExtMap.Find(args->WH);
	bool IC = pWHExt->CanAffectInvulnerable(this->Techno);

	/* 综合免疫判断：
	   包含无敌状态、单位类型免疫、穿透护盾能力等条件 */
	if (!IC || CanBePenetrated(args->WH) || this->Techno->GetTechnoType()->Immune || TechnoExt::IsTypeImmune(this->Techno, args->Attacker))
		return *args->Damage;

	// 伤害计算核心逻辑
	int nDamage = 0;
	int shieldDamage = 0;
	int healthDamage = 0;

	/* 有效伤害处理流程：
	   1. 计算经过护甲调整后的实际伤害
	   2. 根据弹头和护盾类型计算吸收/穿透比例
	   3. 应用伤害范围限制 */
	if (pWHExt->CanTargetHouse(args->SourceHouse, this->Techno) && !args->WH->Temporal)
	{
		// 计算实际伤害（考虑正负值）
		if (*args->Damage > 0)
			nDamage = MapClass::GetTotalDamage(*args->Damage, args->WH, this->GetArmorType(), args->DistanceToEpicenter);
		else
			nDamage = -MapClass::GetTotalDamage(-*args->Damage, args->WH, this->GetArmorType(), args->DistanceToEpicenter);

		// 计算吸收和穿透伤害
		bool affectsShield = pWHExt->Shield_AffectTypes.size() <= 0 || pWHExt->Shield_AffectTypes.Contains(this->Type);
		double absorbPercent = affectsShield ? pWHExt->Shield_AbsorbPercent.Get(this->Type->AbsorbPercent) : this->Type->AbsorbPercent;
		double passPercent = affectsShield ? pWHExt->Shield_PassPercent.Get(this->Type->PassPercent) : this->Type->PassPercent;

		shieldDamage = (int)((double)nDamage * absorbPercent);
		// passthrough damage shouldn't be affected by shield armor
		healthDamage = (int)((double)*args->Damage * passPercent);
	}

	int originalShieldDamage = shieldDamage;
	int min = pWHExt->Shield_ReceivedDamage_Minimum.Get(this->Type->ReceivedDamage_Minimum);
	int max = pWHExt->Shield_ReceivedDamage_Maximum.Get(this->Type->ReceivedDamage_Maximum);
	int minDmg = static_cast<int>(min * pWHExt->Shield_ReceivedDamage_MinMultiplier);
	int maxDmg = static_cast<int>(max * pWHExt->Shield_ReceivedDamage_MaxMultiplier);
	shieldDamage = Math::clamp(shieldDamage, minDmg, maxDmg);

	// 显示护盾伤害数字
	if (Phobos::DisplayDamageNumbers && shieldDamage != 0)
		GeneralUtils::DisplayDamageNumberString(shieldDamage, DamageDisplayType::Shield, this->Techno->GetRenderCoords(), TechnoExt::ExtMap.Find(this->Techno)->DamageNumberOffset);

	/* 正伤害处理分支：
	   - 重置自愈计时器
	   - 处理反击和显形逻辑
	   - 计算护盾破碎或残余生命值 */
	if (shieldDamage > 0)
	{
		bool whModifiersApplied = this->Timers.SelfHealing_WHModifier.InProgress();
		bool restart = whModifiersApplied ? this->SelfHealing_RestartInCombat_Warhead : this->Type->SelfHealing_RestartInCombat;

		// 自愈系统控制逻辑
		if (restart)
		{
			int delay = whModifiersApplied ? this->SelfHealing_RestartInCombatDelay_Warhead : this->Type->SelfHealing_RestartInCombatDelay;

			if (delay > 0)
			{
				this->Timers.SelfHealing_CombatRestart.Start(this->Type->SelfHealing_RestartInCombatDelay);
				this->Timers.SelfHealing.Stop();
			}
			else
			{
				const int rate = whModifiersApplied ? this->SelfHealing_Rate_Warhead : this->Type->SelfHealing_Rate;
				this->Timers.SelfHealing.Start(rate); // when attacked, restart the timer
			}
		}

		// 触发单位响应行为
		if (!pWHExt->Nonprovocative)
			this->ResponseAttack();

		if (pWHExt->DecloakDamagedTargets)
			this->Techno->Uncloak(false);

		// 计算护盾破碎或残余生命
		int residueDamage = shieldDamage - this->HP;

		if (residueDamage >= 0)
		{
			int actualResidueDamage = Math::max(0, int((double)(originalShieldDamage - this->HP) /
				GeneralUtils::GetWarheadVersusArmor(args->WH, this->GetArmorType()))); //only absord percentage damage

			this->BreakShield(pWHExt->Shield_BreakAnim, pWHExt->Shield_BreakWeapon.Get(nullptr));

			return this->Type->AbsorbOverDamage ? healthDamage : actualResidueDamage + healthDamage;
		}
		else
		{
			// 处理护盾命中特效
			if (this->Type->HitFlash && pWHExt->Shield_HitFlash)
			{
				int size = this->Type->HitFlash_FixedSize.Get((shieldDamage * 2));
				SpotlightFlags flags = SpotlightFlags::NoColor;

				if (this->Type->HitFlash_Black)
				{
					flags = SpotlightFlags::NoColor;
				}
				else
				{
					if (!this->Type->HitFlash_Red)
						flags = SpotlightFlags::NoRed;
					if (!this->Type->HitFlash_Green)
						flags |= SpotlightFlags::NoGreen;
					if (!this->Type->HitFlash_Blue)
						flags |= SpotlightFlags::NoBlue;
				}

				MapClass::FlashbangWarheadAt(size, args->WH, this->Techno->Location, true, flags);
			}

			if (!pWHExt->Shield_SkipHitAnim)
				this->WeaponNullifyAnim(pWHExt->Shield_HitAnim);

			this->HP = -residueDamage;

			this->UpdateIdleAnim();

			return healthDamage;
		}
	}
	/* 负伤害处理分支（修复护盾）：
	   - 计算修复量
	   - 更新护盾生命值 */
	else if (shieldDamage < 0)
	{
		const int nLostHP = this->Type->Strength - this->HP;

		if (!nLostHP)
		{
			int result = *args->Damage;

			if (result * GeneralUtils::GetWarheadVersusArmor(args->WH, this->Techno->GetTechnoType()->Armor) > 0)
				result = 0;

			return result;
		}

		const int nRemainLostHP = nLostHP + shieldDamage;

		if (nRemainLostHP < 0)
			this->HP = this->Type->Strength;
		else
			this->HP -= shieldDamage;

		this->UpdateIdleAnim();

		return 0;
	}

	// 零伤害情况直接返回穿透伤害
	// else if (nDamage == 0)
	return healthDamage;
}

/**
 * @brief 处理护盾所属单位的受攻击响应
 * @note 仅在当前玩家拥有该单位时触发相关逻辑：
 * - 对建筑单位通知所属方基地被攻击
 * - 对矿车类单位触发被攻击警告事件和语音提示
 */
void ShieldClass::ResponseAttack()
{
	// 非当前玩家单位不响应攻击事件
	if (this->Techno->Owner != HouseClass::CurrentPlayer)
		return;

	// 建筑类单位处理逻辑：通知所属方基地遇袭
	if (const auto pBld = abstract_cast<BuildingClass*>(this->Techno))
	{
		this->Techno->Owner->BuildingUnderAttack(pBld);
	}
	// 单位类处理逻辑：矿车遇袭特殊处理
	else if (const auto pUnit = abstract_cast<UnitClass*>(this->Techno))
	{
		if (pUnit->Type->Harvester)
		{
			// 创建矿车被攻击的雷达事件并播放语音警报
			const auto pos = pUnit->GetDestination(pUnit);
			if (RadarEventClass::Create(RadarEventType::HarvesterAttacked, CellClass::Coord2Cell(pos)))
				VoxClass::Play(GameStrings::EVA_OreMinerUnderAttack);
		}
	}
}

/**
 * @brief 处理护盾抵消武器命中时的动画效果
 *
 * @param pHitAnim 外部传入的命中动画类型指针，允许为空。若为空则使用护盾自身定义的默认命中动画
 */
void ShieldClass::WeaponNullifyAnim(AnimTypeClass* pHitAnim)
{
	// 当护盾设置为隐藏所有动画时，立即退出处理流程
	if (this->AreAnimsHidden)
		return;

	// 优先使用传入的动画类型，若为空指针则使用护盾类型定义的默认动画
	const auto pAnimType = pHitAnim ? pHitAnim : this->Type->HitAnim;

	// 创建并配置动画对象
	if (pAnimType)
	{
		// 在技术载具坐标处创建动画实例
		auto const pAnim = GameCreate<AnimClass>(pAnimType, this->Techno->GetCoords());
		// 设置动画的所属方和特殊属性
		AnimExt::SetAnimOwnerHouseKind(pAnim, this->Techno->Owner, nullptr, false, true);
		// 建立动画对象与调用者（技术载具）的关联关系
		AnimExt::ExtMap.Find(pAnim)->SetInvoker(this->Techno);
	}
}

/**
 * @brief 判断该护盾是否可以被指定武器攻击
 *
 * @param pWeapon 指向武器类型对象的指针，可能为空指针
 * @return bool
 *   - true : 可以被该武器攻击
 *   - false: 不能被该武器攻击
 */
bool ShieldClass::CanBeTargeted(WeaponTypeClass* pWeapon) const
{
	// 空指针检查：无效武器无法攻击任何目标
	if (!pWeapon)
		return false;

	// 穿透检查或护盾失效条件：
	// 1. 武器弹头可以穿透护盾 或 
	// 2. 护盾当前HP为0
	if (this->CanBePenetrated(pWeapon->Warhead) || !this->HP)
		return true;

	// 常规伤害检查：当武器弹头对该类型护甲存在有效伤害时返回true
	return GeneralUtils::GetWarheadVersusArmor(pWeapon->Warhead, this->GetArmorType()) != 0.0;
}

/**
 * @brief 判断当前护盾是否能被特定弹头穿透
 *
 * @param pWarhead 指向WarheadTypeClass对象的指针，表示用于攻击的弹头类型
 * @return bool
 *   - true : 弹头可以穿透护盾
 *   - false: 弹头无法穿透护盾或参数无效
 */
bool ShieldClass::CanBePenetrated(WarheadTypeClass* pWarhead) const
{
	// 参数有效性检查
	if (!pWarhead)
		return false;

	// 获取弹头类型的扩展数据
	const auto pWHExt = WarheadTypeExt::ExtMap.Find(pWarhead);

	// 获取允许穿透的护盾类型集合（使用Shield_AffectTypes作为默认值）
	const auto affectedTypes = pWHExt->Shield_Penetrate_Types.GetElements(pWHExt->Shield_AffectTypes);

	// 类型过滤检查：存在定义列表且不包含当前护盾类型时拒绝穿透
	if (affectedTypes.size() > 0 && !affectedTypes.contains(this->Type))
		return false;

	// 特殊效果处理：当弹头具有精神控制效果时
	if (pWarhead->Psychedelic)
		return !this->Type->ImmuneToBerserk; // 根据是否免疫狂暴效果返回相反结果

	// 常规穿透判断：返回扩展数据中定义的穿透标志
	return pWHExt->Shield_Penetrate;
}

/**
 * @brief 处理护盾的时间停滞(时空免疫)AI逻辑
 *
 * 当护盾进入时空停滞状态时：
 * 1. 暂停对应的计时器(复活/自愈)
 * 2. 执行隐形检测
 * 3. 根据类型配置处理关联动画的状态
 *
 * @note 该函数没有参数和返回值，作用于ShieldClass实例
 */
void ShieldClass::AI_Temporal()
{
	// 进入时空停滞状态的主逻辑
	if (!this->Temporal)
	{
		this->Temporal = true; // 标记时空状态

		// 根据当前生命值选择要暂停的计时器
		const auto timer = (this->HP <= 0) ? &this->Timers.Respawn : &this->Timers.SelfHealing;
		timer->Pause();// 暂停选中的计时器

		// 执行隐形状态检测
		this->CloakCheck();

		// 处理空闲动画的状态调整
		if (this->IdleAnim)
		{
			// 根据动画配置类型处理不同状态
			switch (this->Type->IdleAnim_TemporalAction)
			{
			case AttachedAnimFlag::Hides:// 完全隐藏动画
				this->KillAnim();
				break;

			case AttachedAnimFlag::Temporal:// 标记动画处于时空状态
				this->IdleAnim->UnderTemporal = true;
				break;

			case AttachedAnimFlag::Paused:// 暂停动画播放
				this->IdleAnim->Pause();
				break;

			case AttachedAnimFlag::PausedTemporal:// 暂停并标记时空状态
				this->IdleAnim->Pause();
				this->IdleAnim->UnderTemporal = true;
				break;
			}
		}
	}
}

/**
 * @brief 护盾AI逻辑主控函数
 *
 * 管理单位护盾的状态更新和逻辑处理，包含状态检查、护盾更新、
 * 动画控制、自愈和重生等核心功能。本函数每帧调用。
 */
void ShieldClass::AI()
{
	// 检查单位是否处于无效状态（被传送/无法移动/被运输等）
	if (!this->Techno || this->Techno->InLimbo || this->Techno->IsImmobilized || this->Techno->Transporter)
		return;

	// 处理单位死亡/损毁状态下的护盾清理
	if (this->Techno->Health <= 0 || !this->Techno->IsAlive || this->Techno->IsSinking)
	{
		if (auto pTechnoExt = TechnoExt::ExtMap.Find(this->Techno))
		{
			pTechnoExt->Shield = nullptr;// 解除护盾与单位的关联
			return;
		}
	}

	// 执行护盾类型转换检查（如遭遇EMP等特殊状态）
	if (this->ConvertCheck())
		return;

	// 更新护盾类型和隐形状态检测
	this->UpdateType();// 根据单位类型更新护盾参数
	this->CloakCheck();// 检测单位隐形状态对护盾的影响

	if (!this->Available)// 护盾当前不可用
		return;

	// 处理时间效应影响（如时间暂停/倒流）
	this->TemporalCheck();

	if (this->Temporal)// 处于时间停滞状态
		return;

	// 核心护盾运行逻辑
	this->OnlineCheck();// 检测护盾在线状态
	this->RespawnShield();// 处理护盾重生逻辑
	this->SelfHealing();// 执行护盾自愈功能

	// 动画系统控制
	double ratio = this->Techno->GetHealthPercentage();

	if (!this->AreAnimsHidden)
	{
		// 生命值比例变化时更新动画状态
		if (GeneralUtils::HasHealthRatioThresholdChanged(LastTechnoHealthRatio, ratio))
			UpdateIdleAnim();

		// 创建护盾激活时的动画效果
		if (!this->Temporal && this->Online && (this->HP > 0 && this->Techno->Health > 0))
			this->CreateAnim();
	}

	// 计时器状态维护
	if (this->Timers.Respawn_WHModifier.Completed())
		this->Timers.Respawn_WHModifier.Stop();

	if (this->Timers.SelfHealing_WHModifier.Completed())
		this->Timers.SelfHealing_WHModifier.Stop();

	// 记录当前生命值比例用于下次比较
	this->LastTechnoHealthRatio = ratio;
}

// The animation is automatically destroyed when the associated unit receives the isCloak statute.
// Therefore, we must zero out the invalid pointer
/**
 * @brief 检查并处理护盾动画在单位隐形时的状态
 *
 * 当关联单位进入隐形状态时，强制终止正在播放的闲置动画。
 * 该函数会在单位隐形状态变更时被调用，用于维护护盾动画与隐形状态的同步。
 */
void ShieldClass::CloakCheck()
{
	// 获取单位的当前隐形状态
	const auto cloakState = this->Techno->CloakState;
	// 更新护盾的隐形标记状态 (包含正在隐形中的过渡状态)
	this->Cloak = cloakState == CloakState::Cloaked || cloakState == CloakState::Cloaking;

	/* 当满足以下条件时强制终止动画：
	 * 1. 单位处于/正在进入隐形状态
	 * 2. 存在正在播放的闲置动画
	 * 3. 该动画类型被标记为需要在隐形时分离 */
	if (this->Cloak && this->IdleAnim && AnimTypeExt::ExtMap.Find(this->IdleAnim->Type)->DetachOnCloak)
		this->KillAnim();
}

void ShieldClass::OnlineCheck()
{
	if (!this->Type->Powered)
		return;

	const auto timer = (this->HP <= 0) ? &this->Timers.Respawn : &this->Timers.SelfHealing;

	auto pTechno = this->Techno;
	bool isActive = !(pTechno->Deactivated || pTechno->IsUnderEMP());

	if (isActive && this->Techno->WhatAmI() == AbstractType::Building)
	{
		auto const pBuilding = static_cast<BuildingClass const*>(this->Techno);
		isActive = pBuilding->IsPowerOnline();
	}

	if (!isActive)
	{
		if (this->Online)
			this->UpdateTint();

		this->Online = false;
		timer->Pause();

		if (this->IdleAnim)
		{
			switch (this->Type->IdleAnim_OfflineAction)
			{
			case AttachedAnimFlag::Hides:
				this->KillAnim();
				break;

			case AttachedAnimFlag::Temporal:
				this->IdleAnim->UnderTemporal = true;
				break;

			case AttachedAnimFlag::Paused:
				this->IdleAnim->Pause();
				break;

			case AttachedAnimFlag::PausedTemporal:
				this->IdleAnim->Pause();
				this->IdleAnim->UnderTemporal = true;
				break;
			}
		}
	}
	else
	{
		if (!this->Online)
			this->UpdateTint();

		this->Online = true;
		timer->Resume();

		if (this->IdleAnim)
		{
			this->IdleAnim->UnderTemporal = false;
			this->IdleAnim->Unpause();
		}
	}
}

void ShieldClass::TemporalCheck()
{
	if (!this->Temporal)
		return;

	this->Temporal = false;

	const auto timer = (this->HP <= 0) ? &this->Timers.Respawn : &this->Timers.SelfHealing;
	timer->Resume();

	if (this->IdleAnim)
	{
		this->IdleAnim->UnderTemporal = false;
		this->IdleAnim->Unpause();
	}
}

// Is used for DeploysInto/UndeploysInto and Type conversion
bool ShieldClass::ConvertCheck()
{
	const auto newID = this->Techno->GetTechnoType();

	if (this->TechnoID == newID)
		return false;

	const auto pTechnoExt = TechnoExt::ExtMap.Find(this->Techno);
	const auto pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(this->Techno->GetTechnoType());
	const auto pOldType = this->Type;
	bool allowTransfer = this->Type->AllowTransfer.Get(Attached);

	// Update shield type.
	if (!allowTransfer && !pTechnoTypeExt->ShieldType->Strength)
	{
		this->KillAnim();
		pTechnoExt->CurrentShieldType = ShieldTypeClass::FindOrAllocate(NONE_STR);
		pTechnoExt->Shield = nullptr;
		this->UpdateTint();

		return true;
	}
	else if (pTechnoTypeExt->ShieldType->Strength)
	{
		pTechnoExt->CurrentShieldType = pTechnoTypeExt->ShieldType;
	}

	const auto pNewType = pTechnoExt->CurrentShieldType;

	// Update shield properties.
	if (pNewType->Strength && this->Available)
	{
		bool isDamaged = this->Techno->GetHealthPercentage() <= RulesClass::Instance->ConditionYellow;
		double healthRatio = this->GetHealthRatio();

		if (pOldType->GetIdleAnimType(isDamaged, healthRatio) != pNewType->GetIdleAnimType(isDamaged, healthRatio))
			this->KillAnim();

		this->HP = (int)round(
			(double)this->HP /
			(double)pOldType->Strength *
			(double)pNewType->Strength
		);
	}
	else
	{
		const auto timer = (this->HP <= 0) ? &this->Timers.Respawn : &this->Timers.SelfHealing;
		if (pNewType->Strength && !this->Available)
		{ // Resume this shield when became Available
			timer->Resume();
			this->Available = true;
		}
		else if (this->Available)
		{ // Pause this shield when became unAvailable
			timer->Pause();
			this->Available = false;
			this->KillAnim();
		}
	}

	this->TechnoID = newID;
	this->UpdateTint();

	return false;
}

void ShieldClass::SelfHealing()
{
	if (this->Timers.SelfHealing_CombatRestart.InProgress())
	{
		return;
	}
	else if (this->Timers.SelfHealing_CombatRestart.Completed())
	{
		const int rate = this->Timers.SelfHealing_WHModifier.InProgress() ? this->SelfHealing_Rate_Warhead : this->Type->SelfHealing_Rate;
		this->Timers.SelfHealing.Start(rate);
		this->Timers.SelfHealing_CombatRestart.Stop();
	}

	const auto pType = this->Type;
	const auto timer = &this->Timers.SelfHealing;
	const auto timerWHModifier = &this->Timers.SelfHealing_WHModifier;

	if (timerWHModifier->Completed() && timer->InProgress())
	{
		double mult = this->SelfHealing_Rate_Warhead > 0 ? Type->SelfHealing_Rate / this->SelfHealing_Rate_Warhead : 1.0;
		timer->TimeLeft = static_cast<int>(timer->GetTimeLeft() * mult);
	}

	const double amount = timerWHModifier->InProgress() ? this->SelfHealing_Warhead : pType->SelfHealing;
	const int rate = timerWHModifier->InProgress() ? this->SelfHealing_Rate_Warhead : pType->SelfHealing_Rate;
	const auto percentageAmount = this->GetPercentageAmount(amount);

	if (percentageAmount != 0)
	{
		if ((this->HP < this->Type->Strength || percentageAmount < 0) && timer->StartTime == -1)
			timer->Start(rate);

		if (this->HP > 0 && timer->Completed())
		{
			timer->Start(rate);
			this->HP += percentageAmount;

			this->UpdateIdleAnim();

			if (this->HP > pType->Strength)
			{
				this->HP = pType->Strength;
				timer->Stop();
			}
			else if (this->HP <= 0)
			{
				this->BreakShield();
			}
		}
	}
}

int ShieldClass::GetPercentageAmount(double iStatus)
{
	if (iStatus == 0)
		return 0;

	if (iStatus >= -1.0 && iStatus <= 1.0)
		return (int)std::round(this->Type->Strength * iStatus);

	return (int)std::trunc(iStatus);
}

void ShieldClass::BreakShield(AnimTypeClass* pBreakAnim, WeaponTypeClass* pBreakWeapon)
{
	this->HP = 0;

	if (this->Type->Respawn)
		this->Timers.Respawn.Start(Timers.Respawn_WHModifier.InProgress() ? Respawn_Rate_Warhead : this->Type->Respawn_Rate);

	this->Timers.SelfHealing.Stop();
	this->KillAnim();

	if (!this->AreAnimsHidden)
	{
		const auto pAnimType = pBreakAnim ? pBreakAnim : this->Type->BreakAnim;

		if (pAnimType)
		{
			auto const pAnim = GameCreate<AnimClass>(pAnimType, this->Techno->Location);

			pAnim->SetOwnerObject(this->Techno);
			AnimExt::SetAnimOwnerHouseKind(pAnim, this->Techno->Owner, nullptr, false, true);
			AnimExt::ExtMap.Find(pAnim)->SetInvoker(this->Techno);
		}
	}

	const auto pWeaponType = pBreakWeapon ? pBreakWeapon : this->Type->BreakWeapon;
	this->LastBreakFrame = Unsorted::CurrentFrame;
	this->UpdateTint();

	if (pWeaponType)
		TechnoExt::FireWeaponAtSelf(this->Techno, pWeaponType);
}

void ShieldClass::RespawnShield()
{
	const auto timer = &this->Timers.Respawn;
	const auto timerWHModifier = &this->Timers.Respawn_WHModifier;

	if (this->HP <= 0 && timer->Completed())
	{
		timer->Stop();
		double amount = timerWHModifier->InProgress() ? Respawn_Warhead : this->Type->Respawn;
		this->HP = this->GetPercentageAmount(amount);
		this->UpdateTint();
	}
	else if (timerWHModifier->Completed() && timer->InProgress())
	{
		double mult = this->Respawn_Rate_Warhead > 0 ? Type->Respawn_Rate / this->Respawn_Rate_Warhead : 1.0;
		timer->TimeLeft = static_cast<int>(timer->GetTimeLeft() * mult);
	}
}

void ShieldClass::SetRespawn(int duration, double amount, int rate, bool resetTimer)
{
	const auto timer = &this->Timers.Respawn;
	const auto timerWHModifier = &this->Timers.Respawn_WHModifier;

	bool modifierTimerInProgress = timerWHModifier->InProgress();
	this->Respawn_Warhead = amount;
	this->Respawn_Rate_Warhead = rate >= 0 ? rate : Type->Respawn_Rate;

	timerWHModifier->Start(duration);

	if (this->HP <= 0 && Respawn_Rate_Warhead >= 0 && resetTimer)
	{
		timer->Start(Respawn_Rate_Warhead);
	}
	else if (timer->InProgress() && !modifierTimerInProgress && this->Respawn_Rate_Warhead != Type->Respawn_Rate)
	{
		double mult = Type->Respawn_Rate > 0 ? this->Respawn_Rate_Warhead / Type->Respawn_Rate : 1.0;
		timer->TimeLeft = static_cast<int>(timer->GetTimeLeft() * mult);
	}
}

void ShieldClass::SetSelfHealing(int duration, double amount, int rate, bool restartInCombat, int restartInCombatDelay, bool resetTimer)
{
	auto timer = &this->Timers.SelfHealing;
	auto timerWHModifier = &this->Timers.SelfHealing_WHModifier;

	bool modifierTimerInProgress = timerWHModifier->InProgress();
	this->SelfHealing_Warhead = amount;
	this->SelfHealing_Rate_Warhead = rate >= 0 ? rate : Type->SelfHealing_Rate;
	this->SelfHealing_RestartInCombat_Warhead = restartInCombat;
	this->SelfHealing_RestartInCombatDelay_Warhead = restartInCombatDelay >= 0 ? restartInCombatDelay : Type->SelfHealing_RestartInCombatDelay;

	timerWHModifier->Start(duration);

	if (resetTimer)
	{
		timer->Start(this->SelfHealing_Rate_Warhead);
	}
	else if (timer->InProgress() && !modifierTimerInProgress && this->SelfHealing_Rate_Warhead != Type->SelfHealing_Rate)
	{
		double mult = Type->SelfHealing_Rate > 0 ? this->SelfHealing_Rate_Warhead / Type->SelfHealing_Rate : 1.0;
		timer->TimeLeft = static_cast<int>(timer->GetTimeLeft() * mult);
	}
}

void ShieldClass::CreateAnim()
{
	auto idleAnimType = this->GetIdleAnimType();

	if (this->Cloak && (!idleAnimType || AnimTypeExt::ExtMap.Find(idleAnimType)->DetachOnCloak))
		return;

	if (!this->IdleAnim && idleAnimType)
	{
		auto const pAnim = GameCreate<AnimClass>(idleAnimType, this->Techno->Location);

		pAnim->SetOwnerObject(this->Techno);
		pAnim->Owner = this->Techno->Owner;
		AnimExt::ExtMap.Find(pAnim)->SetInvoker(this->Techno);
		pAnim->RemainingIterations = 0xFFu;
		this->IdleAnim = pAnim;
	}
}

void ShieldClass::KillAnim()
{
	if (this->IdleAnim)
	{
		this->IdleAnim->UnInit();
		this->IdleAnim = nullptr;
	}
}

void ShieldClass::UpdateIdleAnim()
{
	if (this->IdleAnim && this->IdleAnim->Type != this->GetIdleAnimType())
	{
		this->KillAnim();
		this->CreateAnim();
	}
}

void ShieldClass::UpdateTint()
{
	if (this->Type->HasTint())
		this->Techno->MarkForRedraw();
}

AnimTypeClass* ShieldClass::GetIdleAnimType()
{
	if (!this->Type || !this->Techno)
		return nullptr;

	bool isDamaged = this->Techno->GetHealthPercentage() <= RulesClass::Instance->ConditionYellow;

	return this->Type->GetIdleAnimType(isDamaged, this->GetHealthRatio());
}

bool ShieldClass::IsGreenSP()
{
	return this->Type->GetConditionYellow() * this->Type->Strength.Get() < this->HP;
}

bool ShieldClass::IsYellowSP()
{
	return this->Type->GetConditionRed() * this->Type->Strength.Get() < this->HP && this->HP <= this->Type->GetConditionYellow() * this->Type->Strength.Get();
}

bool ShieldClass::IsRedSP()
{
	return this->HP <= this->Type->GetConditionYellow() * this->Type->Strength.Get();
}

void ShieldClass::DrawShieldBar_Building(const int length, RectangleStruct* pBound)
{
	Point2D position = { 0, 0 };
	const int totalLength = DrawShieldBar_PipAmount(length);
	int frame = this->DrawShieldBar_Pip(true);

	if (totalLength > 0)
	{
		for (int frameIdx = totalLength, deltaX = 0, deltaY = 0;
			frameIdx;
			frameIdx--, deltaX += 4, deltaY -= 2)
		{
			position = TechnoExt::GetBuildingSelectBracketPosition(Techno, BuildingSelectBracketPosition::Top);
			position.X -= deltaX + 6;
			position.Y -= deltaY + 3;

			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPS_SHP,
				frame, &position, pBound, BlitterFlags::Centered | BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}
	}

	if (totalLength < length)
	{
		for (int frameIdx = length - totalLength, deltaX = 4 * totalLength, deltaY = -2 * totalLength;
			frameIdx;
			frameIdx--, deltaX += 4, deltaY -= 2)
		{
			position = TechnoExt::GetBuildingSelectBracketPosition(Techno, BuildingSelectBracketPosition::Top);
			position.X -= deltaX + 6;
			position.Y -= deltaY + 3;

			const int emptyFrame = this->Type->Pips_Building_Empty.Get(RulesExt::Global()->Pips_Shield_Building_Empty.Get(0));

			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPS_SHP,
				emptyFrame, &position, pBound, BlitterFlags::Centered | BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}
	}
}

void ShieldClass::DrawShieldBar_Other(const int length, RectangleStruct* pBound)
{
	auto position = TechnoExt::GetFootSelectBracketPosition(Techno, Anchor(HorizontalPosition::Left, VerticalPosition::Top));
	const auto pipBoard = this->Type->Pips_Background.Get(RulesExt::Global()->Pips_Shield_Background.Get(FileSystem::PIPBRD_SHP));
	int frame;

	position.X -= 1;
	position.Y += this->Techno->GetTechnoType()->PixelSelectionBracketDelta + this->Type->BracketDelta - 3;

	if (length == 8)
		frame = pipBoard->Frames > 2 ? 3 : 1;
	else
		frame = pipBoard->Frames > 2 ? 2 : 0;

	if (this->Techno->IsSelected)
	{
		position.X += length + 1 + (length == 8 ? length + 1 : 0);
		DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, pipBoard,
			frame, &position, pBound, BlitterFlags::Centered | BlitterFlags::bf_400 | BlitterFlags::Alpha, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		position.X -= length + 1 + (length == 8 ? length + 1 : 0);
	}

	frame = this->DrawShieldBar_Pip(false);
	position.Y += 1;

	const int totalLength = DrawShieldBar_PipAmount(length);
	for (int i = 0; i < totalLength; ++i)
	{
		position.X += 2;
		DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPS_SHP,
			frame, &position, pBound, BlitterFlags::Centered | BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}
}

int ShieldClass::DrawShieldBar_Pip(const bool isBuilding) const
{
	const int strength = this->Type->Strength.Get();
	const auto pipsShield = isBuilding ? this->Type->Pips_Building.Get() : this->Type->Pips.Get();
	const auto pipsGlobal = isBuilding ? RulesExt::Global()->Pips_Shield_Building.Get() : RulesExt::Global()->Pips_Shield.Get();

	CoordStruct shieldPip;

	if (pipsShield.X != -1)
		shieldPip = pipsShield;
	else
		shieldPip = pipsGlobal;

	if (this->HP > this->Type->GetConditionYellow() * strength && shieldPip.X != -1)
		return shieldPip.X;
	else if (this->HP > this->Type->GetConditionRed() * strength && (shieldPip.Y != -1 || shieldPip.X != -1))
		return shieldPip.Y == -1 ? shieldPip.X : shieldPip.Y;
	else if (shieldPip.Z != -1 || shieldPip.X != -1)
		return shieldPip.Z == -1 ? shieldPip.X : shieldPip.Z;

	return isBuilding ? 5 : 16;
}

int ShieldClass::DrawShieldBar_PipAmount(int length) const
{
	return this->IsActive()
		? Math::clamp((int)round(this->GetHealthRatio() * length), 1, length)
		: 0;
}

ArmorType ShieldClass::GetArmorType() const
{
	const auto pShieldType = this->Type;

	if (this->Techno && pShieldType->InheritArmorFromTechno)
	{
		const auto pTechnoType = this->Techno->GetTechnoType();

		if (pShieldType->InheritArmor_Allowed.empty() || pShieldType->InheritArmor_Allowed.Contains(pTechnoType)
			&& (pShieldType->InheritArmor_Disallowed.empty() || !pShieldType->InheritArmor_Disallowed.Contains(pTechnoType)))
		{
			return pTechnoType->Armor;
		}
	}

	return pShieldType->Armor.Get();
}

int ShieldClass::GetFramesSinceLastBroken() const
{
	return Unsorted::CurrentFrame - this->LastBreakFrame;
}

void ShieldClass::SetAnimationVisibility(bool visible)
{
	if (!this->AreAnimsHidden && !visible)
		this->KillAnim();

	this->AreAnimsHidden = !visible;
}
