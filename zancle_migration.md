# SFML to Zancle Migration Guide



## Headers

- Catch-all headers such as `<SFML/Graphics.hpp>` have been removed.

- Include exactly what you need instead, e.g.
    `#include <Zancle/Graphics/Sprite.hpp>`



## Global Context State

- Global context state for graphics and audio is not managed automatically by SFML anymore.

- At the beginning of your main, create the appropriate context objects. They must live for the duration of the whole program.

```cpp
int main()
{
    // Create the graphics context
    auto graphicsContext = za::GraphicsContext::create().value();

    // Create an audio context and get the default playback device
    auto audioContext   = za::AudioContext::create().value();
    za::PlaybackDevice playbackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()};

    // ...rest of your application...
}
```



## Designated Initializer Configurations

- Most objects are now initialized using C++20's designated initializer syntax.

```cpp
//
// BEFORE (upstream SFML)
za::RenderWindow window(
    za::VideoMode({static_cast<unsigned int>(gameWidth), static_cast<unsigned int>(gameHeight)}, 32),
    "SFML Tennis",
    za::Style::Titlebar | za::Style::Close);

window.setVerticalSyncEnabled(true);

//
// AFTER (Zancle)
za::RenderWindow window({.size = gameSize.toVec2u(),
                         .bitsPerPixel = 32u,
                         .title = "Zancle Tennis",
                         .resizable = false,
                         .vsync = true});
```

- This affects a lot of types, such as `za::Text`, shapes, sprites, and so on.

```cpp
//
// BEFORE (upstream SFML)
za::CircleShape ball;
ball.setRadius(ballRadius - 3);
ball.setOutlineThickness(2);
ball.setOutlineColor(za::Color::Black);
ball.setFillColor(za::Color::White);
ball.setOrigin({ballRadius / 2.f, ballRadius / 2.f});

//
// AFTER (Zancle)
za::CircleShape ball{{.origin           = {ballRadius / 2.f, ballRadius / 2.f},
                        .fillColor        = za::Color::White,
                        .outlineColor     = za::Color::Black,
                        .outlineThickness = 2.f,
                        .radius           = ballRadius - 3.f}};
```



## Standard Library Replacements

- To optimize compilation speed and debug run-time performance, Zancle uses custom types instead of Standard Library ones.
    - Check out the `ZancleBase` module to see all of them.

- Zancle types use the `pascalCase` convention, and some APIs might be very different (or missing).

- You can still use Standard Library types if you want, but you might have to convert them in some Zancle APIs.
    - However, you'll see a drastic compilation time improvement and debug performance improvement if you choose to use Zancle types (and do not include the Standard headers).

Non-exhaustive table:

| Standard Library   | Zancle                  |
|--------------------|-------------------------|
| `std::optional`    | `zb::Optional`    |
| `std::unique_ptr`  | `zb::UniquePtr`   |
| `std::string_view` | `zb::StringView`  |
| `std::array`       | `zb::Array`       |
| `std::span`        | `zb::Span`        |
| `assert(...)`      | `ZA_ASSERT(...)` |
| `std::uint8_t`     | `zb::U8`          |
| `std::uint16_t`    | `zb::U16`         |
| `std::uint32_t`    | `zb::U32`         |
| `std::int8_t`      | `zb::I8`          |
| `std::int16_t`     | `zb::I16`         |
| `std::int32_t`     | `zb::I32`         |
| `std::size_t`      | `zb::SizeT`       |

Differences that are easy to miss:

- `za::String::substrByPosLen` (like `za::StringView`'s) returns a `za::StringView` into the string, unlike `std::string::substr`, which returns a new string.
    - The view dangles once the string is modified or destroyed. For an owning copy, write `za::String{s.substrByPosLen(pos, len)}`.

- Constructing a `za::String` from a `za::StringView` is explicit (as for `std::string` from `std::string_view`): write `za::String s{view};`, not `za::String s = view;`.
    - Assigning (`s = view;`) and appending (`s += view;`) a view work as usual.

- The math wrappers (`za::sqrt`, `za::sin`, `za::pow`, ...) take exactly `float`, `double`, or `long double`, and the two-argument ones take two arguments of the same type.
    - Unlike `<cmath>`, integers are not converted to `double`, and mixed `float`/`double` arguments are not promoted: convert explicitly (e.g. `za::sqrt(static_cast<float>(n))`), so that every conversion is visible.
    - `za::abs` also accepts signed integers, like `std::abs`.



## Aggregatification Of Types

- Many types in Zancle are now aggregate types (everything is public, no constructors).
    - This improves usage syntax, simplifies the implementation, and improves debug performance / compilation times.

- Notably, `za::Transformable` is now an aggregate. Every transformable object will be affected.

```cpp
//
// BEFORE (upstream SFML)
if (ball.getPosition().y - ballRadius < 0.f)
{
    ballAngle = -ballAngle;
    ball.setPosition({ball.getPosition().x, ballRadius + 0.1f});
}
else if (ball.getPosition().y + ballRadius > gameHeight)
{
    ballAngle = -ballAngle;
    ball.setPosition({ball.getPosition().x, gameHeight - ballRadius - 0.1f});
}

//
// AFTER (Zancle)
if (ball.position.y - ballRadius < 0.f)
{
    ballAngle       = -ballAngle;
    ball.position.y = ballRadius + 0.1f;
}
else if (ball.position.y + ballRadius > gameSize.y)
{
    ballAngle       = -ballAngle;
    ball.position.y = gameSize.y - ballRadius - 0.1f;
}
```



## Audio Multi-Device Support

- Playing sounds/musics now requires an audio device to be specified.

```cpp
//
// BEFORE (upstream SFML)
int main()
{
    // Load sounds
    const za::SoundBuffer ballSoundBuffer(resourcesDir() / "ball.wav");
    za::Sound             ballSound(ballSoundBuffer);

    // Play sound
    ballSound.play();
}

//
// AFTER (Zancle)
int main()
{
    // Create an audio context and get the default playback device
    auto audioContext   = za::AudioContext::create().value();
    za::PlaybackDevice playbackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()};

    // Load sounds
    const auto ballSoundBuffer = za::SoundBuffer::loadFromFile(resourcesDir() / "ball.wav").value();
    za::Sound  ballSound(playbackDevice, ballSoundBuffer);

    // Play sound
    ballSound.play();
}
```



## Optional-Based Factory Functions

- Creation of objects/resources that can fail is done through factory functions returning an `zb::Optional`.
    - This ensures that the user decides how to handle the failure case.

```cpp
//
// BEFORE (upstream SFML)
const za::Texture zancleLogoTexture(resourcesDir() / "zancle_logo.png");

//
// AFTER (Zancle)
const auto zancleLogoTexture = za::Texture::loadFromFile(resourcesDir() / "zancle_logo.png").value();
```

- This change applies to all resources: textures, sound buffers, music, fonts, images, etc...



## Sprites And Shapes Are Textureless

- Sprites and shapes do not point to a texture anymore.
    - The texture is specified during the draw call.

- This improves performance by shrinking the size of these objects (better cache friendliness).

- It also greatly improves safety by preventing dangling texture pointers.

- `nullptr` can be specified if a texture is not used (for the case of shapes).

```cpp
//
// BEFORE (upstream SFML)
const za::Texture zancleLogoTexture(resourcesDir() / "zancle_logo.png");
za::Sprite        zancleLogo(zancleLogoTexture);
zancleLogo.setPosition({170.f, 50.f});
// ...
window.draw(zancleLogo);
window.draw(leftPaddle);
window.draw(rightPaddle);
window.draw(ball);

//
// AFTER (Zancle)
const auto zancleLogoTexture = za::Texture::loadFromFile(resourcesDir() / "zancle_logo.png").value();
const za::Sprite zancleLogo({.position = {170.f, 50.f}});
// ...
window.draw(zancleLogo, {.texture = &zancleLogoTexture});
window.draw(leftPaddle);
window.draw(rightPaddle);
window.draw(ball);
```

Note that textures can be drawn directly in Zancle, without the need of using a sprite:

```cpp
window.draw(zancleLogoTexture, {.position = {170.f, 50.f}});
```



## Shader Uniform Location API

- Shader uniform locations are now retrieved via handles, preventing repeated string lookups.

```cpp
//
// BEFORE (upstream SFML)
shader.setUniform("texture", za::Shader::CurrentTexture);
shader.setUniform("pixel_threshold", (x + y) / 30);

//
// AFTER (Zancle)
auto ulTexture = shader.getUniformLocation("za_u_texture").value(); // cache this
auto ulPixelThreshold = shader.getUniformLocation("pixel_threshold").value(); // cache this

shader.setUniform(ulTexture, za::Shader::CurrentTexture);
shader.setUniform(ulPixelThreshold, (x + y) / 30);
```



## `za::VertexArray` Is Gone

- Just use a `std::vector` or an array.
    - The primitive type is specified on the draw call.



## `za::Drawable` Is Gone

- `za::RenderTarget` has a template member function that accepts anything that exposes `.draw()`.

- Need polymorphism?
    - You don't.

- *Really* need polymorphism?
    - Create a polymorphic wrapper yourself (e.g. use type erasure) and then pass that wrapper to `za::RenderTarget`.
        - This is trivial to do with `std::function`.



## Network Module Factory API

- Socket classes (`za::TcpSocket`, `za::UdpSocket`, `za::TcpListener`) are now constructed through static factory functions returning `zb::Optional<T>`.
    - A socket instance always owns a valid OS handle (or is moved-from): the "created but not initialized" intermediate state is no longer representable.
    - Blocking mode must be specified at creation time.

### `za::TcpSocket`

```cpp
//
// BEFORE (upstream SFML)
za::TcpSocket socket;
socket.connect(address, port);

//
// AFTER (Zancle)
auto socket = za::TcpSocket::create(/* isBlocking */ true).value();
if (socket.connect(address, port) != za::Socket::Status::Done) { /* ... */ }
```

### `za::UdpSocket`

```cpp
//
// BEFORE (upstream SFML)
za::UdpSocket socket;
socket.bind(port);

//
// AFTER (Zancle)
auto socket = za::UdpSocket::create(/* isBlocking */ true).value();
if (socket.bind(port) != za::Socket::Status::Done) { /* ... */ }
```

- `UdpSocket::unbind()` has been removed. Destroy the socket to unbind.
- Binding to `za::IpAddress::Broadcast` is explicitly rejected.

### `za::TcpListener`

- `TcpListener::create` atomically opens the OS handle, binds it, and starts listening. There is no "created but not listening" intermediate state; no separate `listen()` call is needed.

```cpp
//
// BEFORE (upstream SFML)
za::TcpListener listener;
listener.listen(port);

//
// AFTER (Zancle)
auto listener = za::TcpListener::create(port, /* isBlocking */ true).value();
```

- `accept()` now returns a `TcpListener::AcceptResult { Status, Optional<TcpSocket> }` instead of populating a pre-existing socket. The newly accepted connection comes back by value on success.

```cpp
//
// BEFORE (upstream SFML)
za::TcpSocket client;
if (listener.accept(client) == za::Socket::Status::Done) { /* use `client` */ }

//
// AFTER (Zancle)
if (auto result = listener.accept(); result.status == za::Socket::Status::Done)
{
    auto& client = *result.socket; // owned by `result`, move out if you need it elsewhere
    /* use `client` */
}
```

### `disconnect()` is terminal

- `TcpSocket::disconnect()` now returns `void` and invalidates the underlying OS handle. After `disconnect()`, the socket is "dead" and must not be used further; to establish a new connection, construct a fresh `TcpSocket` via the factory.

```cpp
//
// BEFORE (upstream SFML)
socket.disconnect();
socket.connect(address, port); // reconnect with same instance

//
// AFTER (Zancle)
socket.disconnect();

// `socket` is now dead; construct a new one to reconnect:
auto socket2 = za::TcpSocket::create(/* isBlocking */ true).value();
socket2.connect(address, port);
```

### `za::IpAddressUtils` (unchanged API, better diagnostics)

- `za::IpAddressUtils::resolve(stringView)` internally uses `inet_pton` rather than the deprecated `inet_addr`, so it no longer has the historical ambiguity where `255.255.255.255` looked identical to a parse error.
- `za::IpAddressUtils::toString(ipAddress)` is now reentrant (uses `inet_ntop` with a caller-owned buffer instead of `inet_ntoa`'s static buffer) and safe to call from multiple threads concurrently.



## Shader GLSL API Is Different

- Check `Zancle/Graphics/DefaultShader.hpp` to see the default shaders.

For your convenience:

```glsl
// DEFAULT VERTEX SHADER

layout(location = 0) uniform vec3 za_u_mvpRow0;
layout(location = 1) uniform vec3 za_u_mvpRow1;
layout(location = 2) uniform sampler2D za_u_texture;
layout(location = 3) uniform vec2 za_u_invTextureSize;

layout(location = 0) in vec2 za_a_position;
layout(location = 1) in vec4 za_a_color;
layout(location = 2) in vec2 za_a_texCoord;

out vec4 za_v_color;
out vec2 za_v_texCoord;

void main()
{
    vec3 pos = vec3(za_a_position, 1.0);

    gl_Position = vec4(dot(za_u_mvpRow0, pos), dot(za_u_mvpRow1, pos), 0.0, 1.0);
    za_v_color = za_a_color;
    za_v_texCoord = za_a_texCoord * za_u_invTextureSize;
}
```

```glsl
// DEFAULT FRAGMENT SHADER

layout(location = 2) uniform sampler2D za_u_texture;

in vec4 za_v_color;
in vec2 za_v_texCoord;

layout(location = 0) out vec4 za_fragColor;

void main()
{
    za_fragColor = za_v_color * texture(za_u_texture, za_v_texCoord);
}
```

### Built-in uniform locations

| Location | Name                  | Type        | Stage    | Description                             |
|----------|-----------------------|-------------|----------|-----------------------------------------|
| 0        | `za_u_mvpRow0`        | `vec3`      | Vertex   | First row of 2D MVP: `(a00, a01, a02)`  |
| 1        | `za_u_mvpRow1`        | `vec3`      | Vertex   | Second row of 2D MVP: `(a10, a11, a12)` |
| 2        | `za_u_texture`        | `sampler2D` | Fragment | Texture sampler (unit 0)                |
| 3        | `za_u_invTextureSize` | `vec2`      | Vertex   | `(1/texWidth, 1/texHeight)`             |

### Reconstructing `mat4` from `za_u_mvpRow0`/`za_u_mvpRow1`

Zancle uploads only the 6 meaningful values of the 2D affine MVP transform instead of a full `mat4`. If your custom shader needs the full matrix (e.g., for a geometry shader), you can reconstruct it:

```glsl
layout(location = 0) uniform vec3 za_u_mvpRow0;
layout(location = 1) uniform vec3 za_u_mvpRow1;

// Reconstruct the equivalent mat4 (column-major)
mat4 za_u_mvpMatrix = mat4(
    za_u_mvpRow0.x, za_u_mvpRow1.x, 0.0, 0.0,  // column 0
    za_u_mvpRow0.y, za_u_mvpRow1.y, 0.0, 0.0,  // column 1
    0.0,            0.0,            1.0, 0.0,  // column 2
    za_u_mvpRow0.z, za_u_mvpRow1.z, 0.0, 1.0   // column 3
);

// Then use it as before:
gl_Position = za_u_mvpMatrix * vec4(za_a_position, 0.0, 1.0);
```

For most 2D shaders, you can use the more efficient dot-product form directly:

```glsl
gl_Position = vec4(dot(za_u_mvpRow0, vec3(position, 1.0)),
                   dot(za_u_mvpRow1, vec3(position, 1.0)),
                   0.0, 1.0);
```



## Batching

- Manual batching is fully supported to minimize state changes and draw calls.
    - Batch objects are similar to render targets, but not as generic.

```cpp
za::CPUDrawableBatch batch;                 // uses CPU buffers
/* za::PersistentGPUDrawableBatch batch; */ // uses persistent GPU buffer, not available on OpenGL ES

batch.clear();

batch.add(someSprite, someAssociatedTexture);
batch.add(someShape, someAssociatedTexture);
batch.add(someText); // associated texture is stored in the text object

window.draw(batch, commonRenderStates);
```

- Use texture atlases to batch multiple sprites/texts with different textures into a single draw call:

```cpp
za::TextureAtlas atlas{za::Texture::create({1024u, 1024u}, {.smooth = true}).value()};

const za::Rect2f txrSpriteA = atlas.add(za::Image::loadFromFile("spriteA.png").value()).value();
const za::Rect2f txrSpriteB = atlas.add(za::Image::loadFromFile("spriteB.png").value()).value();

const auto fontTuffy = za::Font::openFromFile("resources/tuffy.ttf", &atlas).value();

batch.clear();
// ... add sprites, shapes, texts, etc to batch ...
window.draw(batch, {.texture = &atlas.getTexture()});
```



## Window `getPosition` Changes

Window `getPosition` now returns the top-left corner of the window's contents (not including titlebar, decorations, etc).



## Joystick Query API

- The joystick API has been redesigned from static class methods to a **query object pattern**.
    - Instead of calling `za::Joystick::isButtonPressed(id, button)`, you first obtain an `Optional<Query>` handle for a specific joystick, then call methods on it.

```cpp
//
// BEFORE (upstream SFML)
if (za::Joystick::isConnected(0))
{
    bool pressed = za::Joystick::isButtonPressed(0, 2);
    float pos    = za::Joystick::getAxisPosition(0, za::Joystick::Y);
    unsigned int buttonCount = za::Joystick::getButtonCount(0);

    za::Joystick::Identification id = za::Joystick::getIdentification(0);
    const auto& name = id.name;
}

//
// AFTER (Zancle)
if (const auto query = za::Joystick::query(0); query.hasValue())
{
    bool pressed = query->isButtonPressed(2);
    float pos    = query->getAxisPosition(za::Joystick::Axis::Y);
    unsigned int buttonCount = query->getButtonCount();

    const auto& name     = query->getName();
    unsigned int vendorId = query->getVendorId();
}
```

- If `za::Joystick::query(id)` returns `base::nullOpt`, the joystick is not connected.

- Event-based joystick input still works as before (via `za::Event::JoystickButtonPressed`, etc.).

- If you have no window, call `za::Joystick::update()` manually to refresh the cached state.



## SDL3 Backend

- Zancle transitions to utilizing SDL3 as its backend for window creation, input handling, and platform-specific heavy lifting.
    - This drastically improves platform compatibility (Wayland native support, better controller mapping, smoother resize events) compared to upstream SFML's custom backend code.



## Volume Rescaling

Sound/music volume is now in `[0.0, 1.0]` range instead of `[0.0, 100.0]` range.

```cpp
//
// BEFORE (upstream SFML)
sound.setVolume(50.f); // Half volume

//
// AFTER (Zancle)
sound.setVolume(0.5f); // Half volume
```



## Autobatching

- Zancle features a powerful transparent autobatcher built into `za::RenderTarget`.
    - If enabled, sequential draw calls sharing the same `RenderStates` (texture, shader, blend mode) are aggregated automatically and sent to the GPU in a single operation.
    - You do not need to manually manage `za::CPUDrawableBatch` if you sort your draw calls by texture/state.

```cpp
// Assuming autobatch is enabled (default)
window.draw(sprite1, {.texture = &atlas.getTexture()});
window.draw(sprite2, {.texture = &atlas.getTexture()});
window.draw(sprite3, {.texture = &atlas.getTexture()});
// The above triggers only 1 OpenGL draw call!
```

- You can tune or disable this behavior via `window.setAutoBatchMode(...)`.



## High DPI Support

- Zancle provides proper High DPI support via SDL3's display scaling infrastructure.

- Query the display scale factor at runtime with `window.getDisplayScale()`:

```cpp
//
// BEFORE (upstream SFML)
// No standard mechanism for high DPI support.

//
// AFTER (Zancle)
const float scale = window.getDisplayScale();
// Returns 1.f for standard DPI (96), 2.f for Retina "@2x" displays, etc.
```

- Window sizes are always in **logical pixels**. The OS handles conversion to the physical framebuffer size automatically.

- You can also query the primary display's content scale without a window:

```cpp
const float displayScale = za::VideoModeUtils::getPrimaryDisplayContentScale();
```

- `za::VideoMode` includes a `pixelDensity` field reflecting the HiDPI scale for each video mode.

- Zancle does **not** automatically scale your coordinates -- you must apply `getDisplayScale()` manually for UI elements, text sizing, etc.



## Windows Are Not Closable Anymore

- `za::Window::isOpen()` has been removed. A window object's lifetime dictates its existence.
    - If you need to represent a window that might be closed or destroyed, wrap it in an `zb::Optional<za::RenderWindow>`.

```cpp
//
// BEFORE (upstream SFML)
za::RenderWindow window(...);
while (window.isOpen())
{
    if (event.type == za::Event::Closed)
        window.close();
}

//
// AFTER (Zancle)
auto window = za::RenderWindow::create(...).value();

while (true)
{
    while (const auto event = window.pollEvent())
        if (event->is<za::Event::Closed>())
            return 0;
}

// Window is destroyed (closed) when it goes out of scope here.
```



## Type Renames

- The math vector types have been shortened to align with standard graphics terminology and for conciseness.

| Upstream SFML   | Zancle       |
|-----------------|--------------|
| `za::Vector2i`  | `za::Vec2i`  |
| `za::Vector2u`  | `za::Vec2u`  |
| `za::Vector2f`  | `za::Vec2f`  |
| `za::Vector3i`  | `za::Vec3i`  |
| `za::Vector3u`  | `za::Vec3u`  |
| `za::Vector3f`  | `za::Vec3f`  |
| `za::IntRect`   | `za::Rect2i` |
| `za::FloatRect` | `za::Rect2f` |

- Additional type aliases exist: `za::Vec2uz`, `za::Vec3uz` (for `base::SizeT` components), `za::Rect2u`, `za::Rect2uz`.

- Fixed-width integer types live in `za::base`:

| Upstream SFML | Zancle          |
|---------------|-----------------|
| `za::Int8`    | `zb::I8`  |
| `za::Uint8`   | `zb::U8`  |
| `za::Int16`   | `zb::I16` |
| `za::Uint16`  | `zb::U16` |
| `za::Int32`   | `zb::I32` |
| `za::Uint32`  | `zb::U32` |
| `za::Int64`   | `zb::I64` |
| `za::Uint64`  | `zb::U64` |



## Sound Is Non-Movable

- `za::Sound` is neither copyable nor movable.
    - It maintains internal references to a `PlaybackDevice` and a `SoundBuffer`, and contains embedded miniaudio state that must remain at a stable memory address.

- To manage collections of sounds, use `zb::InPlaceVector` with `reEmplaceByIterator` to reuse slots:

```cpp
//
// Pool of up to 256 concurrent sounds
zb::InPlaceVector<za::Sound, 256> sounds;

// Play a new sound by emplacing it at the end
sounds.emplaceBack(playbackDevice, buffer).play();

// Reuse a stopped slot for a new sound
auto* it = zb::findIf(sounds.begin(), sounds.end(),
    [](const za::Sound& s) { return !s.isPlaying(); });

if (it != sounds.end())
    sounds.reEmplaceByIterator(it, playbackDevice, buffer).play();
```

- For a single optional sound, use `zb::Optional<za::Sound>` with `.emplace(...)` to reconstruct in-place.



## Music Is Non-Movable

- `za::Music` is neither copyable nor movable for the same reasons as `za::Sound` (stable references, streaming thread).

- The same patterns apply: use `zb::Optional<za::Music>` for single instances, or `zb::InPlaceVector` for collections.

```cpp
zb::Optional<za::Music> bgMusic;

// Start playing
bgMusic.emplace(playbackDevice, musicReader);
bgMusic->play();

// Switch to a different track (destroys old, constructs new in-place)
bgMusic.emplace(playbackDevice, otherMusicReader);
bgMusic->play();
```



## MusicReader: Decoupled Music Source

- Music loading and playback are now split into two objects:
    - `za::MusicReader` -- owns the audio file/stream/memory source, is **movable**.
    - `za::Music` -- performs playback from a `MusicReader`, is **non-movable**.

- The `MusicReader` must outlive any `Music` instances that reference it.

```cpp
//
// BEFORE (upstream SFML)
za::Music music;
music.openFromFile("music.ogg");
music.play();

//
// AFTER (Zancle)
auto musicReader = za::MusicReader::openFromFile("music.ogg").value();
za::Music music(playbackDevice, musicReader);
music.play();
```

- This separation means you can keep the `MusicReader` alive and construct/destroy `Music` instances freely (e.g. to switch playback devices).



## `SoundStream` Is A Template, Not An Abstract Base

- `za::SoundStream` is now a class template parameterized by a user-provided `State` type, instead of an abstract base class to inherit from. You write a plain `struct` that exposes the hook methods and hand it to `za::SoundStream<MyState>`.

- The hooks (invoked on the audio thread):

```cpp
bool onGetData(zb::Vector<zb::I16>& outBuffer); // required
void onSeek(za::Time timeOffset);                           // optional
zb::Optional<zb::U64> onLoop();                 // optional
```

- `onSeek` and `onLoop` are detected via `requires`. Omit `onSeek` if your generator can't seek; omit `onLoop` if it never loops.

- `onGetData` now writes samples **into the provided `outBuffer` by reference** instead of returning a `Chunk` pointing at derived-class storage. The buffer is owned by the base class, so there is no pointer escape into the audio thread.

```cpp
//
// BEFORE (upstream SFML)
class MyStream : public za::SoundStream
{
public:
    MyStream() { initialize(channelCount, sampleRate, channelMap); }

private:
    bool onGetData(Chunk& data) override
    {
        data.samples     = m_samples.data();
        data.sampleCount = m_samples.size();
        return true;
    }

    void onSeek(za::Time) override {}

    std::vector<za::Int16> m_samples;
};

MyStream stream;
stream.play();

//
// AFTER (Zancle)
struct MyState
{
    bool onGetData(zb::Vector<zb::I16>& outBuffer)
    {
        outBuffer.resize(1024); // fill into the base-owned buffer
        // ...fill `outBuffer` with 1024 samples...
        return true; // keep streaming (false to stop)
    }

    // No `onSeek` needed -- omit it entirely for generators that can't seek.
    // No `onLoop` needed -- omit it entirely for sources that don't loop.
};

za::SoundStream<MyState> stream(playbackDevice,
                                za::ChannelMap{za::SoundChannel::Mono},
                                44'100u);
stream.play();
```

- The channel map and sample rate are passed to the `SoundStream` constructor directly -- no more `initialize(...)` call in the derived class's constructor.

- Extra constructor arguments after `sampleRate` are forwarded to `MyState`, so `MyState` can hold non-movable members (mutexes, atomics) without requiring the caller to materialize a `MyState` value first:

```cpp
struct MyState
{
    std::mutex mutex;
    zb::Vector<zb::I16> samples;

    MyState(int seed) : samples(seed) {}

    bool onGetData(zb::Vector<zb::I16>& outBuffer) { /* ... */ }
};

za::SoundStream<MyState> stream(playbackDevice, channelMap, sampleRate,
                                /* forwarded to MyState ctor: */ 42);
```

- Destruction is safe by construction. `~SoundStream<State>` drains the audio thread **before** `State` is destroyed, so the audio callback can never touch freed memory.

- Looping is controlled via the standard `setLooping(bool)` inherited from `MiniaudioSoundSource`. When streaming reaches EOF and looping is enabled, the base calls `state.onLoop()`; returning `zb::nullOpt` stops playback, returning a sample offset resumes from there.



## ContextSettings Without Antialiasing And sRGB Support

- `za::ContextSettings` no longer accepts `antialiasingLevel` or `sRgbCapable` for standard window creation.
    - Relying on the OS window manager for MSAA and sRGB is historically buggy and inconsistent across drivers.
    - Instead, Zancle encourages rendering to an `za::RenderTexture` created with `za::RenderTextureCreateSettings` (where MSAA and sRGB are strictly controlled via FBOs), and blitting the final result to the window.



## CoordinateType Is Gone

- `za::CoordinateType` has been removed. Texture coordinates in `za::Vertex` are now **always in pixel units** (not normalized).

- The default vertex shader automatically normalizes pixel coordinates to `[0, 1]` via a precomputed inverse texture size uniform:

```glsl
za_v_texCoord = za_a_texCoord * za_u_invTextureSize;
```

- If you had code that switched between `CoordinateType::Pixels` and `CoordinateType::Normalized`, simply remove those switches -- pixel coordinates are always used now.

```cpp
//
// BEFORE (upstream SFML)
za::Vertex vertex;
vertex.texCoords = {0.5f, 0.5f}; // Could be normalized or pixels depending on CoordinateType

//
// AFTER (Zancle)
za::Vertex vertex;
vertex.texCoords = {128.f, 128.f}; // Always pixel coordinates (e.g. center of a 256x256 texture)
```

- If you write custom shaders, note that the vertex shader receives pixel-space texture coordinates and must normalize them (the default shader does this for you).



## Views Are Not Stateful Anymore

- Views are no longer stored as persistent state on `za::RenderTarget`. Instead, they are passed **per draw call** via `za::RenderStates`.

```cpp
//
// BEFORE (upstream SFML)
za::View gameView({0.f, 0.f, 800.f, 600.f});
window.setView(gameView);
window.draw(sprite);  // uses gameView
window.draw(text);    // uses gameView

za::View uiView = window.getDefaultView();
window.setView(uiView);
window.draw(button);  // uses uiView

//
// AFTER (Zancle)
za::View gameView = za::View::fromRect({{0.f, 0.f}, {800.f, 600.f}});
window.draw(sprite, {.view = gameView, .texture = &texture});
window.draw(text,   {.view = gameView});

za::View uiView = window.computeView(); // default view matching window size
window.draw(button, {.view = uiView});
```

- Use `window.computeView()` to get a default view matching the current window size (replaces `getDefaultView()`).

- Coordinate conversion now takes the target size as a parameter:

```cpp
//
// BEFORE (upstream SFML)
za::Vec2f worldPos = window.mapPixelToCoords(mousePos, gameView);

//
// AFTER (Zancle)
za::Vec2f worldPos = gameView.screenToWorld(mousePos.toVec2f(), window.getSize().toVec2f());
za::Vec2f screenPos = gameView.worldToScreen(entityPos, window.getSize().toVec2f());
```



## Event System Overhaul

- The event system has been completely redesigned from a C-style union to a **type-safe tagged variant**.

- `pollEvent()` now returns `zb::Optional<za::Event>` instead of taking an output parameter.

```cpp
//
// BEFORE (upstream SFML)
za::Event event;
while (window.pollEvent(event))
{
    if (event.type == za::Event::Closed)
        window.close();
    else if (event.type == za::Event::KeyPressed)
        handleKey(event.key.code);
}

//
// AFTER (Zancle)
while (const auto event = window.pollEvent())
{
    if (event->is<za::Event::Closed>())
        return 0;
    else if (const auto* keyPress = event->getIf<za::Event::KeyPressed>())
        handleKey(keyPress->scancode);
}
```

- Each event type is a separate struct (e.g. `za::Event::KeyPressed`, `za::Event::MouseMoved`, etc.) with named fields.

- Three ways to check event types:
    - `event->is<za::Event::Closed>()` -- boolean check.
    - `event->getIf<za::Event::KeyPressed>()` -- returns pointer to data, or `nullptr`.
    - `event->visit(visitor)` -- full visitor pattern.

- **Bulk event handling** is supported via `pollAndHandleEvents`:

```cpp
window.pollAndHandleEvents(
    [&](za::Event::Closed) { mustClose = true; },
    [&](const za::Event::KeyPressed& e) { handleKey(e.scancode); },
    [&](const za::Event::MouseMoved& e) { handleMouse(e.position); }
    // Unhandled event types are silently ignored.
);
```

- New event type: `za::Event::MouseMovedRaw` provides unprocessed mouse delta input (no acceleration/smoothing), useful for camera control in 3D/first-person views.



## Window Styles Replaced By Booleans

- `za::Style` bitfield flags have been replaced by individual boolean fields in `za::WindowSettings`.

```cpp
//
// BEFORE (upstream SFML)
za::RenderWindow window(videoMode, "Title", za::Style::Titlebar | za::Style::Close);

//
// AFTER (Zancle)
auto window = za::RenderWindow::create({.size{800u, 600u},
                                        .title = "Title",
                                        .resizable = false,
                                        .closable = true,
                                        .hasTitlebar = true}).value();
```

- Available boolean fields: `fullscreen`, `resizable`, `closable`, `hasTitlebar`.



## `za::String` Replaced By `za::Utf8String`

- `za::String` has been replaced by `za::Utf8String`, a UTF-8-only string with codepoint iteration support.

- Used throughout the API for window titles, clipboard text, keyboard descriptions, and joystick names -- all of which natively use UTF-8 at the OS / SDL boundary, so no conversion is required.



## Angle Type

- Rotation values are now represented by the dedicated `za::Angle` type instead of raw `float`.

```cpp
//
// BEFORE (upstream SFML)
sprite.setRotation(45.f);
float rot = sprite.getRotation();

//
// AFTER (Zancle)
sprite.rotation = za::degrees(45.f);
float rot = sprite.rotation.asDegrees();
```

- Factory functions: `za::degrees(float)`, `za::radians(float)`.
- Methods: `.asDegrees()`, `.asRadians()`, `.wrapSigned()`, `.wrapUnsigned()`.



## Clock Pause/Resume Support

- `za::Clock` now supports pausing and resuming.

```cpp
za::Clock clock;

clock.stop();               // Pause the clock
// ...
clock.start();              // Resume the clock

Time t1 = clock.restart();  // Reset and keep running
Time t2 = clock.reset();    // Reset and leave paused

bool running = clock.isRunning();
```

- `za::Clock::now()` provides access to the raw monotonic clock without needing an instance.



## Color HSL Support

- `za::Color` now supports HSL (Hue, Saturation, Lightness) color model conversion.

```cpp
// Create a color from HSL
za::Color color = za::Color::fromHSLA({.hue = 120.f, .saturation = 1.f, .lightness = 0.5f});

// Convert to HSL
za::Color::HSL hsl = color.toHSL();

// Hue rotation
za::Color rotated = color.withRotatedHue(90.f);

// Adjust saturation/lightness
za::Color desaturated = color.withSaturation(0.2f);
za::Color darker      = color.withLightness(0.3f);
```

- Convenience factories: `za::Color::whiteWithAlpha(alpha)`, `za::Color::blackWithAlpha(alpha)`.
- Packed integer: `za::Color::fromRGBA(0xFF0000FF)` for opaque red.



## Font And Text API Changes

- `za::Font` uses `openFromFile` (not `loadFromFile`) and returns `zb::Optional<za::Font>`.
    - Fonts optionally accept a `za::TextureAtlas*` for shared atlas packing:

```cpp
//
// BEFORE (upstream SFML)
za::Font font;
font.loadFromFile("font.ttf");
za::Text text(font, "Hello", 30);
text.setFillColor(za::Color::Red);

//
// AFTER (Zancle)
auto font = za::Font::openFromFile("font.ttf").value();
za::Text text(font, {.string = "Hello",
                      .characterSize = 30u,
                      .fillColor = za::Color::Red});
```

- Text styling uses individual booleans instead of a style enum:

```cpp
//
// BEFORE (upstream SFML)
text.setStyle(za::Text::Bold | za::Text::Italic);

//
// AFTER (Zancle)
za::Text text(font, {.string = "Hello",
                      .bold = true,
                      .italic = true});
```

- Fonts are **not copyable** -- only movable.

- Construction of `za::Text` from a temporary font is a **deleted overload** to prevent dangling references.



## New Shape Types

- Zancle adds several new shape types beyond the original circle, rectangle, and convex shapes:

| Shape             | Data Struct                     |
|-------------------|---------------------------------|
| Ellipse           | `za::EllipseShapeData`          |
| Ring (annulus)    | `za::RingShapeData`             |
| Pie slice         | `za::PieSliceShapeData`         |
| Ring pie slice    | `za::RingPieSliceShapeData`     |
| Rounded rectangle | `za::RoundedRectangleShapeData` |
| Star              | `za::StarShapeData`             |
| Arrow             | `za::ArrowShapeData`            |
| Curved arrow      | `za::CurvedArrowShapeData`      |

- All shapes follow the designated-initializer aggregate pattern for construction.



## Keyboard And Mouse API Changes

- `za::Keyboard::Scancode` has been greatly expanded with 200+ scan codes from SDL3.

- New utility functions:
    - `za::Keyboard::localize(Scancode)` -- convert scancode to virtual key.
    - `za::Keyboard::delocalize(Key)` -- convert virtual key to scancode.
    - `za::Keyboard::getDescription(Scancode)` -- human-readable key name.
    - `za::Keyboard::setVirtualKeyboardVisible(bool)` -- for mobile/touch platforms.

- Touch input now provides device enumeration via `za::Touch::getDevices()` and `za::Touch::Device` structs.



## Lifetime Tracking

- Zancle has an optional compile-time lifetime tracking system (`ZA_ENABLE_LIFETIME_TRACKING`) that catches dangling references in debug builds.

- For example, constructing an `za::Text` from a temporary `za::Font` is a compile error (deleted overload). At runtime, if a `SoundBuffer` is destroyed while a `Sound` still references it, the lifetime tracker will assert.

- This prevents the "white square problem" from upstream SFML where a sprite would silently render incorrectly after its texture was destroyed.
