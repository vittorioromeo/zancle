////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Text.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/Keyboard.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToString.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/Decay.hpp"
#include "Zancle/Trait/IsSame.hpp"


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard]] za::String vec2ToString(const za::Vec2i vec2)
{
    return '(' + za::toString(vec2.x) + ", " + za::toString(vec2.y) + ')';
}


////////////////////////////////////////////////////////////
[[nodiscard]] za::String scancodeToString(const za::Keyboard::Scancode scancode)
{
    return za::Keyboard::getDescription(scancode).asBytes();
}

} // namespace


////////////////////////////////////////////////////////////
/// \brief Application class
///
////////////////////////////////////////////////////////////
class Application
{
public:
    // Visitor passed to event->visit; all overloads must return the same type
    struct Visitor
    {
        explicit Visitor(Application& theApplication) : application(theApplication)
        {
        }

        za::Optional<za::String> operator()(za::Event::Closed)
        {
            application.m_mustClose = true;
            return za::nullOpt;
        }

        za::Optional<za::String> operator()(const za::Event::KeyPressed& keyPress)
        {
            // When the enter key is pressed, switch to the next handler type
            if (keyPress.code == za::Keyboard::Key::Enter)
            {
                application.m_handlerType = HandlerType::Overload;
                application.m_handlerText.setString("Current Handler: Overload");
            }

            return za::makeOptional<za::String>("Key Pressed: " + scancodeToString(keyPress.scancode));
        }

        za::Optional<za::String> operator()(const za::Event::KeyReleased& keyRelease)
        {
            return za::makeOptional<za::String>("Key Released: " + scancodeToString(keyRelease.scancode));
        }

        za::Optional<za::String> operator()(const za::Event::MouseMoved& mouseMoved)
        {
            return za::makeOptional<za::String>("Mouse Moved: " + vec2ToString(mouseMoved.position));
        }

        za::Optional<za::String> operator()(const za::Event::MouseButtonPressed&)
        {
            return za::makeOptional<za::String>("Mouse Pressed");
        }

        za::Optional<za::String> operator()(const za::Event::TouchBegan& touchBegan)
        {
            return za::makeOptional<za::String>("Touch Began: " + vec2ToString(touchBegan.position));
        }

        za::Optional<za::String> operator()(const za::Event::TouchEnded& touchEnded)
        {
            return za::makeOptional<za::String>("Touch Ended: " + vec2ToString(touchEnded.position));
        }

        za::Optional<za::String> operator()(const za::Event::TouchMoved& touchMoved)
        {
            return za::makeOptional<za::String>("Touch Moved: " + vec2ToString(touchMoved.position));
        }

        // When defining a visitor, make sure all event types can be handled by it.
        // If you don't intend on exhaustively specifying an operator() for each
        // event type, you can provide a templated operator() that will be selected
        // by overload resolution when no other event type matches.
        template <typename T>
        za::Optional<za::String> operator()(const T&)
        {
            // All unhandled events will end up here
            // application.m_log.emplaceBack("Other Event");
            return za::nullOpt;
        }

        Application& application;
    };

    ////////////////////////////////////////////////////////////
    void runHandlerClassic()
    {
        // The "classical form" of event handling
        // Poll/Wait for events in a loop and handle them
        // individually based on their concrete type
        while (const za::Optional event = m_window.pollEvent())
        {
            if (event->is<za::Event::Closed>())
            {
                m_mustClose = true;
                return;
            }

            if (const auto* keyPress = event->getIf<za::Event::KeyPressed>())
            {
                m_log.emplaceBack("Key Pressed: " + scancodeToString(keyPress->scancode));

                // When the enter key is pressed, switch to the next handler type
                if (keyPress->code == za::Keyboard::Key::Enter)
                {
                    m_handlerType = HandlerType::Visitor;
                    m_handlerText.setString("Current Handler: Visitor");
                }
            }
            else if (const auto* keyRelease = event->getIf<za::Event::KeyReleased>())
            {
                m_log.emplaceBack("Key Released: " + scancodeToString(keyRelease->scancode));
            }
            else if (const auto* mouseMoved = event->getIf<za::Event::MouseMoved>())
            {
                m_log.emplaceBack("Mouse Moved: " + vec2ToString(mouseMoved->position));
            }
            else if (event->is<za::Event::MouseButtonPressed>())
            {
                m_log.emplaceBack("Mouse Pressed");
            }
            else if (const auto* touchBegan = event->getIf<za::Event::TouchBegan>())
            {
                m_log.emplaceBack("Touch Began: " + vec2ToString(touchBegan->position));
            }
            else if (const auto* touchEnded = event->getIf<za::Event::TouchEnded>())
            {
                m_log.emplaceBack("Touch Ended: " + vec2ToString(touchEnded->position));
            }
            else if (const auto* touchMoved = event->getIf<za::Event::TouchMoved>())
            {
                m_log.emplaceBack("Touch Moved: " + vec2ToString(touchMoved->position));
            }
            else
            {
                // All unhandled events will end up here
                // m_log.emplaceBack("Other Event");
            }
        }
    }

    ////////////////////////////////////////////////////////////
    void runHandlerVisitor()
    {
        // Event Visitor
        // A visitor able to visit all event types is passed to the event
        // The visitor's defined operator()s can also return values
        while (const za::Optional event = m_window.pollEvent())
        {
            if (za::Optional logMessage = event->visit(Visitor(*this)))
                m_log.emplaceBack(ZA_MOVE(*logMessage));
        }
    }

    ////////////////////////////////////////////////////////////
    void runHandlerOverload()
    {
        // Overloaded visitation
        // A callable taking a concrete event type is provided per event type you want to handle
        m_window.pollAndHandleEvents([&](za::Event::Closed) { m_mustClose = true; },
                                     [&](const za::Event::KeyPressed& keyPress)
        {
            m_log.emplaceBack("Key Pressed: " + scancodeToString(keyPress.scancode));

            // When the enter key is pressed, switch to the next handler type
            if (keyPress.code == za::Keyboard::Key::Enter)
            {
                m_handlerType = HandlerType::Generic;
                m_handlerText.setString("Current Handler: Generic");
            }
        },
                                     [&](const za::Event::KeyReleased& keyRelease)
        { m_log.emplaceBack("Key Released: " + scancodeToString(keyRelease.scancode)); },
                                     [&](const za::Event::MouseMoved& mouseMoved)
        { m_log.emplaceBack("Mouse Moved: " + vec2ToString(mouseMoved.position)); },
                                     [&](const za::Event::MouseButtonPressed&) { m_log.emplaceBack("Mouse Pressed"); },
                                     [&](const za::Event::TouchBegan& touchBegan)
        { m_log.emplaceBack("Touch Began: " + vec2ToString(touchBegan.position)); },
                                     [&](const za::Event::TouchEnded& touchEnded)
        { m_log.emplaceBack("Touch Ended: " + vec2ToString(touchEnded.position)); },
                                     [&](const za::Event::TouchMoved& touchMoved)
        { m_log.emplaceBack("Touch Moved: " + vec2ToString(touchMoved.position)); });

        // To handle unhandled events, just add the following lambda to the set of handlers
        // [&](const auto&) { m_log.emplaceBack("Other Event"); }
    }

    ////////////////////////////////////////////////////////////
    void runHandlerGeneric()
    {
        // Generic visitation
        // A generic callable is provided that can differentiate by deduced event type
        m_window.pollAndHandleEvents([&](auto&& event)
        {
            // Remove reference and cv-qualifiers
            using T = za::Decay<decltype(event)>;

            if constexpr (za::isSame<T, za::Event::Closed>)
            {
                m_mustClose = true;
            }
            else if constexpr (za::isSame<T, za::Event::KeyPressed>)
            {
                m_log.emplaceBack("Key Pressed: " + scancodeToString(event.scancode));

                // When the enter key is pressed, switch to the next handler type
                if (event.code == za::Keyboard::Key::Enter)
                {
                    m_handlerType = HandlerType::Forward;
                    m_handlerText.setString("Current Handler: Forward");
                }
            }
            else if constexpr (za::isSame<T, za::Event::KeyReleased>)
            {
                m_log.emplaceBack("Key Released: " + scancodeToString(event.scancode));
            }
            else if constexpr (za::isSame<T, za::Event::MouseMoved>)
            {
                m_log.emplaceBack("Mouse Moved: " + vec2ToString(event.position));
            }
            else if constexpr (za::isSame<T, za::Event::MouseButtonPressed>)
            {
                m_log.emplaceBack("Mouse Pressed");
            }
            else if constexpr (za::isSame<T, za::Event::TouchBegan>)
            {
                m_log.emplaceBack("Touch Began: " + vec2ToString(event.position));
            }
            else if constexpr (za::isSame<T, za::Event::TouchEnded>)
            {
                m_log.emplaceBack("Touch Ended: " + vec2ToString(event.position));
            }
            else if constexpr (za::isSame<T, za::Event::TouchMoved>)
            {
                m_log.emplaceBack("Touch Moved: " + vec2ToString(event.position));
            }
            else
            {
                // All unhandled events will end up here
                // m_log.emplaceBack("Other Event");
            }
        });
    }

    ////////////////////////////////////////////////////////////
    void runHandlerForward()
    {
        // Forward to other callable
        // In this case we forward it to our handle member functions
        // we defined for the concrete event types we want to handle
        // When choosing this method a default "catch-all" handler must
        // be available for unhandled events to be forwarded to
        // If you don't want to provide an empty "catch-all" handler
        // you will have to make sure (e.g. via if constexpr) that this
        // lambda doesn't attempt to call a member function that doesn't exist
        m_window.pollAndHandleEvents([this](const auto& event) { handle(event); });
    }

    ////////////////////////////////////////////////////////////
    void run()
    {
        // This example is for demonstration purposes only
        // All the following forms of event handling have equivalent behavior
        // In your own code you should decide which form of event handling
        // suits your needs best and use a single form of event handling
        while (true)
        {
            if (m_handlerType == HandlerType::Classic)
                runHandlerClassic();
            else if (m_handlerType == HandlerType::Visitor)
                runHandlerVisitor();
            else if (m_handlerType == HandlerType::Overload)
                runHandlerOverload();
            else if (m_handlerType == HandlerType::Generic)
                runHandlerGeneric();
            else if (m_handlerType == HandlerType::Forward)
                runHandlerForward();

            if (m_mustClose)
                return;

            // Limit the log to 24 entries
            if (m_log.size() > 24u)
                m_log.erase(m_log.begin(), m_log.begin() + static_cast<int>(m_log.size() - 24u));

            // Draw the contents of the log to the window
            m_window.clear();

            for (za::SizeT i = 0u; i < m_log.size(); ++i)
            {
                m_logText.position = {50.f, static_cast<float>(i * 20) + 50.f};
                m_logText.setString(m_log[i]);
                m_window.draw(m_logText);
            }

            m_window.draw(m_handlerText);
            m_window.draw(m_instructions);
            m_window.display();
        }
    }

    // The following handle methods are called by the forwarding event handler implementation
    // A handle method is defined per event type you want to handle
    // To handle any other events that are left, the templated handle method will be called
    // Overload resolution will prefer the handle methods that fit the event type better
    // before falling back to the templated method
    void handle(za::Event::Closed)
    {
        m_mustClose = true;
    }

    void handle(const za::Event::KeyPressed& keyPress)
    {
        m_log.emplaceBack("Key Pressed: " + scancodeToString(keyPress.scancode));

        // When the enter key is pressed, switch to the next handler type
        if (keyPress.code == za::Keyboard::Key::Enter)
        {
            m_handlerType = HandlerType::Classic;
            m_handlerText.setString("Current Handler: Classic");
        }
    }

    void handle(const za::Event::KeyReleased& keyRelease)
    {
        m_log.emplaceBack("Key Released: " + scancodeToString(keyRelease.scancode));
    }

    void handle(const za::Event::MouseMoved& mouseMoved)
    {
        m_log.emplaceBack("Mouse Moved: " + vec2ToString(mouseMoved.position));
    }

    void handle(const za::Event::MouseButtonPressed&)
    {
        m_log.emplaceBack("Mouse Pressed");
    }

    void handle(const za::Event::TouchBegan& touchBegan)
    {
        m_log.emplaceBack("Touch Began: " + vec2ToString(touchBegan.position));
    }

    void handle(const za::Event::TouchEnded& touchEnded)
    {
        m_log.emplaceBack("Touch Ended: " + vec2ToString(touchEnded.position));
    }

    void handle(const za::Event::TouchMoved& touchMoved)
    {
        m_log.emplaceBack("Touch Moved: " + vec2ToString(touchMoved.position));
    }

    template <typename T>
    void handle(const T&)
    {
        // All unhandled events will end up here
        // m_log.emplaceBack("Other Event");
    }

private:
    enum class HandlerType
    {
        Classic,
        Visitor,
        Overload,
        Generic,
        Forward
    };

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    za::RenderWindow m_window = makeDPIScaledRenderWindow(
                                    {
                                        .size      = {800u, 600u},
                                        .title     = "Zancle Event Handling",
                                        .resizable = true,
                                        .vsync     = true,
                                    })
                                    .value();

    const za::Font m_font{za::Font::openFromFile("resources/tuffy.ttf").value()};

    za::Text m_logText{m_font,
                       {
                           .string        = "",
                           .characterSize = 20u,
                           .fillColor     = za::Color::White,
                       }};

    za::Text m_handlerText{m_font,
                           {
                               .position      = {380.f, 260.f},
                               .string        = "Current Handler: Classic",
                               .characterSize = 24u,
                               .fillColor     = za::Color::White,
                               .bold          = true,
                           }};

    za::Text m_instructions{m_font,
                            {
                                .position      = {380.f, 310.f},
                                .string        = "Press Enter to change handler type",
                                .characterSize = 24u,
                                .fillColor     = za::Color::White,
                                .bold          = true,
                            }};

    za::Vector<za::String> m_log;
    HandlerType            m_handlerType{HandlerType::Classic};
    bool                   m_mustClose{false};
};


////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    auto graphicsContext = za::GraphicsContext::create().value();
    Application{}.run();
}
