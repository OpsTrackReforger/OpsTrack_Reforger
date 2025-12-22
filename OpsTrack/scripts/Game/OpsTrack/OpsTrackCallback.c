class OpsTrackCallback : RestCallback
{
    protected ApiClient m_Client;

    void OpsTrackCallback(ApiClient client)
    {
        m_Client = client;

        // Register modern callbacks
        SetOnSuccess(OnSuccessEx);
        SetOnError(OnErrorEx);
    }

    // SUCCESS callback
    void OnSuccessEx(RestCallback cb)
    {
        string data = cb.GetData();
        int httpCode = cb.GetHttpCode();

        OpsTrackLogger.Info("REST request succeeded. HTTP " + httpCode);
        OpsTrackLogger.Debug("REST response body: " + data);
    }

    // ERROR callback
    void OnErrorEx(RestCallback cb)
    {
        int httpCode = cb.GetHttpCode();
        string data = cb.GetData(); // <-- THIS gives you the JSON error body

        OpsTrackLogger.Error("REST request failed. HTTP " + httpCode);

        if (data && data != "")
            OpsTrackLogger.Error("REST error body: " + data);
        else
            OpsTrackLogger.Error("REST error body: <empty>");

        // Trigger your backoff logic
        m_Client.Backoff();
    }
}