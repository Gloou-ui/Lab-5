#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace
{
constexpr unsigned int windowWidth = 1000;
constexpr unsigned int windowHeight = 700;
constexpr float gravity = 320.f;
constexpr float pi = 3.14159265358979323846f;
constexpr float fadeDuration = 0.8f;

struct Particle
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float age = 0.f;
    float lifetime = 0.f;
    float radius = 0.f;
};

std::optional<sf::Font> loadFont()
{
    const std::vector<std::filesystem::path> candidates = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf"
    };

    for (const auto& path : candidates)
    {
        if (std::filesystem::exists(path))
        {
            sf::Font font;
            if (font.openFromFile(path.string()))
            {
                return font;
            }
        }
    }

    return std::nullopt;
}

void launchBurst(
    std::vector<Particle>& particles,
    sf::Vector2f origin,
    int particleCount,
    float initialSpeed,
    std::mt19937& randomEngine)
{
    particles.clear();
    particles.reserve(static_cast<std::size_t>(particleCount));

    std::uniform_real_distribution<float> angleDistribution(0.f, 2.f * pi);
    std::uniform_real_distribution<float> speedDistribution(0.72f, 1.18f);
    std::uniform_real_distribution<float> lifetimeDistribution(2.2f, 3.8f);
    std::uniform_real_distribution<float> radiusDistribution(2.f, 4.f);
    const std::vector<sf::Color> palette = {
        sf::Color(255, 92, 92),
        sf::Color(255, 195, 70),
        sf::Color(100, 220, 255),
        sf::Color(190, 120, 255),
        sf::Color(130, 255, 150)
    };
    std::uniform_int_distribution<std::size_t> colorDistribution(
        0, palette.size() - 1);

    for (int i = 0; i < particleCount; ++i)
    {
        const float angle = angleDistribution(randomEngine);
        const float speed = initialSpeed * speedDistribution(randomEngine);
        particles.push_back({
            origin,
            {std::cos(angle) * speed, std::sin(angle) * speed},
            palette[colorDistribution(randomEngine)],
            0.f,
            lifetimeDistribution(randomEngine),
            radiusDistribution(randomEngine)
        });
    }
}
}

int main()
{
    auto loadedFont = loadFont();
    if (!loadedFont)
    {
        std::cerr << "Could not find a usable system font for the interface.\n";
        return 1;
    }
    const sf::Font& font = *loadedFont;

    sf::RenderWindow window(
        sf::VideoMode({windowWidth, windowHeight}),
        "SFML Physics Demo");
    window.setFramerateLimit(60);

    std::mt19937 randomEngine(std::random_device{}());
    std::vector<Particle> particles;
    int particleCount = 120;
    float initialSpeed = 230.f;
    bool paused = false;
    sf::Vector2f burstOrigin(windowWidth / 2.f, windowHeight / 2.f);
    float peakRadius = 0.f;

    launchBurst(
        particles, burstOrigin, particleCount, initialSpeed, randomEngine);

    sf::Clock clock;
    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* key =
                         event->getIf<sf::Event::KeyPressed>())
            {
                switch (key->code)
                {
                case sf::Keyboard::Key::Up:
                    initialSpeed = std::clamp(initialSpeed + 20.f, 60.f, 500.f);
                    break;
                case sf::Keyboard::Key::Down:
                    initialSpeed = std::clamp(initialSpeed - 20.f, 60.f, 500.f);
                    break;
                case sf::Keyboard::Key::Right:
                    particleCount = std::clamp(particleCount + 20, 20, 500);
                    break;
                case sf::Keyboard::Key::Left:
                    particleCount = std::clamp(particleCount - 20, 20, 500);
                    break;
                case sf::Keyboard::Key::Space:
                    paused = !paused;
                    break;
                case sf::Keyboard::Key::Enter:
                    launchBurst(
                        particles,
                        burstOrigin,
                        particleCount,
                        initialSpeed,
                        randomEngine);
                    peakRadius = 0.f;
                    break;
                case sf::Keyboard::Key::Escape:
                    window.close();
                    break;
                default:
                    break;
                }
            }
            else if (const auto* mouse =
                         event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouse->button == sf::Mouse::Button::Left)
                {
                    burstOrigin = window.mapPixelToCoords(mouse->position);
                    launchBurst(
                        particles,
                        burstOrigin,
                        particleCount,
                        initialSpeed,
                        randomEngine);
                    peakRadius = 0.f;
                }
            }
        }

        const float dt = std::min(clock.restart().asSeconds(), 0.05f);
        if (!paused)
        {
            for (auto& particle : particles)
            {
                const float movementDt =
                    std::min(dt, std::max(0.f, particle.lifetime - particle.age));
                particle.velocity.y += gravity * movementDt;
                particle.position.x += particle.velocity.x * movementDt;
                particle.position.y += particle.velocity.y * movementDt;
                particle.age += dt;
            }

            particles.erase(
                std::remove_if(
                    particles.begin(),
                    particles.end(),
                    [](const Particle& particle)
                    {
                        return particle.age >= particle.lifetime + fadeDuration;
                    }),
                particles.end());

            for (const auto& particle : particles)
            {
                const float dx = particle.position.x - burstOrigin.x;
                const float dy = particle.position.y - burstOrigin.y;
                peakRadius = std::max(peakRadius, std::sqrt(dx * dx + dy * dy));
            }
        }

        window.clear(sf::Color(12, 16, 32));

        for (const auto& particle : particles)
        {
            const float remaining = std::clamp(
                (particle.lifetime + fadeDuration - particle.age)
                    / fadeDuration,
                0.f,
                1.f);
            sf::CircleShape spark(particle.radius * (0.55f + 0.45f * remaining));
            spark.setOrigin({spark.getRadius(), spark.getRadius()});
            spark.setPosition(particle.position);
            sf::Color color = particle.color;
            color.a = static_cast<std::uint8_t>(255.f * remaining);
            spark.setFillColor(color);
            window.draw(spark);
        }

        sf::RectangleShape panel({510.f, 174.f});
        panel.setPosition({16.f, 16.f});
        panel.setFillColor(sf::Color(0, 0, 0, 175));
        window.draw(panel);

        const std::vector<std::string> lines = {
            "FIREWORKS - " + std::string(paused ? "PAUSED" : "RUNNING"),
            "Initial speed: " + std::to_string(static_cast<int>(initialSpeed))
                + " px/s   (Up/Down)",
            "Particles: " + std::to_string(particleCount) + "   (Right/Left)",
            "Gravity: " + std::to_string(static_cast<int>(gravity)) + " px/s^2",
            "Active: " + std::to_string(particles.size())
                + "   Peak cloud radius: "
                + std::to_string(static_cast<int>(peakRadius)) + " px",
            "Space: pause/resume   Enter: repeat   Click: move burst"
        };

        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            sf::Text text(font, lines[i], 17);
            text.setPosition({28.f, 21.f + static_cast<float>(i) * 25.f});
            text.setFillColor(sf::Color::White);
            window.draw(text);
        }

        window.display();
    }

    return 0;
}
