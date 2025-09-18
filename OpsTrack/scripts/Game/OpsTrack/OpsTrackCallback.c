class OpsTrackCallback : RestCallback
{
    override void OnSuccess(string data, int dataSize)
    {
        Print("[OpsTrack] Success, size=" + dataSize);
        Print("[OpsTrack] Response: " + data);
    }

    override void OnError(int errorCode)
    {
        Print("[OpsTrack] Error code: " + errorCode);
    }

    override void OnTimeout()
    {
        Print("[OpsTrack] Request timed out!");
    }
}
