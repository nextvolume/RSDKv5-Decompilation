extern "C" {
#include <wss.h>
}
#define LockAudioDevice()
#define UnlockAudioDevice() 

namespace RSDK
{
class AudioDevice : public AudioDeviceBase
{
public:

    static bool32 Init();
    static void Release();

    static void FrameInit() {}

    inline static void HandleStreamLoad(ChannelInfo *channel, bool32 async)
    {
/*        if (async)
            SDL_CreateThread((SDL_ThreadFunction)LoadStream, "LoadStream", (void *)channel);
        else
            LoadStream(channel);*/
	    LoadStream(channel);
    }
    
    static AUDIOSTREAM *stream;
    static void AudioCallback(void *data, short *stream, int32 len);
    static int numSamples;

private:
    static uint8 contextInitialized;
    static std::vector<short> buf;
    static float *floatBuf;
    static short *wordBuf;

    static void InitAudioChannels();
    static void InitMixBuffer() {}

};
} // namespace RSDK