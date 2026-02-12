// OpsTrack_ApiClient.c
// Batched REST API client with rate limiting and backoff
// Designed to minimize HTTP requests by combining all data into unified batches

class ApiClient
{
	protected RestContext m_Context;

	// Unified data queues - all sent together in one request
	protected ref array<string> m_ConnectionEvents;
	protected ref array<string> m_CombatEvents;
	protected ref array<string> m_Entities;
	protected ref array<string> m_EntityStates;
	protected ref array<string> m_EntityAssignments;  // entityIds to assign to current mission

	protected ref OpsTrackCallback m_PendingCallback;

	protected int m_LastFlushTick;
	protected bool m_ApiEnabled;
	protected int m_NextRetryTick;
	protected bool m_IsShuttingDown;
	protected bool m_HasPendingRequest;  // Track if we're waiting for a response

	// Configuration - tuned to avoid Enfusion's request limits
	// Enfusion has an internal limit on concurrent requests per host
	// By sending only ONE request every few seconds, we stay well under the limit
	private static const int FLUSH_INTERVAL_MS = 3000;   // Flush every 3 seconds
	private static const int MAX_STATES_PER_BATCH = 500; // Max entity states per request (keeps payload under ~100KB)
	private static const int COOLDOWN_MS = 120000;       // Backoff on error

	// Payload size limits (Enfusion max is 1MB, we stay well under)
	private static const int MAX_PAYLOAD_BYTES = 800000; // 800KB safety limit

	void ApiClient()
	{
		OpsTrackLogger.Info("Initializing ApiClient (unified batch mode)");

		m_LastFlushTick = System.GetTickCount();
		m_ApiEnabled = true;
		m_NextRetryTick = 0;
		m_IsShuttingDown = false;
		m_HasPendingRequest = false;

		// Initialize all queues
		m_ConnectionEvents = new array<string>();
		m_CombatEvents = new array<string>();
		m_Entities = new array<string>();
		m_EntityStates = new array<string>();
		m_EntityAssignments = new array<string>();

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

		// NOTE: We don't use CallLater for the timer anymore.
		// Instead, CheckAndFlush() is called from StateTracker.CaptureAllPositions()
		// which already runs on a 1-second timer during recording.

		OpsTrackLogger.Info("ApiClient initialized successfully");
	}

	void ~ApiClient()
	{
		OpsTrackLogger.Info("ApiClient shutting down");
		m_IsShuttingDown = true;

		// Final flush of any pending data
		if (GetTotalPendingCount() > 0)
		{
			OpsTrackLogger.Warn(string.Format("Final flush: %1 pending items", GetTotalPendingCount()));
			FlushUnified();
		}
	}

	// ============================================
	// PUBLIC QUEUE METHODS - Add data to queues
	// ============================================

	// Queue a combat or connection event
	void Enqueue(string eventJson, OpsTrack_EventType eventType)
	{
		if (!CanSend() || !eventJson || eventJson == "")
			return;

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
	}

	// Queue an entity for creation
	void EnqueueEntity(string entityJson)
	{
		if (!CanSend() || !entityJson || entityJson == "")
			return;

		if (m_Entities)
		{
			m_Entities.Insert(entityJson);
			OpsTrackLogger.Debug(string.Format("Entity queued. Queue size: %1", m_Entities.Count()));
		}
	}

	// Queue entity state (position update)
	void EnqueueEntityState(string stateJson)
	{
		if (!CanSend() || !stateJson || stateJson == "")
			return;

		if (m_EntityStates)
		{
			m_EntityStates.Insert(stateJson);

			// Force flush if we have too many states (prevents payload from getting too large)
			if (m_EntityStates.Count() >= MAX_STATES_PER_BATCH)
			{
				OpsTrackLogger.Info(string.Format("State batch full (%1), forcing flush", MAX_STATES_PER_BATCH));
				FlushUnified();
			}
		}
	}

	// Queue entity assignment to current mission
	void EnqueueEntityAssignment(string entityId)
	{
		if (!CanSend() || !entityId || entityId == "")
			return;

		if (m_EntityAssignments)
			m_EntityAssignments.Insert(entityId);
	}

	// ============================================
	// DIRECT SEND METHODS - For critical one-off requests
	// These should be used sparingly!
	// ============================================

	// Send mission start (must be sent immediately, not batched)
	void SendMissionStart(string payload)
	{
		if (!CanSend() || !m_Context)
			return;

		OpsTrackLogger.Info(string.Format("Sending mission start: %1", payload));
		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, "/missions", payload);
	}

	// Send mission end (must be sent immediately)
	void SendMissionEnd(UUID missionId)
	{
		if (!CanSend() || !m_Context)
			return;

		string endpoint = string.Format("/missions/%1/end", missionId);
		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, endpoint, "{}");
	}

	// ============================================
	// LEGACY METHODS - Kept for compatibility
	// These now queue data instead of sending directly
	// ============================================

	// Legacy: Send entity states - now queues instead
	void SendEntityStates(string payload)
	{
		// Parse the states from the payload and queue them individually
		// For now, we'll just log a warning - StateTracker should call EnqueueEntityState directly
		OpsTrackLogger.Warn("SendEntityStates called - this should use EnqueueEntityState instead");
	}

	// Legacy: Assign entities to mission - now queues instead
	void AssignEntitiesToMission(string payload)
	{
		// This is called with full payload, but we just need the entityIds
		// For now, log warning - callers should use EnqueueEntityAssignment
		OpsTrackLogger.Warn("AssignEntitiesToMission called - this should use EnqueueEntityAssignment instead");
	}

	// ============================================
	// UNIFIED FLUSH - Single request with all data
	// ============================================

	// Called from StateTracker every second - checks if it's time to flush
	void CheckAndFlush()
	{
		if (m_IsShuttingDown)
			return;

		// Skip if we're still waiting for previous request
		if (m_HasPendingRequest)
		{
			OpsTrackLogger.Debug("Skipping flush - previous request still pending");
			return;
		}

		// Check if enough time has passed since last flush
		int now = System.GetTickCount();
		if (now - m_LastFlushTick < FLUSH_INTERVAL_MS)
			return;

		int total = GetTotalPendingCount();
		if (total == 0)
			return;

		// Log queue sizes
		int stateCount = 0;
		int entityCount = 0;
		int assignCount = 0;
		if (m_EntityStates) stateCount = m_EntityStates.Count();
		if (m_Entities) entityCount = m_Entities.Count();
		if (m_EntityAssignments) assignCount = m_EntityAssignments.Count();

		OpsTrackLogger.Info(string.Format("Unified flush: %1 states, %2 entities, %3 assignments", stateCount, entityCount, assignCount));
		FlushUnified();
	}

	// Force flush - used when stopping recording
	void ForceFlush()
	{
		if (GetTotalPendingCount() > 0)
		{
			OpsTrackLogger.Info("Force flushing remaining data...");
			FlushUnified();
		}
	}

	// Send all queued data in a single request
	protected void FlushUnified()
	{
		if (GetTotalPendingCount() == 0)
			return;

		if (!m_Context)
		{
			OpsTrackLogger.Error("Cannot flush: REST context is null");
			return;
		}

		// Limit states per batch to avoid payload size issues
		int statesToSend = MAX_STATES_PER_BATCH;
		if (m_EntityStates && m_EntityStates.Count() < statesToSend)
			statesToSend = m_EntityStates.Count();

		// Build unified payload (with limited states)
		string payload = BuildUnifiedPayload(statesToSend);

		// Check payload size
		int payloadSize = payload.Length();
		if (payloadSize > MAX_PAYLOAD_BYTES)
		{
			OpsTrackLogger.Warn(string.Format("Payload too large (%1 bytes), reducing batch size", payloadSize));
			// Retry with smaller batch
			statesToSend = statesToSend / 2;
			if (statesToSend < 10)
				statesToSend = 10;
			payload = BuildUnifiedPayload(statesToSend);
		}

		// Remove sent items from queues
		ClearSentItems(statesToSend);

		m_LastFlushTick = System.GetTickCount();
		m_HasPendingRequest = true;

		// Send single unified request
		m_PendingCallback = new OpsTrackCallback(this);
		m_Context.POST(m_PendingCallback, "/batch", payload);

		// If there are remaining states, schedule another flush soon
		if (m_EntityStates && m_EntityStates.Count() > 0)
		{
			OpsTrackLogger.Info(string.Format("Batch sent, %1 states remaining in queue", m_EntityStates.Count()));
		}
	}

	// Build payload with a limit on how many states to include
	protected string BuildUnifiedPayload(int maxStates)
	{
		OpsTrackManager manager = OpsTrackManager.GetIfExists();
		string missionIdStr = "null";
		if (manager && manager.IsRecording())
		{
			UUID missionId = manager.GetCurrentMissionId();
			if (!missionId.IsNull())
				missionIdStr = "\"" + missionId + "\"";
		}

		string payload = "{";
		payload = payload + "\"missionId\":" + missionIdStr + ",";

		// Entities array (all entities - these are small)
		payload = payload + "\"entities\":[";
		if (m_Entities)
		{
			for (int i = 0; i < m_Entities.Count(); i++)
			{
				payload = payload + m_Entities[i];
				if (i < m_Entities.Count() - 1)
					payload = payload + ",";
			}
		}
		payload = payload + "],";

		// Entity states array (limited to maxStates)
		payload = payload + "\"states\":[";
		if (m_EntityStates)
		{
			int stateCount = m_EntityStates.Count();
			if (stateCount > maxStates)
				stateCount = maxStates;

			for (int s = 0; s < stateCount; s++)
			{
				payload = payload + m_EntityStates[s];
				if (s < stateCount - 1)
					payload = payload + ",";
			}
		}
		payload = payload + "],";

		// Entity assignments array (all - these are small)
		payload = payload + "\"assignEntityIds\":[";
		if (m_EntityAssignments)
		{
			for (int a = 0; a < m_EntityAssignments.Count(); a++)
			{
				payload = payload + "\"" + m_EntityAssignments[a] + "\"";
				if (a < m_EntityAssignments.Count() - 1)
					payload = payload + ",";
			}
		}
		payload = payload + "],";

		// Connection events array (all - these are rare)
		payload = payload + "\"connectionEvents\":[";
		if (m_ConnectionEvents)
		{
			for (int c = 0; c < m_ConnectionEvents.Count(); c++)
			{
				payload = payload + m_ConnectionEvents[c];
				if (c < m_ConnectionEvents.Count() - 1)
					payload = payload + ",";
			}
		}
		payload = payload + "],";

		// Combat events array
		payload = payload + "\"combatEvents\":[";
		if (m_CombatEvents)
		{
			for (int cb = 0; cb < m_CombatEvents.Count(); cb++)
			{
				payload = payload + m_CombatEvents[cb];
				if (cb < m_CombatEvents.Count() - 1)
					payload = payload + ",";
			}
		}
		payload = payload + "]";

		payload = payload + "}";

		return payload;
	}

	// Clear only the items that were sent (states are limited, others are cleared fully)
	protected void ClearSentItems(int statesSent)
	{
		// Clear all non-state queues (they're always sent in full)
		if (m_ConnectionEvents)
			m_ConnectionEvents.Clear();
		if (m_CombatEvents)
			m_CombatEvents.Clear();
		if (m_Entities)
			m_Entities.Clear();
		if (m_EntityAssignments)
			m_EntityAssignments.Clear();

		// Remove only the states that were sent (first N items)
		if (m_EntityStates && statesSent > 0)
		{
			// Remove from front of array
			for (int i = 0; i < statesSent && m_EntityStates.Count() > 0; i++)
			{
				m_EntityStates.Remove(0);
			}
		}
	}

	protected void ClearAllQueues()
	{
		if (m_ConnectionEvents)
			m_ConnectionEvents.Clear();
		if (m_CombatEvents)
			m_CombatEvents.Clear();
		if (m_Entities)
			m_Entities.Clear();
		if (m_EntityStates)
			m_EntityStates.Clear();
		if (m_EntityAssignments)
			m_EntityAssignments.Clear();
	}

	// ============================================
	// HELPER METHODS
	// ============================================

	protected bool CanSend()
	{
		if (!m_ApiEnabled)
		{
			int now = System.GetTickCount();
			if (now < m_NextRetryTick)
				return false;

			m_ApiEnabled = true;
			OpsTrackLogger.Info("API re-enabled after cooldown");
		}
		return true;
	}

	int GetTotalPendingCount()
	{
		int count = 0;
		if (m_ConnectionEvents)
			count = count + m_ConnectionEvents.Count();
		if (m_CombatEvents)
			count = count + m_CombatEvents.Count();
		if (m_Entities)
			count = count + m_Entities.Count();
		if (m_EntityStates)
			count = count + m_EntityStates.Count();
		if (m_EntityAssignments)
			count = count + m_EntityAssignments.Count();
		return count;
	}

	// Called when request completes successfully
	void OnRequestComplete()
	{
		m_HasPendingRequest = false;
	}

	// Called by callback on error - triggers backoff
	void Backoff()
	{
		m_ApiEnabled = false;
		m_HasPendingRequest = false;
		m_NextRetryTick = System.GetTickCount() + COOLDOWN_MS;

		OpsTrackLogger.Warn(string.Format("API backoff triggered. Will retry in %1 seconds.", COOLDOWN_MS / 1000));

		// Drop pending data during backoff to prevent memory buildup
		ClearAllQueues();
	}
}
