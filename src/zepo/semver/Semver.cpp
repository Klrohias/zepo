//
// Created by qingy on 2024/7/17.
//


#include "Semver.hpp"

#include <stdexcept>
#include <string>

namespace zepo::semver {
    using namespace std::string_literals;

    void Version::checkInitStatus() const {
        if (!version_.has_value()) {
            throw std::runtime_error("the version has not been initialized");
        }
    }

    Version::Version(const std::string_view view) : version_{semver_t{}} {
        const std::string slicedVersion{view};
        if (semver_parse(slicedVersion.data(), &version_.value())) {
            throw std::runtime_error("failed to parse version \""s + view.data() + "\"");
        }
    }

    Version::~Version() {
        if (version_.has_value()) { semver_free(&version_.value()); }
    }

    int Version::getMajor() const {
        checkInitStatus();
        return version_.value().major;
    }

    int Version::getMinor() const {
        checkInitStatus();
        return version_.value().minor;
    }

    int Version::getPatch() const {
        checkInitStatus();
        return version_.value().patch;
    }

    bool Version::operator>(const Version& r) const {
        checkInitStatus();
        r.checkInitStatus();
        return semver_gt(version_.value(), r.version_.value());
    }

    bool Version::operator>=(const Version& r) const {
        checkInitStatus();
        r.checkInitStatus();
        return semver_gte(version_.value(), r.version_.value());
    }

    bool Version::operator<(const Version& r) const {
        checkInitStatus();
        r.checkInitStatus();
        return semver_lt(version_.value(), r.version_.value());
    }

    bool Version::operator<=(const Version& r) const {
        checkInitStatus();
        r.checkInitStatus();
        return semver_lte(version_.value(), r.version_.value());
    }

    bool Version::operator==(const Version& r) const {
        checkInitStatus();
        r.checkInitStatus();
        return semver_eq(version_.value(), r.version_.value());
    }

    bool Version::operator^(const Version& r) const {
        checkInitStatus();
        r.checkInitStatus();
        return semver_satisfies_caret(version_.value(), r.version_.value());
    }

    bool Version::operator|(const Version& r) const {
        checkInitStatus();
        r.checkInitStatus();
        return semver_satisfies_patch(version_.value(), r.version_.value());
    }
}
