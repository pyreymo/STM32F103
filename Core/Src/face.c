#include "face.h"

#include <math.h>
#include <stdlib.h>

#include "cmsis_os.h"

#ifndef PI
#define PI 3.14159265f
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define EYE_CENTER_Y 32
#define EYE_OFFSET_X 28
#define EYE_WIDTH 14
#define EYE_HEIGHT 30
#define EYE_CORNER_RADIUS 4
#define LOOK_OFFSET_X 10

#define LOOK_TRANSITION_MS 120
#define BLINK_DURATION_MS 100
#define BLINK_CLOSED_MS 60
#define BLINK_INTERVAL_MIN_MS 4000
#define BLINK_INTERVAL_MAX_MS 8000
#define LOOK_INTERVAL_MIN_MS 3000
#define LOOK_INTERVAL_MAX_MS 7000
#define LOOK_DURATION_MIN_MS 1000
#define LOOK_DURATION_MAX_MS 2500

typedef struct Animation_t Animation_t;
typedef struct Face_t Face_t;

struct Animation_t {
  void (*start)(Animation_t* self, uint32_t currentTime);
  Animation_t* (*update)(Animation_t* self, uint32_t currentTime);
  void (*draw)(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime);
  Face_t* face;
};

typedef enum { BLINK_STATE_CLOSING, BLINK_STATE_CLOSED, BLINK_STATE_OPENING } BlinkInternalState_t;

typedef struct {
  Animation_t base;
  uint32_t startTime;
  BlinkInternalState_t internalState;
  Animation_t* returnAnimation;
} BlinkAnimation_t;

typedef struct {
  Animation_t base;
  uint32_t nextLookTime;
} NormalEyesAnimation_t;

typedef struct {
  Animation_t base;
  uint32_t startTime;
  uint32_t holdUntilTime;
  int start_offset_x;
  int target_offset_x;
} LookAnimation_t;

struct Face_t {
  Animation_t* currentAnimation;
  uint32_t nextBlinkTime;

  NormalEyesAnimation_t normalEyes;
  BlinkAnimation_t blink;
  LookAnimation_t look;
};

static void NormalEyes_start(Animation_t* self, uint32_t currentTime);
static Animation_t* NormalEyes_update(Animation_t* self, uint32_t currentTime);
static void NormalEyes_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime);

static void Blink_start(Animation_t* self, uint32_t currentTime);
static Animation_t* Blink_update(Animation_t* self, uint32_t currentTime);
static void Blink_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime);

static void Look_start(Animation_t* self, uint32_t currentTime);
static Animation_t* Look_update(Animation_t* self, uint32_t currentTime);
static void Look_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime);

Face_t* Face_Create(void) {
  Face_t* face = (Face_t*)calloc(1, sizeof(Face_t));
  if (face == NULL) {
    return NULL;
  }

  face->normalEyes.base.start = NormalEyes_start;
  face->normalEyes.base.update = NormalEyes_update;
  face->normalEyes.base.draw = NormalEyes_draw;
  face->normalEyes.base.face = face;

  face->blink.base.start = Blink_start;
  face->blink.base.update = Blink_update;
  face->blink.base.draw = Blink_draw;
  face->blink.base.face = face;

  face->look.base.start = Look_start;
  face->look.base.update = Look_update;
  face->look.base.draw = Look_draw;
  face->look.base.face = face;

  Face_Init(face);
  return face;
}

void Face_Destroy(Face_t* face) {
  if (face) {
    free(face);
  }
}

static uint32_t GetRandomInterval(uint32_t min, uint32_t max) {
  if (min >= max) return min;
  return min + (rand() % (max - min + 1));
}
static uint32_t GetNextBlinkTime(uint32_t currentTime) {
  return currentTime + GetRandomInterval(BLINK_INTERVAL_MIN_MS, BLINK_INTERVAL_MAX_MS);
}
static uint32_t GetNextLookTime(uint32_t currentTime) {
  return currentTime + GetRandomInterval(LOOK_INTERVAL_MIN_MS, LOOK_INTERVAL_MAX_MS);
}

void Face_Init(Face_t* face) {
  srand(osKernelGetTickCount());
  face->currentAnimation = &face->normalEyes.base;
  face->nextBlinkTime = GetNextBlinkTime(osKernelGetTickCount());
  face->currentAnimation->start(face->currentAnimation, osKernelGetTickCount());
}

void Face_Update(Face_t* face, uint32_t currentTime) {
  if (!face || !face->currentAnimation) {
    return;
  }
  Animation_t* nextAnimation = face->currentAnimation->update(face->currentAnimation, currentTime);
  if (nextAnimation != face->currentAnimation) {
    if (nextAnimation == &face->blink.base) {
      face->blink.returnAnimation = face->currentAnimation;
    }
    face->currentAnimation = nextAnimation;
    face->currentAnimation->start(face->currentAnimation, currentTime);
  }
}

void Face_Draw(Face_t* face, u8g2_t* u8g2, uint32_t currentTime) {
  if (!face || !face->currentAnimation) {
    return;
  }
  face->currentAnimation->draw(face->currentAnimation, u8g2, currentTime);
}

static void DrawEyes(u8g2_t* u8g2, int eye_height, int x_offset) {
  int screen_center_x = SCREEN_WIDTH / 2;
  int left_eye_center_x = screen_center_x - EYE_OFFSET_X + x_offset;
  int right_eye_center_x = screen_center_x + EYE_OFFSET_X + x_offset;

  if (eye_height <= 2) {
    u8g2_DrawHLine(u8g2, left_eye_center_x - EYE_WIDTH / 2, EYE_CENTER_Y, EYE_WIDTH);
    u8g2_DrawHLine(u8g2, right_eye_center_x - EYE_WIDTH / 2, EYE_CENTER_Y, EYE_WIDTH);
  } else {
    int top_y = EYE_CENTER_Y - eye_height / 2;
    u8g2_DrawRBox(u8g2, left_eye_center_x - EYE_WIDTH / 2, top_y, EYE_WIDTH, eye_height, EYE_CORNER_RADIUS);
    u8g2_DrawRBox(u8g2, right_eye_center_x - EYE_WIDTH / 2, top_y, EYE_WIDTH, eye_height, EYE_CORNER_RADIUS);
  }
}

static void NormalEyes_start(Animation_t* self, uint32_t currentTime) {
  NormalEyesAnimation_t* anim = (NormalEyesAnimation_t*)self;
  anim->nextLookTime = GetNextLookTime(currentTime);
}

static Animation_t* NormalEyes_update(Animation_t* self, uint32_t currentTime) {
  NormalEyesAnimation_t* anim = (NormalEyesAnimation_t*)self;
  if (currentTime >= self->face->nextBlinkTime) {
    return &self->face->blink.base;
  }

  if (currentTime >= anim->nextLookTime) {
    int direction = (rand() % 2 == 0) ? -1 : 1;
    self->face->look.start_offset_x = 0;
    self->face->look.target_offset_x = direction * LOOK_OFFSET_X;
    return &self->face->look.base;
  }

  return self;
}

static void NormalEyes_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime) {
  (void)self;
  (void)currentTime;
  DrawEyes(u8g2, EYE_HEIGHT, 0);
}

static void Blink_start(Animation_t* self, uint32_t currentTime) {
  BlinkAnimation_t* anim = (BlinkAnimation_t*)self;
  anim->startTime = currentTime;
  anim->internalState = BLINK_STATE_CLOSING;
}
static Animation_t* Blink_update(Animation_t* self, uint32_t currentTime) {
  BlinkAnimation_t* anim = (BlinkAnimation_t*)self;
  uint32_t elapsedTime = currentTime - anim->startTime;
  switch (anim->internalState) {
    case BLINK_STATE_CLOSING:
      if (elapsedTime > BLINK_DURATION_MS) anim->internalState = BLINK_STATE_CLOSED;
      break;
    case BLINK_STATE_CLOSED:
      if (elapsedTime > BLINK_DURATION_MS + BLINK_CLOSED_MS) anim->internalState = BLINK_STATE_OPENING;
      break;
    case BLINK_STATE_OPENING:
      if (elapsedTime > BLINK_DURATION_MS + BLINK_CLOSED_MS + BLINK_DURATION_MS) {
        self->face->nextBlinkTime = GetNextBlinkTime(currentTime);
        return anim->returnAnimation;
      }
      break;
  }
  return self;
}

static void Blink_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime) {
  BlinkAnimation_t* anim = (BlinkAnimation_t*)self;
  float eye_height_multiplier = 1.0f;
  uint32_t elapsedTime = currentTime - anim->startTime;
  float progress;
  int current_look_offset = 0;
  if (anim->returnAnimation == &self->face->look.base) {
    LookAnimation_t* look_anim = (LookAnimation_t*)anim->returnAnimation;
    current_look_offset = look_anim->target_offset_x;
  }
  switch (anim->internalState) {
    case BLINK_STATE_CLOSING:
      progress = fminf((float)elapsedTime / BLINK_DURATION_MS, 1.0f);
      eye_height_multiplier = cosf(progress * PI * 0.5f);
      break;
    case BLINK_STATE_CLOSED:
      eye_height_multiplier = 0.0f;
      break;
    case BLINK_STATE_OPENING:
      elapsedTime -= (BLINK_DURATION_MS + BLINK_CLOSED_MS);
      progress = fminf((float)elapsedTime / BLINK_DURATION_MS, 1.0f);
      eye_height_multiplier = sinf(progress * PI * 0.5f);
      break;
  }
  int current_eye_height = (int)(EYE_HEIGHT * eye_height_multiplier);
  DrawEyes(u8g2, current_eye_height, current_look_offset);
}

static void Look_start(Animation_t* self, uint32_t currentTime) {
  LookAnimation_t* anim = (LookAnimation_t*)self;
  uint32_t hold_duration = GetRandomInterval(LOOK_DURATION_MIN_MS, LOOK_DURATION_MAX_MS);
  anim->startTime = currentTime;
  anim->holdUntilTime = currentTime + LOOK_TRANSITION_MS + hold_duration;
}

static Animation_t* Look_update(Animation_t* self, uint32_t currentTime) {
  LookAnimation_t* anim = (LookAnimation_t*)self;
  if (currentTime >= self->face->nextBlinkTime) {
    return &self->face->blink.base;
  }
  if (currentTime >= anim->holdUntilTime) {
    return &self->face->normalEyes.base;
  }
  return self;
}

static void Look_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime) {
  LookAnimation_t* anim = (LookAnimation_t*)self;
  uint32_t elapsedTime = currentTime - anim->startTime;
  float progress = fminf((float)elapsedTime / LOOK_TRANSITION_MS, 1.0f);
  float eased_progress = sinf(progress * PI * 0.5f);
  int current_offset_x = anim->start_offset_x + (int)((anim->target_offset_x - anim->start_offset_x) * eased_progress);
  DrawEyes(u8g2, EYE_HEIGHT, current_offset_x);
}
