#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#define MAX_ENTITIES 512

typedef enum {
  PLAYER,
  ASTEROID,
  BULLET,
} EType;

typedef enum { SMALL, MEDIUM, LARGE } AType;

typedef struct Entity {
  bool active;
  bool thrusters;
  float acceleration;
  float drag;
  Vector2 pos;
  Vector2 velocity;
  float angle;
  EType type;
  int size;
  float rot;
  double cooldown;
  AType atype;
  Vector2 points[10];
} Entity;

typedef struct Game {
  int width;
  int height;
  int ecount;
  int level;
  int asteroids;
  Entity entities[MAX_ENTITIES];
} Game;

void UpdateDrawFrame(void *);
void GameInit(Game *game);
void GameUpdate(Game *game);
void GameDraw(Game *game);

Entity CreatePlayer(Game *game);
void DrawPlayer(Entity *e);

Entity CreateAsteroid(Game *game, AType atype, Vector2 pos);
void DrawAsteroid(Entity *e);

Entity CreateBullet(Entity *p);
void DrawBullet(Entity *e);

void AddEntity(Game *game, Entity entity);
bool IsColliding(Entity *a, Entity *b);
Vector2 Rotate(float x, float y, Vector2 center, float angle);

int main() {
  srand(time(NULL));

  static Game game = {
      .width = 600,
      .height = 600,
      .ecount = 0,
      .level = 1,
      .asteroids = 0,
  };

  GameInit(&game);

  InitWindow(game.width, game.height, "");
#if defined(PLATFORM_WEB)
  emscripten_set_main_loop_arg(UpdateDrawFrame, &game, 0, 1);
#else
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    UpdateDrawFrame(&game);
  }
#endif
  CloseWindow();
  return 0;
}

void GameInit(Game *game) {
  for (int i = 0; i < MAX_ENTITIES; i++) {
    game->entities[i] = (Entity){0};
  }
  game->ecount = 0;
  game->asteroids = 0;
  Vector2 pos = {0, 0};
  AddEntity(game, CreatePlayer(game));
  for (int i = 0; i < game->level * 2; i++) {
    AddEntity(game, CreateAsteroid(game, LARGE, pos));
    game->asteroids += 4;
  }
}

void GameUpdate(Game *game) {
  float dt = GetFrameTime();
  double t = GetTime();

  if (IsKeyDown(KEY_R)) {
    GameInit(game);
  }

  for (int i = 0; i < game->ecount; i++) {
    Entity *e = &game->entities[i];

    if (!e->active) {
      continue;
    }

    e->thrusters = false;

    if (e->type == PLAYER) {
      if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        e->velocity.x += sinf(e->angle) * e->acceleration * dt;
        e->velocity.y -= cosf(e->angle) * e->acceleration * dt;
        e->thrusters = true;
      }

      if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        e->angle += 3 * dt;
      }
      if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        e->angle -= 3 * dt;
      }

      if (IsKeyDown(KEY_SPACE) && t - e->cooldown >= 0.4f) {
        AddEntity(game, CreateBullet(e));
        e->cooldown = t;
      }
    }

    if (e->type == ASTEROID) {
      e->angle += e->rot;

      for (int j = 0; j < game->ecount; j++) {
        Entity *e2 = &game->entities[j];
        if (e == e2 || e2->type == ASTEROID || !e2->active) {
          continue;
        }
        if (IsColliding(e, e2) && e2->type == PLAYER) {
          e->active = false;
          e2->active = false;
          AddEntity(game, CreateAsteroid(game, SMALL, e2->pos));
          AddEntity(game, CreateAsteroid(game, SMALL, e2->pos));
          AddEntity(game, CreateAsteroid(game, SMALL, e2->pos));
          AddEntity(game, CreateAsteroid(game, SMALL, e2->pos));
          AddEntity(game, CreateAsteroid(game, SMALL, e2->pos));
          break;
        }
        if (IsColliding(e, e2) && e2->type == BULLET) {
          e->active = false;
          e2->active = false;
          if (e->atype == SMALL) {
            game->asteroids--;
            if (game->asteroids == 0) {
              game->level++;
              GameInit(game);
            }
          } else if (e->atype == LARGE) {
            AddEntity(game, CreateAsteroid(game, MEDIUM, e->pos));
            AddEntity(game, CreateAsteroid(game, MEDIUM, e->pos));
          } else if (e->atype == MEDIUM) {
            AddEntity(game, CreateAsteroid(game, SMALL, e->pos));
            AddEntity(game, CreateAsteroid(game, SMALL, e->pos));
          }
          break;
        }
      }
    }

    // physics
    e->velocity.x *= expf(-e->drag * dt);
    e->velocity.y *= expf(-e->drag * dt);

    e->pos.x += e->velocity.x * dt;
    e->pos.y += e->velocity.y * dt;

    if (e->pos.x - (e->size * 0.8) > game->width) {
      e->pos.x = -(e->size * 0.8);
      if (e->type == BULLET) {
        e->active = false;
      }
    } else if (e->pos.x + (e->size * 0.8) < 0) {
      e->pos.x = game->width + (e->size * 0.8);
      if (e->type == BULLET) {
        e->active = false;
      }
    }

    if (e->pos.y - (e->size * 0.8) > game->height) {
      e->pos.y = -10;
      if (e->type == BULLET) {
        e->active = false;
      }
    } else if (e->pos.y + (e->size * 0.8) < 0) {
      e->pos.y = game->height + (e->size * 0.8);
      if (e->type == BULLET) {
        e->active = false;
      }
    }
  }
}

void GameDraw(Game *game) {
  if (game->entities[0].type != PLAYER) {
    DrawText("GAME OVER", (game->width / 2) - 65, game->height / 2, 20, WHITE);
  }

  for (int i = 0; i < game->ecount; i++) {
    Entity *e = &game->entities[i];

    if (!e->active) {
      continue;
    }

    switch (e->type) {
    case PLAYER:
      DrawPlayer(e);
      break;
    case ASTEROID:
      DrawAsteroid(e);
      break;
    case BULLET:
      DrawBullet(e);
      break;
    }
  }
}

void UpdateDrawFrame(void *data) {
  Game *game = data;

  GameUpdate(game);

  BeginDrawing();
  ClearBackground(BLACK);
  GameDraw(game);
  EndDrawing();
}

void DrawPlayer(Entity *e) {
  Vector2 front = Rotate(0, -15, e->pos, e->angle);
  Vector2 back = Rotate(0, 15, e->pos, e->angle);
  Vector2 right = Rotate(10, 10, e->pos, e->angle);
  Vector2 left = Rotate(-10, 10, e->pos, e->angle);
  Vector2 rtail = Rotate(8, 6, e->pos, e->angle);
  Vector2 ltail = Rotate(-8, 6, e->pos, e->angle);
  DrawLineV(front, right, WHITE);
  DrawLineV(front, left, WHITE);
  DrawLineV(ltail, rtail, WHITE);
  Vector2 rthrust = Rotate(5, 6, e->pos, e->angle);
  Vector2 lthrust = Rotate(-5, 6, e->pos, e->angle);
  if (e->thrusters) {
    DrawLineV(lthrust, back, WHITE);
    DrawLineV(rthrust, back, WHITE);
  }
}

void DrawAsteroid(Entity *e) {
  Vector2 points[10];
  for (int i = 0; i < 10; i++) {
    points[i] = Rotate(e->points[i].x, e->points[i].y, e->pos, e->angle);
  }
  for (int i = 1; i < 10; i++) {
    DrawLineV(points[i - 1], points[i], WHITE);
  }
  DrawLineV(points[0], points[9], WHITE);
}

Entity CreateAsteroid(Game *game, AType atype, Vector2 pos) {
  float direction = (rand() % 360) * (PI / 180);
  float speed = rand() % 100 + 100;
  int size = 60;

  if (atype == MEDIUM) {
    size = 40;
  } else if (atype == SMALL) {
    size = 20;
  }

  if (pos.x == 0 && pos.y == 0) {
    int position = rand() % ((game->width * 2) + (game->height * 2));
    if (position <= game->width) {
      // top
      pos.x = position;
      pos.y = 0;
    } else if (position <= (game->width + game->height)) {
      // right
      pos.x = game->width;
      pos.y = position - game->width;
    } else if (position <= (game->width * 2 + game->height)) {
      // bottom
      pos.x = (game->width * 2 + game->height) - position;
      pos.y = game->height;
    } else {
      // left
      pos.x = 0;
      pos.y = ((game->width * 2) + (game->height * 2)) - position;
    }
  }

  Entity e = {
      .active = true,
      .thrusters = false,
      .acceleration = 300,
      .drag = 0,
      .pos = pos,
      .velocity = {sinf(direction) * speed, -cosf(direction) * speed},
      .type = ASTEROID,
      .rot = PI / 180 / 2,
      .size = size,
      .atype = atype,
  };

  e.points[0] = Rotate(0, rand() % 10 + e.size / 2, (Vector2){0, 0}, 0);
  for (int i = 1; i < 10; i++) {
    e.points[i] = Rotate(0, rand() % 10 + e.size / 2, (Vector2){0, 0},
                         i * 40 * (PI / 180));
  }

  return e;
}

Entity CreatePlayer(Game *game) {
  Entity e = {
      .active = true,
      .thrusters = false,
      .acceleration = 400,
      .drag = 1,
      .pos = {(game->width / 2) - 5, (game->height / 2) - 5},
      .velocity = {0, 0},
      .angle = 0,
      .size = 10,
      .cooldown = GetTime(),
      .type = PLAYER,
  };
  return e;
}

Entity CreateBullet(Entity *p) {
  float direction = p->angle;
  float speed = 300;

  Entity e = {
      .active = true,
      .thrusters = false,
      .acceleration = 300,
      .drag = 0,
      .pos = Rotate(0, -15, p->pos, p->angle),
      .velocity = {sinf(direction) * speed, -cosf(direction) * speed},
      .type = BULLET,
      .rot = PI / 180 / 2,
      .size = 5,
  };

  e.points[0] = Rotate(0, 5, (Vector2){0, 0}, 0);
  for (int i = 1; i < 10; i++) {
    e.points[i] = Rotate(0, 5, (Vector2){0, 0}, i * 40 * (PI / 180));
  }

  return e;
}

void DrawBullet(Entity *e) {
  Vector2 points[10];
  for (int i = 0; i < 10; i++) {
    points[i] = Rotate(e->points[i].x, e->points[i].y, e->pos, e->angle);
  }
  for (int i = 1; i < 10; i++) {
    DrawLineV(points[i - 1], points[i], WHITE);
  }
  DrawLineV(points[0], points[9], WHITE);
}

void AddEntity(Game *game, Entity entity) {
  for (int i = 0; i < game->ecount; i++) {
    Entity *e = &game->entities[i];
    if (!e->active) {
      game->entities[i] = entity;
      return;
    }
  }

  if (game->ecount < MAX_ENTITIES) {
    game->entities[game->ecount++] = entity;
  }
}

Vector2 Rotate(float x, float y, Vector2 center, float angle) {
  Vector2 pos;
  float s = sinf(angle);
  float c = cosf(angle);
  pos.x = (x * c) - (y * s);
  pos.y = (x * s) + (y * c);
  pos.x += center.x;
  pos.y += center.y;
  return pos;
}

bool IsColliding(Entity *a, Entity *b) {
  float dist =
      sqrtf(powf(a->pos.x - b->pos.x, 2) + powf(a->pos.y - b->pos.y, 2));
  return dist < (a->size * 0.6) + (b->size * 0.6);
}
