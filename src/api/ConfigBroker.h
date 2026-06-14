#ifndef API_CONFIG_BROKER_H
#define API_CONFIG_BROKER_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "storage/PersistentConfig.h"

/**
 * @file ConfigBroker.h
 * @brief Thread-safe hand-off of config changes (AD-1 apply-queue).
 *
 * The networking task stages a fully-validated config; the control loop
 * consumes it on its next tick and applies+persists it. A single mutex-guarded
 * swap is the ONLY shared mutable state between the two contexts, avoiding
 * scattered locking and any partial-update races (NFR-1/NFR-8).
 */
namespace api {

class ConfigBroker {
  public:
    /// Create the mutex. Call once before either task uses the broker.
    void begin();

    /// Stage a new config (networking task). Returns false if locking failed.
    bool stage(const storage::PersistentConfig& cfg);

    /**
     * @brief Consume a staged config if one is pending (control loop).
     * @param out  receives the staged config when true is returned.
     * @return true if a pending config was copied out (caller must apply+save).
     */
    bool consume(storage::PersistentConfig& out);

  private:
    SemaphoreHandle_t mutex_ = nullptr;
    storage::PersistentConfig staged_;
    bool pending_ = false;
};

}  // namespace api

#endif  // API_CONFIG_BROKER_H
