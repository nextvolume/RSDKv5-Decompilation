using ShaderEntry = ShaderEntryBase;

class RenderDevice : public RenderDeviceBase
{
public:
    struct WindowInfo {
        union {
            struct {
                // i wanna do uint32 : 32 but idk if other compilers like that
                uint32 _pad;
                int32 width;
                int32 height;
                int32 refresh_rate;
            };
        } * displays;
    };
    static WindowInfo displayInfo;

    static bool Init();
    static void CopyFrameBuffer();
    static void FlipScreen();
    static void Release(bool32 isRefresh);

    static void RefreshWindow();
    static void GetWindowSize(int32 *width, int32 *height);

    static void SetupImageTexture(int32 width, int32 height, uint8 *imagePixels);
    static void SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);
    static void SetupVideoTexture_YUV422(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);
    static void SetupVideoTexture_YUV444(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);

    static bool ProcessEvents();

    static void InitFPSCap();
    static bool CheckFPSCap();
    static void UpdateFPSCap();

    static void LoadShader(const char *fileName, bool32 linear);

    static inline void ShowCursor(bool32 shown) { /*SDL_ShowCursor(shown);*/ }
    static inline bool GetCursorPos(Vector2 *pos)
    {
	pos->x = mouse_x;
	pos->y = mouse_y;
        return true;
    };

    static inline void SetWindowTitle() { set_window_title(gameVerInfo.gameTitle); };

    
    static void yuv2rgb(uint8_t yValue, uint8_t uValue, uint8_t vValue,
        uint8_t *r, uint8_t *g, uint8_t *b);
    /*static SDL_Window *window;
    static SDL_Renderer *renderer;
    static SDL_Texture *screenTexture[SCREEN_COUNT];

    static SDL_Texture *imageTexture;
*/
    static int getChroma(uint8_t *plane, int stride, int x, int y);
 
private:
    static bool InitShaders();
    static bool SetupRendering();
    static void InitVertexBuffer();
    static bool InitGraphicsAPI();
    static int GetMouseX();
    static int GetMouseY();

    static void GetDisplays();

    static uint32 displayModeIndex;
    static int32 displayModeCount;

    static bool mouseInstalled;

    static unsigned long long targetFreq;
    static unsigned long long curTicks;
    static unsigned long long prevTicks;

    static RenderVertex vertexBuffer[!RETRO_REV02 ? 24 : 60];

    static BITMAP *pointer;
    static BITMAP *create_mouse_pointer(char *data);

    static char mouse_arrow_data[16 * 16];

    // thingo majigo for handling video/image swapping
    static uint8 lastTextureFormat;
    
    static uint8 wasKey[256];
};
