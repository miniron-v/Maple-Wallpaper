#pragma once

#include <optional>

#include "platform/native_types.hpp"

namespace maple::platform {

std::optional<NativeWindowHandle> find_workerw();
bool embed_window(NativeWindowHandle child, NativeWindowHandle workerw);
void detach_window(NativeWindowHandle child);

} // namespace maple::platform
