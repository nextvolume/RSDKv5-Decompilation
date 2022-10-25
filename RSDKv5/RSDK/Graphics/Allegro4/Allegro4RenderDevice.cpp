
uint32 RenderDevice::displayModeIndex = 0;
int32 RenderDevice::displayModeCount  = 0;

unsigned long long RenderDevice::targetFreq = 0;
unsigned long long RenderDevice::curTicks   = 0;
unsigned long long RenderDevice::prevTicks  = 0;

bool RenderDevice::mouseInstalled = false;

RenderVertex RenderDevice::vertexBuffer[!RETRO_REV02 ? 24 : 60];

uint8 RenderDevice::lastTextureFormat = -1;

uint8 RenderDevice::wasKey[256];

BITMAP *RenderDevice::pointer;

#define NORMALIZE(val, minVal, maxVal) ((float)(val) - (float)(minVal)) / ((float)(maxVal) - (float)(minVal))

#ifdef DOS
#define NUM_OF_VGA_MODES 6

static const struct
{
	int mode;
	short w, h;
}VGAModes[NUM_OF_VGA_MODES] = 
{
    { GFX_VGA, 320, 200 },
    { GFX_MODEX, 320, 240 },
    { GFX_MODEX, 360, 240 },
    { GFX_MODEX, 376, 282 },
    { GFX_MODEX, 400, 300 },
    { GFX_MODEX, 256, 240 }
};
#endif


static volatile int fps_cap = 0;
static volatile bool retro_close_button = false;

static void retro_close_button_callback(void)
{
	retro_close_button = true;
}
END_OF_FUNCTION(retro_close_button_callback)

static void fps_cap_handler(void)
{
	fps_cap++;
}
END_OF_FUNCTION(fps_cap_handler)

BITMAP *video_buffer[SCREEN_COUNT];

bool RenderDevice::Init()
{
    allegro_init();
    
    install_keyboard();
    mouseInstalled = install_mouse() > 0;

    float sw = videoSettings.windowWidth, sh = videoSettings.windowHeight;

#if DOS
	videoSettings.windowed = false;
#endif
	
    if (!videoSettings.windowed) {
	if (videoSettings.fsWidth)
	    sw = videoSettings.fsWidth;
	
	if (videoSettings.fsHeight)
	    sh = videoSettings.fsHeight;
    }

    if (videoSettings.scale == 0)
	    videoSettings.scale = 1;

#if DOS
    if ((sw * videoSettings.scale) == 424)
    {
        sw = 320;
        videoSettings.scale = 1;
    }
#endif
    
      set_color_depth(16);
    if (
#if DOS
        videoSettings.useVGAMode ||
#endif
        set_gfx_mode(
#if DOS
	GFX_AUTODETECT,
#else
    !videoSettings.windowed ? GFX_AUTODETECT_FULLSCREEN : GFX_AUTODETECT_WINDOWED,
#endif
        sw * videoSettings.scale, sh * videoSettings.scale,
	0, 0) != 0
    )
    {
#if DOS
        if (!videoSettings.useVGAMode) {
	    PrintLog(PRINT_NORMAL, "Couldn't get hi-color mode.");
	    videoSettings.useVGAMode=1;
	    videoSettings.scale=1;
        } 

	set_color_depth(8);
	
	int m = videoSettings.useVGAMode-1;
	
	if (m < 0 || m >= NUM_OF_VGA_MODES)
		PrintLog(PRINT_NORMAL, "ERROR: Invalid VGA mode specified!");
	
	PrintLog(PRINT_NORMAL,
	    "Using %s video mode %dx%d", (VGAModes[m].mode==GFX_VGA)?"VGA":"Mode-X",
		VGAModes[m].w, VGAModes[m].h);
	
	if (m >= 0 && m < NUM_OF_VGA_MODES && set_gfx_mode(VGAModes[m].mode, VGAModes[m].w,
            VGAModes[m].h, 0, 0) == 0) {
		PALETTE rgbPal;
	    
		for(int x = 0; x < 256; x++)
		{
			int v;    
	
			v = (x & 3);
			rgbPal[x].b = (v << 4) + (v << 2) + v;
			v = (x >> 2) & 7;
			rgbPal[x].g = (v << 3) + v;
			v = (x >> 5) & 7;
			rgbPal[x].r = (v << 3) + v;	    
		}
	    
		set_palette(rgbPal);
    
		videoSettings.scale = 1;
	}
	else
#endif
	{
	    PrintLog(PRINT_NORMAL, "ERROR: Couldn't initialize a graphic mode!");
	    return false;
	}
    }
    
    int w, h;
    
    sw = SCREEN_W / videoSettings.scale;
    sh = SCREEN_H / videoSettings.scale;
   
    if (sw < 320)	w = 320; else w = sw;
    if (sh < 240)	h = 240; else h = sh;
    
    videoSettings.pixWidth = w;
    videoSettings.pixHeight = h;
    
    videoSettings.screenCount = SCREEN_COUNT;
    
    engine.inFocus = true;
    engine.focusState = 1;
    
    for(int x = 0; x < SCREEN_COUNT; x++) {
	video_buffer[x] = create_bitmap(videoSettings.pixWidth, videoSettings.pixHeight);
	clear(video_buffer[x]);
    }
    
    pointer = create_mouse_pointer(mouse_arrow_data);
        
    SetupRendering();
    
    set_close_button_callback(retro_close_button_callback);
    
    InitInputDevices();
    AudioDevice::Init();
    
    bzero(wasKey, sizeof(wasKey));
    
    return true;
}

void RenderDevice::CopyFrameBuffer()
{
	unsigned short c;
	int t;
	
	if (get_color_depth() == 8) {
		for (int s = 0; s < (videoSettings.screenCount > 0 ? 1 : 0); s++) {
        for(int x = 0; x < (videoSettings.pixWidth * videoSettings.pixHeight); x++) {
		c = screens[s].frameBuffer[x];
				
		video_buffer[s]->line[0][x] =
			( (c & 31) >> 3 ) |
			( ( ( (c >> 5) & 63 ) >> 3 ) << 2 )|
			( ( ( (c >> 11) & 31 ) >> 2 ) << 5);
	}
	}
	}
	else
	{

	
	for (int s = 0; s < (videoSettings.screenCount > 0 ? 1 : 0); s++) {
		uint16 *frameBuffer = screens[s].frameBuffer;
		
		for (int32 y = 0; y < SCREEN_YSIZE; ++y) {
			memcpy(video_buffer[s]->line[y], frameBuffer, screens[s].size.x * sizeof(uint16));
			frameBuffer += screens[s].pitch;
		}
	}
	
	}	
}

int RenderDevice::GetMouseX()
{
	return mouse_x * ((float)videoSettings.pixWidth / SCREEN_W);
}

int RenderDevice::GetMouseY()
{
	return mouse_y * ((float)videoSettings.pixHeight / SCREEN_H);
}

void RenderDevice::FlipScreen()
{
	static int curScreen = 0;

	if (mouseInstalled)
		draw_sprite(video_buffer[curScreen], pointer, 
			GetMouseX(), GetMouseY());
		
	stretch_blit(video_buffer[curScreen], screen, 0, 0, video_buffer[curScreen]->w,
		video_buffer[curScreen]->h, 0, 0, SCREEN_W, SCREEN_H);
}

void RenderDevice::Release(bool32 isRefresh)
{

}

void RenderDevice::RefreshWindow()
{
    videoSettings.windowState = WINDOWSTATE_ACTIVE;
}

void RenderDevice::InitFPSCap()
{
    LOCK_VARIABLE(fps_cap);
    LOCK_FUNCTION(fps_cap_handler);
    install_int_ex(fps_cap_handler, BPS_TO_TIMER(60));
}

bool RenderDevice::CheckFPSCap()
{
    return fps_cap > 0;
}

void RenderDevice::UpdateFPSCap() { fps_cap = 0; }

void RenderDevice::InitVertexBuffer()
{
    RenderVertex vertBuffer[sizeof(rsdkVertexBuffer) / sizeof(RenderVertex)];
    memcpy(vertBuffer, rsdkVertexBuffer, sizeof(rsdkVertexBuffer));

    // ignore the last 6 verts, they're scaled to the 1024x512 textures already!
    int32 vertCount = (RETRO_REV02 ? 60 : 24) - 6;

    // Regular in-game screen de-normalization stuff
    for (int32 v = 0; v < vertCount; ++v) {
        RenderVertex *vertex = &vertBuffer[v];
        vertex->pos.x        = NORMALIZE(vertex->pos.x, -1.0, 1.0) * videoSettings.pixWidth;
        vertex->pos.y        = (1.0 - NORMALIZE(vertex->pos.y, -1.0, 1.0)) * SCREEN_YSIZE;

        if (vertex->tex.x)
            vertex->tex.x = screens[0].size.x * (1.0 / textureSize.x);

        if (vertex->tex.y)
            vertex->tex.y = screens[0].size.y * (1.0 / textureSize.y);
    }

    // Fullscreen Image/Video de-normalization stuff
    for (int32 v = 0; v < 6; ++v) {
        RenderVertex *vertex = &vertBuffer[vertCount + v];
        vertex->pos.x        = NORMALIZE(vertex->pos.x, -1.0, 1.0) * videoSettings.pixWidth;
        vertex->pos.y        = (1.0 - NORMALIZE(vertex->pos.y, -1.0, 1.0)) * SCREEN_YSIZE;

        // Set the texture to fill the entire screen with all 1024x512 pixels
        if (vertex->tex.x)
            vertex->tex.x = 1.0f;

        if (vertex->tex.y)
            vertex->tex.y = 1.0f;
    }

    memcpy(vertexBuffer, vertBuffer, sizeof(vertBuffer));
}

bool RenderDevice::InitGraphicsAPI()
{
    videoSettings.shaderSupport = false;

    viewSize.x = 0;
    viewSize.y = 0;

    if (videoSettings.windowed || !videoSettings.exclusiveFS) {
        if (videoSettings.windowed) {
            viewSize.x = videoSettings.windowWidth;
            viewSize.y = videoSettings.windowHeight;
        }
        else {
            viewSize.x = displayWidth[displayModeIndex];
            viewSize.y = displayHeight[displayModeIndex];
        }
    }
    else {
        int32 bufferWidth  = videoSettings.fsWidth;
        int32 bufferHeight = videoSettings.fsHeight;
        if (videoSettings.fsWidth <= 0 || videoSettings.fsHeight <= 0) {
            bufferWidth  = displayWidth[displayModeIndex];
            bufferHeight = displayHeight[displayModeIndex];
        }

        viewSize.x = bufferWidth;
        viewSize.y = bufferHeight;
    }
    
    int32 maxPixHeight = 0;
#if !RETRO_USE_ORIGINAL_CODE
    int32 screenWidth = 0;
#endif
    for (int32 s = 0; s < 4; ++s) {
        if (videoSettings.pixHeight > maxPixHeight)
            maxPixHeight = videoSettings.pixHeight;

        screens[s].size.y = videoSettings.pixHeight;

        float viewAspect  = viewSize.x / viewSize.y;
#if !RETRO_USE_ORIGINAL_CODE
        screenWidth = (int32)((viewAspect * videoSettings.pixHeight) + 3) & 0xFFFFFFFC;
#else
        int32 screenWidth = (int32)((viewAspect * videoSettings.pixHeight) + 3) & 0xFFFFFFFC;
#endif
        if (screenWidth < videoSettings.pixWidth)
            screenWidth = videoSettings.pixWidth;

#if !RETRO_USE_ORIGINAL_CODE
        if (customSettings.maxPixWidth && screenWidth > customSettings.maxPixWidth)
            screenWidth = customSettings.maxPixWidth;
#else
        if (screenWidth > DEFAULT_PIXWIDTH)
            screenWidth = DEFAULT_PIXWIDTH;
#endif

        memset(&screens[s].frameBuffer, 0, sizeof(screens[s].frameBuffer));
        SetScreenSize(s, screenWidth, screens[s].size.y);
    }

    pixelSize.x = screens[0].size.x;
    pixelSize.y = screens[0].size.y;

#if !RETRO_USE_ORIGINAL_CODE
    if (screenWidth <= 512 && maxPixHeight <= 256) {
#else
    if (maxPixHeight <= 256) {
#endif
        textureSize.x = 512.0;
        textureSize.y = 256.0;
    }
    else {
        textureSize.x = 1024.0;
        textureSize.y = 512.0;
    }

    return true;
}

void RenderDevice::LoadShader(const char *fileName, bool32 linear) { PrintLog(PRINT_NORMAL, "This render device does not support shaders!"); }

bool RenderDevice::InitShaders()
{
    int32 maxShaders = 0;
    if (videoSettings.shaderSupport) {
        LoadShader("None", false);
        LoadShader("Clean", true);
        LoadShader("CRT-Yeetron", true);
        LoadShader("CRT-Yee64", true);

#if RETRO_USE_MOD_LOADER
        // a place for mods to load custom shaders
        RunModCallbacks(MODCB_ONSHADERLOAD, NULL);
        userShaderCount = shaderCount;
#endif

        LoadShader("YUV-420", true);
        LoadShader("YUV-422", true);
        LoadShader("YUV-444", true);
        LoadShader("RGB-Image", true);
        maxShaders = shaderCount;
    }
    else {
        for (int32 s = 0; s < SHADER_COUNT; ++s) shaderList[s].linear = true;

        shaderList[0].linear = videoSettings.windowed ? false : shaderList[0].linear;
        maxShaders           = 1;
        shaderCount          = 1;
    }

    videoSettings.shaderID = videoSettings.shaderID >= maxShaders ? 0 : videoSettings.shaderID;

    return true;
}

bool RenderDevice::SetupRendering()
{
    GetDisplays();

    if (!InitGraphicsAPI() || !InitShaders())
        return false;

    int32 size = videoSettings.pixWidth >= SCREEN_YSIZE ? videoSettings.pixWidth : SCREEN_YSIZE;
    scanlines  = (ScanlineInfo *)malloc(size * sizeof(ScanlineInfo));
    memset(scanlines, 0, size * sizeof(ScanlineInfo));

    videoSettings.windowState = WINDOWSTATE_ACTIVE;
    videoSettings.dimMax      = 1.0;
    videoSettings.dimPercent  = 1.0;

    return true;
}

void RenderDevice::GetDisplays()
{
	displayInfo.displays = (decltype(displayInfo.displays))malloc(sizeof(decltype(displayInfo.displays)));
	displayCount = 1;
	
	displayInfo.displays[0].width = SCREEN_W;
	displayInfo.displays[0].height  = SCREEN_H;
	displayInfo.displays[0].refresh_rate = 60;
}

void RenderDevice::GetWindowSize(int32 *width, int32 *height)
{
    *width = displayInfo.displays[0].width;
    *height = displayInfo.displays[0].height;	
}

bool RenderDevice::ProcessEvents()
{
    void *streamBuf = NULL;
    static int wasKey[256];

    isRunning = !retro_close_button;
	
    if (AudioDevice::audioState) {
	
#ifdef RETRO_AUDIODEVICE_ALLEGRO4
    if ( ( streamBuf = get_audio_stream_buffer(AudioDevice::stream) ) ) {
	AudioDevice::AudioCallback(NULL, (short*)streamBuf, AudioDevice::numSamples);
	free_audio_stream_buffer(AudioDevice::stream);
    }
#endif

#if defined(RETRO_AUDIODEVICE_WSS)  \
	    || defined(RETRO_AUDIODEVICE_DOSSOUND)
    AudioDevice::AudioCallback(NULL, NULL, AudioDevice::numSamples);  
#endif
    }
    
    if (key[__allegro_KEY_ESC] && !wasKey[__allegro_KEY_ESC]) {
	if (engine.devMenu) {
#if RETRO_REV0U
		if (sceneInfo.state == ENGINESTATE_DEVMENU || RSDK::Legacy::gameMode == RSDK::Legacy::ENGINE_DEVMENU)
#else
		if (sceneInfo.state == ENGINESTATE_DEVMENU)
#endif
			CloseDevMenu();
		else
			OpenDevMenu();
	}
	
    }
    
    #if !RETRO_USE_ORIGINAL_CODE
    if (key[__allegro_KEY_F1] && !wasKey[__allegro_KEY_F1]) {
                    if (engine.devMenu) {
                        sceneInfo.listPos--;
                        if (sceneInfo.listPos < sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart) {
                            sceneInfo.activeCategory--;
                            if (sceneInfo.activeCategory >= sceneInfo.categoryCount) {
                                sceneInfo.activeCategory = sceneInfo.categoryCount - 1;
                            }
                            sceneInfo.listPos = sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd - 1;
                        }

#if RETRO_REV0U
                        switch (engine.version) {
                            default: break;
                            case 5: LoadScene(); break;
                            case 4:
                            case 3: RSDK::Legacy::stageMode = RSDK::Legacy::STAGEMODE_LOAD; break;
                        }
#else
                        LoadScene();
#endif
                    }
    }
    
    if (key[__allegro_KEY_F2] && !wasKey[__allegro_KEY_F2]) {
if (engine.devMenu) {
                        sceneInfo.listPos++;
                        if (sceneInfo.listPos >= sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd) {
                            sceneInfo.activeCategory++;
                            if (sceneInfo.activeCategory >= sceneInfo.categoryCount) {
                                sceneInfo.activeCategory = 0;
                            }
                            sceneInfo.listPos = sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart;
                        }

#if RETRO_REV0U
                        switch (engine.version) {
                            default: break;
                            case 5: LoadScene(); break;
                            case 4:
                            case 3: RSDK::Legacy::stageMode = RSDK::Legacy::STAGEMODE_LOAD; break;
                        }
#else
                        LoadScene();
#endif
                    }
    }
    
    #endif
    
    if (key[__allegro_KEY_F3] && !wasKey[__allegro_KEY_F3]) {
	if (userShaderCount)
		videoSettings.shaderID = (videoSettings.shaderID + 1) % userShaderCount;
    }
    

#if !RETRO_USE_ORIGINAL_CODE
    if (key[__allegro_KEY_F5] && !wasKey[__allegro_KEY_F5]) {
                    if (engine.devMenu) {
                        // Quick-Reload
#if RETRO_REV0U
                        switch (engine.version) {
                            default: break;
                            case 5: LoadScene(); break;
                            case 4:
                            case 3: RSDK::Legacy::stageMode = RSDK::Legacy::STAGEMODE_LOAD; break;
                        }
#else
                        LoadScene();
#endif
                    }
    }

    if (key[__allegro_KEY_F6] && !wasKey[__allegro_KEY_F6]) {
                    if (engine.devMenu && videoSettings.screenCount > 1)
                        videoSettings.screenCount--;
    }

    if (key[__allegro_KEY_F7] && !wasKey[__allegro_KEY_F7]) {
                    if (engine.devMenu && videoSettings.screenCount < SCREEN_COUNT)
                        videoSettings.screenCount++;
    }

    if (key[__allegro_KEY_F9] && !wasKey[__allegro_KEY_F9]) {
                    if (engine.devMenu)
                        showHitboxes ^= 1;
    }

    if (key[__allegro_KEY_F10] && !wasKey[__allegro_KEY_F10]) {
                    if (engine.devMenu)
                        engine.showPaletteOverlay ^= 1;
    }
#endif
    
    if (key[__allegro_KEY_BACKSPACE] && !wasKey[__allegro_KEY_BACKSPACE]) {
                    if (engine.devMenu)
                        engine.gameSpeed = engine.fastForwardSpeed;
    }

    if ((key[__allegro_KEY_F11] && !wasKey[__allegro_KEY_F11]) ||
	(key[__allegro_KEY_INSERT] && !wasKey[__allegro_KEY_INSERT])) {
		if (engine.devMenu)
                        engine.frameStep = true;
                    
    }
			
    if ((key[__allegro_KEY_F12] && !wasKey[__allegro_KEY_F12]) ||
	(key[__allegro_KEY_PAUSE] && !wasKey[__allegro_KEY_PAUSE])) {
		                    if (engine.devMenu) {
#if RETRO_REV0U
                        switch (engine.version) {
                            default: break;
                            case 5:
                                if (sceneInfo.state != ENGINESTATE_NONE)
                                    sceneInfo.state ^= ENGINESTATE_STEPOVER;
                                break;
                            case 4:
                            case 3:
                                if (RSDK::Legacy::stageMode != ENGINESTATE_NONE)
                                    RSDK::Legacy::stageMode ^= RSDK::Legacy::STAGEMODE_STEPOVER;
                                break;
                        }
#else
                        if (sceneInfo.state != ENGINESTATE_NONE)
                            sceneInfo.state ^= ENGINESTATE_STEPOVER;
#endif
                    }
    }
	
    
    for (int x = 0; x < 256; x++)
	wasKey[x] = key[x];

    touchInfo.down[0] = mouse_b & 1;
    touchInfo.count = touchInfo.down[0] == 1;

    touchInfo.x[0] = (float)GetMouseX() / videoSettings.pixWidth;
    touchInfo.y[0] = (float)GetMouseY() / videoSettings.pixHeight;
    
    return isRunning;
}

void RenderDevice::SetupImageTexture(int32 width, int32 height, uint8 *imagePixels)
{

}

void RenderDevice::yuv2rgb(uint8_t yValue, uint8_t uValue, uint8_t vValue,
        uint8_t *r, uint8_t *g, uint8_t *b) {
    int rTmp = yValue + (1.370705 * (vValue-128)); 
    // or fast integer computing with a small approximation
    // rTmp = yValue + (351*(vValue-128))>>8;
    int gTmp = yValue - (0.698001 * (vValue-128)) - (0.337633 * (uValue-128)); 
    // gTmp = yValue - (179*(vValue-128) + 86*(uValue-128))>>8;
    int bTmp = yValue + (1.732446 * (uValue-128));
    // bTmp = yValue + (443*(uValue-128))>>8;
    *r = CLAMP(rTmp, 0, 255);
    *g = CLAMP(gTmp, 0, 255);
    *b = CLAMP(bTmp, 0, 255);
}

int RenderDevice::getChroma(uint8_t *plane, int stride, int x, int y)
{
	x /= 2;
	y /= 2;
	
	return plane[(stride * y) + x];
}

void RenderDevice::SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
	int F = 4;

	BITMAP *bmp = create_bitmap(width / F, height / F);
		
	uint8_t r, g, b;
	int u = 0, v = 0;
	
	for(int y = 0; y < height; y+=F)
	{
		for(int x = 0; x < width; x+=F)
		{
			u = getChroma(uPlane, strideU, x, y);
			v = getChroma(vPlane, strideV, x, y);
			
			yuv2rgb(yPlane[(strideY * y)+x], u, v,
				&r, &g, &b);
			
			if (get_color_depth() == 8) {
				((uint8_t*)bmp->line[y/F])[x/F] = 
					((r >> 5) << 5) |
					((g >> 5) << 2) |
					(b >> 6);				
			}
			else
				((uint16_t*)bmp->line[y/F])[x/F] =
					((r >> 3) << 11) |
					((g >> 2) << 5) |
					(b >> 3);			
		}
	}
	

	
	stretch_blit(bmp, video_buffer[0], 0, 0, bmp->w, bmp->h, 0, 0, video_buffer[0]->w, video_buffer[0]->h);
	
	destroy_bitmap(bmp);
}

void RenderDevice::SetupVideoTexture_YUV422(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
//	printf("YUV422\n");

}
void RenderDevice::SetupVideoTexture_YUV444(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
//	printf("YUV444\n");

}

char RenderDevice::mouse_arrow_data[16 * 16] =
{
   2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 2, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 2, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0
};

BITMAP *RenderDevice::create_mouse_pointer(char *data)
{
   BITMAP *bmp;
   int x, y;
   int col;
   
   bmp = create_bitmap(16, 16);

   for (y=0; y<16; y++) {
      for (x=0; x<16; x++) {
	 switch (data[x+y*16]) {
	    case 1:  col = makecol(255, 255, 255);  break;
	    case 2:  col = makecol(0, 0, 0);        break;
	    default: col = bmp->vtable->mask_color; break;
	 }
	 putpixel(bmp, x, y, col);
      }
   }

   return bmp;
}
