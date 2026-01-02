// OpsTrack_EntityUtils.c
// Shared utility functions for entity resolution
// Eliminates code duplication between CombatEventSender and CharacterDamageManagerComponent

class OpsTrack_EntityUtils
{
	// Resolves character name from entity
	// Priority: Player name > Editable entity display name > Entity name > "Environment"
	static string ResolveCharacterName(IEntity entity)
	{
		if (!entity)
			return "Environment";
	
		int playerId = SCR_PossessingManagerComponent.GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (playerName && playerName != "")
				return playerName;
		}
	
		SCR_EditableEntityComponent entityComp = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		if (entityComp)
		{
			string displayName = entityComp.GetDisplayName();
			if (displayName && displayName != "")
				return displayName;
		}
	
		string entityName = entity.GetName();
		if (entityName && entityName != "")
			return entityName;
		
		return "Unknown";
	}

	// Resolves weapon name from instigator
	static string ResolveWeaponName(Instigator inst)
	{
		if (!inst)
			return "Unknown";
		
		IEntity ent = inst.GetInstigatorEntity();
		if (!ent)
			return "Unknown";
	
		// Try infantry weapon
		CharacterWeaponManagerComponent weapMgr = CharacterWeaponManagerComponent.Cast(ent.FindComponent(CharacterWeaponManagerComponent));
		if (weapMgr)
		{
			BaseWeaponComponent weapon = weapMgr.GetCurrentWeapon();
			if (weapon)
			{
				UIInfo info = weapon.GetUIInfo();
				if (info)
				{
					string weaponName = info.GetName();
					if (weaponName && weaponName != "")
						return weaponName;
				}
			}
		}
	
		return "Unknown";
	}
	
	// Gets faction from entity, falls back to player ID lookup
	static Faction GetFaction(IEntity entity, int playerID)
	{
		if (!entity)
			return GetFactionFromPlayerID(playerID);
		
		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionAffiliation)
			return GetFactionFromPlayerID(playerID);
		
		return factionAffiliation.GetAffiliatedFaction();
	}
	
	// Gets faction from player ID
	static Faction GetFactionFromPlayerID(int playerID)
	{
		if (playerID <= 0)
			return null;
		
		return SCR_FactionManager.SGetPlayerFaction(playerID);
	}
	
	// Safely gets player identity ID with null checks
	static string GetPlayerIdentityIdSafe(int playerId)
	{
		if (playerId <= 0)
			return "";
		
		BackendApi api = GetGame().GetBackendApi();
		if (!api)
			return "";
		
		return api.GetPlayerIdentityId(playerId);
	}
}
