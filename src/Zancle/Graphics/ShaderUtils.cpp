// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/ShaderUtils.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/IO/IO.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/String/StringView.hpp"
#include "Zancle/String/ToChars.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
////////////////////////////////////////////////////////////
constexpr unsigned int maxGlslIncludeDepth      = 32;
constexpr unsigned int maxIncludeFilenameLength = 256;


////////////////////////////////////////////////////////////
// Read the full contents of a file into a Vector<char>.
[[nodiscard]] za::Optional<za::Vector<char>> readFileContents(const za::Path& path)
{
    za::Vector<char> buffer;
    if (!za::readFromFile(path, buffer))
        return za::nullOpt;

    return za::makeOptional(ZA_MOVE(buffer));
}


////////////////////////////////////////////////////////////
// Recursively preprocess GLSL #include directives
[[nodiscard]] bool preprocessGlslIncludesImpl(
    za::StringView        source,
    const za::Path&       basePath,
    za::Vector<char>&     output,
    za::Vector<za::Path>& includeStack,
    unsigned int          depth)
{
    if (depth > maxGlslIncludeDepth)
    {
        za::priv::errMsg("GLSL #include depth limit exceeded ({})", maxGlslIncludeDepth);
        return false;
    }

    // Emit #line directive to set correct line numbering for this file
    za::ShaderUtils::emitLineDirective(output, 1);

    unsigned int lineNumber = 0;
    za::SizeT    lineStart  = 0;

    while (lineStart < source.size())
    {
        // Find end of current line
        za::SizeT  lineEnd  = source.find('\n', lineStart);
        const bool lastLine = (lineEnd == za::StringView::nPos);

        if (lastLine)
            lineEnd = source.size();

        ++lineNumber;

        // Extract line content (without \n), strip trailing \r
        za::StringView line = source.substrByPosLen(lineStart, lineEnd - lineStart);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.removeSuffix(1);

        // Check for #include directive
        const za::Optional<za::StringView> optIncludedFilename = za::ShaderUtils::parseIncludeDirective(line);

        if (!optIncludedFilename.hasValue())
            return false; // Malformed #include directive

        const za::StringView& includedFilename = *optIncludedFilename;

        if (!includedFilename.empty())
        {
            // Build null-terminated filename for Path construction
            if (includedFilename.size() >= maxIncludeFilenameLength)
            {
                za::priv::errMsg("GLSL #include filename too long ({} characters, maximum is {})",
                                 includedFilename.size(),
                                 maxIncludeFilenameLength - 1);

                return false;
            }

            char filenameBuf[maxIncludeFilenameLength];
            ZA_MEMCPY(filenameBuf, includedFilename.data(), includedFilename.size());
            filenameBuf[includedFilename.size()] = '\0';

            // Resolve include path relative to base directory
            const auto absMaybe = (basePath / za::Path(static_cast<const char*>(filenameBuf))).getAbsolute();
            if (!absMaybe)
            {
                za::priv::errMsg("Failed to resolve absolute path for GLSL #include '{}'",
                                 static_cast<const char*>(filenameBuf));
                return false;
            }
            const za::Path& includePath = *absMaybe;

            // Check for circular includes
            for (const auto& stackPath : includeStack)
            {
                if (stackPath == includePath)
                {
                    za::priv::errMsg("Circular GLSL #include detected for '{}':\n", static_cast<const char*>(filenameBuf));

                    for (const auto& p : includeStack)
                        za::priv::errMsgMulti("  {} ->\n", p);

                    za::priv::errMsgMulti("  {}{}", includePath, '\n');
                    return false;
                }
            }

            // Read the included file
            auto optFileContents = readFileContents(includePath);

            if (!optFileContents.hasValue())
            {
                za::priv::errMsg("Failed to open GLSL #include file '{}'", static_cast<const char*>(filenameBuf));

                if (!includeStack.empty())
                {
                    za::priv::errMsgMulti("\n  Include stack:\n");

                    for (const auto& p : includeStack)
                        za::priv::errMsgMulti("    {}{}", p, '\n');
                }

                return false;
            }

            // Get included source
            za::StringView includedSource{optFileContents->data(), optFileContents->size()};

            // Emit begin-include marker
            {
                constexpr za::StringView prefix{"// >>> begin included from \""};
                constexpr za::StringView suffix{"\" >>>\n"};

                output.emplaceRange(prefix.data(), prefix.size());
                output.emplaceRange(filenameBuf, includedFilename.size());
                output.emplaceRange(suffix.data(), suffix.size());
            }

            // Push to include stack and recursively process
            includeStack.pushBack(includePath);

            if (!preprocessGlslIncludesImpl(includedSource, includePath.getParent(), output, includeStack, depth + 1))
                return false;

            includeStack.popBack();

            // Emit end-include marker
            {
                constexpr za::StringView prefix{"// <<< end included from \""};
                constexpr za::StringView suffix{"\" <<<\n"};

                output.emplaceRange(prefix.data(), prefix.size());
                output.emplaceRange(filenameBuf, includedFilename.size());
                output.emplaceRange(suffix.data(), suffix.size());
            }

            // Restore line numbering for parent file
            za::ShaderUtils::emitLineDirective(output, lineNumber + 1);
        }
        else
        {
            // Copy line as-is
            if (!line.empty())
                output.emplaceRange(line.data(), line.size());

            output.pushBack('\n');
        }

        if (lastLine)
            break;

        lineStart = lineEnd + 1;
    }

    return true;
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
void ShaderUtils::emitLineDirective(za::Vector<char>& buffer, unsigned int lineNumber)
{
    constexpr za::StringView prefix{"#line "};

    char        tmp[16];
    char* const end = za::toChars(tmp, tmp + sizeof(tmp), lineNumber);
    ZA_ASSERT(end != nullptr);

    buffer.emplaceRange(prefix.data(), prefix.size());
    buffer.emplaceRange(tmp, static_cast<za::SizeT>(end - tmp));
    buffer.pushBack('\n');
}


////////////////////////////////////////////////////////////
za::Optional<za::StringView> ShaderUtils::parseIncludeDirective(za::StringView line)
{
    const auto fail = [&](const char* what)
    {
        priv::errMsg("Malformed GLSL #include directive ({}): {}", what, line);
        return za::nullOpt;
    };

    za::SizeT pos = 0;

    // Skip leading whitespace
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
        ++pos;

    // Check for #include
    constexpr za::StringView includeKeyword{"#include"};

    if (line.size() - pos < includeKeyword.size())
        return za::makeOptional<za::StringView>(); // not an include

    if (line.substrByPosLen(pos, includeKeyword.size()) != includeKeyword)
        return za::makeOptional<za::StringView>(); // not an include

    pos += includeKeyword.size();

    // Must be followed by whitespace or quote (not e.g. #includeFoo)
    if (pos < line.size() && line[pos] != ' ' && line[pos] != '\t' && line[pos] != '"')
        return za::makeOptional<za::StringView>(); // not an include

    // At this point we know it's an #include directive -- any further issue is a hard error

    // Skip whitespace after #include
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
        ++pos;

    // Expect opening double quote
    if (pos >= line.size() || line[pos] != '"')
        return fail("expected '\"'");

    ++pos;

    // Find closing double quote
    const za::SizeT filenameStart = pos;
    while (pos < line.size() && line[pos] != '"')
        ++pos;

    if (pos >= line.size())
        return fail("missing closing '\"'");

    if (pos == filenameStart)
        return fail("empty filename");

    return za::makeOptional<za::StringView>(line.substrByPosLen(filenameStart, pos - filenameStart));
}


////////////////////////////////////////////////////////////
bool ShaderUtils::preprocessGlslIncludes(za::StringView source, const Path& shaderPath, za::Vector<char>& output)
{
    output.clear();

    // Strip trailing null that may be present from file reading
    if (!source.empty() && source[source.size() - 1] == '\0')
        source.removeSuffix(1);

    const auto absMaybe = shaderPath.getAbsolute();
    if (!absMaybe)
    {
        priv::errMsg("Failed to resolve absolute path for shader");
        return false;
    }
    const Path& absoluteShaderPath = *absMaybe;

    za::Vector<Path> includeStack;
    includeStack.pushBack(absoluteShaderPath);

    return preprocessGlslIncludesImpl(source, absoluteShaderPath.getParent(), output, includeStack, 0);
}

} // namespace za
