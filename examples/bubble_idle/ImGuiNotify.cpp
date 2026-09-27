#include "IconsFontAwesome6.h"
#include "ImGuiNotify.hpp"

#include "Zancle/ImGui/ImConfigZancle.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"
#include "Zancle/ImGui/IncludeImGuiInternal.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/Memset.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Strlen.hpp"

#include <cstdarg>
#include <cstdio>


const ImGuiWindowFlags NOTIFY_DEFAULT_TOAST_FLAGS = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration |
                                                    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                                    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings;

#define NOTIFY_NULL_OR_EMPTY(str) (!str || !ZA_STRLEN(str))
#define NOTIFY_FORMAT(fn, format, ...) \
    if (format)                        \
    {                                  \
        va_list args;                  \
        va_start(args, format);        \
        fn(format, args);              \
        va_end(args);                  \
    }


////////////////////////////////////////////////////////////
void ImGuiToast::setTitle(const char* const format, va_list args)
{
    vsnprintf(this->title, sizeof(this->title), format, args);
}


////////////////////////////////////////////////////////////
void ImGuiToast::setContent(const char* const format, va_list args)
{
    vsnprintf(this->content, sizeof(this->content), format, args);
}


////////////////////////////////////////////////////////////
void ImGuiToast::setButtonLabel(const char* const format, va_list args)
{
    vsnprintf(this->buttonLabel, sizeof(this->buttonLabel), format, args);
}


////////////////////////////////////////////////////////////
void ImGuiToast::setTitle(const char* const format, ...)
{
    NOTIFY_FORMAT(this->setTitle, format);
}


////////////////////////////////////////////////////////////
void ImGuiToast::setContent(const char* const format, ...)
{
    NOTIFY_FORMAT(this->setContent, format);
}


////////////////////////////////////////////////////////////
void ImGuiToast::setType(const ImGuiToastType& newType)
{
    IM_ASSERT(newType < ImGuiToastType::COUNT);
    this->type = newType;
}


////////////////////////////////////////////////////////////
void ImGuiToast::setWindowFlags(const ImGuiWindowFlags& newFlags)
{
    this->flags = newFlags;
}


////////////////////////////////////////////////////////////
void ImGuiToast::setButtonLabel(const char* const format, ...)
{
    NOTIFY_FORMAT(this->setButtonLabel, format);
}


////////////////////////////////////////////////////////////
const char* ImGuiToast::getTitle()
{
    return this->title;
}


////////////////////////////////////////////////////////////
const char* ImGuiToast::getDefaultTitle()
{
    if (!ZA_STRLEN(this->title))
    {
        switch (this->type)
        {
            case ImGuiToastType::None:
                return nullptr;
            case ImGuiToastType::Success:
                return "Success";
            case ImGuiToastType::Warning:
                return "Warning";
            case ImGuiToastType::Error:
                return "Error";
            case ImGuiToastType::Info:
                return "Info";
            default:
                return nullptr;
        }
    }

    return this->title;
}


////////////////////////////////////////////////////////////
ImGuiToastType ImGuiToast::getType()
{
    return this->type;
}


////////////////////////////////////////////////////////////
ImVec4 ImGuiToast::getColor()
{
    switch (this->type)
    {
        case ImGuiToastType::None:
            return {255, 255, 255, 255}; // White
        case ImGuiToastType::Success:
            return {0, 255, 0, 255}; // Green
        case ImGuiToastType::Warning:
            return {255, 255, 0, 255}; // Yellow
        case ImGuiToastType::Error:
            return {255, 0, 0, 255}; // Error
        case ImGuiToastType::Info:
            return {0, 157, 255, 255}; // Blue
        default:
            return {255, 255, 255, 255}; // White
    }
}


////////////////////////////////////////////////////////////
const char* ImGuiToast::getIcon()
{
    switch (this->type)
    {
        case ImGuiToastType::None:
            return nullptr;
        case ImGuiToastType::Success:
            return ICON_FA_CIRCLE_CHECK; // Font Awesome 6
        case ImGuiToastType::Warning:
            return ICON_FA_TRIANGLE_EXCLAMATION; // Font Awesome 6
        case ImGuiToastType::Error:
            return ICON_FA_CIRCLE_EXCLAMATION; // Font Awesome 6
        case ImGuiToastType::Info:
            return ICON_FA_CIRCLE_INFO; // Font Awesome 6
        default:
            return nullptr;
    }
}


////////////////////////////////////////////////////////////
char* ImGuiToast::getContent()
{
    return this->content;
}


////////////////////////////////////////////////////////////
za::Time ImGuiToast::getElapsedTime()
{
    return za::Clock::now() - this->creationTime;
}


////////////////////////////////////////////////////////////
ImGuiToastPhase ImGuiToast::getPhase()
{
    const za::I64 elapsed = getElapsedTime().asMilliseconds();

    if (elapsed > NOTIFY_FADE_IN_OUT_TIME + this->dismissTime + NOTIFY_FADE_IN_OUT_TIME)
        return ImGuiToastPhase::Expired;

    if (elapsed > NOTIFY_FADE_IN_OUT_TIME + this->dismissTime)
        return ImGuiToastPhase::FadeOut;

    if (elapsed > NOTIFY_FADE_IN_OUT_TIME)
        return ImGuiToastPhase::Wait;

    return ImGuiToastPhase::FadeIn;
}


////////////////////////////////////////////////////////////
float ImGuiToast::getFadePercent()
{
    const ImGuiToastPhase phase   = getPhase();
    const za::I64         elapsed = getElapsedTime().asMilliseconds();

    if (phase == ImGuiToastPhase::FadeIn)
        return (static_cast<float>(elapsed) / static_cast<float>(NOTIFY_FADE_IN_OUT_TIME)) * NOTIFY_OPACITY;

    if (phase == ImGuiToastPhase::FadeOut)
        return (1.f - ((static_cast<float>(elapsed) - static_cast<float>(NOTIFY_FADE_IN_OUT_TIME) -
                        static_cast<float>(this->dismissTime)) /
                       static_cast<float>(NOTIFY_FADE_IN_OUT_TIME))) *
               NOTIFY_OPACITY;

    return 1.f * NOTIFY_OPACITY;
}


////////////////////////////////////////////////////////////
ImGuiWindowFlags ImGuiToast::getWindowFlags()
{
    return this->flags;
}


////////////////////////////////////////////////////////////
const char* ImGuiToast::getButtonLabel()
{
    return this->buttonLabel;
}


////////////////////////////////////////////////////////////
ImGuiToast::ImGuiToast(const ImGuiToastType toastType, const int toastDismissTime) : flags{NOTIFY_DEFAULT_TOAST_FLAGS}
{
    IM_ASSERT(toastType < ImGuiToastType::COUNT);

    this->type        = toastType;
    this->dismissTime = toastDismissTime;

    this->creationTime = za::Clock::now();

    ZA_MEMSET(this->title, 0, sizeof(this->title));
    ZA_MEMSET(this->content, 0, sizeof(this->content));
}


////////////////////////////////////////////////////////////
ImGuiToast::ImGuiToast(const ImGuiToastType toastType, const char* const format, ...) : ImGuiToast(toastType)
{
    NOTIFY_FORMAT(this->setContent, format);
}


////////////////////////////////////////////////////////////
ImGuiToast::ImGuiToast(const ImGuiToastType toastType, const int toastDismissTime, const char* const format, ...) :
    ImGuiToast(toastType, toastDismissTime)
{
    NOTIFY_FORMAT(this->setContent, format);
}


namespace ImGui
{
namespace
{
////////////////////////////////////////////////////////////
za::Vector<ImGuiToast> notifications;

} // namespace


////////////////////////////////////////////////////////////
void InsertNotification(const ImGuiToast& toast)
{
    notifications.pushBack(toast);
}


////////////////////////////////////////////////////////////
void RemoveNotification(const int index)
{
    notifications.eraseAt(static_cast<za::SizeT>(index));
}


////////////////////////////////////////////////////////////
void RenderNotifications(const float                paddingY,
                         const NotificationCallback afterBegin,
                         const NotificationCallback beforeEnd,
                         void* const                userData)
{
    const ImVec2 mainWindowSize = GetMainViewport()->Size;

    float height = 0.f;

    for (za::SizeT i = 0; i < notifications.size(); ++i)
    {
        ImGuiToast* currentToast = &notifications[i];

        // Remove toast if expired
        if (currentToast->getPhase() == ImGuiToastPhase::Expired)
        {
            RemoveNotification(static_cast<int>(i));
            continue;
        }

#if NOTIFY_RENDER_LIMIT > 0
        if (i > NOTIFY_RENDER_LIMIT)
            continue;
#endif

        // Get icon, title and other data
        const char* icon         = currentToast->getIcon();
        const char* title        = currentToast->getTitle();
        const char* content      = currentToast->getContent();
        const char* defaultTitle = currentToast->getDefaultTitle();
        const float opacity      = currentToast->getFadePercent(); // Get opacity based of the current phase

        // Window rendering
        ImVec4 textColor = currentToast->getColor();
        textColor.w      = opacity;

        // Generate new unique name for this toast
        char windowName[50];
#ifdef _WIN32
        sprintf_s(windowName, "##TOAST%d", static_cast<int>(i));
#elif defined(__linux__) || defined(__EMSCRIPTEN__)
        std::sprintf(windowName, "##TOAST%d", static_cast<int>(i));
#elif defined(__APPLE__)
        std::snprintf(windowName, 50, "##TOAST%d", static_cast<int>(i));
#else
        throw "Unsupported platform";
#endif

        // PushStyleColor(ImGuiCol_Text, textColor);
        SetNextWindowBgAlpha(opacity);

        // Set notification window position to bottom right corner of the main window, considering the main window size and location in relation to the display
        ImVec2 mainWindowPos = GetMainViewport()->Pos;
        SetNextWindowPos(ImVec2(mainWindowPos.x + NOTIFY_PADDING_X, mainWindowPos.y + mainWindowSize.y - paddingY - height),
                         ImGuiCond_Always,
                         ImVec2(0.f, 1.f));

        Begin(windowName, nullptr, currentToast->getWindowFlags());
        afterBegin(opacity, userData);

        // Render over all other windows
        BringWindowToDisplayFront(GetCurrentWindow());

        // Here we render the toast content
        {
            PushTextWrapPos(mainWindowSize.x /
                            3.f); // We want to support multi-line text, this will wrap the text after 1/3 of the screen width

            bool wasTitleRendered = false;

            // If an icon is set
            if (!NOTIFY_NULL_OR_EMPTY(icon))
            {
                // Text(icon); // Render icon text
                TextColored(textColor, "%s", icon);
                wasTitleRendered = true;
            }

            // If a title is set
            if (!NOTIFY_NULL_OR_EMPTY(title))
            {
                // If a title and an icon is set, we want to render on same line
                if (!NOTIFY_NULL_OR_EMPTY(icon))
                    SameLine();

                SetWindowFontScale(0.85f);
                Text("%s", title); // Render title text
                wasTitleRendered = true;
            }
            else if (!NOTIFY_NULL_OR_EMPTY(defaultTitle))
            {
                if (!NOTIFY_NULL_OR_EMPTY(icon))
                    SameLine();

                SetWindowFontScale(0.85f);
                Text("%s", defaultTitle); // Render default title text (ImGuiToastType_Success -> "Success", etc...)
                wasTitleRendered = true;
            }

            // If a dismiss button is enabled
            if (NOTIFY_USE_DISMISS_BUTTON)
            {
                // If a title or content is set, we want to render the button on the same line
                if (wasTitleRendered || !NOTIFY_NULL_OR_EMPTY(content))
                    SameLine();

                // Render the dismiss button on the top right corner
                // NEEDS TO BE REWORKED
                float scale = 0.8f;

                if (CalcTextSize(content).x > GetContentRegionAvail().x)
                    scale = 0.8f;

                SetCursorPosX(GetCursorPosX() + (GetWindowSize().x - GetCursorPosX()) * scale);

                // If the button is pressed, we want to remove the notification
                if (Button(ICON_FA_XMARK))
                    RemoveNotification(static_cast<int>(i));
            }

            // In case ANYTHING was rendered in the top, we want to add a small padding so the text (or icon) looks centered vertically
            if (wasTitleRendered && !NOTIFY_NULL_OR_EMPTY(content))
                SetCursorPosY(GetCursorPosY() + 5.f); // Must be a better way to do this!!!!

            // If a content is set
            if (!NOTIFY_NULL_OR_EMPTY(content))
            {
                if (wasTitleRendered)
                {
#if NOTIFY_USE_SEPARATOR
                    Separator();
#endif
                }

                SetWindowFontScale(0.65f);
                Text("%s", content); // Render content text
            }

            PopTextWrapPos();
        }

        // Save height for next toasts
        height += GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;

        // End
        beforeEnd(opacity, userData);
        End();
    }
}

} // namespace ImGui

#undef NOTIFY_NULL_OR_EMPTY
#undef NOTIFY_FORMAT
