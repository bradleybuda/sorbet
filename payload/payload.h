#ifndef RUBY_TYPER_PAYLOAD_H
#define RUBY_TYPER_PAYLOAD_H
#include "common/kvstore/KeyValueStore.h"
#include "core/GlobalState.h"
#include "main/options/options.h"
#include "spdlog/spdlog.h"

namespace sorbet::payload {

void createInitialGlobalState(std::unique_ptr<core::GlobalState> &gs, const realmain::options::Options &options,
                              const std::unique_ptr<OwnedKeyValueStore> &kvstore);

/** Writes the GlobalState to kvstore, but only if it was modified. Returns 'true' if a write happens. */
bool retainGlobalState(std::unique_ptr<core::GlobalState> &gs, const realmain::options::Options &options,
                       const std::unique_ptr<OwnedKeyValueStore> &kvstore);

} // namespace sorbet::payload
#endif // RUBY_TYPER_PAYLOAD_H
