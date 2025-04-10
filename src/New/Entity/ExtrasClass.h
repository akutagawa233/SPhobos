#pragma once

#include <GeneralStructures.h>
#include <SpecificStructures.h>
#include <Ext/TechnoType/Body.h>

class TechnoClass;
class WarheadTypeClass;

class ExtrasClass
{
public:
	static std::vector<ExtrasClass*> Array;

	ExtrasClass();
	ExtrasClass(TechnoClass* pTechno, bool isAttached);
	ExtrasClass(TechnoClass* pTechno) : ExtrasClass(pTechno, false) { };
	~ExtrasClass();

	bool IsAvailable() const//< 基础可用性检查
	{
		return this->Available;
	};

	ExtrasTypeClass* GetType() const
	{
		return this->Type;
	};

	bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	bool Save(PhobosStreamWriter& Stm) const;
private:
	template <typename T>
	bool Serialize(T& Stm);

	void UpdateType(); 
	/// Properties ///
	TechnoClass* Techno;
	TechnoTypeClass* TechnoID;

	bool Available;

	ExtrasTypeClass* Type;
};
