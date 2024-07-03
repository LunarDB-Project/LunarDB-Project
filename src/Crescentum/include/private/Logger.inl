#pragma once

#include <iomanip>
#include <ranges>
#include <sstream>

#include "LunarDB/Crescentum/Logger.hpp"

namespace LunarDB::Crescentum::API {

template <typename... Args>
void Logger::log(ELogLevel level, ELunarModule module, Args&&... args)
{
    auto const now{std::chrono::system_clock::now()};
    auto const logging_time{now - m_log_start_time};
    auto const hms{std::chrono::hh_mm_ss{logging_time}};

    auto const get_log_level = [](auto const log_level) {
        auto const str = LogLevel::toString(log_level);
        auto color = ccolor::dark_gray;

        switch (log_level)
        {
        case ELogLevel::Info:
            color = ccolor::dark_aqua;
            break;
        case ELogLevel::Verbose:
            color = ccolor::reset;
            break;
        case ELogLevel::Warning:
            color = ccolor::gold;
            break;
        case ELogLevel::Error:
            color = ccolor::dark_red;
            break;
        case ELogLevel::Critical:
            color = ccolor::light_red;
            break;
        case ELogLevel::Debug:
            color = ccolor::yellow;
            break;
        default:
            break;
        }

        return std::make_pair(str, color);
    };

    auto const [lvl_str, lvl_color] = get_log_level(level);
    auto static constexpr c_sep_color = ccolor::dark_gray;

    std::ostringstream ss{};

    ss << c_sep_color << "[" << lvl_color << hms.hours().count() << ":" << hms.minutes().count()
       << ":" << hms.seconds().count() << c_sep_color << "] |";
    ss << lvl_color << std::right << std::setw(9) << lvl_str << c_sep_color << " | ";
    ss << lvl_color << std::right << std::setw(15) << LunarModule::toString(module) << c_sep_color
       << " | ";
    ss << lvl_color;
    auto dummy = ss.str();
    ((ss << std::forward<Args>(args) << " "), ...);
    // ss << '\n';

    m_log << ss.str() << std::endl;
    // m_log_queue.push(ss.str());
    // m_log_queue_size++;

    // static constexpr std::size_t c_max_log_queue_size{25};
    // static constexpr std::size_t c_max_log_queue_size{1};
    // if (m_log_queue_size >= c_max_log_queue_size)
    // {
    //     m_log_queue_size -= c_max_log_queue_size;
    //     for (auto const _ : std::ranges::iota_view{0u, c_max_log_queue_size})
    //     {
    //         m_log << m_log_queue.front();
    //         m_log_queue.pop();
    //     }
    // }
}

} // namespace LunarDB::Crescentum::API

/// [00:00:00.000005651] |    TRACE |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000043154] |    DEBUG |     Astral | msg1, msg2
/// [00:00:00.000048138] |  WARNING |     Astral | msg1, msg2
/// [00:00:00.000053103] |    ERROR |     Astral | msg1, msg2
/// [00:00:00.000058106] | CRITICAL |     Astral | msg1, msg2
/// [00:00:00.000063795] |    TRACE | BrightMoon | msg1, msg2
/// [00:00:00.000069119] |     INFO | BrightMoon | msg1, msg2
/// [00:00:00.000074069] |    DEBUG | BrightMoon | msg1, msg2
/// [00:00:00.000079301] |  WARNING | BrightMoon | msg1, msg2
/// [00:00:00.000084066] |    ERROR | BrightMoon | msg1, msg2
/// [00:00:00.000088808] | CRITICAL | BrightMoon | msg1, msg2

/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000058106] | CRITICAL |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000079301] |  WARNING | BrightMoon | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000053103] |    ERROR |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2
/// [00:00:00.000036238] |     INFO |     Astral | msg1, msg2