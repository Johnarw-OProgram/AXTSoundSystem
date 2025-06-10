// --- SoundSystem.h ---
// This file defines the public interface of the sound system DLL.
// It declares the functions that will be exported and accessible
// from other applications (like game engines).

#ifndef SOUNDSYSTEM_H
#define SOUNDSYSTEM_H

#include <string>

// On Windows, these macros are used to correctly export and import
// functions from a DLL.
#ifdef _WIN32
#ifdef SOUNDSYSTEM_EXPORTS
    // If compiling this DLL, export the functions
#define SOUNDSYSTEM_API __declspec(dllexport)
#else
    // If linking to this DLL, import the functions
#define SOUNDSYSTEM_API __declspec(dllimport)
#endif
#else
    // For other platforms (Linux/macOS), use GCC's visibility attributes
#define SOUNDSYSTEM_API __attribute__((visibility("default")))
#endif

// We use 'extern "C"' to prevent C++ name mangling, ensuring that
// the function names are easily callable from other languages or C code.
extern "C" {

    /**
     * @brief Initializes the sound engine.
     * @return True if initialization was successful, false otherwise.
     */
    SOUNDSYSTEM_API bool InitializeSoundSystem();

    /**
     * @brief Deinitializes the sound engine and cleans up resources.
     */
    SOUNDSYSTEM_API void ShutdownSoundSystem();

    /**
     * @brief Loads an audio file into memory.
     * @param filePath The path to the audio file.
     * @param soundId A unique ID to refer to this sound later (e.g., "explosion_sound").
     * @return True if the sound was loaded successfully, false otherwise.
     */
    SOUNDSYSTEM_API bool LoadSound(const char* filePath, const char* soundId);

    /**
     * @brief Unloads a sound from memory.
     * @param soundId The unique ID of the sound to unload.
     */
    SOUNDSYSTEM_API void UnloadSound(const char* soundId);

    /**
     * @brief Plays a loaded sound.
     * @param soundId The unique ID of the sound to play.
     * @param loop If true, the sound will loop indefinitely.
     */
    SOUNDSYSTEM_API void SndPlaySound(const char* soundId, bool loop);

    /**
     * @brief Stops a currently playing sound.
     * @param soundId The unique ID of the sound to stop.
     */
    SOUNDSYSTEM_API void StopSound(const char* soundId);

    /**
     * @brief Pauses a currently playing sound.
     * @param soundId The unique ID of the sound to pause.
     */
    SOUNDSYSTEM_API void PauseSound(const char* soundId);

    /**
     * @brief Resumes a paused sound.
     * @param soundId The unique ID of the sound to resume.
     */
    SOUNDSYSTEM_API void ResumeSound(const char* soundId);

    /**
     * @brief Sets the master volume for all sounds.
     * @param volume A float value between 0.0 (mute) and 1.0 (full volume).
     */
    SOUNDSYSTEM_API void SetMasterVolume(float volume);

    /**
     * @brief Sets the volume for a specific loaded sound.
     * @param soundId The unique ID of the sound.
     * @param volume A float value between 0.0 (mute) and 1.0 (full volume).
     */
    SOUNDSYSTEM_API void SetSoundVolume(const char* soundId, float volume);

    /**
     * @brief Sets the panning for a specific loaded sound.
     * @param soundId The unique ID of the sound.
     * @param pan A float value between -1.0 (full left) and 1.0 (full right), 0.0 for center.
     */
    SOUNDSYSTEM_API void SetSoundPan(const char* soundId, float pan);

    /**
     * @brief Sets the pitch for a specific loaded sound.
     * @param soundId The unique ID of the sound.
     * @param pitch A float value where 1.0 is normal pitch, >1.0 is higher, <1.0 is lower.
     */
    SOUNDSYSTEM_API void SetSoundPitch(const char* soundId, float pitch);

    /**
     * @brief Sets the 3D position of a specific loaded sound.
     * @param soundId The unique ID of the sound.
     * @param x X-coordinate.
     * @param y Y-coordinate.
     * @param z Z-coordinate.
     */
    SOUNDSYSTEM_API void SetSoundPosition(const char* soundId, float x, float y, float z);

    /**
     * @brief Sets the 3D position of the audio listener.
     * @param x X-coordinate.
     * @param y Y-coordinate.
     * @param z Z-coordinate.
     */
    SOUNDSYSTEM_API void SetListenerPosition(float x, float y, float z);

    /**
     * @brief Sets the 3D orientation of the audio listener (forward vector only).
     * @param forwardX X-component of the forward vector.
     * @param forwardY Y-component of the forward vector.
     * @param forwardZ Z-component of the forward vector.
     */
    SOUNDSYSTEM_API void SetListenerOrientation(float forwardX, float forwardY, float forwardZ); // Simplified signature

    /**
     * @brief Checks if a sound is currently playing.
     * @param soundId The unique ID of the sound to check.
     * @return True if the sound is playing, false otherwise.
     */
    SOUNDSYSTEM_API bool IsSoundPlaying(const char* soundId);
}

#endif // SOUNDSYSTEM_H
