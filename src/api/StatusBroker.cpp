#include "api/StatusBroker.h"

namespace api {

void StatusBroker::begin() {
    if (mutex_ == nullptr) {
        mutex_ = xSemaphoreCreateMutex();
    }
}

void StatusBroker::publish(const StatusSnapshot& snap) {
    if (mutex_ == nullptr) {
        return;
    }
    if (xSemaphoreTake(mutex_, 0) == pdTRUE) {
        snap_ = snap;
        xSemaphoreGive(mutex_);
    }
}

bool StatusBroker::read(StatusSnapshot& out) {
    if (mutex_ == nullptr) {
        return false;
    }
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) != pdTRUE) {
        return false;
    }
    out = snap_;
    xSemaphoreGive(mutex_);
    return true;
}

}  // namespace api
