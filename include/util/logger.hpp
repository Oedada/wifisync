#pragma once
#include "core/transport.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/format.h>
#include <cstddef>
#include <string>
#include <vector>

template <>
struct fmt::formatter<std::vector<std::byte>> {
    constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const std::vector<std::byte>& data,
                fmt::format_context& ctx) const {
        auto out = ctx.out();

        *out++ = '{';

        for (size_t i = 0; i < data.size(); ++i) {

            const auto byte = std::to_integer<unsigned char>(data[i]);
            // if(byte == 0){
                // continue;
            // }

            if (byte >= 0x20 && byte <= 0x7E) {
                *out++ = static_cast<char>(byte);
            } else{
                out = fmt::format_to(out, "0x{:02X}", byte);
                *out++ = ' ';
            }
        }

        *out++ = '}';
        return out;
    }
};

// template <> struct fmt::formatter<std::vector<std::byte>> {
    // constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }
// 
    // auto format(std::vector<std::byte> data,
                // fmt::format_context &ctx) const {
        // std::string str(reinterpret_cast<const char *>(data.data()),
                        // data.size());
// 
        // return fmt::format_to(ctx.out(), "{{{}}}", str);
    // }
// };

template <> struct fmt::formatter<RUnit> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(RUnit u, fmt::format_context &ctx) const {
        std::string s;
        s += "RUnit{";
        s += "Modify Type: " + modify_to_string[u.mt] + ", ";
        s += "Unit Type: " + unit_type_to_string[u.ut] + ", ";
        s += "Unit Path: " + u.path + ", ";
        if (u.mt != ModifyType::Deleted) {
            if (u.ut == UnitType::Directory) {
                s += "Count sub-units: " +
                     std::to_string(std::get<DirData>(u.data).subunits_number);
            }
        }
        s += "}";
        return fmt::format_to(ctx.out(), "{}", s);
    }
};

template <> struct fmt::formatter<SUnit> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(SUnit u, fmt::format_context &ctx) const {
        std::string s;
        s += "SUnit{";
        s += "Modify Type: " + modify_to_string[u.mt] + ", ";
        s += "Unit Type: " + unit_type_to_string[u.ut] + ", ";
        s += "Unit Name: " + u.name + ", ";
        if (u.mt != ModifyType::Deleted) {
            if (u.ut == UnitType::Directory) {
                s += "Count sub-units: " +
                     std::to_string(std::get<DirData>(u.data).subunits_number);
            }
        }
        s += "}";
        return fmt::format_to(ctx.out(), "{}", s);
    }
};
