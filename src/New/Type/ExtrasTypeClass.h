#pragma once
#include <Utilities/Enumerable.h>
#include <Utilities/Template.h>
#include <Utilities/GeneralUtils.h>
#include <Ext/Rules/Body.h>
#include <Utilities/TemplateDef.h>

enum class ExtrasTypeList : int
{
	UIName = 0,
	Prerequisite = 1
};

enum class ExtrasEffectList : int
{
	Technos = 0,
	Weapons = 1,
	Warheads = 2,
	Anims = 3,
	Sounds = 4,
	EVAs = 5,
	UnitType = 6,
	Owner = 7,
	Money = 8
};

class ExtrasTypeClass final : public Enumerable<ExtrasTypeClass>
{
public:
	Valueable<ExtrasTypeList> ExtrasType; 

	//各种类型下的属性
	Valueable<ExtrasEffectList> Prerequisite_Condition;
	//效果,可作为条件、属性、效果等
	ValueableVector<TechnoTypeClass*> Effect_Technos;
	ValueableVector<WeaponTypeClass*> Effect_Weapons;
	ValueableVector<WarheadTypeClass*> Effect_Warheads;
    ValueableVector<AnimTypeClass*> Effect_Anims;
    //ValueableVector<SoundTypeClass*> Effect_Sounds;
    //ValueableVector<EvaTypeClass*> Effect_EVAs;
    ValueableVector<UnitTypeClass*> Effect_UnitType;
    ValueableVector<HouseTypeClass*> Effect_Owner;
    ValueableVector<int> Effect_Money;

public:
	ExtrasTypeClass(const char* const pTitle) : Enumerable<ExtrasTypeClass>(pTitle)
		, ExtrasType {  }

		, Prerequisite_Condition { }

		, Effect_Technos { }
        , Effect_Weapons { }
		, Effect_Warheads { }
		, Effect_Anims { }
		, Effect_UnitType { }
		, Effect_Owner { }
		, Effect_Money { }
	{ }

	CanBuildResult SetPrerequisite(TechnoTypeClass* pType, CanBuildResult canBuild);

	void LoadFromINI(CCINIClass* pINI);
	void LoadFromStream(PhobosStreamReader& Stm);
	void SaveToStream(PhobosStreamWriter& Stm);

private:
	template <typename T>
	void Serialize(T& Stm);
};


