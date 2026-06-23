
#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600
#define MAX_LASERS 10
#define MAX_OBSTACLES 5
#define MAX_CLOUDS 4
#define MIDNIGHTBLUE (Color){ 15, 15, 45, 255 }


typedef struct {
    float amplitude;
    float wavelength;
    float speed;
} GerstnerWave;

typedef struct {
    Vector2 position;
    bool active;
} Laser;

typedef struct {
    float x;
    bool active;
    Color color;
} Obstacle;

typedef struct {
    Vector2 position;
    float speed;
    float scale;
} Cloud;

// Global Wave Architectures
GerstnerWave wave1 = { 20.0f, 300.0f, 2.0f };
GerstnerWave wave2 = { 8.0f, 120.0f, 4.0f };

// Math sampler to tie object height directly to the rolling water surface
float GetWaveHeightAt(float target_x, float time, float scrolloffset) {
    float x0 = target_x + scrolloffset;
    for (int i = 0; i < 3; i++) {
        float k1 = (2 * PI) / wave1.wavelength;
        float k2 = (2 * PI) / wave2.wavelength;
        
        float omega = sqrtf(9.8f * k1);
        float omega2 = sqrtf(9.8f * k2);
        
        float phase1 = (k1 * x0) - (omega * time);
        float phase2 = (k2 * x0) - (omega2 * time);
        
        float totalxoffset = wave1.amplitude * sinf(phase1) + wave2.amplitude * sinf(phase2);
        x0 = (target_x + scrolloffset) - totalxoffset;
    }
    
    float k1 = (2 * PI) / wave1.wavelength;
    float k2 = (2 * PI) / wave2.wavelength;
    
    float omega = sqrtf(9.8f * k1);
    float omega2 = sqrtf(9.8f * k2);
    
    float phase1 = (k1 * x0) - (omega * time);
    float phase2 = (k2 * x0) - (omega2 * time);
    
    return 400.0f - (wave1.amplitude * cosf(phase1) + wave2.amplitude * cosf(phase2));
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Presentation: 3-Level Gerstner Ocean Shooter");
    SetTargetFPS(60);

    float time = 0.0f;
    float scrollOffset = 0.0f; 
    int score = 0;
    int currentLevel = 1;
    int lives = 5;
    bool gameOver = false;

    // Progression Modifiers
    float scrollSpeed = 80.0f;
    float obstacleSpeedBonus = 0.0f;

    // Game Entities
    float playerX = 100.0f;
    float boatW = 40.0f, boatH = 20.0f;
    Laser lasers[MAX_LASERS] = { 0 };
    Obstacle obstacles[MAX_OBSTACLES] = { 0 };

    // Initialize Parallax Background Cloud Loop
    Cloud clouds[MAX_CLOUDS];
    for (int i = 0; i < MAX_CLOUDS; i++) {
        clouds[i].position = (Vector2){ (float)(i * 300 + 50), (float)(40 + rand() % 60) };
        clouds[i].speed = (float)(15 + rand() % 20); 
        clouds[i].scale = (float)(30 + rand() % 25);
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (gameOver) {
            // Restart game on ENTER key press
            if (IsKeyPressed(KEY_ENTER)) {
                score = 0;
                currentLevel = 1;
                lives = 5;
                scrollSpeed = 80.0f;
                obstacleSpeedBonus = 0.0f;
                wave1.amplitude = 20.0f;
                wave2.amplitude = 8.0f;
                playerX = 100.0f;
                time = 0.0f;
                scrollOffset = 0.0f;
                for (int i = 0; i < MAX_OBSTACLES; i++) obstacles[i].active = false;
                for (int i = 0; i < MAX_LASERS; i++) lasers[i].active = false;
                gameOver = false;
            }
        } else {
            time += dt;
            scrollOffset += scrollSpeed * dt;

            // --- LEVEL TRANSITION LOGIC ---
            if (score >= 1000 && currentLevel == 1) {
                currentLevel = 2;
                lives = 4;                  // Drops to 4 lives for Level 2
                scrollSpeed = 130.0f;         
                obstacleSpeedBonus = 60.0f;   
                wave1.amplitude = 30.0f;      // Higher waves
                wave2.amplitude = 12.0f;
            } else if (score >= 2000 && currentLevel == 2) {
                currentLevel = 3;
                lives = 3;                  // Drops to 3 lives for Level 3
                scrollSpeed = 180.0f;         
                obstacleSpeedBonus = 120.0f;  
                wave1.amplitude = 42.0f;      // Maximum heavy storm waves
                wave2.amplitude = 18.0f;
            }

            // --- BACKGROUND CLOUD LOOP LOGIC ---
            for (int i = 0; i < MAX_CLOUDS; i++) {
                clouds[i].position.x -= clouds[i].speed * dt;
                if (clouds[i].position.x < -100) {
                    clouds[i].position.x = SCREEN_WIDTH + 50;
                    clouds[i].position.y = (float)(40 + rand() % 60);
                }
            }

            // --- PLAYER CONTROLS ---
            if (IsKeyDown(KEY_LEFT))  playerX -= 250.0f * dt;
            if (IsKeyDown(KEY_RIGHT)) playerX += 250.0f * dt;
            if (playerX < 0) playerX = 0;
            if (playerX > SCREEN_WIDTH - boatW) playerX = SCREEN_WIDTH - boatW;

            float playerY = GetWaveHeightAt(playerX + (boatW / 2.0f), time, scrollOffset);

            // --- LASER LOGIC ---
            if (IsKeyPressed(KEY_SPACE)) {
                for (int i = 0; i < MAX_LASERS; i++) {
                    if (!lasers[i].active) {
                        lasers[i].position = (Vector2){ playerX + boatW, playerY - boatH };
                        lasers[i].active = true;
                        break;
                    }
                }
            }
            for (int i = 0; i < MAX_LASERS; i++) {
                if (lasers[i].active) {
                    lasers[i].position.x += 600.0f * dt;
                    if (lasers[i].position.x > SCREEN_WIDTH) lasers[i].active = false;
                }
            }

            // --- OBSTACLE MANAGEMENT ---
            for (int i = 0; i < MAX_OBSTACLES; i++) {
                if (!obstacles[i].active) {
                    if (rand() % 100 < 2) { 
                        obstacles[i].x = SCREEN_WIDTH + 20;
                        if (currentLevel == 1)      obstacles[i].color = ORANGE;
                        else if (currentLevel == 2) obstacles[i].color = PURPLE;
                        else                        obstacles[i].color = RED;
                        obstacles[i].active = true;
                    }
                } else {
                    obstacles[i].x -= (scrollSpeed + 40.0f + obstacleSpeedBonus) * dt;
                    
                    // If an obstacle leaves the left edge, lose a life
                    if (obstacles[i].x < -40) {
                        obstacles[i].active = false;
                        lives--;
                        if (lives <= 0) {
                            gameOver = true;
                        }
                    }
                }
            }

            // --- ENEMY/LASER COLLISIONS ---
            
            for (int i = 0; i < MAX_LASERS; i++) {
                if (!lasers[i].active) continue;
                for (int j = 0; j < MAX_OBSTACLES; j++) {
                    if (!obstacles[j].active) continue;

                    float obsY = GetWaveHeightAt(obstacles[j].x + 15, time, scrollOffset);
                    Rectangle obsRec = { obstacles[j].x, obsY - 30, 30, 30 };

                    if (CheckCollisionCircleRec(lasers[i].position, 4.0f, obsRec)) {
                        lasers[i].active = false;
                        obstacles[j].active = false;
                        score += 10;
                    }
                }
            }
        }

        // --- RENDER PIPELINE ---
        
        BeginDrawing();
        
        // Dynamic aesthetic shift depending on current level
        
        Color skyColor = SKYBLUE;
        Color waterColor = BLUE;
        Color sunColor = YELLOW;

        if (currentLevel == 2) {
            skyColor = DARKGRAY;
            waterColor = DARKBLUE;
            sunColor = ORANGE;
        } else if (currentLevel == 3) {
            skyColor = BLACK;
            waterColor = MIDNIGHTBLUE;
            sunColor = RED;
        }
        
        ClearBackground(skyColor);
        DrawCircle(850, 90, 45, sunColor); 

        // Render Background Clouds
        for (int i = 0; i < MAX_CLOUDS; i++) {
            DrawCircle((int)clouds[i].position.x, (int)clouds[i].position.y, clouds[i].scale, Fade(WHITE, 0.4f));
            DrawCircle((int)clouds[i].position.x + 30, (int)clouds[i].position.y, clouds[i].scale * 0.8f, Fade(WHITE, 0.4f));
        }

        // Render Ocean Surfaces
        for (int x0 = 0; x0 < SCREEN_WIDTH; x0 += 2) {
            float k1 = (2.0f * PI) / wave1.wavelength;
            float phase1 = k1 * (x0 + scrollOffset) - wave1.speed * time;
            float k2 = (2.0f * PI) / wave2.wavelength;
            float phase2 = k2 * (x0 + scrollOffset) + wave2.speed * time;

            float final_x = x0 - (wave1.amplitude * sinf(phase1) + wave2.amplitude * sinf(phase2));
            float final_y = GetWaveHeightAt(x0, time, scrollOffset);

            DrawLineV((Vector2){ final_x, final_y }, (Vector2){ final_x, (float)SCREEN_HEIGHT }, waterColor);
            DrawPixelV((Vector2){ final_x, final_y }, (currentLevel == 1) ? LIGHTGRAY : WHITE);
        }

        if (!gameOver) {
            // Render Obstacles
            for (int i = 0; i < MAX_OBSTACLES; i++) {
                if (obstacles[i].active) {
                    float obsY = GetWaveHeightAt(obstacles[i].x + 15, time, scrollOffset);
                    DrawRectangleRec((Rectangle){ obstacles[i].x, obsY - 30, 30, 30 }, obstacles[i].color);
                    DrawRectangleLines((int)obstacles[i].x, (int)obsY - 30, 30, 30, BLACK);
                }
            }

            // Render Lasers
            for (int i = 0; i < MAX_LASERS; i++) {
                if (lasers[i].active) DrawCircleV(lasers[i].position, 4.0f, RED);
            }

            // Render Player
            float currentY = GetWaveHeightAt(playerX + (boatW / 2.0f), time, scrollOffset);
            DrawRectangleRec((Rectangle){ playerX, currentY - boatH, boatW, boatH }, MAROON);
            DrawTriangle((Vector2){ playerX + boatW, currentY - boatH }, (Vector2){ playerX + boatW, currentY }, (Vector2){ playerX + boatW + 15, currentY - boatH }, MAROON);


            DrawRectangle(10, 10, 240, 95, Fade(BLACK, 0.4f));      
            DrawText(TextFormat("SCORE: %04d", score), 20, 15, 20, GREEN);  
            DrawText(TextFormat("LEVEL: %d", currentLevel), 20, 40, 18, (currentLevel == 1) ? WHITE : (currentLevel == 2) ? ORANGE : RED);
            char livesString[20];   
            itoa(lives, livesString, 10);   
            DrawText(TextFormat("LIVES: %s", livesString), 20, 65, 16, RED);}   
            else {
                    // GAME OVER Screen state   
                    DrawRectangle  (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));  
                    DrawText("GAME OVER", SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 40, 40, RED); 
                    DrawText("Press ENTER to Restart", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 20, 20, WHITE);  
                    DrawText(TextFormat("Final Score: %04d", score), SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 + 60, 18, GREEN); 
                    
                    }    
                    
                    EndDrawing();   
                    }   
                    
                    CloseWindow();    
                    return 0;   
                    }




