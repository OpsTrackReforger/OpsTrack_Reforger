modded class SCR_CharacterDamageManagerComponent
{


	protected override void OnDamage(notnull BaseDamageContext damageContext)
	{
		// Stop hvis kill events er slået fra eller ikke server
		if (!OpsTrackManager.Get().GetSettings().EnableKillEvents || !Replication.IsServer())
		{
			super.OnDamage(damageContext);
			return;
		}

		IEntity victim = GetOwner();
		if (!victim)
		{
			super.OnDamage(damageContext);
			return;
		}

		// Skip hvis allerede død
		SCR_CharacterControllerComponent victimController = SCR_CharacterControllerComponent.Cast(victim.FindComponent(SCR_CharacterControllerComponent));
		if (victimController && victimController.IsDead())
		{
			super.OnDamage(damageContext);
			return;
		}

		// Instigator fra damage
		Instigator instigator = damageContext.instigator;
		if (!instigator)
		{
			super.OnDamage(damageContext);
			return;
		}

		IEntity killerEntity = instigator.GetInstigatorEntity();

		// Opret InstigatorContextData for at få korrekt relation
		SCR_InstigatorContextData contextData = new SCR_InstigatorContextData(
			GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(victim),
			victim,
			killerEntity,
			instigator,
			false // isDeleted
		);


		// Send event
		CombatEventSender sender = CombatEventSender.Get();
		sender.SendWounded(contextData);

		super.OnDamage(damageContext);
	}

	// Utility
	protected string ResolveCharacterName(IEntity entity)
	{
		if (!entity)
			return "Environment";
	
		int playerId = SCR_PossessingManagerComponent.GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
			return GetGame().GetPlayerManager().GetPlayerName(playerId);
	
		SCR_EditableEntityComponent entityComp = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		if (entityComp)
			return entityComp.GetDisplayName();
	
		return entity.GetName(); // fallback
	}

	// Utility
	protected string ResolveWeaponName(Instigator inst)
	{
		if (!inst) return "Unknown";
		IEntity ent = inst.GetInstigatorEntity();
		if (!ent) return "Unknown";
	
		// Try infantry weapon
		CharacterWeaponManagerComponent weapMgr = CharacterWeaponManagerComponent.Cast(ent.FindComponent(CharacterWeaponManagerComponent));
		if (weapMgr)
		{
			BaseWeaponComponent weapon = weapMgr.GetCurrentWeapon();
			if (weapon)
			{
				UIInfo info = weapon.GetUIInfo();
				if (info)
					return info.GetName();
			}
		}
	
		return "Unknown";
	}
	
	//~ Prioritizes getting the faction of the entity but will try get player faction if the faction changes
	protected Faction GetFaction(IEntity entity, int playerID)
	{
		if (!entity)
			return GetFactionFromPlayerID(playerID);
		
		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionAffiliation)
			return GetFactionFromPlayerID(playerID);
		
		return factionAffiliation.GetAffiliatedFaction();
	}
	
	//------------------------------------------------------------------------------------------------
	//~ Get faction off player ID
	protected Faction GetFactionFromPlayerID(int playerID)
	{
		if (playerID <= 0)
			return null;
		
		return SCR_FactionManager.SGetPlayerFaction(playerID);
	}

};
