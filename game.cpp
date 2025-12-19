#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <optional>
#include <ctime>
#include <iostream>

constexpr float PI = 3.14159265f;

// --- Bullet Structure ---
struct Bullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;

    Bullet(sf::Vector2f position, float angle, float speed) {
        shape.setRadius(7.0f);
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin({7.0f, 7.0f});
        shape.setPosition(position);
        float radians = angle * (PI / 180.0f);
        velocity = { std::cos(radians) * speed, std::sin(radians) * speed };
    }
    void update(float dt) { shape.move(velocity * dt * 60.0f); }
};

struct Laser {
    sf::RectangleShape shape;
    float duration;
    sf::Vector2f direction;
    sf::Vector2f startPos;
};

struct BorderLaser {
    sf::RectangleShape shape;
    float timer;
    sf::Vector2f direction;
    sf::Vector2f startPos;
};

struct Turret {
    sf::CircleShape shape;
    float timer;
    bool hasShot;
};

int main() {
    srand(static_cast<unsigned int>(time(NULL)));
    // 1. Setup Window & View
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "Just Dodge Clone", sf::State::Fullscreen);
    window.setFramerateLimit(60);

    float worldW = 1920.f;
    float worldH = 1080.f;
    sf::View gameView(sf::FloatRect({0.f, 0.f}, {worldW, worldH}));
    window.setView(gameView);

    // 2. GAME VARIABLES (Put new timers here)
    bool isGameOver = false;
    float health = 100.f;
    float bossTimer = 0.f;
    float spawnTimer = 0.f;
    float levelTimer = 0.f;
    bool isTransforming = false;
    float transformTimer = 0.f;
    int lastPhase = -1;
    int currentPhase = 1;
    
    std::vector<Bullet> bullets;
    std::vector<Turret> turrets;
    std::vector<Laser> lasers;
    std::vector<BorderLaser> borderLasers;
    std::vector<sf::RectangleShape> warningLines;
    float turretSpawnTimer = 0.f;
    float borderLaserSpawnTimer = 0.f;

    sf::Clock gameClock, hitTimer;

    // 3. Game Objects
    sf::CircleShape player(15.0f);
    player.setFillColor(sf::Color::Cyan);
    player.setOrigin({15.0f, 15.0f});

    sf::RectangleShape boss({80.f, 80.f});
    boss.setOrigin({40.f, 40.f}); // Initial color set in reset function

    sf::RectangleShape deathScreen({worldW, worldH});
    deathScreen.setFillColor(sf::Color(255, 0, 0, 100));

    // HP Bar
    sf::RectangleShape hpBarBack({40.f, 5.f});
    hpBarBack.setFillColor(sf::Color::Red);
    hpBarBack.setOrigin({20.f, 0.f});

    sf::RectangleShape hpBarFront({40.f, 5.f});
    hpBarFront.setFillColor(sf::Color::Green);
    hpBarFront.setOrigin({20.f, 0.f});

    auto resetGame = [&]() {
        health = 100.f;
        bullets.clear();
        turrets.clear();
        lasers.clear();
        borderLasers.clear();
        warningLines.clear();
        turretSpawnTimer = 0.f;
        borderLaserSpawnTimer = 0.f;
        bossTimer = 0.f;
        spawnTimer = 0.f;
        levelTimer = 0.f;
        currentPhase = 1;
        lastPhase = 1;
        isTransforming = false;
        transformTimer = 0.f;
        boss.setFillColor(sf::Color::Magenta);
        player.setPosition({worldW / 2.f, worldH * 0.8f});
        boss.setPosition({worldW / 2.f, worldH * 0.2f});
        isGameOver = false;
        std::cout << "Game Reset!" << std::endl;
    };

    resetGame();

    // --- MAIN LOOP ---
    while (window.isOpen()) {
        float dt = gameClock.restart().asSeconds();

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) window.close();
                if (isGameOver && keyPressed->code == sf::Keyboard::Key::R) resetGame();
            }
        }

        if (!isGameOver) {
            warningLines.clear();
            levelTimer += dt;
            bossTimer += dt;

            int targetPhase = 1;
            
            if (levelTimer > 30.0f) targetPhase = 4;
            else if (levelTimer > 20.0f) targetPhase = 3;
            else if (levelTimer > 10.0f) targetPhase = 2;

            // Detect Phase Changes
            if (targetPhase != currentPhase && !isTransforming) {
                isTransforming = true;
                transformTimer = 0.f;
                currentPhase = targetPhase;
                
                // Move boss to center for transformation
                boss.setPosition({worldW / 2.f, worldH / 2.f});
            }

            if (isTransforming) {
                transformTimer += dt;
                boss.setFillColor(sf::Color::Blue);
                // Keep boss at center during transformation
                boss.setPosition({worldW / 2.f, worldH / 2.f});

                if (transformTimer >= 2.0f) {
                    isTransforming = false;
                    // Reset boss timer so movement patterns start fresh
                    bossTimer = 0.f;
                    }
                } else {
                    if (currentPhase == 1)
                        boss.setFillColor(sf::Color::Magenta);
                    else if (currentPhase == 2) 
                        boss.setFillColor(sf::Color::Yellow);
                    else if (currentPhase == 3) 
                        boss.setFillColor(sf::Color::Green);
                    else if (currentPhase == 4)
                        boss.setFillColor(sf::Color::Red);
        }

            // Move Boss
            if (isTransforming) {
                // Stay at center during transformation
                boss.setPosition({worldW / 2.f, worldH / 2.f});
            }
            else if (currentPhase == 3) {
                // Move along the border
                float speed = 2000.f; // Speed of boss along border
                float perimeter = 2 * (worldW + worldH);
                float dist = std::fmod(bossTimer * speed, perimeter);
                
                if (dist < worldW) { 
                    // Top Edge (Moving Right)
                    boss.setPosition({dist, 40.f}); 
                } else if (dist < worldW + worldH) { 
                    // Right Edge (Moving Down)
                    boss.setPosition({worldW - 40.f, dist - worldW}); 
                } else if (dist < 2 * worldW + worldH) { 
                    // Bottom Edge (Moving Left)
                    boss.setPosition({worldW - (dist - (worldW + worldH)), worldH - 40.f}); 
                } else { 
                    // Left Edge (Moving Up)
                    boss.setPosition({40.f, worldH - (dist - (2 * worldW + worldH))}); 
                }
            } else {
                // Normal Figure-8 Movement
                boss.setPosition({worldW/2.f + std::cos(bossTimer) * 600.f, 300.f + std::sin(bossTimer * 2.f) * 150.f});
            }
            // Move Player
            float pSpeed = 400.f;
            sf::Vector2f mvt{0.f, 0.f};
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) mvt.y -= pSpeed * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) mvt.y += pSpeed * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) mvt.x -= pSpeed * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) mvt.x += pSpeed * dt;
            player.move(mvt);

            // Keep player inside screen
            sf::Vector2f pos = player.getPosition();
            float r = player.getRadius();
            if (pos.x < r) pos.x = r;
            if (pos.x > worldW - r) pos.x = worldW - r;
            if (pos.y < r) pos.y = r;
            if (pos.y > worldH - r) pos.y = worldH - r;
            player.setPosition(pos);

            // Update HP Bar
            hpBarBack.setPosition({pos.x, pos.y + 20.f});
            hpBarFront.setPosition({pos.x, pos.y + 20.f});
            float hpPercent = std::max(0.f, health / 100.f);
            hpBarFront.setSize({40.f * hpPercent, 5.f});

            // Attack Patterns
            spawnTimer += dt;
            
            // Burst logic for Phase 4
            static int p4Burst = 0;
            float fireRate = 0.06f;
            if (currentPhase == 4) {
                fireRate = 0.1f;
            } else if (currentPhase == 3) {
                // Fire fast (0.3s) for first 2 shots, then wait long (1.5s) for the 3rd reset
                fireRate = (p4Burst < 5) ? 0.3f : 0.75f;
            }

            if (spawnTimer > fireRate && !isTransforming) {
                float angle = std::sin(bossTimer) * 360.f;

                if (currentPhase == 1) {
                    bullets.emplace_back(boss.getPosition(), angle, 9.0f);
                    bullets.emplace_back(boss.getPosition(), angle + 180.f, 9.0f);
                } 
                else if (currentPhase == 2) {
                    for (int i = 0; i < 5; i++) {
                        // 1. Top -> Down (Angle 90)
                        float randomX1 = static_cast<float>(std::rand() % (int)worldW);
                        bullets.emplace_back(sf::Vector2f{randomX1, -50.f}, 90.f, 10.0f);

                        // 2. Bottom -> Up (Angle 270)
                        
                    }
                    spawnTimer = 0.0f;
                }
                else if (currentPhase == 4) {
                    // 6-way chaos
                    static float flowerRotation = 0.f;
                    flowerRotation += 15.f;
                    for (int i= 0; i < 360; i += 60) {
                        bullets.emplace_back(boss.getPosition(), flowerRotation + i, 11.0f);
                    }

                    // Border Lasers
                    borderLaserSpawnTimer += dt;
                    if (borderLaserSpawnTimer > 1.5f) {
                        BorderLaser bl;
                        bl.timer = 0.f;
                        
                        // Random border position
                        int side = rand() % 4;
                        if (side == 0) bl.startPos = {static_cast<float>(rand() % (int)worldW), -50.f}; // Top
                        else if (side == 1) bl.startPos = {worldW + 50.f, static_cast<float>(rand() % (int)worldH)}; // Right
                        else if (side == 2) bl.startPos = {static_cast<float>(rand() % (int)worldW), worldH + 50.f}; // Bottom
                        else bl.startPos = {-50.f, static_cast<float>(rand() % (int)worldH)}; // Left

                        // Aim at player
                        sf::Vector2f d = player.getPosition() - bl.startPos;
                        float angle = std::atan2(d.y, d.x);
                        bl.direction = {std::cos(angle), std::sin(angle)};
                        
                        bl.shape = sf::RectangleShape({3000.f, 2.f}); // Initialize shape
                        bl.shape.setOrigin({0.f, 1.f});
                        bl.shape.setPosition(bl.startPos);
                        bl.shape.setRotation(sf::degrees(angle * 180.f / PI));
                        bl.shape.setFillColor(sf::Color(0, 0, 255, 150)); // Blue warning

                        borderLasers.push_back(bl);
                        borderLaserSpawnTimer = 0.f;
                    }
                }

                else if (currentPhase == 3) {
                    // Shotgun Attack: 3 bullets towards center
                    sf::Vector2f spawnPos = boss.getPosition();

                    // Calculate angle towards center of screen
                    float dx = (worldW / 2.f) - spawnPos.x;
                    float dy = (worldH / 2.f) - spawnPos.y;
                    float baseAngle = std::atan2(dy, dx) * 180.f / PI;

                    // Spawn 3 bullets: Center, Left (-15 deg), Right (+15 deg)
                    bullets.emplace_back(spawnPos, baseAngle, 12.0f);
                    bullets.emplace_back(spawnPos, baseAngle - 15.f, 12.0f);
                    bullets.emplace_back(spawnPos, baseAngle + 15.f, 12.0f);
                    bullets.emplace_back(spawnPos, baseAngle - 30.f, 12.0f);
                    bullets.emplace_back(spawnPos, baseAngle + 30.f, 12.0f);

                    // Increment burst counter
                    p4Burst++;
                    if (p4Burst > 2) p4Burst = 0;
                }
                spawnTimer = 0.0f;
            }

            // Independent Turret Spawning for Phase 4
            if (currentPhase == 3 && !isTransforming) {
                turretSpawnTimer += dt;
                if (turretSpawnTimer > 2.0f) {
                    Turret t;
                    t.shape = sf::CircleShape(40.f, 3);
                    t.shape.setFillColor(sf::Color::Yellow);
                    t.shape.setOrigin({40.f, 40.f});
                    t.shape.setPosition({
                        static_cast<float>(rand() % (int)(worldW - 200) + 100),
                        static_cast<float>(rand() % (int)(worldH - 200) + 100)
                    });
                    t.timer = 0.f;
                    t.hasShot = false;
                    turrets.push_back(t);
                    turretSpawnTimer = 0.f;
                }
            }

            // Update Turrets & Lasers
            for (auto& t : turrets) {
                t.timer += dt;
                
                // Track player for 1.2s, then lock aim
                if (t.timer < 1.2f) {
                    sf::Vector2f d = player.getPosition() - t.shape.getPosition();
                    float angle = std::atan2(d.y, d.x) * 180.f / PI;
                    t.shape.setRotation(sf::degrees(angle + 90.f));
                } else {
                    // Warning color before shooting
                    t.shape.setFillColor(sf::Color::Red);
                    
                    // Indicator Line
                    sf::RectangleShape line({2000.f, 2.f});
                    line.setOrigin({0.f, 1.f});
                    line.setPosition(t.shape.getPosition());
                    line.setRotation(t.shape.getRotation() - sf::degrees(90.f));
                    line.setFillColor(sf::Color(0, 0, 255, 100));
                    warningLines.push_back(line);
                }

                // Fire after 1.7s (giving 0.5s reaction time)
                if (t.timer > 1.7f && !t.hasShot) {
                    Laser l;
                    l.duration = 0.5f;
                    l.startPos = t.shape.getPosition();
                    float currentRot = t.shape.getRotation().asDegrees();
                    float rad = (currentRot - 90.f) * PI / 180.f;
                    l.direction = {std::cos(rad), std::sin(rad)};
                    l.shape.setSize({2000.f, 50.f});
                    l.shape.setOrigin({0.f, 25.f});
                    l.shape.setPosition(l.startPos);
                    l.shape.setRotation(sf::degrees(currentRot - 90.f));
                    l.shape.setFillColor(sf::Color::Cyan);
                    lasers.push_back(l);
                    t.hasShot = true;
                }
            }
            turrets.erase(std::remove_if(turrets.begin(), turrets.end(), [](const Turret& t){ return t.timer > 2.5f; }), turrets.end());

            for (auto& l : lasers) {
                l.duration -= dt;
                sf::Vector2f p = player.getPosition();
                sf::Vector2f s = l.startPos;
                sf::Vector2f d = l.direction;
                sf::Vector2f sp = p - s;
                float projection = sp.x * d.x + sp.y * d.y;
                if (projection > 0) {
                    sf::Vector2f closest = s + d * projection;
                    float distSq = std::pow(p.x - closest.x, 2) + std::pow(p.y - closest.y, 2);
                    if (distSq < std::pow(40.f, 2)) {
                         if (hitTimer.getElapsedTime().asSeconds() > 0.15f) {
                            health -= 10.f;
                            hitTimer.restart();
                        }
                        player.setFillColor(sf::Color::White);
                    }
                }
            }
            lasers.erase(std::remove_if(lasers.begin(), lasers.end(), [](const Laser& l){ return l.duration <= 0.f; }), lasers.end());

            // Update Border Lasers (Phase 3)
            for (auto& bl : borderLasers) {
                bl.timer += dt;
                if (bl.timer < 2.0f) {
                    // Warning Phase
                    bl.shape.setFillColor(sf::Color(0, 0, 255, 150)); // Blue
                    bl.shape.setSize({3000.f, 5.f});
                    bl.shape.setOrigin({0.f, 2.5f});
                } else {
                    // Firing Phase
                    bl.shape.setFillColor(sf::Color::Cyan);
                    bl.shape.setSize({3000.f, 50.f});
                    bl.shape.setOrigin({0.f, 25.f});

                    // Collision
                    sf::Vector2f p = player.getPosition();
                    sf::Vector2f s = bl.startPos;
                    sf::Vector2f d = bl.direction;
                    sf::Vector2f sp = p - s;
                    float projection = sp.x * d.x + sp.y * d.y;
                    if (projection > 0) {
                        sf::Vector2f closest = s + d * projection;
                        float distSq = std::pow(p.x - closest.x, 2) + std::pow(p.y - closest.y, 2);
                        if (distSq < std::pow(40.f, 2)) { // 25 radius + 15 player
                             if (hitTimer.getElapsedTime().asSeconds() > 0.15f) {
                                health -= 10.f;
                                hitTimer.restart();
                            }
                            player.setFillColor(sf::Color::White);
                        }
                    }
                }
            }
            borderLasers.erase(std::remove_if(borderLasers.begin(), borderLasers.end(), [](const BorderLaser& bl){ return bl.timer > 2.5f; }), borderLasers.end());

            // 7. Collision & Clean-up
            auto it = std::remove_if(bullets.begin(), bullets.end(), [&](Bullet& b) {
                b.update(dt);
                auto pos = b.shape.getPosition();

                float dx = pos.x - player.getPosition().x;
                float dy = pos.y - player.getPosition().y;
                if (std::sqrt(dx*dx + dy*dy) < (player.getRadius() + b.shape.getRadius())) {
                    if (hitTimer.getElapsedTime().asSeconds() > 0.15f) {
                        health -= 5.f;
                        hitTimer.restart();
                    }
                    player.setFillColor(sf::Color::White);
                } else {
                    player.setFillColor(sf::Color::Cyan);
                }
                return (pos.x < -100 || pos.x > worldW + 100 || pos.y < -100 || pos.y > worldH + 100);
            });
            bullets.erase(it, bullets.end());

            if (health <= 0) isGameOver = true;
        }

        // 8. Rendering
        window.clear(sf::Color(15, 15, 15));
        window.draw(boss);
        for (auto& t : turrets) window.draw(t.shape);
        for (auto& wl : warningLines) window.draw(wl);
        for (auto& bl : borderLasers) window.draw(bl.shape);
        for (auto& l : lasers) window.draw(l.shape);
        window.draw(player);
        window.draw(hpBarBack);
        window.draw(hpBarFront);
        for (auto& b : bullets) window.draw(b.shape);
        if (isGameOver) window.draw(deathScreen);
        window.display();
    }
    return 0;
}