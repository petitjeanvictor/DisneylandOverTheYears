#include "stdio.h"
#include "math.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "cJSON/cJSON.h"

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

// Moves the camera position closer/farther to/from the camera target
// From raylib's rcamera.h
void CameraZoom(Camera *camera, float zoom)
{
    float distance = Vector3Distance(camera->position, camera->target) + zoom;

    // Distance must be greater than 0
    if (distance <= 0) distance = 0.001f;

    // Set new distance by moving the position along the forward vector
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
    camera->position = Vector3Add(camera->target, Vector3Scale(forward, -distance));
}

#define CAMERA_FIX_2D
#define CAMERA_MOVE_SPEED     0.03f
#define CAMERA_ROTATION_SPEED 0.003f

#ifdef CAMERA_FIX_2D
void UpdateCameraWithInputs(Camera3D *camera) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePositionDelta = GetMouseDelta();
        float dx = mousePositionDelta.x;
        float dy = mousePositionDelta.y;
        camera->position.x += dy * CAMERA_MOVE_SPEED;
        camera->position.z -= dx * CAMERA_MOVE_SPEED;
        camera->target.x   += dy * CAMERA_MOVE_SPEED;
        camera->target.z   -= dx * CAMERA_MOVE_SPEED;
    }
    camera->fovy *= 1.0f + GetMouseWheelMove() * 0.05f;
}
#else // CAMERA_FIX_2D
void UpdateCameraWithInputs(Camera3D *camera) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        Vector2 mousePositionDelta = GetMouseDelta();
        float dx = mousePositionDelta.x;
        float dy = mousePositionDelta.y;
        Vector3 forward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera->up));

        bool rotation = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) ||
                        IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
        if (rotation) {

            // Rotate around point under cursor
            Vector3 centerToCamera = Vector3Subtract(camera->position, camera->target);
            centerToCamera = Vector3RotateByAxisAngle(centerToCamera, camera->up, -CAMERA_ROTATION_SPEED*dx);

            Vector3 newPosition = Vector3RotateByAxisAngle(centerToCamera, right, -CAMERA_ROTATION_SPEED*dy);
            if (newPosition.y > 0.1f) { // Prevent camera from going underground
                camera->position = Vector3Add(camera->target, newPosition);
            }
        } else {
            // Pan the camera
            Vector3 moveDir = Vector3Add(Vector3Scale(right, -CAMERA_MOVE_SPEED*dx),
                                         Vector3Scale(forward, CAMERA_MOVE_SPEED*dy));
            camera->position = Vector3Add(camera->position, moveDir);
            camera->target = Vector3Add(camera->target, moveDir);
        }
    }
    CameraZoom(camera, -GetMouseWheelMove());
}
#endif // CAMERA_FIX_2D

Camera3D GetNewCamera() {
    Camera3D camera = { 0 };
#ifdef CAMERA_FIX_2D
    camera.position = latlon_to_world((Vector2){48.872593f, 2.776753f}, 100.0f);
    camera.target = camera.position; camera.target.y = 0.0f;
    camera.up = (Vector3){ 1.0f, 0.0f, 0.0f };
    camera.projection = CAMERA_ORTHOGRAPHIC;
    camera.fovy = 5.0f;
#else // CAMERA_FIX_2D
    camera.position = latlon_to_world((Vector2){48.868901f, 2.781507f}, 100.0f);
    camera.target = latlon_to_world((Vector2){48.872593f, 2.776753f}, 0.0f);
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.projection = CAMERA_PERSPECTIVE;
    camera.fovy = 45.0f;
#endif // CAMERA_FIX_2D
    return camera;
}

typedef struct {
    Vector2 *items;
    size_t count;
    size_t capacity;
} Contour;

typedef struct {
    size_t *items;
    size_t count;
    size_t capacity;
} NodeIds;



bool ParseContour(const char* filename, Contour* contour) {
    bool result = false;
    cJSON* json = NULL;
    Nob_String_Builder sb = {0};
    Contour temp_contour = {0};
    NodeIds node_ids = {0};

    if (!nob_read_entire_file(filename, &sb)) {
        nob_log(NOB_ERROR, "Could not read file %s", filename);
        nob_return_defer(false);
    }

    json = cJSON_Parse(sb.items);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        nob_log(NOB_ERROR, "Could not parse JSON file %s: %.100s", filename, error_ptr);
        nob_return_defer(false);
    }

    cJSON* elements = cJSON_GetObjectItemCaseSensitive(json, "elements");
    if (elements == NULL || !cJSON_IsArray(elements)) {
        nob_log(NOB_ERROR, "Could not find array 'elements' in JSON file %s", filename);
        nob_return_defer(false);
    }

    cJSON* nodes = NULL;
    for (cJSON* element = elements->child; element != NULL; element = element->next)
    {
        cJSON* type = cJSON_GetObjectItemCaseSensitive(element, "type");
        if (!cJSON_IsString(type)) {
            continue;
        }

        if (strcmp(type->valuestring, "way") == 0) {
            nodes = cJSON_GetObjectItemCaseSensitive(element, "nodes");
        }
        else if (strcmp(type->valuestring, "node") == 0) {
            cJSON* id = cJSON_GetObjectItemCaseSensitive(element, "id");
            cJSON* lat = cJSON_GetObjectItemCaseSensitive(element, "lat");
            cJSON* lon = cJSON_GetObjectItemCaseSensitive(element, "lon");
            if (!cJSON_IsNumber(id) || !cJSON_IsNumber(lat) || !cJSON_IsNumber(lon)) {
                nob_log(NOB_ERROR, "Could not find 'id', 'lat' or 'lon' in element of type 'node'");
                continue;
            }

            nob_da_append(&node_ids, id->valueint);
            nob_da_append(&temp_contour, ((Vector2){ .x = lat->valuedouble, .y = lon->valuedouble }));
        }
    }

    if (nodes == NULL || !cJSON_IsArray(nodes)) {
        nob_log(NOB_ERROR, "Could not find element of type 'way' in JSON file %s", filename);
        nob_return_defer(false);
    }

    for (cJSON* node = nodes->child; node != NULL; node = node->next)
    {
        if (!cJSON_IsNumber(node)) {
            nob_log(NOB_ERROR, "'node' is not a number in 'way'");
            continue;
        }

        size_t id = (size_t)node->valuedouble;
        size_t index = SIZE_MAX;
        for (size_t i = 0; i < node_ids.count; i++) {
            if (node_ids.items[i] == id) {
                index = i;
                break;
            }
        }
        if (index == SIZE_MAX) {
            nob_log(NOB_ERROR, "Could not find node with id %zu", id);
            continue;
        }

        nob_da_append(contour, temp_contour.items[index]);
    }
    result = true;

defer:
    if (json != NULL) cJSON_Delete(json);
    nob_sb_free(sb);
    nob_da_free(node_ids);
    nob_da_free(temp_contour);
    return result;
}

int main() {
    Contour contour = {0};
    if (!ParseContour("assets/buildings/contour.json", &contour)) {
        return 1;
    }

    InitWindow(WIDTH, HEIGHT, "Disneyland Paris over the years");
    SetTargetFPS(60);

    Camera3D camera = GetNewCamera();
    Texture dlp_satellite_texture = LoadTexture("assets/maps/dlp_satellite.png");

    int target_year = 1992;
    float offset_year = 0.0f;
    while (!WindowShouldClose()) {
        UpdateCameraWithInputs(&camera);

        if (IsKeyPressed(KEY_O) && target_year > 1992) {
            target_year--;
            offset_year = 1.0f;
        }
        else if (IsKeyPressed(KEY_P) && target_year < 2025) {
            target_year++;
            offset_year = -1.0f;
        }
        else if (IsKeyPressed(KEY_Z)) {
            camera = GetNewCamera();
        }

#define YEAR_PER_SECOND 1.0f

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

        for (size_t i = 0; i < contour.count; i++) {
            DrawLine3D(latlon_to_world(contour.items[i], 0.0f),
                        latlon_to_world(contour.items[(i + 1) % contour.count], 0.0f),
                        RED);
        }

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