#pragma once
#include <Utilities/Enumerable.h>
#include <Utilities/Template.h>
#include <Utilities/GeneralUtils.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Anchor.h>

class ExtrasTypeClass final : public Enumerable<ExtrasTypeClass>
{
public:
	Valueable<std::string> Type;
	ValueableVector<TechnoTypeClass*> Effect_Technos;

	ExtrasTypeClass(const char* pTitle = NONE_STR) : Enumerable<ExtrasTypeClass>(pTitle)
		, Type { "None" }
		, Effect_Technos { }
	{ }

	void LoadFromINI(CCINIClass* pINI);
	void LoadFromStream(PhobosStreamReader& Stm);
	void SaveToStream(PhobosStreamWriter& Stm);

private:
	template <typename T>
	void Serialize(T& Stm);
};
