#include <fmod.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

#include "Container/Array.h"

class FSoundManager {
public:
    static FSoundManager& GetInstance() {
        static FSoundManager instance;
        return instance;
    }

    bool Initialize() {
        FMOD_RESULT result = FMOD::System_Create(&system);
        if (result != FMOD_OK) {
            std::cerr << "FMOD System_Create failed!" << std::endl;
            return false;
        }

        result = system->init(1024, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
            std::cerr << "FMOD system init failed!" << std::endl;
            return false;
        }

        return true;
    }

    void Shutdown() {
        for (auto& pair : soundMap) {
            pair.second->release();
        }
        soundMap.clear();

        if (system) {
            system->close();
            system->release();
        }
    }

    bool LoadSound(const std::string& name, const std::string& filePath, bool loop = false) {
        if (soundMap.find(name) != soundMap.end()) {
            return true;
        }

        FMOD::Sound* sound = nullptr;
        FMOD_MODE mode = FMOD_DEFAULT | FMOD_LOOP_OFF | FMOD_CREATECOMPRESSEDSAMPLE;

        if (system->createSound(filePath.c_str(), mode, nullptr, &sound) != FMOD_OK) {
            std::cerr << "Failed to load sound: " << filePath << std::endl;
            return false;
        }
        soundMap[name] = sound;
        return true;
    }

    FMOD::Channel* PlaySound(const std::string& name) {
        auto it = soundMap.find(name);
        if (it != soundMap.end()) {
            FMOD::Channel* newChannel = nullptr;
            system->playSound(it->second, nullptr, false, &newChannel);
            if (newChannel) {
                activeChannels.push_back(newChannel);
            }
            return newChannel;
        }
    }

    void UpdateVolume(FMOD::Channel* inChannel, float volume)
    {
        inChannel->setVolume(0.7);
    }

    void Update() {
        system->update();
        activeChannels.erase(
    std::remove_if(activeChannels.begin(), activeChannels.end(),
        [](FMOD::Channel* channel) {
            bool isPlaying = false;
            if (channel) {
                FMOD_RESULT result = channel->isPlaying(&isPlaying);
                if (result != FMOD_OK) {
                    return true; 
                }
            }
            return !isPlaying;
        }),
    activeChannels.end()
);

    }

    void StopAllSounds()
    {
        // 1) 개별 채널을 멈추기
        for (FMOD::Channel* channel : activeChannels)
        {
            if (channel)
            {
                channel->stop();
            }
        }
        activeChannels.clear();

        // 2) 시스템 전체(마스터 채널 그룹) 정지 (Optional)
        FMOD::ChannelGroup* masterGroup = nullptr;
        if (system->getMasterChannelGroup(&masterGroup) == FMOD_OK && masterGroup)
        {
            masterGroup->stop();
        }
    }


    TArray<std::string> GetAllSoundNames() const {
        TArray<std::string> names;
        for (const auto& pair : soundMap)
        {
            names.Add(pair.first);
        }
        return names;
    }

    
private:
    FSoundManager() : system(nullptr) {}
    ~FSoundManager() { Shutdown(); }
    FSoundManager(const FSoundManager&) = delete;
    FSoundManager& operator=(const FSoundManager&) = delete;

    FMOD::System* system;
    std::unordered_map<std::string, FMOD::Sound*> soundMap;
    std::vector<FMOD::Channel*> activeChannels; // ���� ä���� ����
};
