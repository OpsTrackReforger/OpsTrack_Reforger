// OpsTrack_ApiClient.c
// Batched REST API client with rate limiting and backoff

class ApiClient
{
	protected RestContext m_Context;
	protected ref array<string> m_ConnectionEvents;
	protected ref array<string> m_CombatEvents;
	protected ref array<string> m_Entities;
	protected ref OpsTrackCallback m_PendingCallback;

	protected int m_LastFlushTick;
	protected bool m_ApiEnabled;
	protected int m_NextRetryTick;
	protected bool m_IsShuttingDown;

	private static const int FLUSH_INTERVAL_MS = 1000;
	private static const int MAX_BATCH_SIZE = 25;
	private static const int COOLDOWN_MS = 120000;

	void ApiClient()
	{
		OpsTrackLogger.Info("Initializing ApiClient");
		
		m_LastFlushTick = 0;
		m_ApiEnabled = true;
		m_NextRetryTick = 0;
		m_IsShuttingDown = false;
		
		// Initialize arrays
		m_ConnectionEvents = new array<string>();
		m_CombatEvents = new array<string>();
		m_Entities = new array<string>();

		// Get settings
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (!manager)
		{
			OpsTrackLogger.Error("ApiClient: OpsTrackManager not available!");
			return;
		}
		
		OpsTrackSettings settings = manager.GetSettings();
		if (!settings)
		{
			OpsTrackLogger.Error("ApiClient: Settings unavailable!");
			return;
		}

		// Get REST API
		if (!GetGame())
		{
			OpsTrackLogger.Error("ApiClient: GetGame() returned null!");
			return;
		}
		
		RestApi api = GetGame().GetRestApi();
		if (!api)
		{
			OpsTrackLogger.Error("ApiClient: RestApi not available!");
			return;
		}

		// Create context
		m_Context = api.GetContext(settings.ApiBaseUrl);
		if (!m_Context)
		{
			OpsTrackLogger.Error(string.Format("Failed to create REST context for: %1", settings.ApiBaseUrl));
			return;
		}

		m_Context.SetHeaders("Content-Type,application/json,X-Api-Key," + settings.ApiKey);
		OpsTrackLogger.Debug(string.Format("Connected to: %1", settings.ApiBaseUrl));

		// Start flush timer
		if (GetGame().GetCallqueue())
			GetGame().GetCallqueue().CallLater(CheckFlushTimer, 1000, true);
		else
			OpsTrackLogger.Warn("Could not start flush timer: Callqueue unavailable");
		
		OpsTrackLogger.Info("ApiClient initialized");
	}

	void ~ApiClient()
	{
		OpsTrackLogger.Info("ApiClient shutting down");
		
		m_IsShuttingDown = true;

		// Final flush of any pending data
		int pendingCount = GetPendingCount();
		
		if (pendingCount > 0)
		{
			OpsTrackLogger.Warn(string.Format("Final flush: %1 pending items", pendingCount));
			FlushEntities();
			Flush();
		}
	}

	// Queue event (combat or connection)
	void Enqueue(string eventJson, OpsTrack_EventType eventType)
	{
		if (!CanSend())
			return;
		
		if (!eventJson || eventJson == "")
		{
			OpsTrackLogger.Warn("Enqueue called with empty JSON");
			return;
		}

		// Route to appropriate queue based on event type
		if (eventType == OpsTrack_EventType.SELF_HARM ||
			eventType == OpsTrack_EventType.KILL ||
			eventType == OpsTrack_EventType.WOUNDED)
		{
			if (m_CombatEvents)
				m_CombatEvents.Insert(eventJson);
		}
		else if (eventType == OpsTrack_EventType.JOIN ||
				 eventType == OpsTrack_EventType.LEAVE)
		{
			if (m_ConnectionEvents)
				m_ConnectionEvents.Insert(eventJson);
		}
		else
		{
			OpsTrackLogger.Warn(string.Format("Unknown event type: %1", eventType));
			return;
		}

		// Check if we should flush
		int now = System.GetTickCount();
		int totalCount = GetPendingEventCount();

		if (totalCount >= MAX_BATCH_SIZE || now - m_LastFlushTick >= FLUSH_INTERVAL_MS)
		{
			Flush();
		}
	}

	// Queue entity and flush immediately
	// Note: Timer-based batching wasn't working reliably, so we flush on each enqueue
	void EnqueueEntity(string entityJson)
	{
		if (!CanSend())
		{
			OpsTrackLogger.Warn("EnqueueEntity: CanSend() returned false");
			return;
		}

		if (!entityJson || entityJson == "")
		{
			OpsTrackLogger.Warn("EnqueueEntity called with empty JSON");
			return;
		}

		if (m_Entities)
		{
			m_Entities.Insert(entityJson);
			OpsTrackLogger.Debug(string.Format("Entity queued. Queue size: %1", m_Entities.Count()));

			// Flush immediately since timer-based batching isn't working
			FlushEntities();
		}
	}
	
	// Send mission start to /missions endpoint
	void SendMissionStart(string payload)
	{
		if (!CanSend())
		{
			OpsTrackLogger.Warn("SendMissionStart: CanSend() returned false (API in backoff)");
			return;
		}

		if (!m_Context)
		{
			OpsTrackLogger.Error("Cannot send mission start: REST context is null");
			return;
		}

		OpsTrackLogger.Info(string.Format("Sending mission start to /missions: %1", payload));
		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, "/missions", payload);
	}

	// Send mission end to /missions/{id}/end endpoint
	void SendMissionEnd(UUID missionId)
	{
		if (!CanSend())
			return;

		if (!m_Context)
		{
			OpsTrackLogger.Error("Cannot send mission end: REST context is null");
			return;
		}

		string endpoint = string.Format("/missions/%1/end", missionId);

		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, endpoint, "{}");
	}

	// Send entity states batch to /entitystates/batch endpoint
	void SendEntityStates(string payload)
	{
		if (!CanSend())
			return;

		if (!m_Context)
		{
			OpsTrackLogger.Error("Cannot send entity states: REST context is null");
			return;
		}

		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, "/entitystates/batch", payload);
	}

	// Assign entities to a mission via POST /entities/assign-mission
	void AssignEntitiesToMission(string payload)
	{
		if (!CanSend())
		{
			OpsTrackLogger.Warn("AssignEntitiesToMission: CanSend() returned false (API in backoff)");
			return;
		}

		if (!m_Context)
		{
			OpsTrackLogger.Error("Cannot assign entities to mission: REST context is null");
			return;
		}

		OpsTrackLogger.Info(string.Format("Assigning entities to mission: %1", payload));
		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, "/entities/assign-mission", payload);
	}

	// Timer callback - runs every second
	void CheckFlushTimer()
	{
		if (m_IsShuttingDown)
			return;

		// Debug: Log entity queue status
		int entityCount = 0;
		if (m_Entities)
			entityCount = m_Entities.Count();

		// Log every tick to verify timer is running (temporary debug)
		if (entityCount > 0)
			OpsTrackLogger.Debug(string.Format("CheckFlushTimer tick: %1 entities pending", entityCount));

		if (entityCount > 0)
		{
			OpsTrackLogger.Info(string.Format("Flushing %1 entities from timer", entityCount));
			FlushEntities();
		}

		// Flush events if any pending
		int now = System.GetTickCount();
		if (GetPendingEventCount() > 0 && now - m_LastFlushTick >= FLUSH_INTERVAL_MS)
			Flush();
	}

	protected bool CanSend()
	{
		if (!m_ApiEnabled)
		{
			int now = System.GetTickCount();
			if (now < m_NextRetryTick)
				return false;

			// Cooldown expired, re-enable
			m_ApiEnabled = true;
			OpsTrackLogger.Info("API re-enabled after cooldown");
		}
		return true;
	}
	
	// Returns number of pending events (not entities)
	int GetPendingEventCount()
	{
		int count = 0;
		if (m_ConnectionEvents)
			count = count + m_ConnectionEvents.Count();
		if (m_CombatEvents)
			count = count + m_CombatEvents.Count();
		return count;
	}
	
	// Returns total pending count (events + entities)
	int GetPendingCount()
	{
		int count = GetPendingEventCount();
		if (m_Entities)
			count = count + m_Entities.Count();
		return count;
	}

	// Flush events to /events endpoint
	protected void Flush()
	{
		if (GetPendingEventCount() == 0)
			return;
		
		if (!m_Context)
		{
			OpsTrackLogger.Error("Cannot flush: REST context is null");
			return;
		}

		string payload = BuildEventBatchPayload();
		
		// Clear queues after building payload
		if (m_ConnectionEvents)
			m_ConnectionEvents.Clear();
		if (m_CombatEvents)
			m_CombatEvents.Clear();

		m_LastFlushTick = System.GetTickCount();

		// Store callback as ref to prevent garbage collection before response arrives
		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, "/events", payload);
	}

	// Flush entities to /entities/batch endpoint
	protected void FlushEntities()
	{
		if (!m_Entities || m_Entities.Count() == 0)
			return;

		if (!m_Context)
		{
			OpsTrackLogger.Error("Cannot flush entities: REST context is null");
			return;
		}

		string payload = BuildEntityBatchPayload();
		int count = m_Entities.Count();

		// Clear queue after building payload
		m_Entities.Clear();

		OpsTrackLogger.Info(string.Format("Flushing %1 entities to /entities/batch", count));
		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, "/entities/batch", payload);
	}

	protected string BuildEventBatchPayload()
	{
		string payload = "{";

		// connectionEvents array
		payload = payload + "\"connectionEvents\":[";
		if (m_ConnectionEvents)
		{
			for (int i = 0; i < m_ConnectionEvents.Count(); i++)
			{
				payload = payload + m_ConnectionEvents[i];
				if (i < m_ConnectionEvents.Count() - 1)
					payload = payload + ",";
			}
		}
		payload = payload + "],";

		// combatEvents array
		payload = payload + "\"combatEvents\":[";
		if (m_CombatEvents)
		{
			for (int j = 0; j < m_CombatEvents.Count(); j++)
			{
				payload = payload + m_CombatEvents[j];
				if (j < m_CombatEvents.Count() - 1)
					payload = payload + ",";
			}
		}
		payload = payload + "]";

		payload = payload + "}";

		return payload;
	}

	protected string BuildEntityBatchPayload()
	{
		// Get missionId from manager (null if not recording)
		string missionPart = "null";
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		if (manager && manager.IsRecording())
		{
			UUID missionId = manager.GetCurrentMissionId();
			if (!missionId.IsNull())
				missionPart = "\"" + missionId + "\"";
		}

		string payload = "{\"missionId\":" + missionPart + ",\"entities\":[";

		if (m_Entities)
		{
			for (int i = 0; i < m_Entities.Count(); i++)
			{
				payload = payload + m_Entities[i];
				if (i < m_Entities.Count() - 1)
					payload = payload + ",";
			}
		}

		payload = payload + "]}";

		return payload;
	}

	// Called by callback on error - triggers backoff
	void Backoff()
	{
		m_ApiEnabled = false;
		m_NextRetryTick = System.GetTickCount() + COOLDOWN_MS;

		OpsTrackLogger.Warn(string.Format("API backoff triggered. Will retry in %1 seconds.", COOLDOWN_MS / 1000));

		// Drop pending data during backoff
		if (m_ConnectionEvents)
			m_ConnectionEvents.Clear();
		if (m_CombatEvents)
			m_CombatEvents.Clear();
		if (m_Entities)
			m_Entities.Clear();
	}
}
