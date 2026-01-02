// OpsTrack_CharacterDamageManagerComponent.c
// Hooks into damage system for wounded event tracking
// NOTE: Kill events are handled by SCR_BaseGameMode.OnPlayerKilled/OnControllableDestroyed

modded class SCR_CharacterDamageManagerComponent
{
	protected override void OnDamage(notnull BaseDamageContext damageContext)
	{
		// Call super first to apply damage
		super.OnDamage(damageContext);
		
		// Early exit checks
		if (!Replication.IsServer())
			return;
		
		// Safe settings check
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (!manager)
			return;
		
		OpsTrackSettings settings = manager.GetSettings();
		if (!settings || !settings.EnableKillEvents)
			return;

		IEntity victim = GetOwner();
		if (!victim)
			return;

		// Get controller to check death state
		SCR_CharacterControllerComponent victimController = SCR_CharacterControllerComponent.Cast(victim.FindComponent(SCR_CharacterControllerComponent));
		
		// Skip if dead - kills are handled by GameMode callbacks
		if (victimController && victimController.IsDead())
			return;

		// Get instigator from damage context
		Instigator instigator = damageContext.instigator;
		if (!instigator)
			return;

		IEntity killerEntity = instigator.GetInstigatorEntity();
		
		// Skip self-harm in OnDamage - it will be caught as kill or wounded
		// Self-harm that doesn't kill is still a wounded event
		
		// Get victim player ID
		int victimPlayerId = 0;
		if (GetGame() && GetGame().GetPlayerManager())
			victimPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(victim);

		// Create context data
		SCR_InstigatorContextData contextData = new SCR_InstigatorContextData(
			victimPlayerId,
			victim,
			killerEntity,
			instigator,
			false
		);

		// Send wounded event (not dead, just damaged)
		CombatEventSender sender = CombatEventSender.Get();
		if (sender)
			sender.SendWounded(contextData);
	}
}
