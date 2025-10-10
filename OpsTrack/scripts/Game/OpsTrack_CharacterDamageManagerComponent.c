modded class SCR_CharacterDamageManagerComponent
{
	//Save last event-tid as (victim, actor)
	ref map<string, float> m_LastWoundedEvent = new map<string, float>();
	
	protected override void OnDamage(notnull BaseDamageContext damageContext)
	{
		if(!OpsTrackManager.Get().GetSettings().EnableKillEvents)
		{
			super.OnDamage(damageContext);
			return;
		}
		CombatEventSender sender = CombatEventSender.Get();
		
		Instigator instigator = damageContext.instigator;
		IEntity actorEntity = instigator.GetInstigatorEntity();
		IEntity victim = GetOwner();
		SCR_FactionAffiliationComponent victimFaction = SCR_FactionAffiliationComponent.Cast(
   			victim.FindComponent(SCR_FactionAffiliationComponent)
		);
		SCR_FactionAffiliationComponent instigatorFaction = SCR_FactionAffiliationComponent.Cast(
   			actorEntity.FindComponent(SCR_FactionAffiliationComponent)
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
		
		//resolve isBlueOnBlue
		if(victimFaction && instigatorFaction)
		{
			string victimKey = victimFaction.GetAffiliatedFactionKey();
			string instigatorKey = instigatorFaction.GetAffiliatedFactionKey();
			if(victimKey == instigatorKey)
			{
				isBlueOnBlue = true;
			}
		}

		
		
		sender.SendWounded(actorId, victimId, weapon, isBlueOnBlue, distance);
		
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

        return activeWeapon.GetUIInfo().GetName();
    }

}