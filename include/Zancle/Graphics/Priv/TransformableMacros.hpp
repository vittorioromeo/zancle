#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Angle.hpp"         // IWYU pragma: keep
#include "Zancle/Geometry/AutoWrapAngle.hpp" // IWYU pragma: keep
#include "Zancle/Geometry/Priv/Vec2Base.hpp" // IWYU pragma: keep


////////////////////////////////////////////////////////////
/// \brief Define the data members of a transformable object's settings
///
////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_TRANSFORMABLE                                     \
    ::za::Vec2f position{};      /*!< Position of the object in the 2D world */                \
    ::za::Vec2f scale{1.f, 1.f}; /*!< Scale of the object */                                   \
    ::za::Vec2f origin{};        /*!< Origin of translation/rotation/scaling of the object */  \
    /* Not wrapped: when derived from ever-growing values (e.g. elapsed time), keep it bounded \
       (e.g. via `wrapUnsigned()`), as precision degrades with magnitude */                    \
    /* NOLINTNEXTLINE(readability-redundant-member-init) */                                    \
    ::za::Angle rotation{}; /*!< Orientation of the object */                                  \
                                                                                               \
    static_assert(true)


////////////////////////////////////////////////////////////
/// \brief Define the data members of a transformable object
///
////////////////////////////////////////////////////////////
#define ZA_DEFINE_TRANSFORMABLE_DATA_MEMBERS                                                  \
    ::za::Vec2f position{};      /*!< Position of the object in the 2D world */               \
    ::za::Vec2f scale{1.f, 1.f}; /*!< Scale of the object */                                  \
    ::za::Vec2f origin{};        /*!< Origin of translation/rotation/scaling of the object */ \
    /* NOLINTNEXTLINE(readability-redundant-member-init) */                                   \
    ::za::AutoWrapAngle rotation{}; /*!< Orientation of the object */                         \
                                                                                              \
    static_assert(true)
