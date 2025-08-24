#pragma once
#include <InfantryClass.h>
#include <AnimClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Macro.h>
#include <New/Entity/ShieldClass.h>
#include <New/Entity/LaserTrailClass.h>
#include <New/Entity/AttachEffectClass.h>

class BulletClass;

class TechnoExt
{
public:
	using base_type = TechnoClass;

	static constexpr DWORD Canary = 0x55555555;
	static constexpr size_t ExtPointerOffset = 0x34C;
	static constexpr bool ShouldConsiderInvalidatePointer = true;

	class ExtData final : public Extension<TechnoClass>
	{
	public:
		TechnoTypeExt::ExtData* TypeExtData;
		std::unique_ptr<ShieldClass> Shield;
		std::vector<std::unique_ptr<LaserTrailClass>> LaserTrails;
		std::vector<std::unique_ptr<AttachEffectClass>> AttachedEffects;
		AttachEffectTechnoProperties AE;
		TechnoTypeClass* PreviousType; // Type change registered in TechnoClass::AI on current frame and used in FootClass::AI on same frame and reset after.
		std::vector<EBolt*> ElectricBolts; // EBolts are not serialized so do not serialize this either.
		int AnimRefCount; // Used to keep track of how many times this techno is referenced in anims f.ex Invoker, ParentBuilding etc., for pointer invalidation.
		bool ReceiveDamage;
		bool LastKillWasTeamTarget;
		CDTimerClass PassengerDeletionTimer;
		ShieldTypeClass* CurrentShieldType;
		int LastWarpDistance;
		CDTimerClass ChargeTurretTimer; // Used for charge turrets instead of RearmTimer if weapon has ChargeTurret.Delays set.
		CDTimerClass AutoDeathTimer;
		AnimTypeClass* MindControlRingAnimType;
		int DamageNumberOffset;
		int Strafe_BombsDroppedThisRound;
		int CurrentAircraftWeaponIndex;
		bool IsInTunnel;
		bool IsBurrowed;
		bool HasBeenPlacedOnMap; // Set to true on first Unlimbo() call.
		CDTimerClass DeployFireTimer;
		bool SkipTargetChangeResetSequence;
		bool ForceFullRearmDelay;
		bool LastRearmWasFullDelay;
		bool CanCloakDuringRearm; // Current rearm timer was started by DecloakToFire=no weapon.
		int WHAnimRemainingCreationInterval;
		WeaponTypeClass* LastWeaponType;
		CellClass* FiringObstacleCell; // Set on firing if there is an obstacle cell between target and techno, used for updating WaveClass target etc.
		bool IsDetachingForCloak; // Used for checking animation detaching, set to true before calling Detach_All() on techno when this anim is attached to and to false after when cloaking only.
		int BeControlledThreatFrame;
		DWORD LastTargetID;
		int AccumulatedGattlingValue;
		bool ShouldUpdateGattlingValue;
		int AttachedEffectInvokerCount;

		// Used for Passengers.SyncOwner.RevertOnExit instead of TechnoClass::InitialOwner / OriginallyOwnedByHouse,
		// as neither is guaranteed to point to the house the TechnoClass had prior to entering transport and cannot be safely overridden.
		HouseClass* OriginalPassengerOwner;
		bool HasRemainingWarpInDelay;          // Converted from object with Teleport Locomotor to one with a different Locomotor while still phasing in OR set if ChronoSphereDelay > 0.
		int LastWarpInDelay;                   // Last-warp in delay for this unit, used by HasCarryoverWarpInDelay.
		bool IsBeingChronoSphered;             // Set to true on units currently being ChronoSphered, does not apply to Ares-ChronoSphere'd buildings or Chrono reinforcements.
		bool KeepTargetOnMove;
		CellStruct LastSensorsMapCoords;
		CDTimerClass TiberiumEater_Timer;
		bool DelayedFireSequencePaused;
		int DelayedFireWeaponIndex;
		CDTimerClass DelayedFireTimer;
		AnimClass* CurrentDelayedFireAnim;

		AirstrikeClass* AirstrikeTargetingMe;

		CDTimerClass FiringAnimationTimer;

		// cache tint values
		int TintColorOwner;
		int TintColorAllies;
		int TintColorEnemies;
		int TintIntensityOwner;
		int TintIntensityAllies;
		int TintIntensityEnemies;

		int AttackMoveFollowerTempCount;

		// ExtraFire ROF timers - not serialized, reset on load
		std::map<WeaponTypeClass*, CDTimerClass> ExtraFireTimers;

		ExtData(TechnoClass* OwnerObject) : Extension<TechnoClass>(OwnerObject)
			, TypeExtData { nullptr }
			, Shield {}
			, LaserTrails {}
			, AttachedEffects {}
			, AE {}
			, PreviousType { nullptr }
			, ElectricBolts {}
			, AnimRefCount { 0 }
			, ReceiveDamage { false }
			, LastKillWasTeamTarget { false }
			, PassengerDeletionTimer {}
			, CurrentShieldType { nullptr }
			, LastWarpDistance {}
			, ChargeTurretTimer {}
			, AutoDeathTimer {}
			, MindControlRingAnimType { nullptr }
			, DamageNumberOffset { INT32_MIN }
			, Strafe_BombsDroppedThisRound { 0 }
			, CurrentAircraftWeaponIndex {}
			, IsInTunnel { false }
			, IsBurrowed { false }
			, HasBeenPlacedOnMap { false }
			, DeployFireTimer {}
			, SkipTargetChangeResetSequence { false }
			, ForceFullRearmDelay { false }
			, LastRearmWasFullDelay { false }
			, CanCloakDuringRearm { false }
			, WHAnimRemainingCreationInterval { 0 }
			, LastWeaponType {}
			, FiringObstacleCell {}
			, IsDetachingForCloak { false }
			, BeControlledThreatFrame { 0 }
			, LastTargetID { 0xFFFFFFFF }
			, AccumulatedGattlingValue { 0 }
			, ShouldUpdateGattlingValue { false }
			, OriginalPassengerOwner {}
			, HasRemainingWarpInDelay { false }
			, LastWarpInDelay { 0 }
			, IsBeingChronoSphered { false }
			, KeepTargetOnMove { false }
			, LastSensorsMapCoords { CellStruct::Empty }
			, TiberiumEater_Timer {}
			, AirstrikeTargetingMe { nullptr }
			, FiringAnimationTimer {}
			, DelayedFireSequencePaused { false }
			, DelayedFireWeaponIndex { -1 }
			, DelayedFireTimer {}
			, CurrentDelayedFireAnim { nullptr }
			, AttachedEffectInvokerCount { 0 }
			, TintColorOwner { 0 }
			, TintColorAllies { 0 }
			, TintColorEnemies { 0 }
			, TintIntensityOwner { 0 }
			, TintIntensityAllies { 0 }
			, TintIntensityEnemies { 0 }
			, AttackMoveFollowerTempCount { 0 }
			, ExtraFireTimers {}
		{ }

		void OnEarlyUpdate();

		void ApplyInterceptor();
		bool CheckDeathConditions(bool isInLimbo = false);
		void DepletedAmmoActions();
		void EatPassengers();
		void UpdateTiberiumEater();
		void UpdateShield();
		void UpdateOnTunnelEnter();
		void UpdateOnTunnelExit();
		void ApplySpawnLimitRange();
		void UpdateTypeData(TechnoTypeClass* pCurrentType);
		void UpdateTypeData_Foot();
		void UpdateLaserTrails();
		void UpdateAttachEffects();
		void UpdateGattlingRateDownReset();
		void UpdateKeepTargetOnMove();
		void UpdateWarpInDelay();
		void UpdateCumulativeAttachEffects(AttachEffectTypeClass* pAttachEffectType, AttachEffectClass* pRemoved = nullptr);
		void RecalculateStatMultipliers();
		void UpdateTemporal();
		void UpdateMindControlAnim();
		void UpdateRecountBurst();
		void UpdateRearmInEMPState();
		void UpdateRearmInTemporal();
		void InitializeLaserTrails();
		void InitializeAttachEffects();
		void UpdateSelfOwnedAttachEffects();
		bool HasAttachedEffects(std::vector<AttachEffectTypeClass*> attachEffectTypes, bool requireAll, bool ignoreSameSource, TechnoClass* pInvoker, AbstractClass* pSource, std::vector<int> const* minCounts, std::vector<int> const* maxCounts) const;
		int GetAttachedEffectCumulativeCount(AttachEffectTypeClass* pAttachEffectType, bool ignoreSameSource = false, TechnoClass* pInvoker = nullptr, AbstractClass* pSource = nullptr) const;
		void InitializeDisplayInfo();
		void ApplyMindControlRangeLimit();
		int ApplyForceWeaponInRange(AbstractClass* pTarget);
		void ResetDelayedFireTimer();
		void UpdateTintValues();

		virtual ~ExtData() override;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TechnoExt>
	{
	public:
		ExtContainer();
		~ExtContainer();

		virtual bool InvalidateExtDataIgnorable(void* const ptr) const override
		{
			auto const abs = static_cast<AbstractClass*>(ptr)->WhatAmI();

			switch (abs)
			{
			case AbstractType::Airstrike:
				return false;
			default:
				return true;
			}
		}
	};

	static ExtContainer ExtMap;

	static UnitClass* Deployer;

	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);

	static bool IsActive(TechnoClass* pThis);
	static bool IsActiveIgnoreEMP(TechnoClass* pThis);

	static bool IsHarvesting(TechnoClass* pThis);
	static bool HasAvailableDock(TechnoClass* pThis);
	static bool HasRadioLinkWithDock(TechnoClass* pThis);

	static CoordStruct GetFLHAbsoluteCoords(TechnoClass* pThis, CoordStruct flh, bool turretFLH = false);

	static CoordStruct GetBurstFLH(TechnoClass* pThis, int weaponIndex, bool& FLHFound);
	static CoordStruct GetSimpleFLH(InfantryClass* pThis, int weaponIndex, bool& FLHFound);

	static void ChangeOwnerMissionFix(FootClass* pThis);
	static void KillSelf(TechnoClass* pThis, AutoDeathBehavior deathOption, AnimTypeClass* pVanishAnimation, bool isInLimbo = false);
	static void ObjectKilledBy(TechnoClass* pVictim, TechnoClass* pKiller = nullptr, HouseClass* pHouseKiller = nullptr);
	static void UpdateSharedAmmo(TechnoClass* pThis);
	static double GetCurrentSpeedMultiplier(FootClass* pThis);
	static double GetCurrentFirepowerMultiplier(TechnoClass* pThis);
	static void DrawSelfHealPips(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBounds);
	static void DrawInsignia(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBounds);
	static void ApplyGainedSelfHeal(TechnoClass* pThis);
	static void SyncInvulnerability(TechnoClass* pFrom, TechnoClass* pTo);
	static CoordStruct PassengerKickOutLocation(TechnoClass* pThis, FootClass* pPassenger, int maxAttempts);
	static bool AllowedTargetByZone(TechnoClass* pThis, TechnoClass* pTarget, TargetZoneScanType zoneScanType, WeaponTypeClass* pWeapon = nullptr, bool useZone = false, int zone = -1);
	static void UpdateAttachedAnimLayers(TechnoClass* pThis);
	static bool ConvertToType(FootClass* pThis, TechnoTypeClass* toType);
	static bool CanDeployIntoBuilding(UnitClass* pThis, bool noDeploysIntoDefaultValue = false);
	static bool IsTypeImmune(TechnoClass* pThis, TechnoClass* pSource);
	static int GetTintColor(TechnoClass* pThis, bool invulnerability, bool airstrike, bool berserk);
	static int GetCustomTintColor(TechnoClass* pThis);
	static int GetCustomTintIntensity(TechnoClass* pThis);
	static void ApplyCustomTintValues(TechnoClass* pThis, int& color, int& intensity);
	static Point2D GetScreenLocation(TechnoClass* pThis);
	static Point2D GetFootSelectBracketPosition(TechnoClass* pThis, Anchor anchor);
	static Point2D GetBuildingSelectBracketPosition(TechnoClass* pThis, BuildingSelectBracketPosition bracketPosition);
	static void DrawSelectBox(TechnoClass* pThis, const Point2D* pLocation, const RectangleStruct* pBounds, bool drawBefore = false);
	static void ProcessDigitalDisplays(TechnoClass* pThis);
	static void GetValuesForDisplay(TechnoClass* pThis, DisplayInfoType infoType, int& value, int& maxValue, int infoIndex);
	static void GetDigitalDisplayFakeHealth(TechnoClass* pThis, int& value, int& maxValue);
	static void CreateDelayedFireAnim(TechnoClass* pThis, AnimTypeClass* pAnimType, int weaponIndex, bool attach, bool center, bool removeOnNoDelay, bool onTurret, CoordStruct firingCoords);
	static bool HandleDelayedFireWithPauseSequence(TechnoClass* pThis, int weaponIndex, int firingFrame);
	static bool IsHealthInThreshold(TechnoClass* pObject, double min, double max);
	static UnitTypeClass* GetUnitTypeExtra(UnitClass* pUnit);
	static AircraftTypeClass* GetAircraftTypeExtra(AircraftClass* pAircraft);

	// WeaponHelpers.cpp
	static int PickWeaponIndex(TechnoClass* pThis, TechnoClass* pTargetTechno, AbstractClass* pTarget, int weaponIndexOne, int weaponIndexTwo, bool allowFallback = true, bool allowAAFallback = true);
	static void FireWeaponAtSelf(TechnoClass* pThis, WeaponTypeClass* pWeaponType);
	static bool CanFireNoAmmoWeapon(TechnoClass* pThis, int weaponIndex);
	static WeaponTypeClass* GetDeployFireWeapon(TechnoClass* pThis, int& weaponIndex);
	static WeaponTypeClass* GetDeployFireWeapon(TechnoClass* pThis);
	static WeaponTypeClass* GetCurrentWeapon(TechnoClass* pThis, int& weaponIndex, bool getSecondary = false);
	static WeaponTypeClass* GetCurrentWeapon(TechnoClass* pThis, bool getSecondary = false);
	static int GetWeaponIndexAgainstWall(TechnoClass* pThis, OverlayTypeClass* pWallOverlayType);
	static void ApplyKillWeapon(TechnoClass* pThis, TechnoClass* pSource, WarheadTypeClass* pWH);
	static void ApplyRevengeWeapon(TechnoClass* pThis, TechnoClass* pSource, WarheadTypeClass* pWH);
	static bool MultiWeaponCanFire(TechnoClass* const pThis, AbstractClass* const pTarget, WeaponTypeClass* const pWeaponType);
};
