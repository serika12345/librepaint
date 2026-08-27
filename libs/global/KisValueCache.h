/*
 *  SPDX-FileCopyrightText: 2023 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISVALUECACHE_H
#define KISVALUECACHE_H

#include <optional>
#include <utility>

/**
 * A simple class for cache-like structures. It should be parametrized
 * with a policy-class that has `value_type initialize()` method, which
 * would be used for initializing the cache.
 */
template<typename Initializer>
struct KisValueCache : Initializer {
    using value_type = decltype(std::declval<Initializer>().initialize());

    template<typename... Args>
    KisValueCache(Args... args)
        : Initializer(args...)
    {
    }

    const value_type &value() const
    {
        if (!m_value) {
            Initializer &initializer = const_cast<Initializer &>(static_cast<const Initializer &>(*this));
            m_value = initializer.initialize();
        }

        return *m_value;
    }

    operator const value_type &() const
    {
        return value();
    }

    bool isValid() const
    {
        return bool(m_value);
    }

    void clear()
    {
        m_value = std::nullopt;
    }

private:
    mutable std::optional<value_type> m_value;
};

#endif // KISVALUECACHE_H
