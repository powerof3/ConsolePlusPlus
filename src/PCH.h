#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <ClibUtil/simpleINI.hpp>
#include <ClibUtil/string.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

namespace logger = SKSE::log;
namespace string = clib_util::string;
namespace ini = clib_util::ini;

using namespace std::literals;

namespace stl
{
	using namespace SKSE::stl;
}

using Key = RE::BSKeyboardDevice::Keys::Key;

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
