modded class SCR_CharacterDamageManagerComponent
{
	//Save last event-tid as (victim, actor)
	ref map<string, float> m_LastWoundedEvent = new map<string, float>();

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

		// Hent IDs og navne
		int victimId = contextData.GetVictimPlayerID();
		int killerId = contextData.GetKillerPlayerID();

		string victimName = ResolveCharacterName(victim);
		
		if(victimId > 0)
		{
			victimName = GetGame().GetPlayerManager().GetPlayerName(victimId);
		}
		
		
		string killerName = ResolveCharacterName(killerEntity);

		if(killerId > 0)
		{
			killerName = GetGame().GetPlayerManager().GetPlayerName(killerId);
		}
		

		// Faktioner
		string victimFactionName = "Unknown";
		string killerFactionName = "Unknown";
		
		Faction victimFaction = GetFaction(victim, victimId);
		Faction killerFaction = GetFaction(killerEntity, killerId);

		if (victimFaction)
			victimFactionName = victimFaction.GetFactionName();

		if (killerFaction)
			killerFactionName = killerFaction.GetFactionName();

		// Distance
		int distance = 0;
		if (killerEntity && victim)
			distance = (int)vector.Distance(killerEntity.GetOrigin(), victim.GetOrigin());

		// Weapon
		string weaponName = ResolveWeaponName(instigator);

		// Relation
		bool isTeamKill = victimFaction.IsFactionFriendly(killerFaction);

		// Spam-beskyttelse
		string key = string.Format("%1:%2", victimId, killerId);
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_LastWoundedEvent.Contains(key) && now - m_LastWoundedEvent.Get(key) < 200)
		{
			super.OnDamage(damageContext);
			return;
		}
		m_LastWoundedEvent.Set(key, now);

		// Send event
		CombatEventSender sender = CombatEventSender.Get();
		sender.SendWounded(
			killerId,
			killerName,
			killerFactionName,
			victimId,
			victimName,
			victimFactionName,
			weaponName,
			isTeamKill,
			distance
		);

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
