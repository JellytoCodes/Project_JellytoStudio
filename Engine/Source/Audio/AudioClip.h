#pragma once
#include <FMOD/fmod.hpp>

struct AudioClip
{
    FMOD::Sound* sound = nullptr;
    bool         is3D = false;
    bool         isLoop = false;

    bool IsValid() const { return sound != nullptr; }

    void Release()
    {
        if (sound)
        {
            sound->release();
            sound = nullptr;
        }
    }
};
