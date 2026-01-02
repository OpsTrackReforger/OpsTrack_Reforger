// OpsTrack_ApiClient.c
// Batched REST API client with rate limiting and backoff

class ApiClient
{
	protected RestContext m_Context;
	protected ref array<string> m_ConnectionEvents;
	protected ref array<string> m_CombatEvents;
	protected ref OpsTrackCallback m_PendingCallback;  // Keep callback alive

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

		// Final flush of any pending events
		int pendingCount = 0;
		if (m_ConnectionEvents)
			pendingCount = pendingCount + m_ConnectionEvents.Count();
		if (m_CombatEvents)
			pendingCount = pendingCount + m_CombatEvents.Count();
		
		if (pendingCount > 0)
		{
			OpsTrackLogger.Warn(string.Format("Final flush: %1 pending events", pendingCount));
			Flush();
		}
	}

	// Called by event senders to queue events
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
		int totalCount = GetPendingCount();

		if (totalCount >= MAX_BATCH_SIZE || now - m_LastFlushTick >= FLUSH_INTERVAL_MS)
		{
			Flush();
		}
	}

	// Timer callback - runs every second
	void CheckFlushTimer()
	{
		// Stop timer if shutting down
		if (m_IsShuttingDown)
		{
			OpsTrackLogger.Debug("Flush timer stopped (shutdown)");
			return;
		}

		if (GetPendingCount() == 0)
			return;

		int now = System.GetTickCount();
		if (now - m_LastFlushTick >= FLUSH_INTERVAL_MS)
		{
			Flush();
		}
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
	
	// Public for testing - returns number of pending events
	int GetPendingCount()
	{
		int count = 0;
		if (m_ConnectionEvents)
			count = count + m_ConnectionEvents.Count();
		if (m_CombatEvents)
			count = count + m_CombatEvents.Count();
		return count;
	}

	protected void Flush()
	{
		if (GetPendingCount() == 0)
			return;
		
		if (!m_Context)
		{
			OpsTrackLogger.Error("Cannot flush: REST context is null");
			return;
		}

		string payload = BuildBatchPayload();
		
		// Clear queues after building payload
		if (m_ConnectionEvents)
			m_ConnectionEvents.Clear();
		if (m_CombatEvents)
			m_CombatEvents.Clear();

		m_LastFlushTick = System.GetTickCount();

		// Store callback as ref to prevent garbage collection before response arrives
		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, "/events", payload);
		
		OpsTrackLogger.Debug(string.Format("Sending payload: %1", payload));
	}

	protected string BuildBatchPayload()
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

	// Called by callback on error - triggers backoff
	void Backoff()
	{
		m_ApiEnabled = false;
		m_NextRetryTick = System.GetTickCount() + COOLDOWN_MS;

		OpsTrackLogger.Warn(string.Format("API backoff triggered. Will retry in %1 seconds.", COOLDOWN_MS / 1000));

		// Drop pending events during backoff
		if (m_ConnectionEvents)
			m_ConnectionEvents.Clear();
		if (m_CombatEvents)
			m_CombatEvents.Clear();
	}
}
