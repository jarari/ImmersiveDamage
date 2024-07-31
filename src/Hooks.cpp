#include <Configs.h>
#include <Hooks.h>
#include <MenuWatcher.h>
#include <windows.h>
#include <fmt/xchar.h>

uintptr_t DoHitMeOrig;
uintptr_t HandleEntryPointOrig;
uintptr_t AdjustAttackDamageOrig;
std::string strVatsMenu{ "VATSMenu" };
std::string strVatsMult{ "fVATSPlayerDamageMult" };

void Hooks::InitializeHooks()
{
	F4SE::Trampoline& trampoline = F4SE::GetTrampoline();
	DoHitMeOrig = trampoline.write_call<5>(REL::Relocation<uintptr_t>{ REL::ID(1546751), 0x921 }.address(), &HookedDoHitMe);
	AdjustAttackDamageOrig = trampoline.write_call<5>(REL::Relocation<uintptr_t>{ REL::ID(1330650), 0x5EF }.address(), &HookedAdjustAttackDamage);  //HitData::InitializeHitData(Actor *,Actor *,BGSObjectInstanceT<TESObjectWEAP> const &,BGSEquipIndex,bool)+5EF
	trampoline.write_call<5>(REL::Relocation<uintptr_t>{ REL::ID(1327603), 0x1E5 }.address(), &HookedAdjustAttackDamage);                           //HitData::InitializeHitData(Actor *,Actor *,Projectile *)+1E5
	trampoline.write_call<5>(REL::Relocation<uintptr_t>{ REL::ID(568163), 0x16C }.address(), &HookedAdjustAttackDamage);                            //HitData::InitializeExplosionData(Actor *,Actor *,BGSObjectInstanceT<TESObjectWEAP> const &,BGSExplosion &,float,float)+16C
	HandleEntryPointOrig = trampoline.write_call<5>(REL::Relocation<uintptr_t>{ REL::ID(1431014), 0xA9 }.address(), &HookedHandleEntryPoint);
	uint8_t bytes[]{ 0xEB };
	REL::safe_write<uint8_t>(REL::Relocation<uintptr_t>{ REL::ID(228393), 0x25 }.address(), std::span{ bytes });
	RE::UI::GetSingleton()->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(MenuWatcher::GetSingleton());
}
void Hooks::HookedDoHitMe(RE::Actor* a_actor, RE::HitData& a_hitData)
{
	if (Configs::enableLog) {
		if (a_hitData.source.object && (a_hitData.flags & 0x4000) == 0) {
			RE::TESObjectWEAP* weap = a_hitData.source.object->As<RE::TESObjectWEAP>();
			if (weap && weap->weaponData.type == RE::WEAPON_TYPE::kGun) {
				RE::TESObjectWEAP::InstanceData* weapInstance = (RE::TESObjectWEAP::InstanceData*)a_hitData.source.instanceData.get();
				RE::TESAmmo* ammo = weap->weaponData.ammo;
				float attackDamage = weap->weaponData.attackDamage;
				float secondaryDamage = weap->weaponData.secondaryDamage;
				if (weapInstance) {
					ammo = weapInstance->ammo;
					attackDamage = weapInstance->attackDamage;
					secondaryDamage = weapInstance->secondaryDamage;
				}
				float ammoDamage = 0;
				if (ammo) {
					ammoDamage = ammo->data.damage;
				}
				logger::info("----------------------------------------------");
				logger::info("Weapon - {}", weap->fullName.c_str());
				if (ammo) {
					logger::info("Ammo - {}", ammo->fullName.c_str());
					logger::info("Ammo Damage: {}", ammo->data.damage);
				}
				logger::info("Weapon Damage: {}, Secondary Damage: {}", attackDamage, secondaryDamage);
				logger::info("Base Damage: {}", a_hitData.baseDamage);
				logger::info("Calculated Base Damage: {}", a_hitData.calculatedBaseDamage);
				logger::info("Total Damage: {}", a_hitData.totalDamage);
				logger::info("Reduced Damage: {}", a_hitData.reducedDamage);
				logger::info("Crit Multiplier: {}", a_hitData.criticalDamageMult);
				logger::info("Sneak Attack Multiplier: {}", a_hitData.sneakAttackMult);
				if (a_hitData.damageTypes) {
					for (auto it = a_hitData.damageTypes->begin(); it != a_hitData.damageTypes->end(); ++it) {
						logger::info("Damage Type: {} Val: {}", it->first->GetFormEditorID(), it->second.f);
					}
				}
			}
		}
	}

	if (Configs::enableMod) {
		if (a_hitData.source.object && (a_hitData.flags & 0x4000) == 0) {
			RE::TESObjectWEAP* weap = a_hitData.source.object->As<RE::TESObjectWEAP>();
			if (weap && weap->weaponData.type == RE::WEAPON_TYPE::kGun) {
				if (a_hitData.calculatedBaseDamage != 0) {
					if (!Configs::enableCrit && (a_hitData.flags & 0x1008) == 0x8 && a_hitData.criticalDamageMult > 0) {
						a_hitData.baseDamage /= a_hitData.criticalDamageMult;
						a_hitData.calculatedBaseDamage /= a_hitData.criticalDamageMult;
						a_hitData.totalDamage /= a_hitData.criticalDamageMult;
					}
					if (Configs::enableSneak && (a_hitData.flags & 0x1800) == 0x800) {
						a_hitData.baseDamage *= a_hitData.sneakAttackMult;
						a_hitData.calculatedBaseDamage *= a_hitData.sneakAttackMult;
						a_hitData.totalDamage *= a_hitData.sneakAttackMult;
					}
					if (a_actor == RE::PlayerCharacter::GetSingleton()) {
						float vatsMult = 1.f;
						RE::UI* ui = RE::UI::GetSingleton();
						if (ui) {
							bool isMenuOpen = ui->GetMenuOpen(strVatsMenu);
							if (isMenuOpen) {
								RE::GameSettingCollection* collection = RE::GameSettingCollection::GetSingleton();
								for (auto& setting : collection->settings) {
									if (setting.first == strVatsMult) {
										vatsMult = setting.second->GetFloat();
										break;
									}
								}
							}
						}
						if (!Configs::enableVATS) {
							a_hitData.baseDamage /= vatsMult;
							a_hitData.calculatedBaseDamage /= vatsMult;
							a_hitData.totalDamage /= vatsMult;
							a_hitData.blockedDamage /= vatsMult;
							a_hitData.reducedDamage /= vatsMult;
						}
					}

					RE::TESObjectREFR* attacker = a_hitData.attackerHandle.get().get();
					if (attacker) {
						if (a_actor == RE::PlayerCharacter::GetSingleton() && a_actor != attacker) {
							a_hitData.baseDamage *= Configs::playerDamageIncomingMult;
							a_hitData.calculatedBaseDamage *= Configs::playerDamageIncomingMult;
							a_hitData.totalDamage *= Configs::playerDamageIncomingMult;
							a_hitData.blockedDamage *= Configs::playerDamageIncomingMult;
							a_hitData.reducedDamage *= Configs::playerDamageIncomingMult;
						} else if (a_actor != RE::PlayerCharacter::GetSingleton() && attacker == RE::PlayerCharacter::GetSingleton()) {
							a_hitData.baseDamage *= Configs::playerDamageOutgoingMult;
							a_hitData.calculatedBaseDamage *= Configs::playerDamageOutgoingMult;
							a_hitData.totalDamage *= Configs::playerDamageOutgoingMult;
							a_hitData.blockedDamage *= Configs::playerDamageOutgoingMult;
							a_hitData.reducedDamage *= Configs::playerDamageOutgoingMult;
						} else if (a_actor != RE::PlayerCharacter::GetSingleton() && a_actor != attacker && attacker != RE::PlayerCharacter::GetSingleton()) {
							a_hitData.baseDamage *= Configs::npcDamageMult;
							a_hitData.calculatedBaseDamage *= Configs::npcDamageMult;
							a_hitData.totalDamage *= Configs::npcDamageMult;
							a_hitData.blockedDamage *= Configs::npcDamageMult;
							a_hitData.reducedDamage *= Configs::npcDamageMult;
						}
					}
				}
				if (Configs::enableLog) {
					logger::info("Adjusted Base Damage: {}", a_hitData.baseDamage);
					logger::info("Adjusted Calculated Base Damage: {}", a_hitData.calculatedBaseDamage);
					logger::info("Adjusted Total Damage: {}", a_hitData.totalDamage);
				}
			}
		}
	}

	typedef void (*FnDoHitMe)(RE::Actor*, const RE::HitData&);
	FnDoHitMe fn = (FnDoHitMe)DoHitMeOrig;
	if (fn)
		fn(a_actor, a_hitData);

	if (Configs::enableLog) {
		logger::info("Final Base Damage: {}", a_hitData.baseDamage);
		logger::info("Final Calculated Base Damage: {}", a_hitData.calculatedBaseDamage);
		logger::info("Final Total Damage: {}", a_hitData.totalDamage);
	}
}

void Hooks::HookedAdjustAttackDamage(RE::HitData* a_hitData)
{
	bool revertDamage = false;
	float calculatedBaseDamage = a_hitData->calculatedBaseDamage;
	float totalDamage = a_hitData->totalDamage;
	if (Configs::enableMod) {
		if (a_hitData->source.object && (a_hitData->flags & 0x4000) == 0) {
			RE::TESObjectWEAP* weap = a_hitData->source.object->As<RE::TESObjectWEAP>();
			if (weap && weap->weaponData.type == RE::WEAPON_TYPE::kGun) {
				revertDamage = true;
			}
		}
	}

	typedef void (*FnAdjustAttackDamage)(RE::HitData*);
	FnAdjustAttackDamage fn = (FnAdjustAttackDamage)AdjustAttackDamageOrig;
	if (fn)
		fn(a_hitData);

	if (revertDamage) {
		a_hitData->calculatedBaseDamage = calculatedBaseDamage;
		a_hitData->totalDamage = totalDamage;
	}
}

//Args: ENTRY_POINT, Actor*, TESObjectWEAP**, void*, float*
bool Hooks::HookedHandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_actor, ...)
{
	va_list args;

	va_start(args, a_actor);
	RE::TESObjectWEAP** weap = va_arg(args, RE::TESObjectWEAP**);
	void* instance = va_arg(args, void*);
	float* val = va_arg(args, float*);
	va_end(args);

	if (Configs::enableMod) {
		if ((*weap)->weaponData.type == RE::WEAPON_TYPE::kGun) {
			return false;
		}
	}

	typedef bool (*FnHandleEntryPoint)(RE::BGSEntryPoint::ENTRY_POINT, RE::Actor*, ...);
	FnHandleEntryPoint fn = (FnHandleEntryPoint)HandleEntryPointOrig;
	if (fn)
		return fn(a_entryPoint, a_actor, weap, instance, val);
	return false;
}
