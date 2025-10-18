modded class SCR_CharacterDamageManagerComponent
{
	//Save last event-tid as (victim, actor)
	ref map<string, float> m_LastWoundedEvent = new map<string, float>();
	
	protected override void OnDamage(notnull BaseDamageContext damageContext)
	{
		if(!OpsTrackManager.Get().GetSettings().EnableKillEvents && !Replication.IsServer())
		{
			super.OnDamage(damageContext);
			return;
		}
		CombatEventSender sender = CombatEventSender.Get();
		
		Instigator instigator = damageContext.instigator;
		IEntity actorEntity = instigator.GetInstigatorEntity();
		IEntity victim = GetOwner();
		SCR_CharacterFactionAffiliationComponent victimFaction = SCR_CharacterFactionAffiliationComponent.Cast(
   			victim.FindComponent(SCR_CharacterFactionAffiliationComponent)
		);
		SCR_CharacterFactionAffiliationComponent instigatorFaction = SCR_CharacterFactionAffiliationComponent.Cast(
   			actorEntity.FindComponent(SCR_CharacterFactionAffiliationComponent)
		);
		
		
		int actorId = instigator.GetInstigatorPlayerID();
		int victimId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(victim);
		
		string key = string.Format("%1:%2", victimId, actorId);
		float now = GetGame().GetWorld().GetWorldTime();
		if(m_LastWoundedEvent.Contains(key) && now - m_LastWoundedEvent.Get(key) < 200)
		{
			super.OnDamage(damageContext);
			return; //Skip the spam
		}
		m_LastWoundedEvent.Set(key, now);
		
		string weapon = ResolveWeaponName(instigator);
		bool isBlueOnBlue = false;
		int distance = 0;
		
		if(actorEntity && victim)
		{
			vector actorPos = actorEntity.GetOrigin();
			vector victimPos = victim.GetOrigin();
			
			
			distance = (int)vector.Distance(actorPos, victimPos);
		}
		
		//resolve faction names
		string actorFactionName = "Unknown";
		if(instigatorFaction)
		{
			actorFactionName = instigatorFaction.GetAffiliatedFaction().GetFactionName();	
		}
		
		string victimFactionName = "Unknown";
		if(victimFaction)
		{
			victimFactionName = victimFaction.GetAffiliatedFaction().GetFactionName();
		}
		
		
		//resolve isBlueOnBlue
		if(victimFaction && instigatorFaction)
		{
			Faction faction1 = victimFaction.GetAffiliatedFaction();
			Faction faction2 = instigatorFaction.GetAffiliatedFaction();
			if(faction1.IsFactionFriendly(faction2))
			{
				isBlueOnBlue = true;
			}
		}

		
		sender.SendWounded(actorId, actorFactionName, victimId, victimFactionName, weapon, isBlueOnBlue, distance);
		
		super.OnDamage(damageContext);
		
	}
	
	protected string ResolveWeaponName(Instigator inst)
    {
        if (!inst) return "Unknown";
        IEntity killerEntity = inst.GetInstigatorEntity();
        if (!killerEntity) return "Unknown";

        CharacterWeaponManagerComponent weaponMgr = CharacterWeaponManagerComponent.Cast(
            killerEntity.FindComponent(CharacterWeaponManagerComponent)
        );
        if (!weaponMgr) return "Unknown";

        BaseWeaponComponent activeWeapon = weaponMgr.GetCurrentWeapon();
        if (!activeWeapon) return "Unknown";
		
		UIInfo info = activeWeapon.GetUIInfo();
        return info.GetName();
    }

}