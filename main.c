#include "stdio.h"
#include "math.h"

#include "raylib.h"
#include "rlgl.h"

#define HEIGHT 600
#define WIDTH 800

#define MAP_LAT_MIN 48.868276
#define MAP_LAT_MAX 48.876301
#define MAP_LAT_MID ((MAP_LAT_MIN + MAP_LAT_MAX) * 0.5)
#define MAP_LAT_HEIGHT (MAP_LAT_MAX - MAP_LAT_MIN)

#define MAP_LON_MIN 2.768697
#define MAP_LON_MAX 2.784665
#define MAP_LON_MID ((MAP_LON_MIN + MAP_LON_MAX) * 0.5)
#define MAP_LON_WIDTH (MAP_LON_MAX - MAP_LON_MIN)

#define SCALE 0.01f // factor 1/100 to circumvent fixed frustrum limitation
#define LAT_TO_METER (111319.9 * SCALE)
#define LON_TO_METER (73324.94 * SCALE)

Vector3 latlon_to_world(const Vector2 latlon, const float height) {
    return (Vector3){
        (latlon.x - MAP_LAT_MID) * LAT_TO_METER,
        height * SCALE,
        (latlon.y - MAP_LON_MID) * LON_TO_METER
    };
}

#define YEAR_PER_SECOND 1.0f

typedef struct {
    const char *name;
    const int year_from;
    const int year_to;
    const Vector2 latlon; // in degrees
    const Vector2 size; // in meters
    const float height; // in meters
    const Color color;
} Building;

const Building buildings[] = {
    { "Castle", 1992, -1, { 48.873183 , 2.776001 }, { 35, 25 }, 60, { 0xff, 0x00, 0x00, 0xff }},
    { "Hotel", 1992, -1, { 48.87031 , 2.779653 }, { 270, 100 }, 30, { 0x00, 0x00, 0xff, 0xff }},
    { "Space Mountain", 1995, 1998, { 48.874022 , 2.779266 }, { 75, 75 }, 32, { 0x00, 0xff, 0x00, 0xff }},
};
const size_t buildings_count = sizeof(buildings) / sizeof(buildings[0]);

void DrawBuilding(const Building *building, const float z_offset) {
    DrawCube(latlon_to_world(building->latlon, (0.5f * building->height + z_offset)),
             building->size.x * SCALE, building->height * SCALE, building->size.y * SCALE,
             building->color);
}

// From https://www.raylib.com/examples/models/loader.html?name=models_draw_cube_texture
void DrawRectTexture(Texture2D texture, Vector3 position, float width, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    // Set desired texture to be enabled while drawing following vertex data
    rlSetTexture(texture.id);

    // Vertex data transformation can be defined with the commented lines,
    // but in this example we calculate the transformed vertex data directly when calling rlVertex3f()
    //rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
        //rlTranslatef(2.0f, 0.0f, 0.0f);
        //rlRotatef(45, 0, 1, 0);
        //rlScalef(1.0f, 1.0f, 1.0f);

        rlBegin(RL_QUADS);
            rlColor4ub(color.r, color.g, color.b, color.a);
            // Top Face
            rlNormal3f(0.0f, 1.0f, 0.0f);       // Normal Pointing Up
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y, z - length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y, z + length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y, z + length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y, z - length/2);  // Top Right Of The Texture and Quad
        rlEnd();
    //rlPopMatrix();

    rlSetTexture(0);
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
    camera.position = latlon_to_world((Vector2){48.868901f, 2.781507f}, 10.0f);
    camera.target = latlon_to_world((Vector2){48.872593f, 2.776753f}, 0.0f);
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.projection = CAMERA_PERSPECTIVE;
    camera.fovy = 45.0f;

    Texture dlp_satellite_texture = LoadTexture("assets/maps/dlp_satellite.png");

    int target_year = 1992;
    float offset_year = 0.0f;
    bool once = true;
    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_THIRD_PERSON);

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
        DrawText(TextFormat("Year: %d", target_year), 10, 10, 20, RAYWHITE);

        BeginMode3D(camera);
        {
        Vector3 map_position = { 0 };
        DrawRectTexture(dlp_satellite_texture,
            map_position, MAP_LAT_HEIGHT * LAT_TO_METER, MAP_LON_WIDTH * LON_TO_METER,
            WHITE);

        for (size_t i = 0; i < buildings_count; i++) {
            if (current_year >= buildings[i].year_from && (current_year < buildings[i].year_to || buildings[i].year_to == -1))
                DrawBuilding(&buildings[i], 0.0f);
            else if (current_year > buildings[i].year_from - 1 && current_year < buildings[i].year_from)
                DrawBuilding(&buildings[i], map_range(current_year, buildings[i].year_from, buildings[i].year_from - 1, 0.0f, -1.0f / SCALE));
            else if (buildings[i].year_to > 0 && current_year < buildings[i].year_to + 1 && current_year >= buildings[i].year_to)
                DrawBuilding(&buildings[i], map_range(current_year, buildings[i].year_to, buildings[i].year_to + 1, 0.0f, 10.0f / SCALE));
        }
        }
        EndMode3D();
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}