#include "stdio.h"
#include "math.h"
#include "raylib.h"

#define HEIGHT 600
#define WIDTH 800
#define YEAR_PER_SECOND 1.0f

typedef struct {
    const char *name;
    const int year_from;
    const int year_to;
    const Vector2 position;
    const Vector2 size;
    const float height;
    const Color color;
} Building;

const Building buildings[] = {
    { "Castle", 1992, -1, { 0, 0 }, { 1, 1 }, 4.f, { 0xff, 0x00, 0x00, 0xff }},
    { "Hotel", 1992, -1, { 0, -7 }, { 3, 5 }, 2.f, { 0x00, 0x00, 0xff, 0xff }},
    { "Something", 1995, 1998, { 0, 2 }, { 1, 1 }, 0.5f, { 0x00, 0x00, 0xff, 0xff }},
};
const size_t buildings_count = sizeof(buildings) / sizeof(buildings[0]);

void DrawBuilding(const Building *building, const float z_offset) {
    DrawCube((Vector3){ building->position.x, z_offset, building->position.y}, building->size.x, building->height, building->size.y, building->color);
}

float lerp(const float t, const float a, const float b) {
    return a + (b - a) * t;
}

float map_range(const float value, const float from_min, const float from_max, const float to_min, const float to_max) {
    return lerp((value - from_min) / (from_max - from_min), to_min, to_max);
}

int main() {
    InitWindow(WIDTH, HEIGHT, "Disneyland Paris over the years");
    SetTargetFPS(60);
    DisableCursor();

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.projection = CAMERA_PERSPECTIVE;
    camera.fovy = 45.0f;

    int target_year = 1992;
    float offset_year = 0.0f;
    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);

        if (IsKeyPressed(KEY_O) && target_year > 1992) {
            target_year--;
            offset_year = 1.0f;
        }
        else if (IsKeyPressed(KEY_P) && target_year < 2025) {
            target_year++;
            offset_year = -1.0f;
        }

        const float dy = YEAR_PER_SECOND * GetFrameTime();
        if (offset_year > 0.0f) {
            offset_year -= dy;
            if (offset_year < 0.0f) offset_year = 0.0f;
        }
        else if (offset_year < 0.0f) {
            offset_year += dy;
            if (offset_year > 0.0f) offset_year = 0.0f;
        }
        const float current_year = (float)target_year + offset_year;

        BeginDrawing();
        {
        ClearBackground((Color){ 0x18, 0x18, 0x18, 0xff });
        DrawText(TextFormat("Year: %d (%.2f)", target_year, current_year), 10, 10, 20, RAYWHITE);

        BeginMode3D(camera);
        {
        for (size_t i = 0; i < buildings_count; i++) {
            if (current_year >= buildings[i].year_from && (current_year < buildings[i].year_to || buildings[i].year_to == -1))
                DrawBuilding(&buildings[i], 0.0f);
            else if (current_year > buildings[i].year_from - 1 && current_year < buildings[i].year_from)
                DrawBuilding(&buildings[i], map_range(current_year, buildings[i].year_from, buildings[i].year_from - 1, 0.0f, -10.0f));
            else if (buildings[i].year_to > 0 && current_year < buildings[i].year_to + 1 && current_year >= buildings[i].year_to)
                DrawBuilding(&buildings[i], map_range(current_year, buildings[i].year_to, buildings[i].year_to + 1, 0.0f, 10.0f));
        }
        }
        EndMode3D();
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}