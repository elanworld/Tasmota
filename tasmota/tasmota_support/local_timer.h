#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <time.h>
#include <sys/time.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

class TimerManager {
public:
    static void init() {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_flash_init();
        }

        nvs_handle_t handle;
        if (nvs_open("timer", NVS_READWRITE, &handle) == ESP_OK) {
            uint32_t lastSavedTime = 0;
            nvs_get_u32(handle, "saved_time", &lastSavedTime);

            time_t rtcTime = getRTCTime();

            if (rtcTime > 100000) {
                struct timeval tv = { .tv_sec = rtcTime };
                settimeofday(&tv, nullptr);
                ESP_LOGI("TimerManager", "使用 RTC 时间初始化: %ld", rtcTime);
                AddLog(LOG_LEVEL_INFO, PSTR("iBeacon register for advert callbacks"));
            }
            else if (lastSavedTime > 0) {
                struct timeval tv = { .tv_sec = lastSavedTime };
                settimeofday(&tv, nullptr);
                ESP_LOGI("TimerManager", "使用存储时间初始化: %ld", lastSavedTime);
            }
            nvs_close(handle);
        }

        lastSaveMillis = getMillis();
    }

    static void loadInLoop() {
        if (getMillis() - lastSaveMillis >= saveInterval) {
            saveCurrentTime();
            lastSaveMillis = getMillis();
        }
    }

private:
    static constexpr uint32_t saveInterval = 60 * 1000; // 每分钟保存一次
    static uint32_t lastSaveMillis;

    static void saveCurrentTime() {
        time_t now;
        time(&now);

        nvs_handle_t handle;
        if (nvs_open("timer", NVS_READWRITE, &handle) == ESP_OK) {
            nvs_set_u32(handle, "saved_time", now);
            nvs_commit(handle);
            nvs_close(handle);
            ESP_LOGI("TimerManager", "保存时间: %ld", now);
        }
    }

    static time_t getRTCTime() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec;
    }

    static uint32_t getMillis() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    }
};

// 静态成员初始化
uint32_t TimerManager::lastSaveMillis = 0;

#endif // TIMER_MANAGER_H
