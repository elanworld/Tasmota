#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#define ADDLOG(fmt, ...) AddLog(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)

#include <time.h>
#include <sys/time.h>
#include <Arduino.h>
#include "nvs_flash.h"
#include "nvs.h"

// Save interval in milliseconds (60 seconds)
constexpr uint32_t SAVE_INTERVAL = 60 * 1000;
uint32_t lastSaveMillis = 0;

void initTimerManager() {
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
            ADDLOG("RTC time restored: %ld", rtcTime);
            RtcSetTime(tv.tv_sec);
        } else if (lastSavedTime > 0) {
            struct timeval tv = { .tv_sec = lastSavedTime };
            settimeofday(&tv, nullptr);
            ADDLOG("Saved time restored: %ld", lastSavedTime);
            RtcSetTime(tv.tv_sec);
        }
        nvs_close(handle);
    } else {
        ADDLOG("Failed to open NVS for reading time");
    }

    lastSaveMillis = getMillis();
}

void loadInLoop() {
    if (getMillis() - lastSaveMillis >= SAVE_INTERVAL) {
        saveCurrentTime();
        lastSaveMillis = getMillis();
    }
}

void saveCurrentTime() {
    time_t now;
    time(&now);

    nvs_handle_t handle;
    if (nvs_open("timer", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_u32(handle, "saved_time", now);
        nvs_commit(handle);
        nvs_close(handle);
        ADDLOG("Current time saved: %ld", now);
    } else {
        ADDLOG("Failed to open NVS to save time");
    }
}

time_t getRTCTime() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec;
}

uint32_t getMillis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

#endif // TIMER_MANAGER_H
