#pragma once
#include <TechnoTypeClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

#include <New/Type/ShieldTypeClass.h>
#include <New/Type/LaserTrailTypeClass.h>
#include <New/Type/AttachEffectTypeClass.h>
#include <New/Type/Affiliated/InterceptorTypeClass.h>
#include <New/Type/Affiliated/PassengerDeletionTypeClass.h>
#include <New/Type/DigitalDisplayTypeClass.h>
#include <New/Type/SelectBoxTypeClass.h>
#include <New/Type/Affiliated/DroppodTypeClass.h>
#include <New/Type/Affiliated/TiberiumEaterTypeClass.h>
#include <New/Type/Affiliated/CreateUnitTypeClass.h>

class Matrix3D;
class ParticleSystemTypeClass;
class TechnoTypeExt
{
public:
	using base_type = TechnoTypeClass;

	static constexpr DWORD Canary = 0x11111111;
	static constexpr size_t ExtPointerOffset = 0xDF4;

	class ExtData final : public Extension<TechnoTypeClass>
	{
	public:
		Valueable<bool> HealthBar_Hide;
		Valueable<CSFText> UIDescription;
		Valueable<bool> LowSelectionPriority;
		PhobosFixedString<0x20> GroupAs;
		Valueable<int> RadarJamRadius;
		Nullable<int> InhibitorRange;
		Nullable<int> DesignatorRange;
		
		// Custom armor name for HitAnim system
		std::string CustomArmorName;
		Valueable<float> FactoryPlant_Multiplier;
		Valueable<Leptons> MindControlRangeLimit;
		Valueable<AffectedHouse> MindControlLink_VisibleToHouse;

		std::unique_ptr<InterceptorTypeClass> InterceptorType;

		Valueable<PartialVector3D<int>> TurretOffset;
		Nullable<bool> TurretShadow;
		Valueable<int> ShadowIndex_Frame;
		std::map<int, int> ShadowIndices;
		Valueable<bool> Spawner_LimitRange;
		Valueable<int> Spawner_ExtraLimitRange;
		int SpawnerRange;
		int EliteSpawnerRange;
		Nullable<int> Spawner_DelayFrames;
		Valueable<bool> Spawner_AttackImmediately;
		Valueable<bool> Spawner_UseTurretFacing;
		Nullable<bool> Harvester_Counted;
		Valueable<bool> Promote_IncludeSpawns;
		Valueable<bool> ImmuneToCrit;
		Valueable<bool> MultiMindControl_ReleaseVictim;
		Valueable<int> CameoPriority;
		Valueable<bool> NoManualMove;
		Nullable<bool> AllowFire_IroncurtainedTarget;
		Valueable<bool> IsDummy;
		Nullable<int> InitialStrength;
		Valueable<bool> ReloadInTransport;
		Valueable<bool> ForbidParallelAIQueues;

		int TintColorAirstrike;
		Nullable<int> LaserTargetColor;
		Nullable<ColorStruct> AirstrikeLineColor;

		Valueable<ShieldTypeClass*> ShieldType;
		std::unique_ptr<PassengerDeletionTypeClass> PassengerDeletionType;
		std::unique_ptr<DroppodTypeClass> DroppodType;
		std::unique_ptr<TiberiumEaterTypeClass> TiberiumEaterType;

		Nullable<float> HarvesterDumpAmount;

		Valueable<int> Ammo_AddOnDeploy;
		Valueable<int> Ammo_AutoDeployMinimumAmount;
		Valueable<int> Ammo_AutoDeployMaximumAmount;
		Valueable<int> Ammo_DeployUnlockMinimumAmount;
		Valueable<int> Ammo_DeployUnlockMaximumAmount;

		Nullable<AutoDeathBehavior> AutoDeath_Behavior;
		Valueable<AnimTypeClass*> AutoDeath_VanishAnimation;
		Valueable<bool> AutoDeath_OnAmmoDepletion;
		Valueable<int> AutoDeath_AfterDelay;
		ValueableVector<TechnoTypeClass*> AutoDeath_TechnosDontExist;
		Valueable<bool> AutoDeath_TechnosDontExist_Any;
		Valueable<bool> AutoDeath_TechnosDontExist_AllowLimboed;
		Valueable<AffectedHouse> AutoDeath_TechnosDontExist_Houses;
		ValueableVector<TechnoTypeClass*> AutoDeath_TechnosExist;
		Valueable<bool> AutoDeath_TechnosExist_Any;
		Valueable<bool> AutoDeath_TechnosExist_AllowLimboed;
		Valueable<AffectedHouse> AutoDeath_TechnosExist_Houses;

		Valueable<SlaveChangeOwnerType> Slaved_OwnerWhenMasterKilled;
		NullableIdx<VocClass> SlavesFreeSound;
		NullableIdx<VocClass> SellSound;
		NullableIdx<VoxClass> EVA_Sold;

		Nullable<bool> CombatAlert;
		Nullable<bool> CombatAlert_NotBuilding;
		Nullable<bool> CombatAlert_UseFeedbackVoice;
		Nullable<bool> CombatAlert_UseAttackVoice;
		Nullable<bool> CombatAlert_UseEVA;
		NullableIdx<VoxClass> CombatAlert_EVA;

		NullableIdx<VocClass> VoiceCreated;
		NullableIdx<VocClass> VoicePickup; // Used by carryalls instead of VoiceMove if set.

		Nullable<AnimTypeClass*> WarpOut;
		Nullable<AnimTypeClass*> WarpIn;
		Nullable<AnimTypeClass*> WarpAway;
		Nullable<bool> ChronoTrigger;
		Nullable<int> ChronoDistanceFactor;
		Nullable<int> ChronoMinimumDelay;
		Nullable<int> ChronoRangeMinimum;
		Nullable<int> ChronoDelay;
		Nullable<int> ChronoSpherePreDelay;
		Nullable<int> ChronoSphereDelay;

		Valueable<WeaponTypeClass*> WarpInWeapon;
		Nullable<WeaponTypeClass*> WarpInMinRangeWeapon;
		Valueable<WeaponTypeClass*> WarpOutWeapon;
		Valueable<bool> WarpInWeapon_UseDistanceAsDamage;

		int SubterraneanSpeed;
		Nullable<int> SubterraneanHeight;

		ValueableVector<AnimTypeClass*> OreGathering_Anims;
		ValueableVector<int> OreGathering_Tiberiums;
		ValueableVector<int> OreGathering_FramesPerDir;

		std::vector<std::vector<CoordStruct>> WeaponBurstFLHs;
		std::vector<std::vector<CoordStruct>> EliteWeaponBurstFLHs;
		std::vector<CoordStruct> AlternateFLHs;
		Valueable<bool> AlternateFLH_OnTurret;

		Valueable<bool> DestroyAnim_Random;
		Valueable<bool> NotHuman_RandomDeathSequence;

		Valueable<InfantryTypeClass*> DefaultDisguise;
		Valueable<bool> UseDisguiseMovementSpeed;

		Nullable<int> OpenTopped_RangeBonus;
		Nullable<float> OpenTopped_DamageMultiplier;
		Nullable<int> OpenTopped_WarpDistance;
		Valueable<bool> OpenTopped_IgnoreRangefinding;
		Valueable<bool> OpenTopped_AllowFiringIfDeactivated;
		Valueable<bool> OpenTopped_ShareTransportTarget;
		Valueable<bool> OpenTopped_UseTransportRangeModifiers;
		Valueable<bool> OpenTopped_CheckTransportDisableWeapons;

		Valueable<bool> AutoFire;
		Valueable<bool> AutoFire_TargetSelf;

		Valueable<bool> NoSecondaryWeaponFallback;
		Valueable<bool> NoSecondaryWeaponFallback_AllowAA;

		Valueable<int> NoAmmoWeapon;
		Valueable<int> NoAmmoAmount;

		// ExtraFire weapons
		ValueableVector<WeaponTypeClass*> ExtraFire_Primary;
		ValueableVector<WeaponTypeClass*> ExtraFire_Secondary;
		NullableVector<WeaponTypeClass*> ExtraFire_ElitePrimary;
		NullableVector<WeaponTypeClass*> ExtraFire_EliteSecondary;
		
		// ExtraFire FLH coordinates
		Valueable<CoordStruct> ExtraFire_PrimaryFLH;
		Valueable<CoordStruct> ExtraFire_SecondaryFLH;
		Nullable<CoordStruct> ExtraFire_ElitePrimaryFLH;
		Nullable<CoordStruct> ExtraFire_EliteSecondaryFLH;

		Valueable<bool> JumpjetRotateOnCrash;
		Nullable<int> ShadowSizeCharacteristicHeight;

		Valueable<bool> DeployingAnim_AllowAnyDirection;
		Valueable<bool> DeployingAnim_KeepUnitVisible;
		Valueable<bool> DeployingAnim_ReverseForUndeploy;
		Valueable<bool> DeployingAnim_UseUnitDrawer;

		Valueable<CSFText> EnemyUIName;

		bool ForceWeapon_Check;
		Valueable<int> ForceWeapon_Naval_Decloaked;
		Valueable<int> ForceWeapon_Cloaked;
		Valueable<int> ForceWeapon_Disguised;
		Valueable<int> ForceWeapon_UnderEMP;
		Valueable<bool> ForceWeapon_InRange_TechnoOnly;
		ValueableVector<int> ForceWeapon_InRange;
		ValueableVector<double> ForceWeapon_InRange_Overrides;
		Valueable<bool> ForceWeapon_InRange_ApplyRangeModifiers;
		ValueableVector<int> ForceAAWeapon_InRange;
		ValueableVector<double> ForceAAWeapon_InRange_Overrides;
		Valueable<bool> ForceAAWeapon_InRange_ApplyRangeModifiers;
		Valueable<int> ForceWeapon_Buildings;
		Valueable<int> ForceWeapon_Defenses;
		Valueable<int> ForceWeapon_Infantry;
		Valueable<int> ForceWeapon_Naval_Units;
		Valueable<int> ForceWeapon_Units;
		Valueable<int> ForceWeapon_Aircraft;
		Valueable<int> ForceAAWeapon_Infantry;
		Valueable<int> ForceAAWeapon_Units;
		Valueable<int> ForceAAWeapon_Aircraft;

		Valueable<bool> Ammo_Shared;
		Valueable<int> Ammo_Shared_Group;

		Nullable<SelfHealGainType> SelfHealGainType;
		Valueable<bool> Passengers_SyncOwner;
		Valueable<bool> Passengers_SyncOwner_RevertOnExit;

		Nullable<bool> IronCurtain_KeptOnDeploy;
		Nullable<IronCurtainEffect> IronCurtain_Effect;
		Nullable<WarheadTypeClass*> IronCurtain_KillWarhead;
		Nullable<bool> ForceShield_KeptOnDeploy;
		Nullable<IronCurtainEffect> ForceShield_Effect;
		Nullable<WarheadTypeClass*> ForceShield_KillWarhead;
		Valueable<bool> Explodes_KillPassengers;
		Valueable<bool> Explodes_DuringBuildup;
		Nullable<int> DeployFireWeapon;
		Valueable<TargetZoneScanType> TargetZoneScanType;

		Promotable<SHPStruct*> Insignia;
		Valueable<Vector3D<int>> InsigniaFrames;
		Promotable<int> InsigniaFrame;
		Nullable<bool> Insignia_ShowEnemy;
		std::vector<Promotable<SHPStruct*>> Insignia_Weapon;
		std::vector<Promotable<int>> InsigniaFrame_Weapon;
		std::vector<Valueable<Vector3D<int>>> InsigniaFrames_Weapon;
		std::vector<Promotable<SHPStruct*>> Insignia_Passengers;
		std::vector<Promotable<int>> InsigniaFrame_Passengers;
		std::vector<Valueable<Vector3D<int>>> InsigniaFrames_Passengers;

		Valueable<bool> JumpjetTilt;
		Valueable<double> JumpjetTilt_ForwardAccelFactor;
		Valueable<double> JumpjetTilt_ForwardSpeedFactor;
		Valueable<double> JumpjetTilt_SidewaysRotationFactor;
		Valueable<double> JumpjetTilt_SidewaysSpeedFactor;

		Nullable<bool> TiltsWhenCrushes_Vehicles;
		Nullable<bool> TiltsWhenCrushes_Overlays;
		Nullable<double> CrushForwardTiltPerFrame;
		Valueable<double> CrushOverlayExtraForwardTilt;
		Valueable<double> CrushSlowdownMultiplier;
		Valueable<bool> SkipCrushSlowdown;

		Valueable<bool> DigitalDisplay_Disable;
		ValueableVector<DigitalDisplayTypeClass*> DigitalDisplayTypes;

		Nullable<SelectBoxTypeClass*> SelectBox;
		Valueable<bool> HideSelectBox;

		Valueable<int> AmmoPipFrame;
		Valueable<int> EmptyAmmoPipFrame;
		Valueable<int> AmmoPipWrapStartFrame;
		Nullable<Point2D> AmmoPipSize;
		Valueable<Point2D> AmmoPipOffset;

		Valueable<bool> ShowSpawnsPips;
		Valueable<int> SpawnsPipFrame;
		Valueable<int> EmptySpawnsPipFrame;
		Nullable<Point2D> SpawnsPipSize;
		Valueable<Point2D> SpawnsPipOffset;

		Nullable<Leptons> SpawnDistanceFromTarget;
		Nullable<int> SpawnHeight;
		Nullable<int> LandingDir;

		Valueable<TechnoTypeClass*> Convert_HumanToComputer;
		Valueable<TechnoTypeClass*> Convert_ComputerToHuman;
		Valueable<bool> Convert_ResetMindControl;

		Valueable<double> CrateGoodie_RerollChance;

		Nullable<ColorStruct> Tint_Color;
		Valueable<double> Tint_Intensity;
		Valueable<AffectedHouse> Tint_VisibleToHouses;

		Valueable<WeaponTypeClass*> RevengeWeapon;
		Valueable<AffectedHouse> RevengeWeapon_AffectsHouses;

		AEAttachInfoTypeClass AttachEffects;

		Nullable<bool> RecountBurst;

		ValueableVector<TechnoTypeClass*> BuildLimitGroup_Types;
		ValueableVector<int> BuildLimitGroup_Nums;
		Valueable<int> BuildLimitGroup_Factor;
		Valueable<bool> BuildLimitGroup_ContentIfAnyMatch;
		Valueable<bool> BuildLimitGroup_NotBuildableIfQueueMatch;
		ValueableVector<TechnoTypeClass*> BuildLimitGroup_ExtraLimit_Types;
		ValueableVector<int> BuildLimitGroup_ExtraLimit_Nums;
		ValueableVector<int> BuildLimitGroup_ExtraLimit_MaxCount;
		Valueable<int> BuildLimitGroup_ExtraLimit_MaxNum;

		Nullable<bool> AmphibiousEnter;
		Nullable<bool> AmphibiousUnload;
		Nullable<bool> NoQueueUpToEnter;
		Nullable<bool> NoQueueUpToUnload;
		Valueable<bool> Passengers_BySize;

		Valueable<int> RateDown_Delay;
		Valueable<bool> RateDown_Reset;
		Valueable<int> RateDown_Cover_Value;
		Valueable<int> RateDown_Cover_AmmoBelow;

		Nullable<bool> NoRearm_UnderEMP;
		Nullable<bool> NoRearm_Temporal;
		Nullable<bool> NoReload_UnderEMP;
		Nullable<bool> NoReload_Temporal;
		Nullable<bool> NoTurret_TrackTarget;

		Nullable<AnimTypeClass*> Wake;
		Nullable<AnimTypeClass*> Wake_Grapple;
		Nullable<AnimTypeClass*> Wake_Sinking;

		Nullable<int> AINormalTargetingDelay;
		Nullable<int> PlayerNormalTargetingDelay;
		Nullable<int> AIGuardAreaTargetingDelay;
		Nullable<int> PlayerGuardAreaTargetingDelay;
		Nullable<int> AIAttackMoveTargetingDelay;
		Nullable<int> PlayerAttackMoveTargetingDelay;
		Nullable<bool> DistributeTargetingFrame;

		Nullable<bool> AttackMove_Aggressive;
		Nullable<bool> AttackMove_UpdateTarget;

		Valueable<bool> BunkerableAnyway;
		Valueable<bool> KeepTargetOnMove;
		Valueable<bool> KeepTargetOnMove_NoMorePursuit;
		Valueable<Leptons> KeepTargetOnMove_ExtraDistance;

		Valueable<int> Power;

		Nullable<bool> AllowAirstrike;

		Nullable<TechnoTypeClass*> Image_ConditionYellow;
		Nullable<TechnoTypeClass*> Image_ConditionRed;
		Nullable<UnitTypeClass*> WaterImage_ConditionYellow;
		Nullable<UnitTypeClass*> WaterImage_ConditionRed;

		Nullable<int> InitialSpawnsNumber;
		ValueableVector<AircraftTypeClass*> Spawns_Queue;

		Valueable<Leptons> Spawner_RecycleRange;
		Valueable<AnimTypeClass*> Spawner_RecycleAnim;
		Valueable<CoordStruct> Spawner_RecycleCoord;
		Valueable<bool> Spawner_RecycleOnTurret;

		Nullable<bool> Sinkable;
		Valueable<bool> Sinkable_SquidGrab;
		Valueable<int> SinkSpeed;

		Nullable<double> ProneSpeed;
		Nullable<double> DamagedSpeed;

		Nullable<AnimTypeClass*> Promote_VeteranAnimation;
		Nullable<AnimTypeClass*> Promote_EliteAnimation;

		Nullable<AffectedHouse> RadarInvisibleToHouse;

		struct LaserTrailDataEntry
		{
			ValueableIdx<LaserTrailTypeClass> idxType;
			Valueable<CoordStruct> FLH;
			Valueable<bool> IsOnTurret;
			LaserTrailTypeClass* GetType() const { return LaserTrailTypeClass::Array[idxType].get(); }
		};

		std::vector<LaserTrailDataEntry> LaserTrailData;
		Valueable<bool> OnlyUseLandSequences;
		Nullable<CoordStruct> PronePrimaryFireFLH;
		Nullable<CoordStruct> ProneSecondaryFireFLH;
		Nullable<CoordStruct> DeployedPrimaryFireFLH;
		Nullable<CoordStruct> DeployedSecondaryFireFLH;
		std::vector<std::vector<CoordStruct>> CrouchedWeaponBurstFLHs;
		std::vector<std::vector<CoordStruct>> EliteCrouchedWeaponBurstFLHs;
		std::vector<std::vector<CoordStruct>> DeployedWeaponBurstFLHs;
		std::vector<std::vector<CoordStruct>> EliteDeployedWeaponBurstFLHs;

		Valueable<bool> SuppressKillWeapons;
		ValueableVector<WeaponTypeClass*> SuppressKillWeapons_Types;

		Valueable<bool> DigitalDisplay_Health_FakeAtDisguise;

		NullableVector<int> Overload_Count;
		NullableVector<int> Overload_Damage;
		NullableVector<int> Overload_Frames;
		NullableIdx<VocClass> Overload_DeathSound;
		Nullable<ParticleSystemTypeClass*> Overload_ParticleSys;
		Valueable<int> Overload_ParticleSysCount;

		Valueable<bool> Harvester_CanGuardArea;
		Nullable<bool> HarvesterScanAfterUnload;

		Nullable<bool> ExtendedAircraftMissions_SmoothMoving;
		Nullable<bool> ExtendedAircraftMissions_EarlyDescend;
		Nullable<bool> ExtendedAircraftMissions_RearApproach;

		Valueable<double> FallingDownDamage;
		Nullable<double> FallingDownDamage_Water;

		Valueable<bool> FiringForceScatter;

		Valueable<int> FireUp;
		Valueable<bool> FireUp_ResetInRetarget;
		//Nullable<int> SecondaryFire;

		Nullable<bool> DebrisTypes_Limit;
		ValueableVector<int> DebrisMinimums;

		Valueable<int> EngineerRepairAmount;

		Valueable<bool> AttackMove_Follow;
		Valueable<bool> AttackMove_Follow_IncludeAir;
		Valueable<bool> AttackMove_Follow_IfMindControlIsFull;
		Nullable<bool> AttackMove_StopWhenTargetAcquired;
		Valueable<bool> AttackMove_PursuitTarget;

		Valueable<bool> MultiWeapon;
		ValueableVector<bool> MultiWeapon_IsSecondary;
		Valueable<int> MultiWeapon_SelectCount;
		bool ReadMultiWeapon;

		Nullable<int> BattlePoints;

		ValueableIdx<VocClass> VoiceIFVRepair;
		ValueableVector<int> VoiceWeaponAttacks;
		ValueableVector<int> VoiceEliteWeaponAttacks;

		Nullable<bool> InfantryAutoDeploy;
		Valueable<int> Suspicious;
		Valueable<bool> AttackIfSuspicious;

		ExtData(TechnoTypeClass* OwnerObject) : Extension<TechnoTypeClass>(OwnerObject)
			, HealthBar_Hide { false }
			, UIDescription {}
			, LowSelectionPriority { false }
			, GroupAs { NONE_STR }
			, RadarJamRadius { 0 }
			, InhibitorRange {}
			, DesignatorRange { }
			, CustomArmorName {}
			, FactoryPlant_Multiplier { 1.0 }
			, MindControlRangeLimit {}
			, MindControlLink_VisibleToHouse{ AffectedHouse::All }

			, InterceptorType { nullptr }

			, TurretOffset { { 0, 0, 0 } }
			, TurretShadow { }
			, ShadowIndices { }
			, ShadowIndex_Frame { 0 }
			, Spawner_LimitRange { false }
			, Spawner_ExtraLimitRange { 0 }
			, SpawnerRange { 0 }
			, EliteSpawnerRange { 0 }
			, Spawner_DelayFrames {}
			, Spawner_AttackImmediately { false }
			, Spawner_UseTurretFacing { false }
			, Harvester_Counted {}
			, Promote_IncludeSpawns { false }
			, ImmuneToCrit { false }
			, MultiMindControl_ReleaseVictim { false }
			, CameoPriority { 0 }
			, NoManualMove { false }
			, InitialStrength {}
			, ReloadInTransport { false }
			, ForbidParallelAIQueues { false }
			, TintColorAirstrike { 0 }
			, LaserTargetColor {}
			, AirstrikeLineColor {}
			, ShieldType { nullptr }
			, PassengerDeletionType { nullptr }

			, WarpOut {}
			, WarpIn {}
			, WarpAway {}
			, ChronoTrigger {}
			, ChronoDistanceFactor {}
			, ChronoMinimumDelay {}
			, ChronoRangeMinimum {}
			, ChronoDelay {}
			, ChronoSpherePreDelay {}
			, ChronoSphereDelay {}
			, WarpInWeapon {}
			, WarpInMinRangeWeapon {}
			, WarpOutWeapon {}
			, WarpInWeapon_UseDistanceAsDamage { false }

			, SubterraneanSpeed { -1 }
			, SubterraneanHeight {}

			, OreGathering_Anims {}
			, OreGathering_Tiberiums {}
			, OreGathering_FramesPerDir {}
			, LaserTrailData {}
			, AlternateFLH_OnTurret { true }
			, DestroyAnim_Random { true }
			, NotHuman_RandomDeathSequence { false }

			, DefaultDisguise {}
			, UseDisguiseMovementSpeed {}

			, OpenTopped_RangeBonus {}
			, OpenTopped_DamageMultiplier {}
			, OpenTopped_WarpDistance {}
			, OpenTopped_IgnoreRangefinding { false }
			, OpenTopped_AllowFiringIfDeactivated { true }
			, OpenTopped_ShareTransportTarget { true }
			, OpenTopped_UseTransportRangeModifiers { false }
			, OpenTopped_CheckTransportDisableWeapons { false }

			, AutoFire { false }
			, AutoFire_TargetSelf { false }
			, NoSecondaryWeaponFallback { false }
			, NoSecondaryWeaponFallback_AllowAA { false }
			, NoAmmoWeapon { -1 }
			, NoAmmoAmount { 0 }
			, ExtraFire_Primary {}
			, ExtraFire_Secondary {}
			, ExtraFire_ElitePrimary {}
			, ExtraFire_EliteSecondary {}
			, ExtraFire_PrimaryFLH { { 0, 0, 0 } }
			, ExtraFire_SecondaryFLH { { 0, 0, 0 } }
			, ExtraFire_ElitePrimaryFLH {}
			, ExtraFire_EliteSecondaryFLH {}
			, JumpjetRotateOnCrash { true }
			, ShadowSizeCharacteristicHeight { }
			, DeployingAnim_AllowAnyDirection { false }
			, DeployingAnim_KeepUnitVisible { false }
			, DeployingAnim_ReverseForUndeploy { true }
			, DeployingAnim_UseUnitDrawer { true }

			, HarvesterDumpAmount {}

			, Ammo_AddOnDeploy { 0 }
			, Ammo_AutoDeployMinimumAmount { -1 }
			, Ammo_AutoDeployMaximumAmount { -1 }
			, Ammo_DeployUnlockMinimumAmount { -1 }
			, Ammo_DeployUnlockMaximumAmount { -1 }

			, AutoDeath_Behavior { }
			, AutoDeath_VanishAnimation {}
			, AutoDeath_OnAmmoDepletion { false }
			, AutoDeath_AfterDelay { 0 }
			, AutoDeath_TechnosDontExist {}
			, AutoDeath_TechnosDontExist_Any { false }
			, AutoDeath_TechnosDontExist_AllowLimboed { false }
			, AutoDeath_TechnosDontExist_Houses { AffectedHouse::Owner }
			, AutoDeath_TechnosExist {}
			, AutoDeath_TechnosExist_Any { true }
			, AutoDeath_TechnosExist_AllowLimboed { true }
			, AutoDeath_TechnosExist_Houses { AffectedHouse::Owner }

			, Slaved_OwnerWhenMasterKilled { SlaveChangeOwnerType::Killer }
			, SlavesFreeSound {}
			, SellSound {}
			, EVA_Sold {}

			, CombatAlert {}
			, CombatAlert_NotBuilding {}
			, CombatAlert_UseFeedbackVoice {}
			, CombatAlert_UseAttackVoice {}
			, CombatAlert_UseEVA {}
			, CombatAlert_EVA {}

			, EnemyUIName {}

			, VoiceCreated {}
			, VoicePickup {}

			, ForceWeapon_Check { false }
			, ForceWeapon_Naval_Decloaked { -1 }
			, ForceWeapon_Cloaked { -1 }
			, ForceWeapon_Disguised { -1 }
			, ForceWeapon_UnderEMP { -1 }
			, ForceWeapon_InRange_TechnoOnly { true }
			, ForceWeapon_InRange {}
			, ForceWeapon_InRange_Overrides {}
			, ForceWeapon_InRange_ApplyRangeModifiers { false }
			, ForceAAWeapon_InRange {}
			, ForceAAWeapon_InRange_Overrides {}
			, ForceAAWeapon_InRange_ApplyRangeModifiers { false }
			, ForceWeapon_Buildings { -1 }
			, ForceWeapon_Defenses { -1 }
			, ForceWeapon_Infantry { -1 }
			, ForceWeapon_Naval_Units { -1 }
			, ForceWeapon_Units { -1 }
			, ForceWeapon_Aircraft { -1 }
			, ForceAAWeapon_Infantry { -1 }
			, ForceAAWeapon_Units { -1 }
			, ForceAAWeapon_Aircraft { -1 }

			, Ammo_Shared { false }
			, Ammo_Shared_Group { -1 }

			, SelfHealGainType {}
			, Passengers_SyncOwner { false }
			, Passengers_SyncOwner_RevertOnExit { true }

			, OnlyUseLandSequences { false }

			, PronePrimaryFireFLH {}
			, ProneSecondaryFireFLH {}
			, DeployedPrimaryFireFLH {}
			, DeployedSecondaryFireFLH {}

			, IronCurtain_KeptOnDeploy {}
			, IronCurtain_Effect {}
			, IronCurtain_KillWarhead {}
			, ForceShield_KeptOnDeploy {}
			, ForceShield_Effect {}
			, ForceShield_KillWarhead {}

			, Explodes_KillPassengers { true }
			, Explodes_DuringBuildup { true }
			, DeployFireWeapon {}
			, TargetZoneScanType { TargetZoneScanType::Same }

			, Insignia {}
			, InsigniaFrames { { -1, -1, -1 } }
			, InsigniaFrame { -1 }
			, Insignia_ShowEnemy {}
			, Insignia_Weapon {}
			, InsigniaFrame_Weapon {}
			, InsigniaFrames_Weapon {}
			, Insignia_Passengers {}
			, InsigniaFrame_Passengers {}
			, InsigniaFrames_Passengers {}

			, JumpjetTilt { false }
			, JumpjetTilt_ForwardAccelFactor { 1.0 }
			, JumpjetTilt_ForwardSpeedFactor { 1.0 }
			, JumpjetTilt_SidewaysRotationFactor { 1.0 }
			, JumpjetTilt_SidewaysSpeedFactor { 1.0 }

			, TiltsWhenCrushes_Vehicles {}
			, TiltsWhenCrushes_Overlays {}
			, CrushSlowdownMultiplier { 0.2 }
			, CrushForwardTiltPerFrame {}
			, CrushOverlayExtraForwardTilt { 0.02 }
			, SkipCrushSlowdown { false }

			, DigitalDisplay_Disable { false }
			, DigitalDisplayTypes {}

			, SelectBox {}
			, HideSelectBox { false }

			, AmmoPipFrame { 13 }
			, EmptyAmmoPipFrame { -1 }
			, AmmoPipWrapStartFrame { 14 }
			, AmmoPipSize {}
			, AmmoPipOffset { { 0,0 } }

			, ShowSpawnsPips { true }
			, SpawnsPipFrame { 1 }
			, EmptySpawnsPipFrame { 0 }
			, SpawnsPipSize {}
			, SpawnsPipOffset { { 0,0 } }

			, SpawnDistanceFromTarget {}
			, SpawnHeight {}
			, LandingDir {}
			, DroppodType {}
			, TiberiumEaterType {}

			, Convert_HumanToComputer { }
			, Convert_ComputerToHuman { }
			, Convert_ResetMindControl { false }

			, CrateGoodie_RerollChance { 0.0 }

			, Tint_Color {}
			, Tint_Intensity { 0.0 }
			, Tint_VisibleToHouses { AffectedHouse::All }

			, RevengeWeapon {}
			, RevengeWeapon_AffectsHouses { AffectedHouse::All }

			, AttachEffects {}

			, RecountBurst {}

			, BuildLimitGroup_Types {}
			, BuildLimitGroup_Nums {}
			, BuildLimitGroup_Factor { 1 }
			, BuildLimitGroup_ContentIfAnyMatch { false }
			, BuildLimitGroup_NotBuildableIfQueueMatch { false }
			, BuildLimitGroup_ExtraLimit_Types {}
			, BuildLimitGroup_ExtraLimit_Nums {}
			, BuildLimitGroup_ExtraLimit_MaxCount {}
			, BuildLimitGroup_ExtraLimit_MaxNum { 0 }

			, AmphibiousEnter {}
			, AmphibiousUnload {}
			, NoQueueUpToEnter {}
			, NoQueueUpToUnload {}
			, Passengers_BySize { true }

			, RateDown_Delay { 0 }
			, RateDown_Reset { false }
			, RateDown_Cover_Value { 0 }
			, RateDown_Cover_AmmoBelow { -2 }

			, NoRearm_UnderEMP {}
			, NoRearm_Temporal {}
			, NoReload_UnderEMP {}
			, NoReload_Temporal {}
			, NoTurret_TrackTarget {}

			, Wake { }
			, Wake_Grapple { }
			, Wake_Sinking { }

			, AINormalTargetingDelay {}
			, PlayerNormalTargetingDelay {}
			, AIGuardAreaTargetingDelay {}
			, PlayerGuardAreaTargetingDelay {}
			, AIAttackMoveTargetingDelay {}
			, PlayerAttackMoveTargetingDelay {}
			, DistributeTargetingFrame {}

			, DigitalDisplay_Health_FakeAtDisguise { true }

			, AttackMove_Aggressive {}
			, AttackMove_UpdateTarget {}

			, BunkerableAnyway { false }
			, KeepTargetOnMove { false }
			, KeepTargetOnMove_NoMorePursuit { true }
			, KeepTargetOnMove_ExtraDistance { Leptons(0) }

			, Power { }

			, AllowAirstrike { }

			, Image_ConditionYellow { }
			, Image_ConditionRed { }
			, WaterImage_ConditionYellow { }
			, WaterImage_ConditionRed { }

			, InitialSpawnsNumber { }
			, Spawns_Queue { }

			, Spawner_RecycleRange { Leptons(-1) }
			, Spawner_RecycleAnim { }
			, Spawner_RecycleCoord { {0,0,0} }
			, Spawner_RecycleOnTurret { false }

			, Sinkable { }
			, Sinkable_SquidGrab { true }
			, SinkSpeed { 5 }

			, ProneSpeed { }
			, DamagedSpeed { }

			, SuppressKillWeapons { false }
			, SuppressKillWeapons_Types { }

			, Promote_VeteranAnimation { }
			, Promote_EliteAnimation { }

			, RadarInvisibleToHouse {}

			, Overload_Count {}
			, Overload_Damage {}
			, Overload_Frames {}
			, Overload_DeathSound {}
			, Overload_ParticleSys {}
			, Overload_ParticleSysCount { 5 }

			, Harvester_CanGuardArea { false }
			, HarvesterScanAfterUnload {}

			, ExtendedAircraftMissions_SmoothMoving {}
			, ExtendedAircraftMissions_EarlyDescend {}
			, ExtendedAircraftMissions_RearApproach {}

			, FallingDownDamage { 1.0 }
			, FallingDownDamage_Water {}

			, FiringForceScatter { true }

			, FireUp { -1 }
			, FireUp_ResetInRetarget { true }
			//, SecondaryFire {}

			, DebrisTypes_Limit {}
			, DebrisMinimums {}

			, EngineerRepairAmount { 0 }

			, AttackMove_Follow { false }
			, AttackMove_Follow_IncludeAir { false }
			, AttackMove_Follow_IfMindControlIsFull { false }
			, AttackMove_StopWhenTargetAcquired { }
			, AttackMove_PursuitTarget { false }

			, MultiWeapon { false }
			, MultiWeapon_IsSecondary {}
			, MultiWeapon_SelectCount { 2 }
			, ReadMultiWeapon { false }

			, BattlePoints {}

			, VoiceIFVRepair { -1 }
			, VoiceWeaponAttacks {}
			, VoiceEliteWeaponAttacks {}

			, InfantryAutoDeploy {}
			, Suspicious { 0 }
			, AttackIfSuspicious { false }

		{ }

		void FireExtraWeapons(TechnoClass* pThis, AbstractClass* pTarget, int weaponIndex) const;
		static bool ExtraFireInProgress;

		virtual ~ExtData() = default;
		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void Initialize() override { }

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		void LoadFromINIByWhatAmI(INI_EX& exArtINI, const char* pArtSection);

		void ApplyTurretOffset(Matrix3D* mtx, double factor = 1.0);
		void CalculateSpawnerRange();
		bool IsSecondary(int nWeaponIndex);

		int SelectForceWeapon(TechnoClass* pThis, AbstractClass* pTarget);
		int SelectMultiWeapon(TechnoClass* const pThis, AbstractClass* const pTarget);

		// Ares 0.A
		const char* GetSelectionGroupID() const;

	private:
		template <typename T>
		void Serialize(T& Stm);

		void ParseBurstFLHs(INI_EX& exArtINI, const char* pArtSection, std::vector<std::vector<CoordStruct>>& nFLH, std::vector<std::vector<CoordStruct>>& nEFlh, const char* pPrefixTag);
		void ParseVoiceWeaponAttacks(INI_EX& exINI, const char* pSection, ValueableVector<int>& n, ValueableVector<int>& nE);
	};

	class ExtContainer final : public Container<TechnoTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
	static bool SelectWeaponMutex;

	static void ApplyTurretOffset(TechnoTypeClass* pType, Matrix3D* mtx, double factor = 1.0);
	static TechnoTypeClass* GetTechnoType(ObjectTypeClass* pType);

	static TechnoClass* CreateUnit(CreateUnitTypeClass* pCreateUnit, DirType facing, DirType* secondaryFacing,
	CoordStruct location, HouseClass* pOwner, TechnoClass* pInvoker, HouseClass* pInvokerHouse);

	static WeaponTypeClass* GetWeaponType(TechnoTypeClass* pThis, int weaponIndex, bool isElite);

	// Ares 0.A
	static const char* GetSelectionGroupID(ObjectTypeClass* pType);
	static bool HasSelectionGroupID(ObjectTypeClass* pType, const char* pID);
};
