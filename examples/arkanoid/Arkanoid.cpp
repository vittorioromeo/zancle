#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Graphics/CircleShape.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/RectangleShape.hpp"
#include "Zancle/Graphics/RectangleShapeData.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"

#include "Zancle/Window/Event.hpp" // IWYU pragma: keep
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Keyboard.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Fabs.hpp"

#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"


namespace
{

constexpr za::Vec2f resolution{800.f, 600.f};

class Game
{
private:
    static constexpr float     ballSpeed{3.f};
    static constexpr float     playerSpeed{6.f};
    static constexpr za::Vec2f brickSize{50.f, 24.f};

    za::CircleShape m_ball;
    za::Vec2f       m_ballVelocity;

    za::RectangleShape m_player;
    za::Vec2f          m_playerVelocity;

    za::Vector<za::RectangleShape> m_bricks;

    void createBrickGrid()
    {
        constexpr za::Vec2f offset{50.f, 50.f};

        constexpr za::SizeT nBricksPerRow = 13;
        constexpr za::SizeT nRows         = 4;

        constexpr float spacing = 120.f / 14.f;

        za::Vec2f next{0.f, 0.f};

        for (za::SizeT y = 0; y < nRows; ++y)
        {
            for (za::SizeT x = 0; x < nBricksPerRow; ++x)
            {
                m_bricks.emplaceBack(
                    za::RectangleShapeData{.position         = offset + next,
                                           .origin           = brickSize / 2.f,
                                           .fillColor        = za::Color::DarkGreen,
                                           .outlineColor     = za::Color::Green,
                                           .outlineThickness = 2.f,
                                           .size             = brickSize});

                next.x += spacing + brickSize.x;
            }

            next.x = 0;
            next.y += spacing + brickSize.y;
        }
    }

    bool performBallBrickCollisionResolution(const za::RectangleShape& brick)
    {
        const auto ballBounds  = m_ball.getGlobalBounds();
        const auto brickBounds = brick.getGlobalBounds();

        if (!brickBounds.intersects(ballBounds))
            return false;

        const float overlapLeft{ballBounds.getRight() - brickBounds.getLeft()};
        const float overlapRight{brickBounds.getRight() - ballBounds.getLeft()};
        const float overlapTop{ballBounds.getBottom() - brickBounds.getTop()};
        const float overlapBottom{brickBounds.getBottom() - ballBounds.getTop()};

        const bool ballFromLeft(za::fabs(overlapLeft) < za::fabs(overlapRight));
        const bool ballFromTop(za::fabs(overlapTop) < za::fabs(overlapBottom));

        const float minOverlapX{ballFromLeft ? overlapLeft : overlapRight};
        const float minOverlapY{ballFromTop ? overlapTop : overlapBottom};

        if (za::fabs(minOverlapX) < za::fabs(minOverlapY))
            m_ballVelocity.x = ballFromLeft ? -ballSpeed : ballSpeed;
        else
            m_ballVelocity.y = ballFromTop ? -ballSpeed : ballSpeed;

        return true;
    }

    void updateBallCollisionsAgainstBoundaries()
    {
        const auto [ballLeft, ballTop]     = m_ball.getGlobalTopLeft();
        const auto [ballRight, ballBottom] = m_ball.getGlobalBottomRight();

        const float boundaryLeft   = 0.f;
        const float boundaryRight  = resolution.x;
        const float boundaryTop    = 0.f;
        const float boundaryBottom = resolution.y;

        if (ballLeft < boundaryLeft)
        {
            m_ballVelocity.x *= -1.f;
            m_ball.setGlobalLeft(boundaryLeft);
        }
        else if (ballRight > boundaryRight)
        {
            m_ballVelocity.x *= -1.f;
            m_ball.setGlobalRight(boundaryRight);
        }

        if (ballTop < boundaryTop)
        {
            m_ballVelocity.y *= -1.f;
            m_ball.setGlobalTop(boundaryTop);
        }
        else if (ballBottom > boundaryBottom)
        {
            m_ballVelocity.y *= -1.f;
            m_ball.setGlobalBottom(boundaryBottom);
        }
    }

    void updateBallCollisionsAgainstPlayer()
    {
        if (!m_player.getGlobalBounds().intersects(m_ball.getGlobalBounds()))
            return;

        m_ballVelocity.y = -ballSpeed;

        if (m_ball.position.x < m_player.position.x)
            m_ballVelocity.x = -ballSpeed;
        else
            m_ballVelocity.x = ballSpeed;
    }

    void updateBallCollisionsAgainstBricks()
    {
        for (auto* it = m_bricks.begin(); it != m_bricks.end(); ++it)
        {
            if (performBallBrickCollisionResolution(*it))
            {
                za::genericSwap(*it, m_bricks.back());
                m_bricks.popBack();
                break;
            }
        }
    }

public:
    Game() :
        m_ball{{.position         = resolution / 2.f,
                .origin           = {6.f, 6.f},
                .fillColor        = za::Color::DarkGreen,
                .outlineColor     = za::Color::Green,
                .outlineThickness = 2.f,
                .radius           = 12.f}},
        m_ballVelocity{ballSpeed, ballSpeed},
        m_player{{.position         = {resolution.x / 2.f, resolution.y - 24.f * 2},
                  .origin           = {64.f, 12.f},
                  .fillColor        = za::Color::DarkGreen,
                  .outlineColor     = za::Color::Green,
                  .outlineThickness = 2.f,
                  .size             = {128.f, 24.f}}},
        m_playerVelocity{0.f, 0.f}
    {
        createBrickGrid();
    }

    void update()
    {
        //
        // Ball movement
        m_ball.position += m_ballVelocity;

        //
        // Ball collisions
        updateBallCollisionsAgainstBoundaries();
        updateBallCollisionsAgainstPlayer();
        updateBallCollisionsAgainstBricks();

        //
        // Player input
        if (za::Keyboard::isKeyPressed(za::Keyboard::Key::Left))
            m_playerVelocity.x = -playerSpeed;
        else if (za::Keyboard::isKeyPressed(za::Keyboard::Key::Right))
            m_playerVelocity.x = playerSpeed;
        else
            m_playerVelocity.x = 0;

        //
        // Player movement
        m_player.position += m_playerVelocity;

        //
        // Player collision versus boundaries
        const float boundaryLeft  = 0.f;
        const float boundaryRight = resolution.x;

        if (m_player.getGlobalLeft() < boundaryLeft)
            m_player.setGlobalLeft(boundaryLeft);
        else if (m_player.getGlobalRight() > boundaryRight)
            m_player.setGlobalRight(boundaryRight);
    }

    void draw(za::RenderTarget& renderTarget, const za::RenderStates& states) const
    {
        renderTarget.withLockedRenderStates(states).drawAll(m_ball, m_player, m_bricks);
    }
};

} // namespace


int main()
{
    //
    //
    // Set up graphics context
    auto graphicsContext = za::GraphicsContext::create().value();

    //
    //
    // Set up window and render texture
    auto window = makeDPIScaledRenderWindow(
                      {
                          .size           = resolution.toVec2u(),
                          .title          = "Arkanoid",
                          .resizable      = true,
                          .vsync          = true,
                          .frametimeLimit = 144u,
                      })
                      .value();

    auto windowView = computeAspectRatioAwareView(window.getSize().toVec2f(), resolution);
    auto worldView  = za::View::fromScreenSize(resolution);

    auto rtGame = makeAARenderTexture(resolution.toVec2u(), {.sampleCount = 8u}).value();

    //
    //
    // Set up game and simulation loop
    Game game;

    while (true)
    {
        while (za::Optional event = window.pollEvent())
        {
            if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return 0;

            if (handleAspectRatioAwareResize(*event, resolution, windowView))
                continue;
        }

        game.update();

        rtGame.clear();
        rtGame.draw(game, {.view = worldView});
        rtGame.display();

        window.clear();
        window.draw(rtGame.getTexture(), {.view = windowView});
        window.display();
    }

    return 0;
}
