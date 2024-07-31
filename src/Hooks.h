#pragma once

namespace Hooks {
	void InitializeHooks();
	void HookedDoHitMe(RE::Actor* a_actor, RE::HitData& a_hitData);
	void HookedAdjustAttackDamage(RE::HitData* a_hitData);
	bool HookedHandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_actor, ...);
}
