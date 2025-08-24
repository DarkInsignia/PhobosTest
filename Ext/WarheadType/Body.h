#pragma once
#include <WarheadTypeClass.h>
#include <SuperWeaponTypeClass.h>
#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Enum.h>
#include <New/Type/ShieldTypeClass.h>
#include <Ext/Bullet/Body.h>
#include <Ext/Techno/Body.h>
#include <New/Type/Affiliated/TypeConvertGroup.h>

typedef std::vector<std::tuple< std::vector<int>, std::vector<int>, TransactValueType>> TransactData;

class WarheadTypeExt
{
public:
	using base_type = WarheadTypeClass;

	static constexpr DWORD Canary = 0x22222222;
	static constexpr size_t ExtPointerOffset = 0x18;

	class ExtData final : public Extension<WarheadTypeClass>
	{
	public:

		Valueable<int> Reveal;
		Valueable<int> CreateGap;
		Valueable<int> TransactMoney;
		Valueable<bool> TransactMoney_Display;
		Valueable<AffectedHouse> TransactMoney_Display_Houses;
		Valueable<bool> TransactMoney_Display_AtFirer;
		Valueable<Point2D> TransactMoney_Display_Offset;

		Valueable<int> StealMoney_Amount;
		Valueable<bool> StealMoney_Display;
		Valueable<AffectedHouse> StealMoney_Display_Houses;
		Valueable<Point2D> StealMoney_Display_Offset;
		
		Valueable<bool> Transact;
		Valueable<bool> Transact_SpreadAmongTargets;
		Valueable<int> Transact_Experience_Source_Flat;
		Valueable<double> Transact_Experience_Source_Percent;
		Valueable<bool> Transact_Experience_Source_Percent_CalcFromTarget;
		Valueable<int> Transact_Experience_Target_Flat;
		Valueable<double> Transact_Experience_Target_Percent;
		Valueable<bool> Transact_Experience_Target_Percent_CalcFromSource;
		Valueable<bool> Transact_Experience_IgnoreNotTrainable;
		
		TechnoClass* IntendedTarget;
		
		NullableVector<AnimTypeClass*> SplashList;
		Valueable<bool> SplashList_PickRandom;
		Valueable<bool> SplashList_CreateAll;
		Valueable<int> SplashList_CreationInterval;
		Valueable<Leptons> SplashList_ScatterMin;
		Valueable<Leptons> SplashList_ScatterMax;
		Valueable<bool> AnimList_PickRandom;
		Valueable<bool> AnimList_CreateAll;
		Valueable<int> AnimList_CreationInterval;
		Valueable<Leptons> AnimList_ScatterMin;
		Valueable<Leptons> AnimList_ScatterMax;
		Valueable<bool> CreateAnimsOnZeroDamage;
		Valueable<bool> Conventional_IgnoreUnits;
		Valueable<bool> RemoveDisguise;
		Valueable<bool> RemoveMindControl;
		Nullable<bool> RemoveParasite;
		Valueable<bool> DecloakDamagedTargets;
		Valueable<bool> ShakeIsLocal;
		Valueable<bool> ApplyModifiersOnNegativeDamage;
		Valueable<bool> PenetratesIronCurtain;
		Nullable<bool> PenetratesForceShield;
		Valueable<double> Rocker_AmplitudeMultiplier;
		Nullable<int> Rocker_AmplitudeOverride;

		Valueable<double> Crit_Chance;
		Valueable<bool> Crit_ApplyChancePerTarget;
		Valueable<int> Crit_ExtraDamage;
		Valueable<bool> Crit_ExtraDamage_ApplyFirepowerMult;
		Valueable<WarheadTypeClass*> Crit_Warhead;
		Valueable<bool> Crit_Warhead_FullDetonation;
		Valueable<AffectedTarget> Crit_Affects;
		Valueable<AffectedHouse> Crit_AffectsHouses;
		ValueableVector<AnimTypeClass*> Crit_AnimList;
		Nullable<bool> Crit_AnimList_PickRandom;
		Nullable<bool> Crit_AnimList_CreateAll;
		ValueableVector<AnimTypeClass*> Crit_ActiveChanceAnims;
		Valueable<bool> Crit_AnimOnAffectedTargets;
		Valueable<double> Crit_AffectBelowPercent;
		Valueable<double> Crit_AffectAbovePercent;
		Valueable<bool> Crit_SuppressWhenIntercepted;

		Nullable<AnimTypeClass*> MindControl_Anim;
		Nullable<int> MindControl_ThreatDelay;

		Valueable<bool> Shield_Penetrate;
		Valueable<bool> Shield_Break;
		Valueable<AnimTypeClass*> Shield_BreakAnim;
		Valueable<AnimTypeClass*> Shield_HitAnim;
		Valueable<bool> Shield_SkipHitAnim;
		Valueable<bool> Shield_HitFlash;
		Nullable<WeaponTypeClass*> Shield_BreakWeapon;

		Nullable<double> Shield_AbsorbPercent;
		Nullable<double> Shield_PassPercent;
		Nullable<int> Shield_ReceivedDamage_Minimum;
		Nullable<int> Shield_ReceivedDamage_Maximum;
		Valueable<double> Shield_ReceivedDamage_MinMultiplier;
		Valueable<double> Shield_ReceivedDamage_MaxMultiplier;

		Valueable<int> Shield_Respawn_Duration;
		Nullable<double> Shield_Respawn_Amount;
		Valueable<int> Shield_Respawn_Rate;
		Valueable<bool> Shield_Respawn_RestartTimer;
		Valueable<int> Shield_SelfHealing_Duration;
		Nullable<double> Shield_SelfHealing_Amount;
		Valueable<int> Shield_SelfHealing_Rate;
		Nullable<bool> Shield_SelfHealing_RestartInCombat;
		Valueable<int> Shield_SelfHealing_RestartInCombatDelay;
		Valueable<bool> Shield_SelfHealing_RestartTimer;

		std::vector<Powerup> SpawnsCrate_Types;
		std::vector<int> SpawnsCrate_Weights;

		ValueableVector<ShieldTypeClass*> Shield_AttachTypes;
		ValueableVector<ShieldTypeClass*> Shield_RemoveTypes;
		Valueable<bool> Shield_RemoveAll;
		Valueable<bool> Shield_ReplaceOnly;
		Valueable<bool> Shield_ReplaceNonRespawning;
		Valueable<bool> Shield_InheritStateOnReplace;
		Valueable<int> Shield_MinimumReplaceDelay;
		ValueableVector<ShieldTypeClass*> Shield_AffectTypes;
		NullableVector<ShieldTypeClass*> Shield_Penetrate_Types;
		NullableVector<ShieldTypeClass*> Shield_Break_Types;
		NullableVector<ShieldTypeClass*> Shield_Respawn_Types;
		NullableVector<ShieldTypeClass*> Shield_SelfHealing_Types;

		Valueable<int> NotHuman_DeathSequence;
		ValueableIdxVector<SuperWeaponTypeClass> LaunchSW;
		Valueable<bool> LaunchSW_RealLaunch;
		Valueable<bool> LaunchSW_IgnoreInhibitors;
		Valueable<bool> LaunchSW_IgnoreDesignators;
		Valueable<bool> LaunchSW_DisplayMoney;
		Valueable<AffectedHouse> LaunchSW_DisplayMoney_Houses;
		Valueable<Point2D> LaunchSW_DisplayMoney_Offset;

		Valueable<bool> AllowDamageOnSelf;
		NullableVector<AnimTypeClass*> DebrisAnims;
		Valueable<bool> Debris_Conventional;
		Nullable<bool> DebrisTypes_Limit;
		ValueableVector<int> DebrisMinimums;

		Valueable<bool> DetonateOnAllMapObjects;
		Valueable<bool> DetonateOnAllMapObjects_Full;
		Valueable<bool> DetonateOnAllMapObjects_RequireVerses;
		Valueable<AffectedTarget> DetonateOnAllMapObjects_AffectTargets;
		Valueable<AffectedHouse> DetonateOnAllMapObjects_AffectHouses;
		ValueableVector<TechnoTypeClass*> DetonateOnAllMapObjects_AffectTypes;
		ValueableVector<TechnoTypeClass*> DetonateOnAllMapObjects_IgnoreTypes;

		std::vector<TypeConvertGroup> Convert_Pairs;
		AEAttachInfoTypeClass AttachEffects;

		Valueable<bool> InflictLocomotor;
		Valueable<bool> RemoveInflictedLocomotor;

		Valueable<AffectedTarget> Parasite_CullingTarget;
		NullableIdx<AnimTypeClass> Parasite_GrappleAnim;

		Valueable<bool> Nonprovocative;

		Nullable<int> CombatLightDetailLevel;
		Valueable<double> CombatLightChance;
		Valueable<bool> CLIsBlack;
		Nullable<bool> Particle_AlphaImageIsLightFlash;

		Nullable<double> DamageOwnerMultiplier;
		Nullable<double> DamageAlliesMultiplier;
		Nullable<double> DamageEnemiesMultiplier;
		Valueable<double> DamageSourceHealthMultiplier;
		Valueable<double> DamageTargetHealthMultiplier;

		Valueable<bool> SuppressRevengeWeapons;
		ValueableVector<WeaponTypeClass*> SuppressRevengeWeapons_Types;
		Valueable<bool> SuppressReflectDamage;
		ValueableVector<AttachEffectTypeClass*> SuppressReflectDamage_Types;
		std::vector<std::string> SuppressReflectDamage_Groups;

		Valueable<bool> BuildingSell;
		Valueable<bool> BuildingSell_IgnoreUnsellable;
		Valueable<bool> BuildingUndeploy;
		Valueable<bool> BuildingUndeploy_Leave;

		Nullable<bool> CombatAlert_Suppress;

		Valueable<WeaponTypeClass*> KillWeapon;
		Valueable<WeaponTypeClass*> KillWeapon_OnFirer;
		Valueable<AffectedHouse> KillWeapon_AffectsHouses;
		Valueable<AffectedHouse> KillWeapon_OnFirer_AffectsHouses;
		Valueable<AffectedTarget> KillWeapon_Affects;
		Valueable<AffectedTarget> KillWeapon_OnFirer_Affects;

		Valueable<int> ElectricAssaultLevel;

		Valueable<AffectedTarget> AirstrikeTargets;

		Valueable<double> AffectsBelowPercent;
		Valueable<double> AffectsAbovePercent;
		Valueable<bool> AffectsNeutral;

		// Ares tags
		// http://ares-developers.github.io/Ares-docs/new/warheads/general.html
		Valueable<bool> AffectsEnemies;
		Nullable<bool> AffectsOwner;
		Valueable<bool> EffectsRequireVerses;
		Valueable<bool> Malicious;

		double Crit_RandomBuffer;
		double Crit_CurrentChance;
		bool Crit_Active;
		bool InDamageArea;
		bool WasDetonatedOnAllMapObjects;
		bool Splashed;
		bool Reflected;
		int RemainingAnimCreationInterval;
		bool PossibleCellSpreadDetonate;
		bool HealthCheck;
		TechnoClass* DamageAreaTarget;

		Valueable<bool> CanKill;

		std::map<std::string, AnimTypeClass*> ArmorHitAnim;
		static std::map<std::string, std::string> ArmorTypeInheritance; // maps custom armor -> base armor

	private:
		Valueable<double> Shield_Respawn_Rate_InMinutes;
		Valueable<double> Shield_SelfHealing_Rate_InMinutes;

	public:
		ExtData(WarheadTypeClass* OwnerObject) : Extension<WarheadTypeClass>(OwnerObject)
			, Reveal { 0 }
			, CreateGap { 0 }
			, TransactMoney { 0 }
			, TransactMoney_Display { false }
			, TransactMoney_Display_Houses { AffectedHouse::All }
			, TransactMoney_Display_AtFirer { false }
			, TransactMoney_Display_Offset { { 0, 0 } }
			, StealMoney_Amount { 0 }
			, StealMoney_Display { false }
			, StealMoney_Display_Houses { AffectedHouse::All }
			, StealMoney_Display_Offset { { 0, 0 } }
			, Transact { false }
			, Transact_SpreadAmongTargets { false }
			, Transact_Experience_Source_Flat { 0 }
			, Transact_Experience_Source_Percent { 0.0 }
			, Transact_Experience_Source_Percent_CalcFromTarget { false }
			, Transact_Experience_Target_Flat { 0 }
			, Transact_Experience_Target_Percent { 0.0 }
			, Transact_Experience_Target_Percent_CalcFromSource { false }
			, Transact_Experience_IgnoreNotTrainable { true }
			, IntendedTarget { nullptr }
			, SplashList {}
			, SplashList_PickRandom { false }
			, SplashList_CreateAll { false }
			, SplashList_CreationInterval { 0 }
			, SplashList_ScatterMin { Leptons(0) }
			, SplashList_ScatterMax { Leptons(0) }
			, AnimList_PickRandom { false }
			, AnimList_CreateAll { false }
			, AnimList_CreationInterval { 0 }
			, AnimList_ScatterMin { Leptons(0) }
			, AnimList_ScatterMax { Leptons(0) }
			, CreateAnimsOnZeroDamage { false }
			, Conventional_IgnoreUnits { false }
			, RemoveDisguise { false }
			, RemoveMindControl { false }
			, RemoveParasite {}
			, DecloakDamagedTargets { true }
			, ShakeIsLocal { false }
			, ApplyModifiersOnNegativeDamage { false }
			, PenetratesIronCurtain { false }
			, PenetratesForceShield {}
			, Rocker_AmplitudeMultiplier { 1.0 }
			, Rocker_AmplitudeOverride { }

			, Crit_Chance { 0.0 }
			, Crit_ApplyChancePerTarget { false }
			, Crit_ExtraDamage { 0 }
			, Crit_ExtraDamage_ApplyFirepowerMult { false }
			, Crit_Warhead {}
			, Crit_Warhead_FullDetonation { true }
			, Crit_Affects { AffectedTarget::All }
			, Crit_AffectsHouses { AffectedHouse::All }
			, Crit_AnimList {}
			, Crit_AnimList_PickRandom {}
			, Crit_AnimList_CreateAll {}
			, Crit_ActiveChanceAnims {}
			, Crit_AnimOnAffectedTargets { false }
			, Crit_AffectBelowPercent { 1.0 }
			, Crit_AffectAbovePercent { 0.0 }
			, Crit_SuppressWhenIntercepted { false }

			, MindControl_Anim {}
			, MindControl_ThreatDelay {}

			, Shield_Penetrate { false }
			, Shield_Break { false }
			, Shield_BreakAnim {}
			, Shield_HitAnim {}
			, Shield_SkipHitAnim { false }
			, Shield_HitFlash { true }
			, Shield_BreakWeapon {}
			, Shield_AbsorbPercent {}
			, Shield_PassPercent {}
			, Shield_ReceivedDamage_Minimum {}
			, Shield_ReceivedDamage_Maximum {}
			, Shield_ReceivedDamage_MinMultiplier { 1.0 }
			, Shield_ReceivedDamage_MaxMultiplier { 1.0 }

			, Shield_Respawn_Duration { 0 }
			, Shield_Respawn_Amount { 0.0 }
			, Shield_Respawn_Rate { -1 }
			, Shield_Respawn_Rate_InMinutes { -1.0 }
			, Shield_Respawn_RestartTimer { false }
			, Shield_SelfHealing_Duration { 0 }
			, Shield_SelfHealing_Amount { }
			, Shield_SelfHealing_Rate { -1 }
			, Shield_SelfHealing_Rate_InMinutes { -1.0 }
			, Shield_SelfHealing_RestartInCombat {}
			, Shield_SelfHealing_RestartInCombatDelay { -1 }
			, Shield_SelfHealing_RestartTimer { false }
			, Shield_AttachTypes {}
			, Shield_RemoveTypes {}
			, Shield_RemoveAll { false }
			, Shield_ReplaceOnly { false }
			, Shield_ReplaceNonRespawning { false }
			, Shield_InheritStateOnReplace { false }
			, Shield_MinimumReplaceDelay { 0 }
			, Shield_AffectTypes {}
			, Shield_Penetrate_Types {}
			, Shield_Break_Types {}
			, Shield_Respawn_Types {}
			, Shield_SelfHealing_Types {}

			, SpawnsCrate_Types {}
			, SpawnsCrate_Weights {}

			, NotHuman_DeathSequence { -1 }
			, LaunchSW {}
			, LaunchSW_RealLaunch { true }
			, LaunchSW_IgnoreInhibitors { false }
			, LaunchSW_IgnoreDesignators { true }
			, LaunchSW_DisplayMoney { false }
			, LaunchSW_DisplayMoney_Houses { AffectedHouse::All }
			, LaunchSW_DisplayMoney_Offset { { 0, 0 } }

			, AllowDamageOnSelf { false }
			, DebrisAnims {}
			, Debris_Conventional { false }
			, DebrisTypes_Limit {}
			, DebrisMinimums {}

			, DetonateOnAllMapObjects { false }
			, DetonateOnAllMapObjects_Full { true }
			, DetonateOnAllMapObjects_RequireVerses { false }
			, DetonateOnAllMapObjects_AffectTargets { AffectedTarget::None }
			, DetonateOnAllMapObjects_AffectHouses { AffectedHouse::None }
			, DetonateOnAllMapObjects_AffectTypes {}
			, DetonateOnAllMapObjects_IgnoreTypes {}

			, Convert_Pairs {}
			, AttachEffects {}

			, InflictLocomotor { false }
			, RemoveInflictedLocomotor { false }

			, Parasite_CullingTarget { AffectedTarget::Infantry }
			, Parasite_GrappleAnim {}

			, Nonprovocative { false }

			, CombatLightDetailLevel {}
			, CombatLightChance { 1.0 }
			, CLIsBlack { false }
			, Particle_AlphaImageIsLightFlash {}

			, DamageOwnerMultiplier {}
			, DamageAlliesMultiplier {}
			, DamageEnemiesMultiplier {}
			, DamageSourceHealthMultiplier { 0.0 }
			, DamageTargetHealthMultiplier { 0.0 }

			, SuppressRevengeWeapons { false }
			, SuppressRevengeWeapons_Types {}
			, SuppressReflectDamage { false }
			, SuppressReflectDamage_Types {}
			, SuppressReflectDamage_Groups {}

			, BuildingSell { false }
			, BuildingSell_IgnoreUnsellable { false }
			, BuildingUndeploy { false }
			, BuildingUndeploy_Leave { false }

			, CombatAlert_Suppress {}

			, ElectricAssaultLevel { 1 }

			, AirstrikeTargets { AffectedTarget::Building }

			, AffectsBelowPercent { 1.0 }
			, AffectsAbovePercent { 0.0 }
			, AffectsNeutral { true }

			, AffectsEnemies { true }
			, AffectsOwner {}
			, EffectsRequireVerses { true }
			, Malicious { true }

			, Crit_RandomBuffer { 0.0 }
			, Crit_CurrentChance { 0.0 }
			, Crit_Active { false }
			, InDamageArea { true }
			, WasDetonatedOnAllMapObjects { false }
			, Splashed { false }
			, Reflected { false }
			, RemainingAnimCreationInterval { 0 }
			, PossibleCellSpreadDetonate { false }
			, HealthCheck { false }
			, DamageAreaTarget {}

			, CanKill { true }

			, KillWeapon {}
			, KillWeapon_OnFirer {}
			, KillWeapon_AffectsHouses { AffectedHouse::All }
			, KillWeapon_OnFirer_AffectsHouses { AffectedHouse::All }
			, KillWeapon_Affects { AffectedTarget::All }
			, KillWeapon_OnFirer_Affects { AffectedTarget::All }
		{ }

		void ApplyConvert(HouseClass* pHouse, TechnoClass* pTarget);
		void ApplyLocomotorInfliction(TechnoClass* pTarget);
		void ApplyLocomotorInflictionReset(TechnoClass* pTarget);
	public:
		bool CanTargetHouse(HouseClass* pHouse, TechnoClass* pTechno) const;
		bool CanAffectTarget(TechnoClass* pTarget) const;
		bool CanAffectInvulnerable(TechnoClass* pTarget) const;
		bool EligibleForFullMapDetonation(TechnoClass* pTechno, HouseClass* pOwner) const;
		bool IsHealthInThreshold(TechnoClass* pTarget) const;
		AnimTypeClass* GetArmorHitAnim(Armor armor) const;
		AnimTypeClass* GetArmorHitAnim(const char* armorName) const;
		AnimTypeClass* GetArmorHitAnimWithFallback(const char* armorName) const;
		
		static void LoadArmorTypeInheritance(CCINIClass* pINI);
	static void ReloadAllHitAnimData(CCINIClass* pINI);

		virtual ~ExtData() = default;
		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);

	public:
		// Detonate.cpp
		void Detonate(TechnoClass* pOwner, HouseClass* pHouse, BulletExt::ExtData* pBullet, CoordStruct coords);
		void InterceptBullets(TechnoClass* pOwner, BulletClass* pInterceptor, const CoordStruct& coords);
		DamageAreaResult DamageAreaWithTarget(const CoordStruct& coords, int damage, TechnoClass* pSource, WarheadTypeClass* pWH, bool affectsTiberium, HouseClass* pSourceHouse, TechnoClass* pTarget);
	private:
		void DetonateOnOneUnit(HouseClass* pHouse, TechnoClass* pTarget, TechnoClass* pOwner = nullptr, bool bulletWasIntercepted = false);
		void ApplyRemoveDisguise(HouseClass* pHouse, TechnoClass* pTarget);
		void ApplyRemoveMindControl(TechnoClass* pTarget);
		void ApplyCrit(HouseClass* pHouse, TechnoClass* pTarget, TechnoClass* Owner);
		void ApplyShieldModifiers(TechnoClass* pTarget);
		void ApplyAttachEffects(TechnoClass* pTarget, HouseClass* pInvokerHouse, TechnoClass* pInvoker);
		void ApplyStealMoney(TechnoClass* pOwner, TechnoClass* pTarget) const;
		void ApplyBuildingUndeploy(TechnoClass* pTarget);
		double GetCritChance(TechnoClass* pFirer) const;
		
		// Transact.cpp
		void TransactOnOneUnit(TechnoClass* pTarget, TechnoClass* pOwner, int targets);
		void TransactOnAllUnits(std::vector<TechnoClass*>& nVec, HouseClass* pHouse, TechnoClass* pOwner);
		int TransactGetValue(TechnoClass* pTarget, TechnoClass* pOwner, int flat, double percent, bool calcFromTarget);
		TransactData TransactGetSourceAndTarget(TechnoClass* pTarget, TechnoTypeClass* pTargetType, TechnoClass* pOwner, TechnoTypeClass* pOwnerType, int targets);
		int TransactOneValue(TechnoClass* pTechno, TechnoTypeClass* pTechnoType, int transactValue, TransactValueType valueType);
		
		bool CanDealDamage(TechnoClass* pTechno, int damageIn, int distanceFromEpicenter, int& DamageResult, bool effectsRequireDamage = false) const;
		bool CanDealDamage(TechnoClass* pTechno, bool Bypass = false, bool SkipVerses = false, bool checkImmune = true, bool checkLimbo = true) const;
	};

	class ExtContainer final : public Container<WarheadTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);

	static void DetonateAt(WarheadTypeClass* pThis, AbstractClass* pTarget, TechnoClass* pOwner, int damage, HouseClass* pFiringHouse = nullptr);
	static void DetonateAt(WarheadTypeClass* pThis, const CoordStruct& coords, TechnoClass* pOwner, int damage, HouseClass* pFiringHouse = nullptr, AbstractClass* pTarget = nullptr);
};
