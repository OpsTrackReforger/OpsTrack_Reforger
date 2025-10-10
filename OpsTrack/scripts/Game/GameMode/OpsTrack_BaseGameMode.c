// OpsTrack_BaseGameMode.c
modded class SCR_BaseGameMode
{
    ref ConnectionEventSender m_ConnectionEvents;
    bool m_OpsTrackEnabled;

    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);

		
		//WTF is this vibe shit
        if (Replication.IsServer())
        {
            OpsTrackSettings settings = OpsTrackManager.Get().GetSettings();

            // Respect feature toggle
            m_OpsTrackEnabled = settings && settings.EnableConnectionEvents;

            if (m_OpsTrackEnabled)
            {
                m_ConnectionEvents = ConnectionEventSender.Get();
                OpsTrackLogger.Info("Mod is running on the server! Connection events enabled.");
            }
            else
            {
                OpsTrackLogger.Info("Mod is running on the server! Connection events disabled by settings.");

            }
        }

    }

    override void OnPlayerRegistered(int playerId)
    {
        super.OnPlayerRegistered(playerId);

        if (Replication.IsServer() && m_OpsTrackEnabled && m_ConnectionEvents)
            m_ConnectionEvents.SendJoin(playerId);
    }

    override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
    {
        if (Replication.IsServer() && m_OpsTrackEnabled && m_ConnectionEvents)
            m_ConnectionEvents.SendLeave(playerId);

        // Behold super til sidst som du allerede gør
        super.OnPlayerDisconnected(playerId, cause, timeout);
    }
}	

