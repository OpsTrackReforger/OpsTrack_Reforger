//OpsTrack_EntityManager.c
//MAnages entity creation, caching and API communication

class OpsTrack_EntityManager
{
	private static ref OpsTrack_EntityManager s_Instance;
	
	//Cache: sessionPlayerId -> entityId
	private ref map<int, UUID> m_PlayerEntities;
	
    private void OpsTrack_EntityManager()
    {
        m_PlayerEntities = new map<int, UUID>();
    }
    
    static OpsTrack_EntityManager Get()
    {
        if (!s_Instance)
            s_Instance = new OpsTrack_EntityManager();
        return s_Instance;
    }
	
	
	// Create entity for player (should be called at spawn)
    // Return existing entityId if player has spawned before
    UUID GetOrCreatePlayerEntity(int sessionPlayerId, string name, string faction)
    {
        // Check cache first (respawn returns existing entity)
        if (m_PlayerEntities.Contains(sessionPlayerId))
            return m_PlayerEntities.Get(sessionPlayerId);
        
        // generate new UUID
        UUID entityId = UUID.GenV4();
        
        // get player unique ID (Reforger Identity)
        string playerId = OpsTrack_EntityUtils.GetPlayerIdentityIdSafe(sessionPlayerId);
        
        // Opret entity objekt
        OpsTrack_Entity entity = new OpsTrack_Entity(
            entityId,
            name,
            faction,
            playerId,
            OpsTrack_EntityType.PLAYER
        );

        // Save in cache
        m_PlayerEntities.Set(sessionPlayerId, entityId);
        
        // Send til API
        SendEntityToApi(entity);
        
        OpsTrackLogger.Info(string.Format("Created entity %1 for player %2", entityId, sessionPlayerId));
        
        return entityId;
    }
	
    // Get entityId from cache (for CombatEvents)
    UUID GetEntityId(int sessionPlayerId)
    {
        if (sessionPlayerId <= 0)
            return UUID.NULL_UUID;
        
        if (m_PlayerEntities.Contains(sessionPlayerId))
            return m_PlayerEntities.Get(sessionPlayerId);
        
        return UUID.NULL_UUID;
    }
    
    // clear cache (at server or mission restart for example)
    void ClearCache()
    {
        m_PlayerEntities.Clear();
        OpsTrackLogger.Info("Entity cache cleared");
    }

    // Get all cached entity IDs (for assigning to mission)
    array<UUID> GetAllEntityIds()
    {
        array<UUID> ids = {};
        for (int i = 0; i < m_PlayerEntities.Count(); i++)
        {
            ids.Insert(m_PlayerEntities.GetElement(i));
        }
        return ids;
    }

    // Send entity to API
    protected void SendEntityToApi(OpsTrack_Entity entity)
    {
        OpsTrackManager manager = OpsTrackManager.Get();
        if (!manager)
        {
            OpsTrackLogger.Error("Cannot send entity: OpsTrackManager not available");
            return;
        }

        ApiClient api = manager.GetApiClient();
        if (!api)
        {
            OpsTrackLogger.Error("Cannot send entity: ApiClient not available");
            return;
        }

        string payload = entity.AsPayload();
        OpsTrackLogger.Debug(string.Format("Sending entity to API queue: %1", payload));
        api.EnqueueEntity(payload);
    }
}