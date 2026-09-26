// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/GLCheck.hpp"

#include "Zancle/GLUtils/GlContext.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Base/Assert.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
void glCheckError(const unsigned int openGlError, const char* const file, const unsigned int line, const char* const expression)
{
    if (!GlContext::hasActiveThreadLocalGlContext())
    {
        errMsg(
            "An internal OpenGL call failed in {}({}).\nExpression:\n   {}\nNo active OpenGL context on calling "
            "thread.\n",
            Path{file}.getFilename(),
            line,
            expression);

#ifdef ZA_FATAL_OPENGL_ERRORS
        ZA_ASSERT(false && "OpenGL error (fatal OpenGL errors enabled)");
#endif
    }

    const auto logError = [&](const char* const error, const char* const description)
    {
        errMsg("An internal OpenGL call failed in {}({}).\nExpression:\n   {}\nError description:\n   {}\n   {}{}",
               Path{file}.getFilename(),
               line,
               expression,
               error,
               description,
               '\n');

#ifdef ZA_FATAL_OPENGL_ERRORS
        ZA_ASSERT(false && "OpenGL error (fatal OpenGL errors enabled)");
#endif
    };

    ZA_ASSERT(openGlError != GL_NO_ERROR);

    switch (openGlError)
    {
        case GL_INVALID_ENUM:
            logError("GL_INVALID_ENUM", "An unacceptable value has been specified for an enumerated argument.");
            return;

        case GL_INVALID_VALUE:
            logError("GL_INVALID_VALUE", "A numeric argument is out of range.");
            return;

        case GL_INVALID_OPERATION:
            logError("GL_INVALID_OPERATION", "The specified operation is not allowed in the current state.");
            return;

        case GL_STACK_OVERFLOW:
            logError("GL_STACK_OVERFLOW", "This command would cause a stack overflow.");
            return;

        case GL_STACK_UNDERFLOW:
            logError("GL_STACK_UNDERFLOW", "This command would cause a stack underflow.");
            return;

        case GL_OUT_OF_MEMORY:
            logError("GL_OUT_OF_MEMORY", "There is not enough memory left to execute the command.");
            return;

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            logError("GL_INVALID_FRAMEBUFFER_OPERATION",
                     "The object bound to FRAMEBUFFER_BINDING is not \"framebuffer complete\".");
            return;

        default:
            logError("Unknown error", "Unknown description");
            return;
    }
}


////////////////////////////////////////////////////////////
void printUncheckedPriorGlError()
{
    errMsg("(EXISTING UNCHECKED ERROR BEFORE THIS EXPRESSION)\n");
}

} // namespace za::priv
