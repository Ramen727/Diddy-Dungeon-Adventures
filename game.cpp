#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <optional>
#include <ctime>
#include <cstdlib>

constexpr float PI = 3.14159265f;

// --- Projectile Structures ---
struct Bullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    sf::Vector2f startPos; 
    bool isPoprock = false;  
    bool isShard = false;    
    bool isPhase3Shot = false; 
    float lifeTime = -1.0f; 
    float aliveTime = 0.f;

    Bullet(sf::Vector2f position, float angle, float speed, float radius, bool pop = false, float life = -1.0f) {
        shape.setRadius(radius);
        shape.setFillColor(sf::Color::Red); // Default fallback color
        shape.setOrigin({radius, radius}); 
        shape.setPosition(position);
        startPos = position;
        isPoprock = pop;
        lifeTime = life;
        
        float radians = angle * (PI / 180.0f);
        velocity = { std::cos(radians) * speed, std::sin(radians) * speed };
    }
    void update(float dt) { 
        shape.move(velocity * dt * 60.0f); 
        aliveTime += dt;
    }
};

struct PlayerBullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    PlayerBullet(sf::Vector2f pos, sf::Vector2f target) {
        shape.setRadius(5.f);
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin({5.f, 5.f});
        shape.setPosition(pos);
        sf::Vector2f dir = target - pos;
        float mag = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        velocity = (mag != 0) ? (dir / mag) * 15.f : sf::Vector2f(0, 0);
    }
    void update() { shape.move(velocity); }
};

int main() {
    srand(static_cast<unsigned int>(time(NULL)));
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Oryx Sanctuary Mini", sf::State::Fullscreen);
    window.setFramerateLimit(60);

    float worldW = 1920.f;
    float worldH = 1080.f;
    sf::View gameView(sf::FloatRect({0.f, 0.f}, {worldW, worldH}));
    window.setView(gameView);

    bool isGameOver = false, isVictory = false;
    float playerHealth = 1000.f, bossMaxHP = 8000.f, bossCurrentHP = bossMaxHP;
    float bossTimer = 0.f, spawnTimer = 0.f, survivalSpawnTimer = 0.f, patternTimer = 0.f, warningTimer = 0.f, survivalTimer = 0.f;
    bool isWarning = false, isSurvival = false, isSurvivalWarning = false;
    int currentPhase = 1, nextThreshold = 1;
    
    std::vector<Bullet> bullets;
    std::vector<PlayerBullet> playerBullets;
    sf::Clock gameClock, hitTimer, shootTimer, shotgunTimer;

    sf::CircleShape player(15.0f);
    player.setFillColor(sf::Color::Cyan);
    player.setOrigin({15.f, 15.f});
    
    sf::RectangleShape boss({80.f, 80.f});
    boss.setOrigin({40.f, 40.f});

    sf::RectangleShape leftVoid, rightVoid;
    leftVoid.setFillColor(sf::Color(20, 0, 40));
    rightVoid.setFillColor(sf::Color(20, 0, 40));

    sf::RectangleShape bHPB({800.f, 20.f});
    bHPB.setFillColor(sf::Color(50, 50, 50));
    bHPB.setOrigin({400.f, 0.f});
    bHPB.setPosition({worldW/2.f, 30.f});
    
    sf::RectangleShape bHPF({800.f, 20.f});
    bHPF.setFillColor(sf::Color::Red);
    bHPF.setOrigin({400.f, 0.f});
    bHPF.setPosition({worldW/2.f, 30.f});
    
    sf::RectangleShape hpB({40.f, 5.f}), hpF({40.f, 5.f});
    hpB.setFillColor(sf::Color::Red);
    hpF.setFillColor(sf::Color::Green);

    auto resetGame = [&]() {
        playerHealth = 1000.f;
        bossCurrentHP = bossMaxHP;
        bullets.clear();
        playerBullets.clear();
        
        bossTimer = spawnTimer = survivalSpawnTimer = patternTimer = warningTimer = survivalTimer = 0.f;
        isSurvival = isSurvivalWarning = isWarning = isGameOver = isVictory = false;
        nextThreshold = 1;
        currentPhase = 1;
        
        player.setPosition({worldW/2.f, worldH * 0.8f});
        boss.setPosition({worldW/2.f, 200.f});
    };
    resetGame();

    while (window.isOpen()) {
        float dt = gameClock.restart().asSeconds();
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
                if ((isGameOver || isVictory) && kp->code == sf::Keyboard::Key::R) {
                    resetGame();
                }
            }
        }

        if (!isGameOver && !isVictory) {
            sf::Vector2f bPos = boss.getPosition();
            sf::Vector2f pPos = player.getPosition();

            if (!isSurvival && !isSurvivalWarning && (bossCurrentHP/bossMaxHP) < (1.0f - (nextThreshold * 0.3f))) {
                isSurvivalWarning = true;
                warningTimer = 0.f;
                bullets.clear();
                survivalSpawnTimer = 0.f;
            }
            
            if (!isSurvival && !isSurvivalWarning) {
                patternTimer += dt;
                float dur = (currentPhase == 6) ? 6.f : 8.f;
                if (patternTimer >= dur) {
                    isWarning = true;
                    warningTimer = 0.f;
                    
                    int nextP;
                    do {
                        nextP = (rand()%6)+1;
                    } while (nextP == currentPhase);
                    
                    currentPhase = nextP;
                    patternTimer = 0.f;
                }
            }

            // --- BOSS STATE & MOVEMENT ---
            if (isSurvivalWarning) {
                warningTimer += dt;
                boss.setFillColor(static_cast<int>(warningTimer * 15) % 2 == 0 ? sf::Color::Blue : sf::Color::Black);
                sf::Vector2f target = (nextThreshold == 1) ? sf::Vector2f(worldW/2.f, 150.f) : sf::Vector2f(worldW/2.f, worldH/2.f);
                boss.move((target - bPos) * 0.05f);
                
                if (warningTimer >= 2.0f) {
                    isSurvivalWarning = false;
                    isSurvival = true;
                    survivalTimer = 0.f;
                    survivalSpawnTimer = 0.f;
                    nextThreshold++;
                }
            } else if (isSurvival) {
                survivalTimer += dt;
                boss.setFillColor(nextThreshold == 4 ? sf::Color(255, 69, 0) : sf::Color::Blue);
                sf::Vector2f target = (nextThreshold == 2) ? sf::Vector2f(worldW/2.f, 150.f) : sf::Vector2f(worldW/2.f, worldH/2.f);
                boss.move((target - bPos) * 0.05f);
                
                if (survivalTimer >= 15.0f) {
                    isSurvival = false;
                    isWarning = true;
                    warningTimer = 0.f;
                    bullets.clear();
                }
            } else if (isWarning) {
                warningTimer += dt;
                sf::Color tc;
                if (currentPhase == 1) tc = sf::Color::Magenta;
                else if (currentPhase == 2) tc = sf::Color::Yellow;
                else if (currentPhase == 3) tc = sf::Color::Green;
                else if (currentPhase == 4) tc = sf::Color::Red;
                else if (currentPhase == 5) tc = sf::Color::White;
                else tc = sf::Color(255, 128, 0);
                
                if (static_cast<int>(warningTimer * 12) % 2 == 0) {
                    boss.setFillColor(sf::Color::White);
                } else {
                    boss.setFillColor(tc);
                }
                
                sf::Vector2f sp = (currentPhase == 2) ? sf::Vector2f(worldW/2.f, 200.f) :
                                  (currentPhase == 3) ? sf::Vector2f(worldW/2.f+400.f, worldH/2.f) :
                                  (currentPhase == 6) ? sf::Vector2f(worldW-100.f, worldH/2.f) :
                                  sf::Vector2f(worldW/2.f, worldH/2.f);
                boss.move((sp - bPos) * 0.08f);
                
                if (warningTimer >= 2.0f) {
                    isWarning = false;
                    bossTimer = 0.f;
                    spawnTimer = 0.f;
                }
            } else {
                bossTimer += dt;
                if (currentPhase == 1 || currentPhase == 5) {
                    boss.move(((sf::Vector2f(worldW/2.f, worldH/2.f)) - bPos) * 0.05f);
                }
                else if (currentPhase == 2) {
                    boss.setPosition({worldW/2.f + std::cos(bossTimer) * 600.f, 200.f + std::sin(bossTimer * 2.f) * 100.f});
                }
                else if (currentPhase == 3) {
                    float a = bossTimer * 1.25f;
                    boss.setPosition({worldW/2.f + std::cos(a) * 400.f, worldH/2.f + std::sin(a) * 400.f});
                }
                else if (currentPhase == 4) {
                    static sf::Vector2f cp = bPos;
                    cp += (pPos - cp) * 0.015f;
                    boss.setPosition(cp + sf::Vector2f(std::cos(bossTimer * 3.f) * 200.f, std::sin(bossTimer * 3.f) * 200.f));
                }
                else if (currentPhase == 6) {
                    float a = -bossTimer * 0.5f;
                    boss.setPosition({worldW/2.f + std::cos(a) * (worldW/2.f - 100.f), worldH/2.f + std::sin(a) * (worldH/2.f - 100.f)});
                }
            }

            // --- PLAYER CONTROLS ---
            sf::Vector2f mvt{0.f, 0.f};
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) mvt.y -= 500.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) mvt.y += 500.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) mvt.x -= 500.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) mvt.x += 500.f * dt;
            player.move(mvt);
            
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && shootTimer.getElapsedTime().asSeconds() > 0.12f) {
                playerBullets.emplace_back(pPos, window.mapPixelToCoords(sf::Mouse::getPosition(window)));
                shootTimer.restart();
            }
            
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) && shotgunTimer.getElapsedTime().asSeconds() > 0.5f) {
                for (int i = 0; i < 360; i += 30) {
                    float r = (float)i * PI / 180.f;
                    playerBullets.emplace_back(pPos, pPos + sf::Vector2f(std::cos(r)*100.f, std::sin(r)*100.f));
                }
                shotgunTimer.restart();
            }
            
            pPos = player.getPosition();
            pPos.x = std::clamp(pPos.x, 15.f, worldW - 15.f);
            pPos.y = std::clamp(pPos.y, 15.f, worldH - 15.f);
            player.setPosition(pPos);

            // --- SHOOTING LOGIC ---
            if (isSurvival) {
                survivalSpawnTimer += dt;
                if (nextThreshold == 2) { // SURVIVAL 1: KNIFE WALL
                    static float wT = 0.f;
                    wT += dt;
                    
                    float cHW = (12.f * 55.f) / 2.f;
                    float lB = (worldW/2.f) - cHW, rB = (worldW/2.f) + cHW;
                    leftVoid.setSize({lB, worldH});
                    rightVoid.setSize({worldW - rB, worldH});
                    rightVoid.setPosition({rB, 0.f});
                    
                    if (pPos.x < lB || pPos.x > rB) playerHealth = 0; // Void Kill
                    
                    if (wT > 1.1f) {
                        int gS = rand() % 10;
                        for (int i = 0; i < 12; ++i) {
                            if (i == gS || i == gS + 1) continue;
                            Bullet b({lB + (i*55.f), boss.getPosition().y}, 90.f, 8.f, 15.f);
                            b.shape.setFillColor(sf::Color(220, 220, 220));
                            b.shape.setScale({0.6f, 1.8f});
                            bullets.push_back(b);
                        }
                        wT = 0.f;
                    }
                } else if (nextThreshold == 3) { // SURVIVAL 2: DENSE CROSS + SIDE STREAMS
                    if (survivalSpawnTimer > 0.08f) {
                        static float cR = 0;
                        cR += 2.0f;
                        
                        for (int i = 0; i < 4; i++) {
                            float a = cR + (i * 90.f);
                            for (float d = 60.f; d < 1200.f; d += 35.f) {
                                float rad = a * PI / 180.f;
                                bullets.push_back(Bullet(boss.getPosition() + sf::Vector2f(std::cos(rad)*d, std::sin(rad)*d), a, 0.0f, 11.0f, false, 0.12f));
                                bullets.back().shape.setFillColor(sf::Color(0, 255, 100));
                            }
                        }
                        
                        // Corrected Color-fixed Side Streams
                        sf::Vector2f sideSpawns[] = { {0, (float)(rand()%(int)worldH)}, {worldW, (float)(rand()%(int)worldH)}, {(float)(rand()%(int)worldW), 0}, {(float)(rand()%(int)worldW), worldH} };
                        float sideAngles[] = { 0.f, 180.f, 90.f, 270.f };
                        for (int k = 0; k < 4; ++k) {
                            Bullet sb(sideSpawns[k], sideAngles[k], 7.f, 7.f);
                            sb.shape.setFillColor(sf::Color(150, 255, 150));
                            bullets.push_back(sb);
                        }
                        survivalSpawnTimer = 0.f;
                    }
                } else if (nextThreshold == 4) { // SURVIVAL 3: MIASMA + TOWERS
                    if (survivalSpawnTimer > 0.05f) {
                        static float tR = 0;
                        tR += 2.0f;
                        
                        for (int i = 0; i < 4; i++) {
                            float w = std::sin(survivalTimer * 5.f + i) * 15.f;
                            bullets.push_back(Bullet(boss.getPosition(), tR + (i*90.f) + w, 7.f, 7.f));
                            bullets.back().shape.setFillColor(sf::Color(255, 150, 0));
                        }
                        
                        static float prT = 0;
                        prT += survivalSpawnTimer;
                        if (prT > 0.7f) {
                            Bullet b(boss.getPosition(), (float)(rand()%360), 4.f, 15.f, true);
                            b.shape.setFillColor(sf::Color::Yellow);
                            bullets.push_back(b);
                            prT = 0;
                        }
                        
                        static float towerT = 0, towerR = 0;
                        towerT += survivalSpawnTimer;
                        towerR += 2.5f;
                        if (towerT > 0.65f) {
                            sf::Vector2f towerPos[] = {{100,100}, {worldW-100,100}, {100,worldH-100}, {worldW-100,worldH-100}};
                            for (auto& tp : towerPos) {
                                for (int j = 0; j < 360; j += 60) {
                                    Bullet tb(tp, (float)j + towerR, 4.5f, 9.0f);
                                    tb.shape.setFillColor(sf::Color(255, 100, 255));
                                    bullets.push_back(tb);
                                }
                            }
                            towerT = 0;
                        }
                        survivalSpawnTimer = 0.f;
                    }
                }
            } else if (!isWarning && !isSurvivalWarning) {
                spawnTimer += dt;
                if (spawnTimer > 0.12f) {
                    // --- NORMAL ATTACK PHASES ---
                    if (currentPhase == 1) { // Spiral
                        static float r = 0;
                        r += 20.f;
                        for (int i = 0; i < 360; i += 30) {
                            bullets.emplace_back(boss.getPosition(), (float)i + r, 5.0f, 7.0f);
                        }
                        spawnTimer = -0.28f;
                    }
                    else if (currentPhase == 2) { // Rain
                        for (int i = 0; i < 12; i++) {
                            bullets.emplace_back(sf::Vector2f{(float)(rand() % (int)worldW), -20.f}, 90.f, 7.f + (rand() % 4), 7.f);
                        }
                        spawnTimer = -0.03f;
                    }
                    else if (currentPhase == 3) { // Gaps
                        sf::Vector2f tp = player.getPosition() - boss.getPosition();
                        float aP = std::atan2(tp.y, tp.x)*180.f/PI;
                        for (int i = 0; i < 360; i += 10) {
                            if (std::abs(std::fmod((float)i - aP + 540.f, 360.f) - 180.f) > 15.f) {
                                Bullet b(boss.getPosition(), (float)i, 16.f, 7.f, false);
                                b.isPhase3Shot = true;
                                b.shape.setFillColor(sf::Color::Green);
                                bullets.push_back(b);
                            }
                        }
                        spawnTimer = -0.05f;
                    }
                    else if (currentPhase == 4) { // Cross
                        static float r = 0;
                        r += 15.f;
                        for (int i = 0; i < 360; i += 60) {
                            bullets.emplace_back(boss.getPosition(), (float)i + r, 13.f, 7.f);
                            bullets.emplace_back(boss.getPosition(), (float)i + r, 8.f, 7.f);
                        }
                        spawnTimer = -0.18f;
                    }
                    else if (currentPhase == 5) { // Shotgun
                        float a = std::atan2(pPos.y - boss.getPosition().y, pPos.x - boss.getPosition().x) * 180.f / PI;
                        for (int i = -3; i <= 3; i++) {
                            Bullet b(boss.getPosition(), a + (i * 15.f), 14.f, 25.f);
                            b.shape.setFillColor(sf::Color(255, 165, 0));
                            bullets.push_back(b);
                        }
                        spawnTimer = -0.38f;
                    }
                    else if (currentPhase == 6) { // Stream
                        float a = std::atan2(pPos.y - boss.getPosition().y, pPos.x - boss.getPosition().x) * 180.f / PI;
                        for (int i = -1; i <= 1; i++) {
                            bullets.emplace_back(boss.getPosition(), a + (i * 10.f), 14.f, 9.f);
                        }
                        spawnTimer = 0.f;
                    }
                }
            }

            // --- COLLISION, EXPLOSIONS, CLEANUP ---
            std::vector<Bullet> shards;
            for (auto it = playerBullets.begin(); it != playerBullets.end(); ) {
                it->update();
                if (it->shape.getGlobalBounds().findIntersection(boss.getGlobalBounds())) {
                    if (!isSurvival) bossCurrentHP -= 15.f;
                    it = playerBullets.erase(it);
                } else if (it->shape.getPosition().x < 0 || it->shape.getPosition().x > worldW || 
                          it->shape.getPosition().y < 0 || it->shape.getPosition().y > worldH) {
                    it = playerBullets.erase(it);
                } else {
                    ++it;
                }
            }
            auto itB = std::remove_if(bullets.begin(), bullets.end(), [&](Bullet& b) {
                b.update(dt); if (b.isPoprock) { float d = std::sqrt(std::pow(b.shape.getPosition().x - b.startPos.x, 2) + std::pow(b.shape.getPosition().y - b.startPos.y, 2)); if (d > 400.f) { for (int j = 0; j < 360; j += 30) { Bullet s(b.shape.getPosition(), (float)j, 5.0f, 6.0f); s.shape.setFillColor(b.shape.getFillColor()); shards.push_back(s); } return true; } }
                if (b.lifeTime > 0 && b.aliveTime >= b.lifeTime) return true; if (b.isPhase3Shot && (std::pow(b.shape.getPosition().x-b.startPos.x,2)+std::pow(b.shape.getPosition().y-b.startPos.y,2) > 850*850)) return true;
                if (b.shape.getGlobalBounds().findIntersection(player.getGlobalBounds())) { if (hitTimer.getElapsedTime().asSeconds() > 0.15f) { playerHealth -= 10.f; hitTimer.restart(); } return true; }
                return (b.shape.getPosition().x < -150 || b.shape.getPosition().x > worldW + 150 || b.shape.getPosition().y < -150 || b.shape.getPosition().y > worldH + 150);
            }); bullets.erase(itB, bullets.end()); bullets.insert(bullets.end(), shards.begin(), shards.end());
            bHPF.setSize({800.f * (std::max(0.f, bossCurrentHP / bossMaxHP)), 20.f});
            hpF.setSize({40.f * (std::max(0.f, playerHealth / 1000.f)), 5.f});
            hpB.setPosition({pPos.x, pPos.y + 25.f});
            hpF.setPosition({pPos.x, pPos.y + 25.f});
            
            if (playerHealth <= 0) isGameOver = true;
            if (bossCurrentHP <= 0) isVictory = true;
        }

        // --- DRAWING ---
        window.clear(sf::Color(10, 10, 15));
        
        if (!isVictory) window.draw(boss);
        
        if (isSurvival && nextThreshold == 2) {
            window.draw(leftVoid);
            window.draw(rightVoid);
        }
        
        if (isSurvival && nextThreshold == 4) {
            sf::RectangleShape tower({40.f, 40.f});
            tower.setOrigin({20.f, 20.f});
            tower.setFillColor(sf::Color(150, 0, 150));
            sf::Vector2f towerPos[] = {{100,100}, {worldW-100,100}, {100,worldH-100}, {worldW-100,worldH-100}};
            for (auto& tp : towerPos) {
                tower.setPosition(tp);
                window.draw(tower);
            }
        }
        
        for (auto& b : bullets) window.draw(b.shape);
        for (auto& pb : playerBullets) window.draw(pb.shape);
        
        window.draw(player);
        window.draw(bHPB);
        window.draw(bHPF);
        window.draw(hpB);
        window.draw(hpF);
        
        if (isGameOver) {
            sf::RectangleShape ds({worldW, worldH});
            ds.setFillColor({255,0,0,100});
            window.draw(ds);
        }
        if (isVictory) {
            sf::RectangleShape vs({worldW, worldH});
            vs.setFillColor({0,255,100,100});
            window.draw(vs);
        }
        
        window.display();
    }
    return 0;
}