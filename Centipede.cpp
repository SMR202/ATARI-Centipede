#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

// Game states
enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    INSTRUCTIONS,
    HIGH_SCORES
};

// Initializing Dimensions.
const int resolutionX = 960;
const int resolutionY = 960;
const int boxPixelsX = 32;
const int boxPixelsY = 32;
const int gameRows = resolutionX / boxPixelsX; // Total rows on grid
const int gameColumns = resolutionY / boxPixelsY; // Total columns on grid

// Initializing GameGrid.
int gameGrid[gameRows][gameColumns] = {};

// The following exist purely for readability.
const int x = 0;
const int y = 1;
const int exists = 2;

// Constants for animations
const float ANIMATION_FRAME_TIME = 0.1f; // Time to switch animation frames (seconds)

// Structure for animations
struct Animation {
    int currentFrame = 0;
    int totalFrames = 0;
    float elapsedTime = 0.0f;
    bool isPlaying = false;
};

// Structure for player data
struct PlayerData {
    float position[2]; // x, y
    Animation animation;
    int lives;
    int score;
    string name;
    bool isMoving;
    bool isInvulnerable;
    float invulnerabilityTime;
};

// Structure for high score entries
struct HighScoreEntry {
  string name;
  int score;
  
  // Default constructor
  HighScoreEntry() : name(""), score(0) {}
  
  // Constructor for convenience
  HighScoreEntry(string n, int s) : name(n), score(s) {}
  
  // Operator for sorting
  bool operator<(const HighScoreEntry& other) const {
    return score > other.score; // Sort in descending order
  }
};

// Global variables for high scores
vector<HighScoreEntry> highScores;
const string HIGH_SCORE_FILE = "high_scores.txt";
const int MAX_HIGH_SCORES = 10;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Function declarations                                                   //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

// Helper functions
void initializeGame(PlayerData& player, float centipede[][10], float centipedeheads[][10], float mush[][6], int& nmush, 
                   float& flea, float spider[], float scorpion[], int& centipedeLength, int& level, int& heads);
void loadHighScores();
void saveHighScores(const string& playerName, int score);
void checkForHighScore(PlayerData& player);

// Menu functions
void handleMenuInput(GameState& gameState, sf::RenderWindow& window);
void drawMenu(sf::RenderWindow& window, sf::Font& font, const vector<string>& menuOptions, int selectedOption);
void drawInstructions(sf::RenderWindow& window, sf::Font& font);
void drawHighScores(sf::RenderWindow& window, sf::Font& font);
void drawGameOver(sf::RenderWindow& window, sf::Font& font, PlayerData& player);
void drawPauseMenu(sf::RenderWindow& window, sf::Font& font);

// Animation functions
void updateAnimation(Animation& anim, float deltaTime);
void setupAnimations(PlayerData& player);

// Gameplay functions
void drawPlayer(sf::RenderWindow& window, PlayerData& player, sf::Sprite& playerSprite, float deltaTime);
void moveBullet(float bullet[], sf::Clock& bulletClock);
void drawBullet(sf::RenderWindow& window, float bullet[], sf::Sprite& bulletSprite, float mush[][6], int nmush, int& score);
void movePlayer(PlayerData& player, bool moveLeft, bool moveRight, bool moveUp, bool moveDown, float playerSpeed, float mush[][6], int nmush, float deltaTime);
void mushrooms(sf::RenderWindow& window, float mush[][6], sf::Sprite& mushSprite, sf::Texture& mushTexture, int nmush);
void moveCentipede(int centipedeLength, float centipede[][10], float mush[][6], int nmush, sf::RenderWindow& window, sf::Sprite& centipedeSprite, sf::Sprite& cheadSprite, float deltaTime);
void drawCentipede(sf::RenderWindow& window, sf::Sprite& centipedeSprite, sf::Sprite& cheadSprite, int centipedeLength, float centipede[][10], int i, float deltaTime);
bool mushroomxcentipede(int centipedeLength, float centipede[][10], float mush[][6], int nmush, int i);
void bulletxcentipede(int centipedeLength, float centipede[][10], float bullet[3], float mush[][6], int& nmush, sf::Sound& killSound, int& score, int level);
void MakingHeads(int& h, float centipedeheads[][10], float centipede[][10], sf::Clock& headClock, float mush[][6], int nmush, sf::RenderWindow& window, sf::Sprite& cheadSprite, float deltaTime);
void bulletxhead(int& h, float centipedeheads[][10], float bullet[3], float mush[][6], int& nmush, sf::Sound& killSound, int& score);
void isPlayerhit(PlayerData& player, float centipede[][10], int centipedeLength, float mush[][6], int& nmush, float centipedeheads[][10], sf::Sound& hitSound);
void FleasDrop(float flea[5], float mush[][6], int& nmush, sf::RenderWindow& window, sf::Sprite& fleaSprite, float deltaTime);
void moveSpider(float spider[5], float mush[][6], int& nmush, sf::RenderWindow& window, sf::Sprite& spiderSprite, PlayerData& player, float bullet[3], sf::Sound& hitSound, float deltaTime);
void moveScorpion(float scorpion[5], float mush[][6], int& nmush, sf::RenderWindow& window, sf::Sprite& scorpionSprite, PlayerData& player, float bullet[3], float deltaTime);
void nextLevel(int& centipedeLength, float centipede[][10], float mush[][6], int nmush, float flea[5], float spider[5], float scorpion[5], int& score, 
              int startColumn, int startRow, int& level, float centipedeheads[][10], sf::Sound& levelupSound);
void createParticleEffect(sf::RenderWindow& window, float posX, float posY, sf::Color color);
void drawHUD(sf::RenderWindow& window, sf::Font& font, PlayerData& player, int level);

int main() {
    // Set random seed
    srand(time(0));
    
    // Initialize game state
    GameState gameState = MENU;
    
    // Declaring RenderWindow.
    sf::RenderWindow window(sf::VideoMode(resolutionX, resolutionY), "Centipede", sf::Style::Close | sf::Style::Titlebar);
    window.setSize(sf::Vector2u(640, 640)); // Recommended for 1366x768 (768p) displays.
    window.setPosition(sf::Vector2i(100, 0));
    
    // Clock for timing
    sf::Clock gameClock;
    float deltaTime;
    
    // Initializing Background Music.
    sf::Music bgMusic;
    sf::Music menuMusic;
    bgMusic.openFromFile("Music/field_of_hopes.ogg");
    menuMusic.openFromFile("Music/field_of_hopes.ogg");
    menuMusic.setLoop(true);
    bgMusic.setLoop(true);
    menuMusic.setVolume(50);
    bgMusic.setVolume(40);
    menuMusic.play();
    
    // Sound effects
    sf::SoundBuffer bulletSoundBuffer;
    sf::Sound bulletSound;
    bulletSoundBuffer.loadFromFile("Sound Effects/fire1.wav");
    bulletSound.setBuffer(bulletSoundBuffer);
    
    sf::SoundBuffer playerdiedBuffer;
    sf::Sound playerdiedSound;
    playerdiedBuffer.loadFromFile("Sound Effects/death.wav");
    playerdiedSound.setBuffer(playerdiedBuffer);
    
    sf::SoundBuffer killBuffer;
    sf::Sound killSound;
    killBuffer.loadFromFile("Sound Effects/death.wav");
    killSound.setBuffer(killBuffer);
    
    sf::SoundBuffer levelupBuffer;
    sf::Sound levelupSound;
    levelupBuffer.loadFromFile("Sound Effects/1up.wav");
    levelupSound.setBuffer(levelupBuffer);
    
    sf::SoundBuffer hitBuffer;
    sf::Sound hitSound;
    hitBuffer.loadFromFile("Sound Effects/kill.wav");
    hitSound.setBuffer(hitBuffer);
    
    sf::SoundBuffer menuSelectBuffer;
    sf::Sound menuSelectSound;
    menuSelectBuffer.loadFromFile("Sound Effects/newBeat.wav");
    menuSelectSound.setBuffer(menuSelectBuffer);
    
    // Initializing Background.
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    backgroundTexture.loadFromFile("Textures/orange_forest.png");
    backgroundSprite.setTexture(backgroundTexture);
    backgroundSprite.setColor(sf::Color(255, 255, 255, 255 * 0.20)); // Reduces Opacity to 20%
    
    // Menu background
    sf::Texture menuBackgroundTexture;
    sf::Sprite menuBackgroundSprite;
    menuBackgroundTexture.loadFromFile("Textures/menu_background.png");
    menuBackgroundSprite.setTexture(menuBackgroundTexture);
    
    // Menu options
    vector<string> menuOptions = {"Play Game", "Instructions", "High Scores", "Exit"};
    int selectedOption = 0;
    
    // Font loading
    sf::Font font;
    font.loadFromFile("/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf");
    
    // Load high scores
    loadHighScores();
    
    // Initializing Player data
    PlayerData player;
    player.position[x] = (gameColumns / 2) * boxPixelsX;
    player.position[y] = (gameColumns) * boxPixelsY;
    player.animation.totalFrames = 4;
    player.lives = 3;
    player.score = 0;
    player.isInvulnerable = false;
    player.invulnerabilityTime = 0.0f;
    player.name = "Player";
    
    sf::Texture playerTexture;
    sf::Sprite playerSprite;
    playerTexture.loadFromFile("Textures/player.png");
    playerSprite.setTexture(playerTexture);
    playerSprite.setTextureRect(sf::IntRect(0, 0, boxPixelsX, boxPixelsY));
    const float playerSpeed = 200.0f; // Pixels per second
    
    int startRow = rand() % 10; // Random starting row top 10 of centipede
    int level = 1;
    
    // Initializing mushroom and mushroom Sprites.
    float mush[50][6] = {0};
    sf::Texture mushTexture;
    sf::Sprite mushSprite;
    mushTexture.loadFromFile("Textures/mushroom.png");
    mushSprite.setTexture(mushTexture);
    
    int nmush = (rand() % 11 + 20);
    for (int i = 0; i < nmush; i++) {
        mush[i][2] = 0; // Hit counter
        mush[i][3] = false; // No mushrooms exist
        mush[i][4] = false; // mush eat
        mush[i][5] = false; // poisonous?
    }
    
    // Initializing Bullet and Bullet Sprites.
    float bullet[3] = {};
    bullet[x] = player.position[x];
    bullet[y] = player.position[y] - boxPixelsY;
    bullet[exists] = false;
    sf::Clock bulletClock;
    sf::Texture bulletTexture;
    sf::Sprite bulletSprite;
    bulletTexture.loadFromFile("Textures/bullet.png");
    bulletSprite.setTexture(bulletTexture);
    
    // Initialize Centipede
    sf::Clock headClock;
    sf::Texture centipedeTexture;
    sf::Sprite centipedeSprite;
    centipedeTexture.loadFromFile("Textures/c_body_left_walk.png");
    centipedeSprite.setTexture(centipedeTexture);
    sf::Texture cheadTexture;
    sf::Sprite cheadSprite;
    cheadTexture.loadFromFile("Textures/c_head_left_walk.png");
    cheadSprite.setTexture(cheadTexture);
    int centipedeLength = 12;
    float centipede[12][10] = {0};
    int startColumn = gameColumns - centipedeLength;
    
    // Centipede heads array
    float centipedeheads[12][10] = {0};
    int heads = 0;
    
    // Initialize enemies
    // Fleas
    sf::Texture fleaTexture;
    sf::Sprite fleaSprite;
    fleaTexture.loadFromFile("Textures/flea.png");
    fleaSprite.setTexture(fleaTexture);
    float flea[5] = {0};
    flea[x] = 15 * boxPixelsX;
    flea[y] = 0;
    flea[2] = false; // doesn't exist yet
    
    // Spider
    sf::Texture spiderTexture;
    sf::Sprite spiderSprite;
    spiderTexture.loadFromFile("Textures/spider_and_score.png");
    spiderSprite.setTexture(spiderTexture);
    float spider[5] = {0};
    spider[x] = 0;
    spider[y] = 20 * boxPixelsY;
    spider[2] = true; // exists
    spider[3] = true; // direction right
    spider[4] = true; // direction down
    
    // Scorpion
    sf::Texture scorpionTexture;
    sf::Sprite scorpionSprite;
    scorpionTexture.loadFromFile("Textures/scorpion.png");
    scorpionSprite.setTexture(scorpionTexture);
    float scorpion[5] = {0};
    scorpion[x] = 0;
    scorpion[y] = 26 * boxPixelsY;
    scorpion[2] = true; // exists
    scorpion[3] = true; // direction right
    
    // Particle effect texture
    sf::Texture particleTexture;
    particleTexture.loadFromFile("Textures/bullet.png");
    
    // Game initialization function (populates mushrooms, sets up centipede, etc.)
    initializeGame(player, centipede, centipedeheads, mush, nmush, *flea, spider, scorpion, centipedeLength, level, heads);
    
    // Main game loop
    while (window.isOpen()) {
        // Calculate delta time
        deltaTime = gameClock.restart().asSeconds();
        
        // Handle events
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
                window.close();
                return 0;
            }
            
            // Handle key presses
            if (e.type == sf::Event::KeyPressed) {
                if (gameState == MENU) {
                    // Menu controls
                    if (e.key.code == sf::Keyboard::Up) {
                        selectedOption = (selectedOption - 1 + menuOptions.size()) % menuOptions.size();
                        menuSelectSound.play();
                    }
                    else if (e.key.code == sf::Keyboard::Down) {
                        selectedOption = (selectedOption + 1) % menuOptions.size();
                        menuSelectSound.play();
                    }
                    else if (e.key.code == sf::Keyboard::Return) {
                        menuSelectSound.play();
                        switch (selectedOption) {
                            case 0: // Play
                                gameState = PLAYING;
                                menuMusic.stop();
                                bgMusic.play();
                                // Reset game state for a new game
                                initializeGame(player, centipede, centipedeheads, mush, nmush, *flea, spider, scorpion, centipedeLength, level, heads);
                                break;
                            case 1: // Instructions
                                gameState = INSTRUCTIONS;
                                break;
                            case 2: // High Scores
                                gameState = HIGH_SCORES;
                                break;
                            case 3: // Exit
                                window.close();
                                return 0;
                        }
                    }
                }
                else if (gameState == PLAYING) {
                    // In-game controls
                    if (e.key.code == sf::Keyboard::Space) {
                        // Only fire a new bullet if one does not exist
                        if (!bullet[exists]) {
                            bullet[x] = player.position[x] + boxPixelsX/2 - 4; // Center the bullet
                            bullet[y] = player.position[y] - boxPixelsY/2;
                            bullet[exists] = true;
                            bulletSound.play();
                        }
                    }
                    else if (e.key.code == sf::Keyboard::Escape) {
                        gameState = PAUSED;
                        bgMusic.pause();
                    }
                    else if (e.key.code == sf::Keyboard::P) {
                        gameState = PAUSED;
                        bgMusic.pause();
                    }
                }
                else if (gameState == PAUSED) {
                    if (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::P) {
                        gameState = PLAYING;
                        bgMusic.play();
                    }
                }
                else if (gameState == GAME_OVER || gameState == INSTRUCTIONS || gameState == HIGH_SCORES) {
                    if (e.key.code == sf::Keyboard::Escape) {
                        gameState = MENU;
                        if (!menuMusic.getStatus()) {
                            bgMusic.stop();
                            menuMusic.play();
                        }
                    }
                    else if (e.key.code == sf::Keyboard::Return && gameState == GAME_OVER) {
                        // Check for high score and save if needed
                        checkForHighScore(player);
                        gameState = MENU;
                        if (!menuMusic.getStatus()) {
                            bgMusic.stop();
                            menuMusic.play();
                        }
                    }
                }
            }
        }
        
        // Clear the window
        window.clear(sf::Color(0, 0, 0));
        
        // Game state machine
        switch (gameState) {
            case MENU:
                window.draw(menuBackgroundSprite);
                drawMenu(window, font, menuOptions, selectedOption);
                break;
                
            case PLAYING: {
                // Draw background
                window.draw(backgroundSprite);
                
                // Check if player is still alive
                if (player.lives <= 0) {
                    gameState = GAME_OVER;
                    playerdiedSound.play();
                    break;
                }
                
                // Process player movement
                bool moveLeft = false;
                bool moveRight = false;
                bool moveUp = false;
                bool moveDown = false;
                                
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                    moveLeft = true;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                    moveRight = true;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                    moveUp = true;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                    moveDown = true;
                
                // Update invulnerability timer
                if (player.isInvulnerable) {
                    player.invulnerabilityTime -= deltaTime;
                    if (player.invulnerabilityTime <= 0) {
                        player.isInvulnerable = false;
                    }
                }
                
                // Update game elements
                movePlayer(player, moveLeft, moveRight, moveUp, moveDown, playerSpeed, mush, nmush, deltaTime);
                bulletxcentipede(centipedeLength, centipede, bullet, mush, nmush, killSound, player.score, level);
                moveCentipede(centipedeLength, centipede, mush, nmush, window, centipedeSprite, cheadSprite, deltaTime);
                MakingHeads(heads, centipedeheads, centipede, headClock, mush, nmush, window, cheadSprite, deltaTime);
                bulletxhead(heads, centipedeheads, bullet, mush, nmush, killSound, player.score);
                FleasDrop(flea, mush, nmush, window, fleaSprite, deltaTime);
                moveSpider(spider, mush, nmush, window, spiderSprite, player, bullet, hitSound, deltaTime);
                moveScorpion(scorpion, mush, nmush, window, scorpionSprite, player, bullet, deltaTime);
                mushrooms(window, mush, mushSprite, mushTexture, nmush);
                
                if (bullet[exists]) {
                    moveBullet(bullet, bulletClock);
                    drawBullet(window, bullet, bulletSprite, mush, nmush, player.score);
                }
                
                // Draw player
                drawPlayer(window, player, playerSprite, deltaTime);
                
                // Check for collision with enemies
                if (!player.isInvulnerable) {
                    isPlayerhit(player, centipede, centipedeLength, mush, nmush, centipedeheads, hitSound);
                }
                
                // Check for next level
                nextLevel(centipedeLength, centipede, mush, nmush, flea, spider, scorpion, player.score, 
                         startColumn, startRow, level, centipedeheads, levelupSound);
                
                // Award extra lives at certain score thresholds
                static int lastLifeScore = 0;
                if (player.score >= 10000 && lastLifeScore < 10000 ||
                    player.score >= 20000 && lastLifeScore < 20000 ||
                    player.score >= 50000 && lastLifeScore < 50000) {
                    player.lives++;
                    levelupSound.play();
                    lastLifeScore = player.score;
                }
                
                // Cap maximum lives at 6
                if (player.lives > 6) {
                    player.lives = 6;
                }
                
                // Cap maximum score
                if (player.score > 999999) {
                    player.score = 999999;
                }
                
                // Draw HUD last to be on top
                drawHUD(window, font, player, level);
                break;
            }
                
            case PAUSED: {
                // Draw paused game state
                window.draw(backgroundSprite);
                drawPauseMenu(window, font);
                break;
            }
                
            case GAME_OVER: {
                // Draw game over screen
                drawGameOver(window, font, player);
                break;
            }
                
            case INSTRUCTIONS: {
                // Draw instructions screen
                drawInstructions(window, font);
                break;
            }
                
            case HIGH_SCORES: {
                // Draw high scores screen
                drawHighScores(window, font);
                break;
            }
        }
        
        // Display the window
        window.display();
    }
    
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Function implementations                                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

// Helper functions
void initializeGame(PlayerData& player, float centipede[][10], float centipedeheads[][10], float mush[][6], int& nmush, 
                   float& flea, float spider[], float scorpion[], int& centipedeLength, int& level, int& heads) {
    // Reset player
    player.position[x] = (gameColumns / 2) * boxPixelsX;
    player.position[y] = (gameColumns - 1) * boxPixelsY;
    player.lives = 3;
    player.score = 0;
    player.isInvulnerable = false;
    
    // Reset level
    level = 1;
    
    // Reset mushrooms
    nmush = (rand() % 11 + 20);
    for (int i = 0; i < 50; i++) {
        mush[i][2] = 0; // Hit counter
        mush[i][3] = false; // No mushrooms exist
        mush[i][4] = false; // mush eat
        mush[i][5] = false; // poisonous?
    }
    
    int startRow = rand() % 10; // Random starting row
    
    // Create random mushrooms
    for (int i = 0; i < nmush; i++) {
        mush[i][0] = rand() % (resolutionX - boxPixelsX);
        do {
            mush[i][1] = rand() % (resolutionY - 3 * boxPixelsY);
        } while ((int)mush[i][1] / boxPixelsY == startRow || 
                 (int)mush[i][1] / boxPixelsY == startRow + 1 || 
                 (int)mush[i][1] / boxPixelsY == startRow - 1);
        mush[i][3] = true; // Mushroom exists
    }
    
    // Reset centipede
    for (int i = 0; i < centipedeLength; i++) {
        centipede[i][2] = false; // head exists or no
        centipede[i][3] = true; // direction left?
        centipede[i][4] = true; // segment exists?
    }
    centipede[0][2] = true; // first segment is head
    
    // Position centipede
    int startColumn = gameColumns - centipedeLength;
    for (int i = 0; i < centipedeLength; i++) {
        centipede[i][x] = (startColumn + i) * boxPixelsX;
        centipede[i][y] = startRow * boxPixelsY;
    }
    
    // Reset centipede heads
    heads = 0;
    for (int p = 0; p < 12; p++) {
        centipedeheads[p][x] = (gameColumns - 1) * boxPixelsX;
        centipedeheads[p][y] = (gameRows - 3) * boxPixelsX;
        centipedeheads[p][2] = false; // head doesn't exist
        centipedeheads[p][3] = true; // direction left
    }
    
    // Reset flea
    flea = 0;
    flea = 15 * boxPixelsX;
    (&flea)[1] = 0;
    (&flea)[2] = false; // doesn't exist yet
    
    // Reset spider
    spider[x] = 0;
    spider[y] = 20 * boxPixelsY;
    spider[2] = true; // exists
    spider[3] = true; // direction right
    spider[4] = true; // direction down
    
    // Reset scorpion
    scorpion[x] = 0;
    scorpion[y] = 26 * boxPixelsY;
    scorpion[2] = true; // exists
    scorpion[3] = true; // direction right
}

void loadHighScores() {
    highScores.clear();
    ifstream file(HIGH_SCORE_FILE);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string name;
            int score;
            ss >> name >> score;
            highScores.push_back(HighScoreEntry(name, score));
        }
        file.close();
        
        // Sort high scores
        sort(highScores.begin(), highScores.end());
        
        // Truncate to maximum number of scores if needed
        if (highScores.size() > MAX_HIGH_SCORES) {
            highScores.resize(MAX_HIGH_SCORES);
        }
    }
}

void saveHighScores(const string& playerName, int score) {
    // Add new score
    highScores.push_back(HighScoreEntry(playerName, score));
    
    // Sort scores
    sort(highScores.begin(), highScores.end());
    
    // Truncate to maximum
    if (highScores.size() > MAX_HIGH_SCORES) {
        highScores.resize(MAX_HIGH_SCORES);
    }
    
    // Save to file
    ofstream file(HIGH_SCORE_FILE);
    if (file.is_open()) {
        for (const auto& entry : highScores) {
            file << entry.name << " " << entry.score << endl;
        }
        file.close();
    }
}

void checkForHighScore(PlayerData& player) {
    // Check if score qualifies for high score
    if (highScores.size() < MAX_HIGH_SCORES || player.score > highScores.back().score) {
        saveHighScores(player.name, player.score);
    }
}

// Menu functions
void drawMenu(sf::RenderWindow& window, sf::Font& font, const vector<string>& menuOptions, int selectedOption) {
    // Draw title
    sf::Text titleText("CENTIPEDE", font, 60);
    titleText.setStyle(sf::Text::Bold);
    titleText.setFillColor(sf::Color::Green);
    titleText.setPosition(resolutionX / 2 - titleText.getGlobalBounds().width / 2, 120);
    window.draw(titleText);
    
    // Draw subtitle
    sf::Text subtitleText("Enhanced Edition", font, 30);
    subtitleText.setStyle(sf::Text::Italic);
    subtitleText.setFillColor(sf::Color(255, 165, 0)); // Orange
    subtitleText.setPosition(resolutionX / 2 - subtitleText.getGlobalBounds().width / 2, 200);
    window.draw(subtitleText);
    
    // Draw menu options
    for (size_t i = 0; i < menuOptions.size(); i++) {
        sf::Text optionText(menuOptions[i], font, 40);
        if (i == selectedOption) {
            optionText.setFillColor(sf::Color::Red);
        } else {
            optionText.setFillColor(sf::Color::White);
        }
        optionText.setPosition(resolutionX / 2 - optionText.getGlobalBounds().width / 2, 300 + i * 60);
        window.draw(optionText);
    }
}

void drawInstructions(sf::RenderWindow& window, sf::Font& font) {
    // Draw instructions
    sf::Text instructionsText("Instructions:\n\nUse arrow keys to move.\nPress Space to shoot.\nAvoid enemies and obstacles.\n\nPress ESC to return to menu.", font, 30);
    instructionsText.setFillColor(sf::Color::White);
    instructionsText.setPosition(50, 100);
    window.draw(instructionsText);
}

void drawHighScores(sf::RenderWindow& window, sf::Font& font) {
    // Draw high scores
    sf::Text highScoresText("High Scores:", font, 40);
    highScoresText.setFillColor(sf::Color::White);
    highScoresText.setPosition(50, 50);
    window.draw(highScoresText);
    
    for (size_t i = 0; i < highScores.size(); i++) {
        sf::Text scoreText(to_string(i + 1) + ". " + highScores[i].name + " - " + to_string(highScores[i].score), font, 30);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(50, 100 + i * 40);
        window.draw(scoreText);
    }
}

void drawGameOver(sf::RenderWindow& window, sf::Font& font, PlayerData& player) {
    // Draw game over screen
    sf::Text gameOverText("GAME OVER", font, 60);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setPosition(resolutionX / 2 - gameOverText.getGlobalBounds().width / 2, 200);
    window.draw(gameOverText);
    
    sf::Text scoreText("Score: " + to_string(player.score), font, 40);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(resolutionX / 2 - scoreText.getGlobalBounds().width / 2, 300);
    window.draw(scoreText);
    
    sf::Text promptText("Press Enter to return to menu", font, 30);
    promptText.setFillColor(sf::Color::White);
    promptText.setPosition(resolutionX / 2 - promptText.getGlobalBounds().width / 2, 400);
    window.draw(promptText);
}

void drawPauseMenu(sf::RenderWindow& window, sf::Font& font) {
    // Draw pause menu
    sf::Text pauseText("PAUSED", font, 60);
    pauseText.setFillColor(sf::Color::Yellow);
    pauseText.setPosition(resolutionX / 2 - pauseText.getGlobalBounds().width / 2, 200);
    window.draw(pauseText);
    
    sf::Text promptText("Press P or ESC to resume", font, 30);
    promptText.setFillColor(sf::Color::White);
    promptText.setPosition(resolutionX / 2 - promptText.getGlobalBounds().width / 2, 300);
    window.draw(promptText);
}

// Animation functions
void updateAnimation(Animation& anim, float deltaTime) {
    if (anim.isPlaying) {
        anim.elapsedTime += deltaTime;
        if (anim.elapsedTime >= ANIMATION_FRAME_TIME) {
            anim.elapsedTime = 0.0f;
            anim.currentFrame = (anim.currentFrame + 1) % anim.totalFrames;
        }
    }
}

void setupAnimations(PlayerData& player) {
    player.animation.totalFrames = 4;
    player.animation.isPlaying = true;
}

// Gameplay functions
void drawPlayer(sf::RenderWindow& window, PlayerData& player, sf::Sprite& playerSprite, float deltaTime) {
    updateAnimation(player.animation, deltaTime);
    playerSprite.setTextureRect(sf::IntRect(player.animation.currentFrame * boxPixelsX, 0, boxPixelsX, boxPixelsY));
    playerSprite.setPosition(player.position[x], player.position[y]);
    window.draw(playerSprite);
}

void moveBullet(float bullet[], sf::Clock& bulletClock) {
    if (bulletClock.getElapsedTime().asMilliseconds() < 20)
        return;

    bulletClock.restart();
    bullet[y] -= 20; //changed to 20 from 10
    if (bullet[y] < -32)
        bullet[exists] = false;
}

void drawBullet(sf::RenderWindow& window, float bullet[], sf::Sprite& bulletSprite, float mush[][6], int nmush, int& score) {
    bulletSprite.setPosition(bullet[x], bullet[y]);
    window.draw(bulletSprite);
    // Check for collisions with mushrooms
    for (int i = 0; i < nmush; i++) {
        if (mush[i][3] && bullet[x] < mush[i][0] + boxPixelsX && bullet[x] + boxPixelsX > mush[i][0] && bullet[y] < mush[i][1] + boxPixelsY && bullet[y] + boxPixelsY > mush[i][1])
        // ^Bullet within the boundaries(left, right, top, bottom) of the mushroom?
        {

            mush[i][2]++; // Increment the hit counter

            if (mush[i][2] >= 2) {
                // Destroing the mushroom if it has been hit twice
                mush[i][3] = false; // Set mushroom existence to false
                score += 1;
            }

            // Reset the bullet
            bullet[exists] = false;
        }
    }
}

void movePlayer(PlayerData& player, bool moveLeft, bool moveRight, bool moveUp, bool moveDown, float playerSpeed, float mush[][6], int nmush, float deltaTime) {
    float prevPlayerX = player.position[x];
    float prevPlayerY = player.position[y];

    if (moveLeft)
        player.position[x] -= playerSpeed * deltaTime;
    if (moveRight)
        player.position[x] += playerSpeed * deltaTime;
    if (moveUp)
        player.position[y] -= playerSpeed * deltaTime;
    if (moveDown)
        player.position[y] += playerSpeed * deltaTime;

    // Ensure the player stays within the game window
    if (player.position[x] < 0)
        player.position[x] = 0;
    if (player.position[x] > resolutionX - boxPixelsX)
        player.position[x] = resolutionX - boxPixelsX;
    if (player.position[y] < 0)
        player.position[y] = 0;
    if (player.position[y] < ((gameColumns - 5.0) / gameColumns) * (resolutionY - boxPixelsY))
        player.position[y] = ((gameColumns - 5.0) / gameColumns) * (resolutionY - boxPixelsY);
    if (player.position[y] > resolutionY - boxPixelsY)
        player.position[y] = resolutionY - boxPixelsY;

    // Check for collisions with mushrooms
    for (int i = 0; i < nmush; i++) {
        if (mush[i][3] && !mush[i][5]) {
            if (player.position[x] < mush[i][0] + boxPixelsX && player.position[x] + boxPixelsX > mush[i][0] &&
                player.position[y] < mush[i][1] + boxPixelsY && player.position[y] + boxPixelsY > mush[i][1]) {
                // Restore the original position in case of collision with mushrooms
                player.position[x] = prevPlayerX;
                player.position[y] = prevPlayerY;
                break; // Break the loop after handling one collision
            }
        }
    }
}

void mushrooms(sf::RenderWindow& window, float mush[][6], sf::Sprite& mushSprite, sf::Texture& mushTexture, int nmush) {

    for (int i = 0; i < nmush; i++) {

        if (mush[i][3]) {

            if ((int) mush[i][2] == 0 && mush[i][5]) {
                mushSprite.setTextureRect(sf::IntRect(0, boxPixelsY, boxPixelsX, boxPixelsY));
            } else if ((int) mush[i][2] == 1 && mush[i][5]) {
                mushSprite.setTextureRect(sf::IntRect(3 * boxPixelsX, boxPixelsY, boxPixelsX, boxPixelsY));
            } else if ((int) mush[i][2] == 0) {
                mushSprite.setTextureRect(sf::IntRect(0, 0, boxPixelsX, boxPixelsY));
            } else if ((int) mush[i][2] == 1) {
                mushSprite.setTextureRect(sf::IntRect(3 * boxPixelsX, 0, boxPixelsX, boxPixelsY));
            }
            mushSprite.setPosition(mush[i][0], mush[i][1]);
            window.draw(mushSprite);

        }
    }
}

void MakingHeads(int& h, float centipedeheads[][10], float centipede[][10], sf::Clock& headClock, float mush[][6], int nmush, sf::RenderWindow& window, sf::Sprite& cheadSprite, float deltaTime) {

    if ((centipede[0][y] >= resolutionY - 6 * boxPixelsY) && headClock.getElapsedTime().asSeconds() > 5.0) {
        if (h < 12) {
            centipedeheads[h++][2] = true;
        }
        headClock.restart();
    }
    static int hdown = true;
    for (int i = 0; i < 12; i++) {

        if (centipedeheads[i][2]) {
            bool mushexists = mushroomxcentipede(h, centipedeheads, mush, nmush, i);
            // Check if the centipede hits the screen edge or mushrooms
            if (centipedeheads[i][x] < 0 || centipedeheads[i][x] > resolutionX - boxPixelsX || mushexists) {
                centipedeheads[i][3] = !centipedeheads[i][3]; //Changing the direction

                if (centipedeheads[i][y] >= resolutionY - boxPixelsY) //Checking for player box boundaries
                    hdown = false;
                if (centipedeheads[i][y] <= resolutionY - 6 * boxPixelsY)
                    hdown = true;
                // Move hdown a row if hitting the edges    
                if (hdown == true)
                    centipedeheads[i][y] += boxPixelsY;
                else
                    centipedeheads[i][y] -= boxPixelsY;

            }
            if (centipedeheads[i][3] == true) {
                centipedeheads[i][x] = centipedeheads[i][x] - 0.11; //Speed of centipedeheads        
            } else
                centipedeheads[i][x] = centipedeheads[i][x] + 0.11;

            cheadSprite.setTextureRect(sf::IntRect(0, 0, boxPixelsX, boxPixelsY));
            cheadSprite.setPosition(centipedeheads[i][x], centipedeheads[i][y]);
            window.draw(cheadSprite);
        }
    }

}

void moveCentipede(int centipedeLength, float centipede[][10], float mush[][6], int nmush, sf::RenderWindow& window, sf::Sprite& centipedeSprite, sf::Sprite& cheadSprite, float deltaTime) {
    static bool down = true;

    for (int i = 0; i < centipedeLength; i++) {
        bool mushexists = mushroomxcentipede(centipedeLength, centipede, mush, nmush, i);
        // Check if the centipede hits the screen edge or mushrooms
        if (centipede[i][x] < 0 || centipede[i][x] > resolutionX - boxPixelsX || mushexists) {
            centipede[i][3] = !centipede[i][3]; //Changing the direction

            if (centipede[i][y] >= resolutionY - boxPixelsY) //Checking for player box boundaries
                down = false;
            if (centipede[i][y] <= resolutionY - 6 * boxPixelsY)
                down = true;
            // Move down a row if hitting the edges    
            if (down == true)
                centipede[i][y] += boxPixelsY;
            else
                centipede[i][y] -= boxPixelsY;

        }
        if (centipede[i][3] == true) {
            centipede[i][x] = centipede[i][x] - 0.1; //Speed of centipede        
        } else
            centipede[i][x] = centipede[i][x] + 0.1;

        drawCentipede(window, centipedeSprite, cheadSprite, centipedeLength, centipede, i, deltaTime);
    }

}

void drawCentipede(sf::RenderWindow& window, sf::Sprite& centipedeSprite, sf::Sprite& cheadSprite, int centipedeLength, float centipede[][10], int i, float deltaTime) {
    if (centipede[i][2] == true && centipede[i][4]) {
        cheadSprite.setTextureRect(sf::IntRect(0, 0, boxPixelsX, boxPixelsY));
        cheadSprite.setPosition(centipede[i][x], centipede[i][y]);
        window.draw(cheadSprite);
    } else if (centipede[i][4]) {
        centipedeSprite.setTextureRect(sf::IntRect(0, 0, boxPixelsX, boxPixelsY));
        centipedeSprite.setPosition(centipede[i][x], centipede[i][y]);
        window.draw(centipedeSprite);
    }
}

bool mushroomxcentipede(int centipedeLength, float centipede[][10], float mush[][6], int nmush, int i) {

    for (int j = 0; j < nmush; j++) {
        if (centipede[i][4]) {
            if (mush[j][3] && centipede[i][x] < mush[j][0] + boxPixelsX && centipede[i][x] + boxPixelsX > mush[j][0] && centipede[i][y] < mush[j][1] + boxPixelsY && centipede[i][y] + boxPixelsY > mush[j][1]) {
                // ^if Centipede segment collided with a mushroom, move down a row,Perimeters logic same as bullet hits mush
                return true;
            }
        }
    }
    return false;
}

void bulletxcentipede(int centipedeLength, float centipede[][10], float bullet[3], float mush[][6], int& nmush, sf::Sound& killSound, int& score, int level) {

    for (int i = 0; i < centipedeLength; i++) {

        if (centipede[i][4]) {
            if (bullet[exists] && bullet[x] < centipede[i][x] + boxPixelsX && bullet[x] + boxPixelsX > centipede[i][x] && bullet[y] < centipede[i][y] + boxPixelsY && bullet[y] + boxPixelsY > centipede[i][y]) {

                if (centipede[i][y] >= resolutionY - 6 * boxPixelsY) {
                    // Add a new mushroom where the bullet hit
                    mush[nmush][0] = centipede[i][x]; // X position
                    mush[nmush][1] = centipede[i][y]; // Y position
                    mush[nmush][2] = 0; // Hit counter
                    mush[nmush][3] = true; // Mushroom exists
                    mush[nmush][4] = false; // Mush eat
                    mush[nmush][5] = true; //Poisonous
                    nmush++;
                }
                if (centipede[i][2]) {
                    score += 20;
                } else {
                    score += 10;
                }
                // ^if Bullet hit a centipede segment, v split the centipede
                centipede[i][4] = false;
                centipede[i + 1][2] = true; //new head
                bullet[exists] = false; // Reset the bullet

                if (centipede[i][2]) //only when earlier levels to get rid of all segments of one centipede
                {
                    killSound.play();
                    int j = i + 1;
                    while (centipede[j][4] && j < centipedeLength) {
                        centipede[j][4] = false;
                        j++;
                    }

                }
            }

        }

    }
}

void bulletxhead(int& h, float centipedeheads[][10], float bullet[3], float mush[][6], int& nmush, sf::Sound& killSound, int& score) {
    for (int i = 0; i < h; i++) {

        if (centipedeheads[i][2]) {
            if (bullet[exists] && bullet[x] < centipedeheads[i][x] + boxPixelsX && bullet[x] + boxPixelsX > centipedeheads[i][x] && bullet[y] < centipedeheads[i][y] + boxPixelsY && bullet[y] + boxPixelsY > centipedeheads[i][y]) {

                // Add a new mushroom where the bullet hit
                mush[nmush][0] = centipedeheads[i][x]; // X position
                mush[nmush][1] = centipedeheads[i][y]; // Y position
                mush[nmush][2] = 0; // Hit counter
                mush[nmush][3] = true; // Mushroom exists
                mush[nmush][4] = false; // Mush eat
                mush[nmush][5] = true; //Poisonous
                nmush++;
                score += 20;
                centipedeheads[i][2] = false;
                bullet[exists] = false; // Reset the bullet
                killSound.play();
            }
        }

    }
}

void isPlayerhit(PlayerData& player, float centipede[][10], int centipedeLength, float mush[][6], int& nmush, float centipedeheads[][10], sf::Sound& hitSound) {

    for (int i = 0; i < centipedeLength; i++) {
        if (centipede[i][4]) {
            if (player.position[x] + boxPixelsX > centipede[i][x] && player.position[x] < centipede[i][x] + boxPixelsX && player.position[y] + boxPixelsY > centipede[i][y] && player.position[y] < centipede[i][y] + boxPixelsY) {
                player.lives--;
                player.isInvulnerable = true;
                player.invulnerabilityTime = 2.0f; // 2 seconds of invulnerability
                hitSound.play();
            }
        }
        if (centipedeheads[i][2]) {
            if (player.position[x] + boxPixelsX > centipedeheads[i][x] && player.position[x] < centipedeheads[i][x] + boxPixelsX && player.position[y] + boxPixelsY > centipedeheads[i][y] && player.position[y] < centipedeheads[i][y] + boxPixelsY) {
                player.lives--;
                player.isInvulnerable = true;
                player.invulnerabilityTime = 2.0f; // 2 seconds of invulnerability
                hitSound.play();
            }
        }
    }

    // Check for collisions with poisonous mushrooms
    for (int i = 0; i < nmush; i++) {
        if (mush[i][3] && mush[i][5]) {
            if (player.position[x] < mush[i][0] + boxPixelsX && player.position[x] + boxPixelsX > mush[i][0] &&
                player.position[y] < mush[i][1] + boxPixelsY && player.position[y] + boxPixelsY > mush[i][1]) {

                player.lives--;
                player.isInvulnerable = true;
                player.invulnerabilityTime = 2.0f; // 2 seconds of invulnerability
                hitSound.play();

            }
        }
    }

}

void FleasDrop(float flea[5], float mush[][6], int& nmush, sf::RenderWindow& window, sf::Sprite& fleaSprite, float deltaTime) {
    int MushinArea = 0;
    static bool flag = true; //to overcome floating point equivilace issue
    for (int i = 0; i < nmush; i++) {
        if (mush[i][y] >= resolutionY - 6 * boxPixelsY && mush[i][3]) {
            MushinArea++;
        }
    }
    if (MushinArea == 3) {
        flea[2] = true;
    }
    if (flea[2]) {
        //Draw
        fleaSprite.setTextureRect(sf::IntRect(0, 0, boxPixelsX, boxPixelsY));
        fleaSprite.setPosition(flea[x], flea[y]);
        window.draw(fleaSprite);

        flea[y] += 0.1; //flea speed

        //Trail
        if ((int) flea[y] == 15 * boxPixelsY && flag) {
            for (int i = 0; i < 3; i++) {
                mush[nmush][x] = flea[x];
                mush[nmush][y] = flea[y] + (boxPixelsY + 2) * i;
                mush[nmush][2] = 0; // Hit counter
                mush[nmush][3] = true; // Mushroom exists
                mush[nmush][4] = false; // Mush eat
                mush[nmush][5] = false; //Poisonous?
                nmush++;
            }
            flag = false;
        }

        if (flea[y] > resolutionY - boxPixelsY)
            flea[2] = false;
    }
}

void moveSpider(float spider[5], float mush[][6], int& nmush, sf::RenderWindow& window, sf::Sprite& spiderSprite, PlayerData& player, float bullet[3], sf::Sound& hitSound, float deltaTime) {
    if (spider[2]) {
        static bool died = false;
        static bool kills = false;

        static sf::Clock deathTimer;

        // If the spider is hit by a bullet
        if (bullet[exists] && bullet[x] < spider[x] + boxPixelsX && bullet[x] + boxPixelsX > spider[x] && bullet[y] < spider[y] + boxPixelsY && bullet[y] + boxPixelsY > spider[y]) {
            bullet[exists] = false;
            if (player.position[y] - spider[y] < 100) {
                spiderSprite.setTextureRect(sf::IntRect(3.9 * boxPixelsX, 0, 1.9 * boxPixelsX, 2 * boxPixelsY));
                player.score += 900;
            } else if (player.position[y] - spider[y] < 150) {
                spiderSprite.setTextureRect(sf::IntRect(1.9 * boxPixelsX, 0, 1.9 * boxPixelsX, 2 * boxPixelsY));
                player.score += 600;
            } else {
                spiderSprite.setTextureRect(sf::IntRect(0, 0, 1.9 * boxPixelsX, 2 * boxPixelsY));
                player.score += 300;
            }
            died = true;
            deathTimer.restart();
        }
        // If the spider is not hit by a bullet
        else if (!died) {
            spiderSprite.setTextureRect(sf::IntRect(7.5 * boxPixelsX, 0, 1.9 * boxPixelsX, boxPixelsY));
        }

        // If the spider has been hit and 2 seconds have passed
        if (died && deathTimer.getElapsedTime().asSeconds() > 0.5) {
            spider[2] = false;
            died = false;
        }
        spiderSprite.setPosition(spider[x], spider[y]);
        window.draw(spiderSprite);
        if (died == false) {
            if ((int) spider[x] == 20 * boxPixelsX) {
                spider[3] = false; //left
            } else if ((int) spider[x] == 0) {
                spider[3] = true; //right
            }
            if ((int) spider[y] == resolutionY - 10 * boxPixelsY) {
                spider[4] = true; //down
            } else if ((int) spider[y] == resolutionY - boxPixelsY) {
                spider[4] = false; //up
            }

            if (spider[3]) {
                spider[x] += 0.05;
            } else {
                spider[x] -= 0.05;
            }

            if (spider[4]) {
                spider[y] += 0.05;
            } else {
                spider[y] -= 0.05;
            }

            //Check for collision with spider
            if (!kills && player.position[x] < spider[x] + boxPixelsX && player.position[x] + boxPixelsX > spider[x] && player.position[y] < spider[y] + boxPixelsY && player.position[y] + boxPixelsY > spider[y]) {

                player.lives--;
                player.isInvulnerable = true;
                player.invulnerabilityTime = 2.0f; // 2 seconds of invulnerability
                hitSound.play();
                kills = true;

            }
            //Eating mushrooms
            for (int i = 0; i < nmush; i++) {

                if (mush[i][3]) {
                    if (spider[x] < mush[i][0] + boxPixelsX && spider[x] + boxPixelsX > mush[i][0] && spider[y] < mush[i][1] + boxPixelsY && spider[y] + boxPixelsY > mush[i][1]) {
                        mush[i][3] = false;
                    }
                }
            }
        }
    }
}

void moveScorpion(float scorpion[5], float mush[][6], int& nmush, sf::RenderWindow& window, sf::Sprite& scorpionSprite, PlayerData& player, float bullet[3], float deltaTime) {
    if (scorpion[2]) {

        //draw
        scorpionSprite.setTextureRect(sf::IntRect(0, 0, 2 * boxPixelsX, boxPixelsY));
        scorpionSprite.setPosition(scorpion[x], scorpion[y]);
        window.draw(scorpionSprite);

        if (scorpion[x] < 0 || scorpion[x] > resolutionX - 2 * boxPixelsX) {
            scorpion[3] = !scorpion[3];
        }
        if (scorpion[3]) {
            scorpion[x] += 0.2;
        } else {
            scorpion[x] -= 0.2;
        }
        //Killing scorpion
        if (bullet[exists] && bullet[x] < scorpion[x] + boxPixelsX && bullet[x] + boxPixelsX > scorpion[x] && bullet[y] < scorpion[y] + boxPixelsY && bullet[y] + boxPixelsY > scorpion[y]) {
            bullet[exists] = false;
            scorpion[2] = false;
            player.score += 1000;
        }
        //Poisonous mushrooms
        for (int i = 0; i < nmush; i++) {

            if (mush[i][3]) {
                if (scorpion[x] < mush[i][0] + boxPixelsX && scorpion[x] + boxPixelsX > mush[i][0] && scorpion[y] < mush[i][1] + boxPixelsY && scorpion[y] + boxPixelsY > mush[i][1]) {
                    mush[i][5] = true;
                }
            }
        }

    }

}

void nextLevel(int& centipedeLength, float centipede[][10], float mush[][6], int nmush, float flea[5], float spider[5], float scorpion[5], int& score, 
              int startColumn, int startRow, int& level, float centipedeheads[][10], sf::Sound& levelupSound) {

    bool LevelCheck = true;
    for (int i = 0; i < centipedeLength; i++) {
        if (centipede[i][4]) {
            LevelCheck = false;
            break;
        }
    }
    for (int i = 0; i < centipedeLength; i++) {
        if (centipedeheads[i][2]) {
            LevelCheck = false;
            break;
        }
    }
    if (LevelCheck) {
    levelupSound.play();
        level++;
        for (int i = 0; i < centipedeLength; i++) {
            centipede[i][2] = false; //head exists or no
            centipede[i][3] = true; //direction left?
            centipede[i][4] = true; //segment exists?
        }
        centipede[0][2] = true; //head 
        for (int i = 0; i < centipedeLength; i++) {
            centipede[i][x] = (startColumn + i) * boxPixelsX; // Increase x position for each segment
            centipede[i][y] = startRow * boxPixelsY;
        }
        for (int p = 0; p < 12; p++) {
            centipedeheads[p][x] = (gameColumns - 1) * boxPixelsX;
            centipedeheads[p][y] = (gameRows - 3) * boxPixelsX;
            centipedeheads[p][2] = false;
            centipedeheads[p][3] = true;
        }
        for (int i = 0; i < nmush; i++) {
            if (!mush[i][3])
                score += 5; //Regenerating score
            mush[i][3] = true;
            mush[i][5] = false;
            mush[i][2] = 0;
        }
        flea[x] = 15 * boxPixelsX;
        flea[y] = 0;
        flea[2] = false; //doesn't exist yet

        spider[x] = 0;
        spider[y] = 20 * boxPixelsY;
        spider[2] = true; //exists
        spider[3] = true; //direction right
        spider[4] = true; //direction down

        scorpion[x] = 0;
        scorpion[y] = 26 * boxPixelsY;
        scorpion[2] = true; //exists
        scorpion[3] = true; //direction right

    }
}

void createParticleEffect(sf::RenderWindow& window, float posX, float posY, sf::Color color) {
    // Create particle effect
    sf::CircleShape particle(2);
    particle.setFillColor(color);
    particle.setPosition(posX, posY);
    window.draw(particle);
}

void drawHUD(sf::RenderWindow& window, sf::Font& font, PlayerData& player, int level) {
    // Draw score
    sf::Text scoreText("Score: " + to_string(player.score), font, 24);
    scoreText.setFillColor(sf::Color::Red);
    scoreText.setPosition(10, 9);
    window.draw(scoreText);
    
    // Draw lives
    sf::Text livesText("Lives: " + to_string(player.lives), font, 24);
    livesText.setFillColor(sf::Color::Green);
    livesText.setPosition(10, 35);
    window.draw(livesText);
    
    // Draw level
    sf::Text levelText("Level: " + to_string(level), font, 24);
    levelText.setFillColor(sf::Color::White);
    levelText.setPosition(10, 61);
    window.draw(levelText);
}
