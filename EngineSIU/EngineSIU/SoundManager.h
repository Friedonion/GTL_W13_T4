#pragma once
#include <fmod.hpp>
#include <fmod_errors.h> // FMOD_ErrorString 사용을 위해 추가
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm> // std::remove_if 사용을 위해 추가

#include "Container/Array.h"

class FSoundManager 
{
public:
    static FSoundManager& GetInstance() 
    {
        static FSoundManager instance;
        return instance;
    }

    bool Initialize(int maxChannels = 1024, FMOD_INITFLAGS flags = FMOD_INIT_NORMAL, void* extraDriverData = nullptr) {
        FMOD_RESULT result = FMOD::System_Create(&system);
        if (result != FMOD_OK) {
            std::cerr << "FMOD System_Create failed! Error: " << FMOD_ErrorString(result) << std::endl;
            return false;
        }

        // 3D 사운드 기본 설정 (선택 사항, 예: 도플러 효과, 거리 계수 등)
        result = system->set3DSettings(1.0f, 1.0f, 1.0f); // dopplerscale, distancefactor, rolloffscale
        if (result != FMOD_OK) {
            std::cerr << "FMOD system set3DSettings failed! Error: " << FMOD_ErrorString(result) << std::endl;
        }

        result = system->init(maxChannels, flags, extraDriverData);
        if (result != FMOD_OK) {
            std::cerr << "FMOD system init failed! Error: " << FMOD_ErrorString(result) << std::endl;
            system->release(); // 시스템 생성 후 init 실패 시 release
            system = nullptr;
            return false;
        }

        // 리스너 초기화 (0번 리스너, 기본 위치 및 방향)
        FMOD_VECTOR pos = { 0.0f, 0.0f, 0.0f };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
        FMOD_VECTOR forward = { 1.0f, 0.0f, 0.0f }; // X축을 정면으로 가정
        FMOD_VECTOR up = { 0.0f, 0.0f, 1.0f };   // Z축을 위로 가정
        UpdateListenerAttributes(0, pos, vel, forward, up); // 내부에서 system->set3DListenerAttributes 호출

        return true;
    }

    void Shutdown() {
        StopAllSounds(); // 모든 활성 채널 정지

        for (auto& pair : soundMap) {
            if (pair.second) {
                pair.second->release();
            }
        }
        soundMap.clear();

        if (system) {
            system->close();
            system->release();
            system = nullptr;
        }
    }

    // is3D, minDistance, maxDistance 파라미터 추가
    bool LoadSound(const std::string& name, const std::string& filePath, bool loop = false, bool is3D = false, float minDistance = 1.0f, float maxDistance = 1000.0f) {
        if (soundMap.count(name)) { // find 대신 count 사용 가능 (C++11 이상)
            // 이미 로드된 사운드에 대한 처리 정책 (예: 경고 후 true 반환)
            std::cout << "Sound '" << name << "' already loaded." << std::endl;
            return true;
        }

        FMOD::Sound* sound = nullptr;
        FMOD_MODE mode = FMOD_CREATECOMPRESSEDSAMPLE; // 기본값

        if (loop) {
            mode |= FMOD_LOOP_NORMAL;
        }
        else {
            mode |= FMOD_LOOP_OFF;
        }

        if (is3D) {
            mode |= FMOD_3D;
            mode |= FMOD_3D_WORLDRELATIVE; // 일반적인 3D 사운드에 적합
            // FMOD_3D_IGNOREGEOMETRY, FMOD_3D_INVERSEROLLOFF (기본값), FMOD_3D_LINEARROLLOFF 등 다양한 롤오프 모델 설정 가능
        }
        else {
            mode |= FMOD_2D;
        }

        FMOD_RESULT result = system->createSound(filePath.c_str(), mode, nullptr, &sound);
        if (result != FMOD_OK) {
            std::cerr << "Failed to load sound: " << filePath << " Error: " << FMOD_ErrorString(result) << std::endl;
            return false;
        }

        if (is3D && sound) {
            result = sound->set3DMinMaxDistance(minDistance, maxDistance);
            if (result != FMOD_OK) {
                std::cerr << "Failed to set 3D min/max distance for sound: " << name << " Error: " << FMOD_ErrorString(result) << std::endl;
                // 로드는 성공했으므로 계속 진행하거나, 여기서 false를 반환할 수 있습니다.
            }
        }

        soundMap[name] = sound;
        return true;
    }

    // 기존 PlaySound는 2D 사운드 재생용으로 명확히 하거나 수정
    FMOD::Channel* PlaySound2D(const std::string& name, bool paused = false) {
        auto it = soundMap.find(name);
        if (it != soundMap.end()) {
            FMOD::Channel* newChannel = nullptr;
            // 사운드가 2D로 로드되었는지 확인하는 로직 추가 가능 (getMode)
            FMOD_RESULT result = system->playSound(it->second, nullptr, paused, &newChannel);
            if (result == FMOD_OK && newChannel) {
                activeChannels.push_back(newChannel);
                return newChannel;
            }
            else if (result != FMOD_OK) {
                std::cerr << "Failed to play 2D sound: " << name << " Error: " << FMOD_ErrorString(result) << std::endl;
            }
        }
        else {
            std::cerr << "Sound not found for 2D playback: " << name << std::endl;
        }
        return nullptr;
    }

    // PlaySoundAtLocation 함수
    FMOD::Channel* PlaySoundAtLocation(const std::string& name, const FMOD_VECTOR& position, const FMOD_VECTOR& velocity = { 0.0f, 0.0f, 0.0f }, bool startPaused = false) {
        auto it = soundMap.find(name);
        if (it == soundMap.end()) {
            std::cerr << "Sound not found for 3D playback: " << name << std::endl;
            return nullptr;
        }

        FMOD::Sound* soundToPlay = it->second;
        FMOD_MODE soundMode;
        soundToPlay->getMode(&soundMode);

        if (!(soundMode & FMOD_3D)) {
            // 3D로 로드되지 않은 사운드에 대한 처리 (경고, 2D로 재생, 또는 에러)
            std::cerr << "Warning: Sound '" << name << "' was not loaded as a 3D sound. Playing at listener's location or as 2D." << std::endl;
            return PlaySound2D(name, startPaused); // 혹은 다른 처리
        }

        FMOD::Channel* channel = nullptr;
        // 중요: paused를 true로 설정하여 속성 설정 후 재생 시작
        FMOD_RESULT result = system->playSound(soundToPlay, nullptr, true, &channel);
        if (result != FMOD_OK || !channel) {
            std::cerr << "Failed to play sound (playSound step): " << name << " for 3D. Error: " << FMOD_ErrorString(result) << std::endl;
            return nullptr;
        }

        result = channel->set3DAttributes(&position, &velocity);
        if (result != FMOD_OK) {
            std::cerr << "Failed to set 3D attributes for channel. Error: " << FMOD_ErrorString(result) << std::endl;
            channel->stop(); // 속성 설정 실패 시 채널 정지
            // activeChannels에는 아직 추가되지 않았으므로 여기서 제거할 필요는 없음
            return nullptr;
        }

        // 채널별로 min/max distance 오버라이드 가능
        float channelMinDist = 0.5f, channelMaxDist = 500.0f;
        channel->set3DMinMaxDistance(channelMinDist, channelMaxDist);

        if (!startPaused) {
            result = channel->setPaused(false); // 실제 재생 시작
            if (result != FMOD_OK) {
                std::cerr << "Failed to unpause channel. Error: " << FMOD_ErrorString(result) << std::endl;
                channel->stop();
                return nullptr;
            }
        }

        activeChannels.push_back(channel);
        return channel;
    }

    // 리스너 속성 업데이트 함수 (리스너 ID는 보통 0)
    void UpdateListenerAttributes(int listenerId, const FMOD_VECTOR& pos, const FMOD_VECTOR& vel, const FMOD_VECTOR& forward, const FMOD_VECTOR& up) {
        if (system) {
            FMOD_RESULT result = system->set3DListenerAttributes(listenerId, &pos, &vel, &forward, &up);
            if (result != FMOD_OK) {
                std::cerr << "Failed to set 3D listener attributes. Error: " << FMOD_ErrorString(result) << std::endl;
            }
        }
    }


    void Update() {
        if (system) {
            system->update();
        }

        // 활성 채널 정리 (기존 로직 개선)
        activeChannels.erase(
            std::remove_if(activeChannels.begin(), activeChannels.end(),
                [](FMOD::Channel* channel) {
                    if (!channel) return true; // nullptr 채널 제거

                    bool isPlaying = false;
                    FMOD_RESULT result = channel->isPlaying(&isPlaying);

                    // 채널이 유효하지 않거나 (이미 중지/릴리즈됨), 도난당했거나, 재생 중이 아니면 제거
                    if (result == FMOD_ERR_INVALID_HANDLE || result == FMOD_ERR_CHANNEL_STOLEN || (result == FMOD_OK && !isPlaying)) {
                        return true;
                    }
                    // 그 외의 에러는 로그를 남길 수 있음 (선택 사항)
                    if (result != FMOD_OK) {
                        std::cerr << "Error checking channel state: " << FMOD_ErrorString(result) << std::endl;
                    }
                    return false; // 그 외의 경우는 유지
                }),
            activeChannels.end()
        );
    }

    void StopAllSounds() {
        for (FMOD::Channel* channel : activeChannels) {
            if (channel) {
                channel->stop();
            }
        }
        activeChannels.clear();

        // 마스터 채널 그룹을 정지하는 것은 모든 소리를 즉시 멈추지만,
        // 개별 채널의 페이드 아웃 등을 무시할 수 있으므로 주의해서 사용합니다.
        FMOD::ChannelGroup* masterGroup = nullptr;
        if (system && system->getMasterChannelGroup(&masterGroup) == FMOD_OK && masterGroup) {
            masterGroup->stop();
        }
    }

    TArray<std::string> GetAllSoundNames() const {
        TArray<std::string> names;
        for (const auto& pair : soundMap) {
            names.Add(pair.first);
        }
        return names;
    }

    // FMOD 시스템 객체에 직접 접근해야 할 경우 (고급 사용)
    FMOD::System* GetFMODSystem() const {
        return system;
    }

private:
    FSoundManager() : system(nullptr) {}
    ~FSoundManager() {
        Shutdown(); // 소멸자에서 Shutdown 호출은 유지
    }
    FSoundManager(const FSoundManager&) = delete;
    FSoundManager& operator=(const FSoundManager&) = delete;

    FMOD::System* system;
    std::unordered_map<std::string, FMOD::Sound*> soundMap;
    std::vector<FMOD::Channel*> activeChannels;
};
