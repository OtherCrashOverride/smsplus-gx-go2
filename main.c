/*
smsplus-gx-go2 - Smsplus-gx port for the ODROID-GO Advance
Copyright (C) 2020  OtherCrashOverride

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <go2/display.h>
#include <go2/audio.h>
#include <go2/input.h>
#include <drm/drm_fourcc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <stdbool.h>
#include <pwd.h>

// smsplus-gx
#include "core/shared.h"



static go2_display_t* display;
static go2_presenter_t* presenter;
static go2_audio_t* audio;
static go2_input_t* go2input;
static go2_gamepad_state_t gamepadState;
static go2_gamepad_state_t previousState;

static volatile bool isRunning = true;


#define VIDEO_WIDTH 256
#define VIDEO_HEIGHT 192
#define GAMEGEAR_WIDTH (160)
#define GAMEGEAR_HEIGHT (144)
uint16 palette[PALETTE_SIZE];
uint8_t sms_framebuffer[VIDEO_WIDTH * VIDEO_HEIGHT];


#define SOUND_FREQUENCY 44100
#define SOUND_CHANNEL_COUNT (2)
//static int16_t* sampleBuffer;

static void InitSound()
{
    printf("Sound: SOUND_FREQUENCY=%d\n", SOUND_FREQUENCY);

    audio = go2_audio_create(SOUND_FREQUENCY);
}

static void ProcessAudio(uint8_t* samples, int frameCount)
{
    go2_audio_submit(audio, (const short*)samples, frameCount);
}

static void InitJoysticks()
{
    go2input = go2_input_create();
}

static void ReadJoysticks()
{
    go2_input_gamepad_read(go2input, &gamepadState);

    if (gamepadState.buttons.f1)
        isRunning = false;
}


static const char* FileNameFromPath(const char* fullpath)
{
    // Find last slash
    const char* ptr = strrchr(fullpath,'/');
    if (!ptr)
    {
        ptr = fullpath;
    }
    else
    {
        ++ptr;
    } 

    return ptr;   
}

static char* PathCombine(const char* path, const char* filename)
{
    int len = strlen(path);
    int total_len = len + strlen(filename);

    char* result = NULL;

    if (path[len-1] != '/')
    {
        ++total_len;
        result = (char*)calloc(total_len + 1, 1);
        strcpy(result, path);
        strcat(result, "/");
        strcat(result, filename);
    }
    else
    {
        result = (char*)calloc(total_len + 1, 1);
        strcpy(result, path);
        strcat(result, filename);
    }
    
    return result;
}

static int LoadState(const char* saveName)
{
    FILE* file = fopen(saveName, "rb");
	if (!file)
		return -1;

    system_load_state(file);

    fclose(file);

    return 0;
}

static void SaveState(const char* saveName)
{
    FILE* file = fopen(saveName, "wb");
	if (!file)
    {
		abort();
    }

    system_save_state(file);

    fclose(file);
}

void system_manage_sram(uint8 *sram, int slot, int mode)
{
    printf("system_manage_sram\n");
    //sram_load();
}

static void game_step()
{
    // Map thumbstick to dpad
    const float TRIM = 0.35f;

    if (gamepadState.thumb.y < -TRIM) gamepadState.dpad.up = ButtonState_Pressed;
    if (gamepadState.thumb.y > TRIM) gamepadState.dpad.down = ButtonState_Pressed;
    if (gamepadState.thumb.x < -TRIM) gamepadState.dpad.left = ButtonState_Pressed;
    if (gamepadState.thumb.x > TRIM) gamepadState.dpad.right = ButtonState_Pressed;


    int smsButtons=0;
    if (gamepadState.dpad.up) smsButtons |= INPUT_UP;
    if (gamepadState.dpad.down) smsButtons |= INPUT_DOWN;
    if (gamepadState.dpad.left) smsButtons |= INPUT_LEFT;
    if (gamepadState.dpad.right) smsButtons |= INPUT_RIGHT;
    if (gamepadState.buttons.a) smsButtons |= INPUT_BUTTON2;
    if (gamepadState.buttons.b) smsButtons |= INPUT_BUTTON1;

    int smsSystem=0;
    if (gamepadState.buttons.f4) smsSystem |= INPUT_START;
    if (gamepadState.buttons.f3) smsSystem |= INPUT_PAUSE;

    input.pad[0]=smsButtons;
    input.system=smsSystem;


    if (sms.console == CONSOLE_COLECO)
    {
        input.system = 0;
        coleco.keypad[0] = 0xff;
        coleco.keypad[1] = 0xff;

        // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, *, #
        switch (cart.crc)
        {
            case 0x798002a2:    // Frogger
            case 0x32b95be0:    // Frogger
            case 0x9cc3fabc:    // Alcazar
            case 0x964db3bc:    // Fraction Fever
                if (gamepadState.buttons.f4)
                {
                    coleco.keypad[0] = 10; // *
                }

                // if (previousState.values[ODROID_INPUT_SELECT] &&
                //     !joystick.values[ODROID_INPUT_SELECT])
                // {
                //     system_reset();
                // }
                break;

            case 0x1796de5e:    // Boulder Dash
            case 0x5933ac18:    // Boulder Dash
            case 0x6e5c4b11:    // Boulder Dash
                if (gamepadState.buttons.f4)
                {
                    coleco.keypad[0] = 11; // #
                }

                if (gamepadState.buttons.f4 &&
                    gamepadState.dpad.left)
                {
                    coleco.keypad[0] = 1;
                }

                // if (previousState.values[ODROID_INPUT_SELECT] &&
                //     !joystick.values[ODROID_INPUT_SELECT])
                // {
                //     system_reset();
                // }
                break;
            case 0x109699e2:    // Dr. Seuss's Fix-Up The Mix-Up Puzzler
            case 0x614bb621:    // Decathlon
                if (gamepadState.buttons.f4)
                {
                    coleco.keypad[0] = 1;
                }
                if (gamepadState.buttons.f4 &&
                    gamepadState.dpad.left)
                {
                    coleco.keypad[0] = 10; // *
                }
                break;

            default:
                if (gamepadState.buttons.f4)
                {
                    coleco.keypad[0] = 1;
                }

                // if (previousState.values[ODROID_INPUT_SELECT] &&
                //     !joystick.values[ODROID_INPUT_SELECT])
                // {
                //     system_reset();
                // }
                break;
        }

        if (previousState.buttons.f6 & !gamepadState.buttons.f6)
        {
            system_reset();
        }
    }

    previousState = gamepadState;

    // if (0 || (frame % 2) == 0)
    // {
        system_frame(0);

    //     xQueueSend(vidQueue, &bitmap.data, portMAX_DELAY);

    //     currentFramebuffer = currentFramebuffer ? 0 : 1;
    //     bitmap.data = framebuffer[currentFramebuffer];
    // }
    // else
    // {
    //     system_frame(1);
    // }

    // // Create a buffer for audio if needed
    // if (!audioBuffer || audioBufferCount < snd.sample_count)
    // {
    //     if (audioBuffer)
    //         free(audioBuffer);

    //     size_t bufferSize = snd.sample_count * 2 * sizeof(int16_t);
    //     //audioBuffer = malloc(bufferSize);
    //     audioBuffer = heap_caps_malloc(bufferSize, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    //     if (!audioBuffer)
    //         abort();

    //     audioBufferCount = snd.sample_count;

    //     printf("app_main: Created audio buffer (%d bytes).\n", bufferSize);
    // }

    uint32_t audioBuffer[SOUND_FREQUENCY / 60 * 4];

    // Process audio
    for (int x = 0; x < snd.sample_count; x++)
    {
        uint32_t sample;

        // if (muteFrameCount < 60 * 2)
        // {
        //     // When the emulator starts, audible poping is generated.
        //     // Audio should be disabled during this startup period.
        //     sample = 0;
        //     ++muteFrameCount;
        // }
        // else
        {
            sample = (snd.output[0][x] << 16) | snd.output[1][x];
            //sample = snd.output[0][x] << 2;
        }

        audioBuffer[x] = sample;
    }

    // send audio

    //odroid_audio_submit((short*)audioBuffer, snd.sample_count - 1);
    ProcessAudio((uint8_t*)audioBuffer, snd.sample_count - 1);
}

int main (int argc, char **argv)
{
    display = go2_display_create();
    presenter = go2_presenter_create(display, DRM_FORMAT_RGB565, 0xff080808);

    go2_surface_t* fbsurface = go2_surface_create(display, VIDEO_WIDTH, VIDEO_HEIGHT, DRM_FORMAT_RGB565);
    uint16_t* framebuffer = (uint16_t*)go2_surface_map(fbsurface); //malloc(VIDEO_WIDTH * VIDEO_HEIGHT * sizeof(uint16_t));
    if (!framebuffer) abort();

    int fbStride = go2_surface_stride_get(fbsurface) / sizeof(uint16_t);


    // Print help if no game specified
    if(argc < 2)
    {
        printf("USAGE: %s romfile\n", argv[0]);
        return 1;
    }


    InitSound();
    InitJoysticks();


    const char* romfile = argv[1];
    load_rom(romfile, argc > 2 ? argv[2] : NULL);

    sms.use_fm = 0;
    bitmap.width = VIDEO_WIDTH;
	bitmap.height = VIDEO_HEIGHT;
	bitmap.pitch = bitmap.width;
	//bitmap.depth = 8;
    bitmap.data = sms_framebuffer;

    // cart.pages = (cartSize / 0x4000);
    // cart.rom = romAddress;

    //system_init2(AUDIO_SAMPLE_RATE);
    set_option_defaults();

    option.sndrate = SOUND_FREQUENCY;
    option.overscan = 0;
    option.extra_gg = 0;

    system_init2();
    system_reset();



    const char* fileName = FileNameFromPath(romfile);
    
    char* saveName = (char*)malloc(strlen(fileName) + 4 + 1);
    strcpy(saveName, fileName);
    strcat(saveName, ".sav");


    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;

    char* savePath = PathCombine(homedir, saveName);
    printf("savePath='%s'\n", savePath);
    LoadState(savePath);




    int totalFrames = 0;
    double totalElapsed = 0.0;

    //Stopwatch_Reset();
    //Stopwatch_Start();

    const bool isGameGear = (sms.console == CONSOLE_GG) | (sms.console == CONSOLE_GGMS);

    while(isRunning)
    {
        ReadJoysticks();

        
        game_step();

        
        uint8_t* src = sms_framebuffer;
        uint16_t* dst = framebuffer;

        render_copy_palette(palette);
        for (int y = 0; y < VIDEO_HEIGHT; ++y)
        {
            for (int x = 0; x < VIDEO_WIDTH; ++x)
            {                
                uint8_t val = src[x] & PIXEL_MASK;
                dst[x] = palette[val];
            }

            src += bitmap.pitch;
            dst += fbStride;
        }


        if (isGameGear)
        {
            go2_presenter_post( presenter,
                                fbsurface,
                                48, 0, GAMEGEAR_WIDTH, GAMEGEAR_HEIGHT,
                                0, ((480 - 426) / 2), 320, 426,
                                GO2_ROTATION_DEGREES_270);   
        }
        else
        {
            go2_presenter_post( presenter,
                                fbsurface,
                                0, 0, VIDEO_WIDTH, VIDEO_HEIGHT,
                                0, ((480 - 426) / 2), 320, 426,
                                GO2_ROTATION_DEGREES_270);           
        }
        

#if 0
        ++totalFrames;


        // Measure FPS
        totalElapsed += Stopwatch_Elapsed();

        if (totalElapsed >= 1.0)
        {
            int fps = (int)(totalFrames / totalElapsed);
            fprintf(stderr, "FPS: %i\n", fps);

            totalFrames = 0;
            totalElapsed = 0;
        }

        Stopwatch_Reset();
#endif
    }

    SaveState(savePath);
    free(savePath);

    // Clean up
    go2_input_destroy(go2input);
    go2_audio_destroy(audio);
    go2_surface_destroy(fbsurface);
    go2_presenter_destroy(presenter);
    go2_display_destroy(display);

    return 0;
}
