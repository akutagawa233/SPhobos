#include "ExtrasTypeClass.h"

#include <TacticalClass.h>
#include <SpawnManagerClass.h>

#include <Ext/Techno/Body.h>
#include <Ext/House/Body.h>

template<>
const char* Enumerable<ExtrasTypeClass>::GetMainSection()
{
	return "ExtrasTypes";
}

//设置额外的前提条件
CanBuildResult ExtrasTypeClass::SetPrerequisite(TechnoTypeClass* pType, CanBuildResult canBuild)
{
	const auto pCurrent = HouseClass::CurrentPlayer;
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	CanBuildResult pCanBuild = canBuild;
	//const auto& pExtrasTypes = pTypeExt->ExtrasType;
	if (this->ExtrasType == ExtrasTypeList::Prerequisite)
	{
		const auto& TechnoList = this->Effect_Technos;
		if (!TechnoList.empty())
		{
			for (auto pTechno : TechnoList)
			{
				if (HouseExt::CountOwnedPresentExt(HouseClass::CurrentPlayer, pTechno, true, true) == 0)
					return canBuild;
			}
			pCanBuild = CanBuildResult::Buildable;
		}
	}
	return pCanBuild;
	//return canBuild;
}

void ExtrasTypeClass::LoadFromINI(CCINIClass* pINI)
{
	const char* pSection = this->Name;

	if (INIClass::IsBlank(pSection))
		return;

	INI_EX exINI(pINI);

	this->ExtrasType.Read(exINI, pSection, "ExtrasType");

	this->Prerequisite_Condition.Read(exINI, pSection, "Prerequisite.Condition");

	this->Effect_Technos.Read(exINI, pSection, "Effect.Technos");
	this->Effect_Weapons.Read(exINI, pSection, "Effect.Weapons");
    this->Effect_Warheads.Read(exINI, pSection, "Effect.Warheads");
    this->Effect_Anims.Read(exINI, pSection, "Effect.Anims");
    //this->Effect_Sounds.Read(exINI, pSection, "Effect.Sounds");
    //this->Effect_EVAs.Read(exINI, pSection, "Effect.EVAs");
    this->Effect_UnitType.Read(exINI, pSection, "Effect.UnitType");
    this->Effect_Owner.Read(exINI, pSection, "Effect.Owner");
    this->Effect_Money.Read(exINI, pSection, "Effect.Money");
    

}

template <typename T>
void ExtrasTypeClass::Serialize(T& Stm)
{
	Stm
		.Process(this->ExtrasType)

        .Process(this->Prerequisite_Condition)

		.Process(this->Effect_Technos)
        .Process(this->Effect_Weapons)
        .Process(this->Effect_Warheads)
        .Process(this->Effect_Anims)
        //.Process(this->Effect_Sounds)
        //.Process(this->Effect_EVAs)
        .Process(this->Effect_UnitType)
        .Process(this->Effect_Owner)
        .Process(this->Effect_Money)
		;
}

void ExtrasTypeClass::LoadFromStream(PhobosStreamReader& Stm)
{
	this->Serialize(Stm);
}

void ExtrasTypeClass::SaveToStream(PhobosStreamWriter& Stm)
{
	this->Serialize(Stm);
}

//枚举类方便读取
namespace detail
{
	template <>//读取ExtrasTypeList
	inline bool read<ExtrasTypeList>(ExtrasTypeList& value, INI_EX& parser, const char* pSection, const char* pKey)
	{
		if (parser.ReadString(pSection, pKey))
		{
			auto str = parser.value();
			if (_strcmpi(str, "UIName") == 0)
			{
				value = ExtrasTypeList::UIName;
			}
			else if (_strcmpi(str, "Prerequisite") == 0)
			{
				value = ExtrasTypeList::Prerequisite;
			}
			else
			{
				Debug::INIParseFailed(pSection, pKey, str, "ExtrasType info type is invalid");
				return false;
			}
		}
		return false;
	}
	template <>
	inline bool read<ExtrasEffectList>(ExtrasEffectList& value, INI_EX& parser, const char* pSection, const char* pKey)
	{
		if (parser.ReadString(pSection, pKey))
		{
			auto str = parser.value();
			if (_strcmpi(str, "Technos") == 0)
			{
				value = ExtrasEffectList::Technos;
			}
			else if (_strcmpi(str, "Weapons") == 0)
			{
				value = ExtrasEffectList::Weapons;
			}
			else if (_strcmpi(str, "Warheads") == 0)
			{
				value = ExtrasEffectList::Warheads;
			}
			else if (_strcmpi(str, "Anims") == 0)
			{
				value = ExtrasEffectList::Anims;
			}
			else if (_strcmpi(str, "Sounds") == 0)
			{
                value = ExtrasEffectList::Sounds;
			}
			else if (_strcmpi(str, "EVAs") == 0)
			{
				value = ExtrasEffectList::EVAs;
			}
			else if (_strcmpi(str, "UnitType") == 0)
			{
				value = ExtrasEffectList::UnitType;
			}
			else if (_strcmpi(str, "Owner") == 0)
			{
				value = ExtrasEffectList::Owner;
			}
			else if (_strcmpi(str, "Money") == 0)
			{
				value = ExtrasEffectList::Money;
			}
			else
			{
				Debug::INIParseFailed(pSection, pKey, str, "ExtrasEffect info type is invalid");
				return false;
			}
		}
		return false;
	}
}
