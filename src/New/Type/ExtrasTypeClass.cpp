#include "ExtrasTypeClass.h"

#include <TacticalClass.h>
#include <SpawnManagerClass.h>

#include <Ext/Techno/Body.h>

template<>
const char* Enumerable<ExtrasTypeClass>::GetMainSection()
{
	return "ExtrasTypeClass";
}

void ExtrasTypeClass::LoadFromINI(CCINIClass* pINI)
{
	const char* section = this->Name;

	INI_EX exINI(pINI);


}

template <typename T>
void ExtrasTypeClass::Serialize(T& Stm)
{
	Stm

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
