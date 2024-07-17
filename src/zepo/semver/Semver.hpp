//
// Created by qingy on 2024/7/17.
//

#pragma once
#ifndef ZEPO_SEMVER_HPP
#define ZEPO_SEMVER_HPP

#include <optional>
#include <semver.h>
#include <string_view>

namespace zepo::semver {
    class Version {
        std::optional<semver_t> version_{};

        void checkInitStatus() const;

    public:
        explicit Version(std::string_view view);

        Version(const Version&) = delete;

        Version(Version&&) = default;

        Version& operator=(const Version&) = delete;

        Version& operator=(Version&&) = default;

        ~Version();

        [[nodiscard]] int getMajor() const;

        [[nodiscard]] int getMinor() const;

        [[nodiscard]] int getPatch() const;

        bool operator>(const Version& r) const;

        bool operator>=(const Version& r) const;

        bool operator<(const Version& r) const;

        bool operator<=(const Version& r) const;

        bool operator==(const Version& r) const;

        bool operator^(const Version& r) const;

        bool operator|(const Version& r) const;
    };
}

#endif //ZEPO_SEMVER_HPP
