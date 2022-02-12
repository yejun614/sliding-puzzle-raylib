#include <vector>
#include <cmath>

#include "raylib.h"
#include "rlgl.h"

#define RAYLIB_NEW_RLGL

#define LETTER_BOUNDRY_SIZE    0.25f
#define TEXT_MAX_LAYERS        32
#define LETTER_BOUNDRY_COLOR   VIOLET

bool SHOW_LETTER_BOUNDRY = false;
bool SHOW_TEXT_BOUNDRY = false;

struct PuzzleTile {
  short index;
};

typedef std::vector<std::vector<PuzzleTile> > Board;

// Draw codepoint at specified position in 3D space
void DrawTextCodepoint3D(Font font, int codepoint, Vector3 position, float fontSize, bool backface, Color tint) {
  // Character index position in sprite font
  // NOTE: In case a codepoint is not available in the font, index returned points to '?'
  int index = GetGlyphIndex(font, codepoint);
  float scale = fontSize / (float)font.baseSize;

  // Character detination rectangle on screen
  // NOTE: We consider charsPadding on drawing
  position.x += (float)(font.glyphs[index].offsetX - font.glyphPadding) / (float)(font.baseSize * scale);
  position.y += (float)(font.glyphs[index].offsetY - font.glyphPadding) / (float)(font.baseSize * scale);

  // Character source rectangle from texture atlas
  // NOTE: We consider chars padding when drawing, it could be required for outline/glow shader effects
  Rectangle srcRec = { font.recs[index].x - (float)font.glyphPadding, font.recs[index].y - (float)font.glyphPadding,
                      font.recs[index].width + 2.0f * font.glyphPadding, font.recs[index].height + 2.0f * font.glyphPadding };

  float width = (float)(font.recs[index].width + 2.0f * font.glyphPadding) / (float)font.baseSize * scale;
  float height = (float)(font.recs[index].height + 2.0f * font.glyphPadding) / (float)font.baseSize * scale;

  if (font.texture.id > 0) {
    const float x = 0.0f;
    const float y = 0.0f;
    const float z = 0.0f;

    // normalized texture coordinates of the glyph inside the font texture (0.0f -> 1.0f)
    const float tx = srcRec.x / font.texture.width;
    const float ty = srcRec.y / font.texture.height;
    const float tw = (srcRec.x + srcRec.width) / font.texture.width;
    const float th = (srcRec.y + srcRec.height) / font.texture.height;

    if (SHOW_LETTER_BOUNDRY)
      DrawCubeWiresV((Vector3){ position.x + width / 2, position.y, position.z + height / 2 },
                     (Vector3){ width, LETTER_BOUNDRY_SIZE, height },
                     LETTER_BOUNDRY_COLOR);
                     
    rlCheckRenderBatchLimit(4 + 4 * backface);
    rlSetTexture(font.texture.id);

    rlPushMatrix();
      rlTranslatef(position.x, position.y, position.z);

      rlBegin(RL_QUADS);
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);

        // Front Face
        rlNormal3f(0, 1, 0);                                            // Normal Pointing Up
        rlTexCoord2f(tx, ty); rlVertex3f(x,         y, z);              // Top Left Of The Texture and Quad
        rlTexCoord2f(tx, th); rlVertex3f(x,         y, z + height);     // Bottom Left Of The Texture and Quad
        rlTexCoord2f(tw, th); rlVertex3f(x + width, y, z + height);     // Bottom Right Of The Texture and Quad
        rlTexCoord2f(tw, ty); rlVertex3f(x + width, y, z);              // Top Right Of The Texture and Quad

        if (backface) {
          rlNormal3f(0, -1, 0);                                           // Normal Pointing Up
          rlTexCoord2f(tx, ty); rlVertex3f(x,         y, z);              // Top Left Of The Texture and Quad
          rlTexCoord2f(tw, ty); rlVertex3f(x + width, y, z);              // Bottom Left Of The Texture and Quad
          rlTexCoord2f(tw, th); rlVertex3f(x + width, y, z + height);     // Bottom Right Of The Texture and Quad
          rlTexCoord2f(tx, th); rlVertex3f(x,         y, z + height);     // Top Right Of The Texture and Quad
       }

      rlEnd();
    rlPopMatrix();

    rlSetTexture(0);
  }
}

void DrawText3D(Font font, const char *text, Vector3 position, float fontSize, float fontSpacing, float lineSpacing, bool backface, Color tint) {
  int length = TextLength(text);     // Total length in bytes of the text, scanned by codepoints in loop

  float textOffsetY = 0;             // Offset between lines(on line break '\n')
  float textOffsetX = 0;             // Offset X to next character to draw

  float scale = fontSize / (float)font.baseSize;

  for (int i = 0; i < length;) {
    // Get next codepoint from byte string and glyph index in font
    int codepointByteCount = 0;
    int codepoint = GetCodepoint(&text[i], &codepointByteCount);
    int index = GetGlyphIndex(font, codepoint);

    // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
    // but we need to draw all of the bad bytes using the '?' symbol moving one byte

    if (codepoint == 0x3f) codepointByteCount = 1;

    if (codepoint == '\n') {
      // NOTE: Fixed line spacing of 1.5 line-height
      // TODO: Support custom line spacing defined by user
      textOffsetY += scale + lineSpacing / (float)font.baseSize * scale;
      textOffsetX = 0;

    } else {
      if ((codepoint != ' ') && (codepoint != '\t')) {
        DrawTextCodepoint3D(font, codepoint,
                            (Vector3){ position.x + textOffsetX, position.y, position.z + textOffsetY },
                            fontSize, backface, tint);
      }

      if (font.glyphs[index].advanceX == 0) textOffsetX += (float)(font.recs[index].width + fontSpacing) / (float)font.baseSize * scale;
      else textOffsetX += (float)(font.glyphs[index].advanceX + fontSpacing) / (float)font.baseSize * scale;
    }

    i += codepointByteCount;    // Move text bytes counter to next codepoint
  }
}

inline int num_len(int num) {
  int count = 0;

  while (num > 0) {
    count ++;
    num /= 10;
  }

  return count;
}

inline void num_str(int num, char* str) {
  int len = num_len(num);

  str[len] = '\0';

  for (int i = len - 1; i >= 0; i --) {
    str[i] = num % 10 + '0';
    num /= 10;
  }
}

void make_board(Board& board, const int width, const int height) {
  board.assign(height, std::vector<PuzzleTile>(width) );
  short index = 0;

  for (int y = 0; y < height; y ++) {
    for (int x = 0; x < width; x ++) {
      board[y][x].index = index ++;
    }
  }
}

void draw_board(Board& board, Vector3 position, Vector3 cubeSize) {
  Vector3 current;
  current.y = position.y;
  current.z = position.z;

  for (auto yaxis : board) {
    current.x = position.x;

    for (PuzzleTile tile : yaxis) {
      if (tile.index >= 0) {
        DrawCubeV(current, cubeSize, LIGHTGRAY);
        DrawCubeWiresV(current, cubeSize, DARKGRAY);
      }

      current.x += cubeSize.x;
    }

    current.y -= cubeSize.y;
  }
}

void draw_board_index(Board& board, Vector3 position, Vector3 cubeSize,
                      Font font, float fontSize, float fontSpacing, float lineSpacing, Color color) {
  Vector3 current;
  current.y = position.z + 0.7f;
  current.z = position.y * -1;
  current.z -= 2;

  char str[30];

  for (auto yaxis : board) {
    current.x = position.x;
    current.x -= 2;

    for (PuzzleTile tile : yaxis) {
      if (tile.index >= 0) {
        num_str(tile.index + 1, str);
        DrawText3D(font, str, current, fontSize, fontSpacing, lineSpacing, true, color);
      }

      current.x += cubeSize.x;
    }

    current.z += cubeSize.y;
  }
}

int main() {
  const int screenWidth = 800;
  const int screenHeight = 480;

  InitWindow(screenWidth, screenHeight, "Sliding Puzzle Game");
  SetTargetFPS(60);

  Camera3D camera;
  camera.position = (Vector3){ 0.0f, 0.0f, 30.0f };
  camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
  camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  float cameraSpeed = 1.0f;

  Font font = GetFontDefault();
  float fontSize = 8, fontSpacing = 0.5f, lineSpacing = -1;

  Board board;
  make_board(board, 4, 4);
  board[0][0].index = -1;

  Vector3 boardPosition = { -7, 7.5f, 0 },
          cubeSize = { 5, 5, 1 };

  while (!WindowShouldClose()) {
    UpdateCamera(&camera);

    if (IsKeyDown('W')) camera.position.y += cameraSpeed;
    if (IsKeyDown('S')) camera.position.y -= cameraSpeed;
    if (IsKeyDown('A')) camera.position.x -= cameraSpeed;
    if (IsKeyDown('D')) camera.position.x += cameraSpeed;
    if (IsKeyDown('Z')) camera.position = (Vector3) { 0, 0, 30.0f };
    
    BeginDrawing();

      ClearBackground(RAYWHITE);

      BeginMode3D(camera);
        draw_board(board, boardPosition, cubeSize);

        rlPushMatrix();
          rlRotatef(90, 1, 0, 0);
          //rlRotatef(180, 0, 0, 1);

          //DrawText3D(font, "Test Font", {0, 1, 0}, fontSize, fontSpacing, lineSpacing, true, BLACK);

          draw_board_index(board, boardPosition, cubeSize,
                           font, fontSize*2, fontSpacing, lineSpacing, BLACK);
        rlPopMatrix();
      EndMode3D();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
