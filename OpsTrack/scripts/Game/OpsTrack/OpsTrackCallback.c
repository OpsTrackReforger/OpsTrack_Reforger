class OpsTrackCallback : RestCallback
{
    override void OnSuccess(string data, int dataSize)
    {
        OpsTrackLogger.Info("REST request succeeded. Response size = " + dataSize);
        OpsTrackLogger.Debug("REST response body: " + data);
    }

    override void OnError(int errorCode)
    {
        OpsTrackLogger.Error("REST request failed with error code: " + errorCode);
    }

    override void OnTimeout()
    {
        OpsTrackLogger.Error("REST request timed out.");
    }
}