// --- SoundSystem.cpp ---
// This file contains the implementation of the sound system functions
// using the miniaudio library.

// Define SOUNDSYSTEM_EXPORTS before including SoundSystem.h in the DLL project
// so that functions are marked for export.
#define SOUNDSYSTEM_EXPORTS

#include "SoundSystem.h" // Include our own header for the API definition
#include <iostream>      // For logging to console
#include <map>           // To store and manage loaded sounds
#include <sstream>       // For building string messages for MessageBox
#include <algorithm>     // For std::clamp

// Include Windows API header for MessageBox if compiling on Windows
#ifdef _WIN32
#include <windows.h>
#endif

// Miniaudio header. IMPORTANT: Define MA_NO_DECODER_WAV, MA_NO_DECODER_MP3, etc.
// if you only want to support specific formats to reduce library size.
// For broad support, just include it as is.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Global miniaudio engine instance. This manages the audio device and playback.
static ma_engine g_engine;

// A map to store pointers to ma_sound objects, indexed by their string IDs.
// This allows us to manage multiple loaded and playing sounds.
static std::map<std::string, ma_sound*> g_loadedSounds;

extern "C" {

    SOUNDSYSTEM_API bool InitializeSoundSystem() {
        // Configure the miniaudio engine.
        ma_engine_config engineConfig = ma_engine_config_init();
        // You can customize the engine config here if needed, e.g., sample rate, channels.
        // For 3D audio, miniaudio automatically handles listener and sound properties.
        // No specific engine flags are needed for 3D init, it's handled by sound flags.

        // Initialize the miniaudio engine.
        // This will find and open the default audio device.
        ma_result result = ma_engine_init(&engineConfig, &g_engine);
        if (result != MA_SUCCESS) {
            std::ostringstream oss;
            oss << "SoundSystem ERROR: Failed to initialize miniaudio engine. Result: " << result;
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
            return false;
        }

        std::cout << "SoundSystem: Initialized successfully." << std::endl;
        return true;
    }

    SOUNDSYSTEM_API void ShutdownSoundSystem() {
        // Iterate through all loaded sounds and uninitialize them to free resources.
        for (auto const& [soundId, soundPtr] : g_loadedSounds) {
            if (soundPtr) {
                ma_sound_uninit(soundPtr); // Uninitialize the sound
                delete soundPtr;            // Free the dynamically allocated ma_sound object
            }
        }
        g_loadedSounds.clear(); // Clear the map

        // Uninitialize the miniaudio engine.
        ma_engine_uninit(&g_engine);
        std::cout << "SoundSystem: Shut down successfully." << std::endl;
    }

    SOUNDSYSTEM_API bool LoadSound(const char* filePath, const char* soundId) {
        if (!filePath || !soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: LoadSound received null filePath or soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: LoadSound received null filePath or soundId." << std::endl;
            return false;
        }
        std::string s_soundId = soundId;

        // Check if the sound ID already exists to prevent duplicates.
        if (g_loadedSounds.count(s_soundId)) {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Sound ID '" << s_soundId << "' already loaded. Ignoring.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
            return true; // Already loaded, consider it successful for idempotence
        }

        // Dynamically allocate a new ma_sound object.
        ma_sound* pSound = new (std::nothrow) ma_sound();
        if (!pSound) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: Failed to allocate memory for new sound.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: Failed to allocate memory for new sound." << std::endl;
            return false;
        }

        // Initialize the sound with flags for decoding. Pitch and 3D are handled by default
        // or set via their respective functions after initialization.
        ma_result result = ma_sound_init_from_file(&g_engine, filePath, MA_SOUND_FLAG_DECODE, NULL, NULL, pSound);
        if (result != MA_SUCCESS) {
            std::ostringstream oss;
            oss << "SoundSystem ERROR: Failed to load sound '" << filePath << "'. Result: " << result;
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
            delete pSound; // Clean up allocated memory on failure
            return false;
        }

        // Store the newly loaded sound in our map.
        g_loadedSounds[s_soundId] = pSound;
        std::cout << "SoundSystem: Loaded sound '" << filePath << "' as ID '" << s_soundId << "'." << std::endl;
        return true;
    }

    SOUNDSYSTEM_API void UnloadSound(const char* soundId) {
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: UnloadSound received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: UnloadSound received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            // Stop the sound if it's playing before uninitializing.
            if (ma_sound_is_playing(it->second)) {
                ma_sound_stop(it->second);
            }
            ma_sound_uninit(it->second); // Uninitialize the miniaudio sound object
            delete it->second;           // Free the dynamically allocated memory
            g_loadedSounds.erase(it);    // Remove from the map
            std::cout << "SoundSystem: Unloaded sound with ID '" << s_soundId << "'." << std::endl;
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to unload non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void SndPlaySound(const char* soundId, bool loop) { // Renamed from PlaySound
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: SndPlaySound received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: SndPlaySound received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            ma_sound* pSound = it->second;

            // Stop the sound if it's already playing before restarting,
            // to allow for re-triggering one-shot sounds or resetting loops.
            if (ma_sound_is_playing(pSound)) {
                ma_sound_stop(pSound);
                // Reset cursor to start for immediate replay
                ma_sound_seek_to_pcm_frame(pSound, 0);
            }

            ma_sound_set_looping(pSound, loop); // Set looping state
            ma_result result = ma_sound_start(pSound); // Start playing the sound
            if (result != MA_SUCCESS) {
                std::ostringstream oss;
                oss << "SoundSystem ERROR: Failed to play sound with ID '" << s_soundId << "'. Result: " << result;
#ifdef _WIN32
                MessageBoxA(NULL, oss.str().c_str(), "Sound System Error", MB_ICONERROR | MB_OK);
#endif
                std::cerr << oss.str() << std::endl;
            }
            else {
                std::cout << "SoundSystem: Playing sound ID '" << s_soundId << "' (Looping: " << (loop ? "Yes" : "No") << ")." << std::endl;
            }
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to play non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void StopSound(const char* soundId) {
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: StopSound received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: StopSound received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            ma_sound* pSound = it->second;
            if (ma_sound_is_playing(pSound)) {
                ma_result result = ma_sound_stop(pSound); // Stop the sound
                if (result != MA_SUCCESS) {
                    std::ostringstream oss;
                    oss << "SoundSystem ERROR: Failed to stop sound with ID '" << s_soundId << "'. Result: " << result;
#ifdef _WIN32
                    MessageBoxA(NULL, oss.str().c_str(), "Sound System Error", MB_ICONERROR | MB_OK);
#endif
                    std::cerr << oss.str() << std::endl;
                }
                else {
                    // Reset cursor to start when stopping, so it's ready for replay.
                    ma_sound_seek_to_pcm_frame(pSound, 0);
                    std::cout << "SoundSystem: Stopped sound ID '" << s_soundId << "'." << std::endl;
                }
            }
            else {
                std::cout << "SoundSystem: Sound ID '" << s_soundId << "' is not playing. No action needed." << std::endl;
            }
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to stop non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void PauseSound(const char* soundId) {
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: PauseSound received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: PauseSound received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            ma_sound* pSound = it->second;
            ma_result result = ma_sound_stop(pSound); // In miniaudio, stop and start are used for pause/resume as well.
            if (result != MA_SUCCESS) {
                std::ostringstream oss;
                oss << "SoundSystem ERROR: Failed to pause sound with ID '" << s_soundId << "'. Result: " << result;
#ifdef _WIN32
                MessageBoxA(NULL, oss.str().c_str(), "Sound System Error", MB_ICONERROR | MB_OK);
#endif
                std::cerr << oss.str() << std::endl;
            }
            else {
                std::cout << "SoundSystem: Paused sound ID '" << s_soundId << "'." << std::endl;
            }
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to pause non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void ResumeSound(const char* soundId) {
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: ResumeSound received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: ResumeSound received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            ma_sound* pSound = it->second;
            ma_result result = ma_sound_start(pSound);
            if (result != MA_SUCCESS) {
                std::ostringstream oss;
                oss << "SoundSystem ERROR: Failed to resume sound with ID '" << s_soundId << "'. Result: " << result;
#ifdef _WIN32
                MessageBoxA(NULL, oss.str().c_str(), "Sound System Error", MB_ICONERROR | MB_OK);
#endif
                std::cerr << oss.str() << std::endl;
            }
            else {
                std::cout << "SoundSystem: Resumed sound ID '" << s_soundId << "'." << std::endl;
            }
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to resume non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void SetMasterVolume(float volume) {
        // Clamp volume to be within 0.0 and 1.0
        volume = std::clamp(volume, 0.0f, 1.0f);

        // No need to capture return value, as ma_engine_set_volume returns void
        ma_engine_set_volume(&g_engine, volume);
        std::cout << "SoundSystem: Master volume set to " << volume << "." << std::endl;
    }

    SOUNDSYSTEM_API void SetSoundVolume(const char* soundId, float volume) {
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: SetSoundVolume received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: SetSoundVolume received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            ma_sound* pSound = it->second;
            // Clamp volume to be within 0.0 and 1.0
            volume = std::clamp(volume, 0.0f, 1.0f);
            // No need to capture return value, as ma_sound_set_volume returns void
            ma_sound_set_volume(pSound, volume);
            std::cout << "SoundSystem: Volume for sound ID '" << s_soundId << "' set to " << volume << "." << std::endl;
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to set volume for non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void SetSoundPan(const char* soundId, float pan) {
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: SetSoundPan received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: SetSoundPan received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            ma_sound* pSound = it->second;
            // Clamp pan to be within -1.0 and 1.0
            pan = std::clamp(pan, -1.0f, 1.0f);
            // No need to capture return value, as ma_sound_set_pan returns void
            ma_sound_set_pan(pSound, pan);
            std::cout << "SoundSystem: Pan for sound ID '" << s_soundId << "' set to " << pan << "." << std::endl;
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to set pan for non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void SetSoundPitch(const char* soundId, float pitch) {
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: SetSoundPitch received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: SetSoundPitch received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            ma_sound* pSound = it->second;
            // Pitch should generally be positive. If 0 or negative, miniaudio might behave unexpectedly.
            if (pitch <= 0.0f) pitch = 0.001f; // Ensure a small positive value to avoid issues
            // No need to capture return value, as ma_sound_set_pitch returns void
            ma_sound_set_pitch(pSound, pitch);
            std::cout << "SoundSystem: Pitch for sound ID '" << s_soundId << "' set to " << pitch << "." << std::endl;
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to set pitch for non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void SetSoundPosition(const char* soundId, float x, float y, float z) {
        if (!soundId) {
#ifdef _WIN32
            MessageBoxA(NULL, "SoundSystem ERROR: SetSoundPosition received null soundId.", "Sound System Error", MB_ICONERROR | MB_OK);
#endif
            std::cerr << "SoundSystem ERROR: SetSoundPosition received null soundId." << std::endl;
            return;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            ma_sound* pSound = it->second;
            // No need to capture return value, as ma_sound_set_position returns void
            ma_sound_set_position(pSound, x, y, z);
            std::cout << "SoundSystem: Position for sound ID '" << s_soundId << "' set to (" << x << ", " << y << ", " << z << ")." << std::endl;
        }
        else {
            std::ostringstream oss;
            oss << "SoundSystem WARNING: Attempted to set position for non-existent sound ID '" << s_soundId << "'.";
#ifdef _WIN32
            MessageBoxA(NULL, oss.str().c_str(), "Sound System Warning", MB_ICONWARNING | MB_OK);
#endif
            std::cerr << oss.str() << std::endl;
        }
    }

    SOUNDSYSTEM_API void SetListenerPosition(float x, float y, float z) {
        // No need to capture return value, as ma_engine_listener_set_position returns void
        ma_engine_listener_set_position(&g_engine, 0, x, y, z); // Listener 0 is the default
        std::cout << "SoundSystem: Listener position set to (" << x << ", " << y << ", " << z << ")." << std::endl;
    }

    SOUNDSYSTEM_API void SetListenerOrientation(float forwardX, float forwardY, float forwardZ) { // Simplified signature
        // The ma_engine_listener_set_direction function (with 4 arguments) sets the "at" (forward) vector.
        // If your miniaudio.h does not define ma_engine_listener_set_up,
        // then the up vector is either implicitly handled or not directly settable via an API.
        ma_engine_listener_set_direction(&g_engine, 0, forwardX, forwardY, forwardZ);

        std::cout << "SoundSystem: Listener orientation set (Forward: (" << forwardX << ", " << forwardY << ", " << forwardZ << "))." << std::endl;
    }

    SOUNDSYSTEM_API bool IsSoundPlaying(const char* soundId) {
        if (!soundId) {
            return false;
        }
        std::string s_soundId = soundId;

        auto it = g_loadedSounds.find(s_soundId);
        if (it != g_loadedSounds.end()) {
            return ma_sound_is_playing(it->second);
        }
        return false;
    }

} // extern "C"
