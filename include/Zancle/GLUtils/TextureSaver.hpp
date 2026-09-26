#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/Export.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief RAII guard that saves and restores the current 2D texture binding
///
/// On construction, queries `GL_TEXTURE_BINDING_2D` for the active
/// texture unit and stores it. On destruction, the texture binding is
/// restored via `glBindTexture(GL_TEXTURE_2D, ...)`.
///
/// \note Only the `GL_TEXTURE_2D` binding for the *current* active
///       texture unit is saved; other texture targets and units are
///       unaffected.
///
////////////////////////////////////////////////////////////
class ZA_GLUTILS_API TextureSaver
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor: snapshot the current 2D texture binding
    ///
    ////////////////////////////////////////////////////////////
    TextureSaver();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor: restore the previously saved 2D texture binding
    ///
    ////////////////////////////////////////////////////////////
    ~TextureSaver();

    ////////////////////////////////////////////////////////////
    TextureSaver(const TextureSaver&)            = delete;
    TextureSaver& operator=(const TextureSaver&) = delete;

    ////////////////////////////////////////////////////////////
    TextureSaver(TextureSaver&&)            = delete;
    TextureSaver& operator=(TextureSaver&&) = delete;

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    int m_textureBinding; //!< Saved `GL_TEXTURE_BINDING_2D` to restore
};

} // namespace za::priv
