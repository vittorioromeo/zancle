////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "../bubble_idle/TextEffectWiggle.cpp" // TODO P0: move to ExampleUtils
#include "../bubble_idle/TextEffectWiggle.hpp" // TODO P1: avoid the relative path...?

#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Audio/AudioContext.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"
#include "Zancle/Audio/Sound.hpp"
#include "Zancle/Audio/SoundBuffer.hpp"

#include "Zancle/Graphics/CircleShape.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/RectangleShape.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Texture.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Keyboard.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Random/FastNonCryptoRng.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Fabs.hpp"

#ifdef ZA_SYSTEM_IOS
    #include "Zancle/Main.hpp"
#endif

namespace
{
za::Path resourcesDir()
{
#ifdef ZA_SYSTEM_IOS
    return "";
#else
    return "resources";
#endif
}
} // namespace


////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    za::FastNonCryptoRng rng(/* seed */ 1234);

    // Define some constants
    constexpr za::Vec2f gameSize{800.f, 600.f};
    constexpr za::Vec2f paddleSize{25.f, 100.f};
    constexpr float     ballRadius = 10.f;

    // Create the graphics context
    auto graphicsContext = za::GraphicsContext::create().value();

    // Create the window of the application
    auto window = makeDPIScaledRenderWindow(
                      {
                          .size         = gameSize.toVec2u(),
                          .bitsPerPixel = 32u,
                          .title        = "Zancle Tennis",
                          .resizable    = true,
                          .vsync        = true,
                      })
                      .value();

    auto windowView = computeAspectRatioAwareView(window.getSize().toVec2f(), gameSize);

    // Create an audio context and get the default playback device
    auto               audioContext = za::AudioContext::create().value();
    za::PlaybackDevice playbackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()};

    // Load the sounds used in the game
    const auto ballSoundBuffer = za::SoundBuffer::loadFromFile(resourcesDir() / "ball.wav").value();
    za::Sound  ballSound(playbackDevice, ballSoundBuffer);

    // Create the Zancle logo texture (the asset is the actual Zancle logo image):
    const auto zancleLogoTexture = za::Texture::loadFromFile(resourcesDir() / "zancle_logo.png").value();

    // Create the left paddle
    za::RectangleShape leftPaddle{
        {.origin           = paddleSize / 2.f,
         .fillColor        = {100u, 100u, 200u},
         .outlineColor     = za::Color::Black,
         .outlineThickness = 3.f,
         .size             = paddleSize - za::Vec2f{3.f, 3.f}}};

    // Create the right paddle
    za::RectangleShape rightPaddle = leftPaddle;
    rightPaddle.setFillColor({200u, 100u, 100u});

    // Create the ball
    za::CircleShape ball{{.origin           = {ballRadius / 2.f, ballRadius / 2.f},
                          .fillColor        = za::Color::White,
                          .outlineColor     = za::Color::Black,
                          .outlineThickness = 2.f,
                          .radius           = ballRadius - 3.f}};

    // Open the text font
    const auto font = za::Font::openFromFile(resourcesDir() / "tuffy.ttf").value();

    // Initialize the pause message
    TextEffectWiggle wiggleTextEffect(10.f, 1.75f);
    za::Text         pauseMessage(font,
                                  {
                                      .position = {170.f, 200.f},
#ifdef ZA_SYSTEM_IOS
                              .string = "Welcome to Zancle Tennis!\nTouch the screen to start the game.",
#else
                              .string = "Welcome to Zancle Tennis!\n\nPress space to start the game.",
#endif
                              .characterSize = 40u,

                              .fillColor        = za::Color::White,
                              .outlineColor     = za::Color::Black,
                              .outlineThickness = 2.f,
                          });

    // Define game constants
    constexpr float paddleSpeed = 400.f;
    constexpr float ballSpeed   = 400.f;

    // Define the paddles properties
    za::Clock      aiTimer;
    const za::Time aiTime           = za::seconds(0.1f);
    float          rightPaddleSpeed = 0.f;
    za::Angle      ballAngle        = za::degrees(0); // to be changed later

    za::Clock clock;
    bool      isPlaying = false;

    while (true)
    {
        // Handle events
        while (const za::Optional event = window.pollEvent())
        {
            if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return 0;

            if (handleAspectRatioAwareResize(*event, gameSize, windowView))
                continue;

            // Space key pressed: play
            if ((event->is<za::Event::KeyPressed>() &&
                 event->getIf<za::Event::KeyPressed>()->code == za::Keyboard::Key::Space) ||
                event->is<za::Event::TouchBegan>())
            {
                if (!isPlaying)
                {
                    // (re)start the game
                    isPlaying = true;
                    clock.restart();

                    // Reset the position of the paddles and ball
                    leftPaddle.position  = {10.f + paddleSize.x / 2.f, gameSize.y / 2.f};
                    rightPaddle.position = {gameSize.x - 10.f - paddleSize.x / 2.f, gameSize.y / 2.f};
                    ball.position        = gameSize / 2.f;

                    // Reset the ball angle
                    do
                    {
                        // Avoid a near-vertical initial angle.
                        ballAngle = za::degrees(rng.getF(0.f, 360.f));
                    } while (za::fabs(za::cos(ballAngle.asRadians())) < 0.7f);
                }
            }
        }

        const float deltaTime = clock.restart().asSeconds();

        if (isPlaying)
        {
            // Move the player's paddle
            if (za::Keyboard::isKeyPressed(za::Keyboard::Key::Up) && (leftPaddle.position.y - paddleSize.y / 2 > 5.f))
            {
                leftPaddle.position.y += -paddleSpeed * deltaTime;
            }
            else if (za::Keyboard::isKeyPressed(za::Keyboard::Key::Down) &&
                     (leftPaddle.position.y + paddleSize.y / 2 < gameSize.y - 5.f))
            {
                leftPaddle.position.y += paddleSpeed * deltaTime;
            }

            // Move the computer's paddle
            if (((rightPaddleSpeed < 0.f) && (rightPaddle.position.y - paddleSize.y / 2 > 5.f)) ||
                ((rightPaddleSpeed > 0.f) && (rightPaddle.position.y + paddleSize.y / 2 < gameSize.y - 5.f)))
            {
                rightPaddle.position.y += rightPaddleSpeed * deltaTime;
            }

            // Update the computer's paddle direction according to the ball position
            if (aiTimer.getElapsedTime() > aiTime)
            {
                aiTimer.restart();
                if (ball.position.y + ballRadius > rightPaddle.position.y + paddleSize.y / 2)
                    rightPaddleSpeed = paddleSpeed;
                else if (ball.position.y - ballRadius < rightPaddle.position.y - paddleSize.y / 2)
                    rightPaddleSpeed = -paddleSpeed;
                else
                    rightPaddleSpeed = 0.f;
            }

            // Move the ball
            ball.position += za::Vec2f::fromAngle(ballSpeed * deltaTime, ballAngle);

#ifdef ZA_SYSTEM_IOS
            const za::String inputString = "Touch the screen to restart.";
#else
            const za::String inputString = "Press space to restart or\nescape to exit.";
#endif

            // Check collisions between the ball and the screen
            if (ball.position.x - ballRadius < 0.f)
            {
                isPlaying = false;
                pauseMessage.setString("You Lost!\n\n" + inputString);
            }
            else if (ball.position.x + ballRadius > gameSize.x)
            {
                isPlaying = false;
                pauseMessage.setString("You Won!\n\n" + inputString);
            }

            if (ball.position.y - ballRadius < 0.f)
            {
                ballSound.play();
                ballAngle       = -ballAngle;
                ball.position.y = ballRadius + 0.1f;
            }
            else if (ball.position.y + ballRadius > gameSize.y)
            {
                ballSound.play();
                ballAngle       = -ballAngle;
                ball.position.y = gameSize.y - ballRadius - 0.1f;
            }

            // Check the collisions between the ball and the paddles
            // Left Paddle
            if (ball.position.x - ballRadius < leftPaddle.position.x + paddleSize.x / 2 &&
                ball.position.x - ballRadius > leftPaddle.position.x &&
                ball.position.y + ballRadius >= leftPaddle.position.y - paddleSize.y / 2 &&
                ball.position.y - ballRadius <= leftPaddle.position.y + paddleSize.y / 2)
            {
                if (ball.position.y > leftPaddle.position.y)
                    ballAngle = za::degrees(180) - ballAngle + za::degrees(rng.getF(0.f, 20.f));
                else
                    ballAngle = za::degrees(180) - ballAngle - za::degrees(rng.getF(0.f, 20.f));

                ballSound.play();
                ball.position.x = leftPaddle.position.x + ballRadius + paddleSize.x / 2 + 0.1f;
            }

            // Right Paddle
            if (ball.position.x + ballRadius > rightPaddle.position.x - paddleSize.x / 2 &&
                ball.position.x + ballRadius < rightPaddle.position.x &&
                ball.position.y + ballRadius >= rightPaddle.position.y - paddleSize.y / 2 &&
                ball.position.y - ballRadius <= rightPaddle.position.y + paddleSize.y / 2)
            {
                if (ball.position.y > rightPaddle.position.y)
                    ballAngle = za::degrees(180) - ballAngle + za::degrees(rng.getF(0.f, 20.f));
                else
                    ballAngle = za::degrees(180) - ballAngle - za::degrees(rng.getF(0.f, 20.f));

                ballSound.play();
                ball.position.x = rightPaddle.position.x - ballRadius - paddleSize.x / 2 - 0.1f;
            }
        }

        // Clear the window
        window.clear({50u, 50u, 50u});

        const auto drawCtx = window.withRenderStates({.view = windowView});

        if (isPlaying)
        {
            drawCtx.drawAll(leftPaddle, rightPaddle, ball);
        }
        else
        {
            wiggleTextEffect.advance(deltaTime);

            wiggleTextEffect.apply(pauseMessage);
            drawCtx.draw(pauseMessage);
            wiggleTextEffect.unapply(pauseMessage);

            drawCtx.draw(zancleLogoTexture, {.position = {170.f, 50.f}});
        }

        // Display things on screen
        window.display();
    }

    return 0;
}
