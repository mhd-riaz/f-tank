#include "api/ConfigBroker.h"

namespace api {

void ConfigBroker::begin() {
    if (mutex_ == nullptr) {
        mutex_ = xSemaphoreCreateMutex();
    }
}

bool ConfigBroker::stage(const storage::PersistentConfig& cfg) {
    if (mutex_ == nullptr) {
        return false;
    }
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    staged_ = cfg;
    pending_ = true;
    xSemaphoreGive(mutex_);
    return true;
}

bool ConfigBroker::consume(storage::PersistentConfig& out) {
    if (mutex_ == nullptr) {
        return false;
    }
    // Non-blocking from the control loop: skip this tick if the lock is held.
    if (xSemaphoreTake(mutex_, 0) != pdTRUE) {
        return false;
    }
    bool had = false;
    if (pending_) {
        out = staged_;
        pending_ = false;
        had = true;
    }
    xSemaphoreGive(mutex_);
    return had;
}

}  // namespace api
