#include "face.h"

#include <math.h>
#include <stdlib.h>

#include "cmsis_os.h"

/* --- 私有宏定义 --- */
#ifndef PI
#define PI 3.14159265f
#endif

/* --- 屏幕和眼睛参数 --- */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define EYE_CENTER_Y 32
#define EYE_OFFSET_X 28
#define EYE_WIDTH 12
#define EYE_HEIGHT 24
#define EYE_CORNER_RADIUS 4

/* --- 动画时间参数 --- */
#define BLINK_DURATION_MS 100
#define BLINK_CLOSED_MS 60
#define BLINK_INTERVAL_MIN_MS 4000
#define BLINK_INTERVAL_MAX_MS 8000

/* ========================================================================= */
/* --- 1. 通用动画接口定义 --- */
/* ========================================================================= */

// 前向声明
typedef struct Animation_t Animation_t;
typedef struct Face_t Face_t;

/**
 * @brief 动画接口结构体
 */
struct Animation_t {
  void (*start)(Animation_t* self, uint32_t currentTime);

  /**
   * @brief 更新动画状态并决定下一个状态
   * @param self 指向动画实例的指针
   * @param currentTime 当前时间戳
   * @return 指向下一个状态的动画指针。如果状态不改变，则返回 self。
   */
  Animation_t* (*update)(Animation_t* self, uint32_t currentTime);

  void (*draw)(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime);

  /**
   * @brief NEW: 指向父级 Face 实例的指针，用于访问其他动画状态
   */
  Face_t* face;
};

/* ========================================================================= */
/* --- 2. 各个动画的具体实现 --- */
/* ========================================================================= */

// --- 2.1 眨眼动画 (Blink) ---
typedef enum { BLINK_STATE_CLOSING, BLINK_STATE_CLOSED, BLINK_STATE_OPENING } BlinkInternalState_t;

typedef struct {
  Animation_t base;
  uint32_t startTime;
  BlinkInternalState_t internalState;
} BlinkAnimation_t;

// --- 2.2 正常睁眼动画 (NormalEyes) ---
typedef struct {
  Animation_t base;
  uint32_t nextBlinkTime;
} NormalEyesAnimation_t;

/**
 * @brief NEW: 核心状态机结构体，封装了所有状态
 */
struct Face_t {
  Animation_t* currentAnimation;

  // 将所有动画实例和它们的数据作为 Face_t 的成员
  // 这样它们就有了实例作用域，而不是全局作用域
  NormalEyesAnimation_t normalEyes;
  BlinkAnimation_t blink;
  // 如果要添加新动画，只需在这里添加即可
  // e.g., SadAnimation_t sad;
};

/* --- 动画函数的原型声明 --- */
static void NormalEyes_start(Animation_t* self, uint32_t currentTime);
static Animation_t* NormalEyes_update(Animation_t* self, uint32_t currentTime);
static void NormalEyes_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime);

static void Blink_start(Animation_t* self, uint32_t currentTime);
static Animation_t* Blink_update(Animation_t* self, uint32_t currentTime);
static void Blink_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime);

/* ========================================================================= */
/* --- 3. 公共 API 实现 --- */
/* ========================================================================= */

Face_t* Face_Create(void) {
  Face_t* face = (Face_t*)calloc(1, sizeof(Face_t));
  if (face == NULL) {
    return NULL;
  }

  // --- 初始化 NormalEyes 动画 ---
  face->normalEyes.base.start = NormalEyes_start;
  face->normalEyes.base.update = NormalEyes_update;
  face->normalEyes.base.draw = NormalEyes_draw;
  face->normalEyes.base.face = face;

  // --- 初始化 Blink 动画 ---
  face->blink.base.start = Blink_start;
  face->blink.base.update = Blink_update;
  face->blink.base.draw = Blink_draw;
  face->blink.base.face = face;

  // ... 如果有新动画，在这里初始化 ...

  // 设置初始状态
  Face_Init(face);

  return face;
}

void Face_Destroy(Face_t* face) {
  if (face) {
    free(face);
  }
}

void Face_Init(Face_t* face) {
  srand(osKernelGetTickCount());

  // 初始状态为正常睁眼
  face->currentAnimation = &face->normalEyes.base;
  face->currentAnimation->start(face->currentAnimation, osKernelGetTickCount());
}

void Face_Update(Face_t* face, uint32_t currentTime) {
  if (!face || !face->currentAnimation) {
    return;
  }

  // 1. 调用当前状态的 update 函数，它会返回下一个应有的状态
  Animation_t* nextAnimation = face->currentAnimation->update(face->currentAnimation, currentTime);

  // 2. 如果状态发生了变化
  if (nextAnimation != face->currentAnimation) {
    // 切换到新状态
    face->currentAnimation = nextAnimation;
    // 调用新状态的 start 函数进行初始化
    face->currentAnimation->start(face->currentAnimation, currentTime);
  }
}

void Face_Draw(Face_t* face, u8g2_t* u8g2, uint32_t currentTime) {
  if (!face || !face->currentAnimation) {
    return;
  }
  // 调用当前动画的绘制函数
  face->currentAnimation->draw(face->currentAnimation, u8g2, currentTime);
}

/* ========================================================================= */
/* --- 4. 动画实现函数定义 --- */
/* ========================================================================= */

// --- Helper Functions ---
static uint32_t GetNextBlinkTime(uint32_t currentTime) {
  uint32_t interval = BLINK_INTERVAL_MIN_MS + (rand() % (BLINK_INTERVAL_MAX_MS - BLINK_INTERVAL_MIN_MS + 1));
  return currentTime + interval;
}

static void DrawEyes(u8g2_t* u8g2, int eye_height) {
  int screen_center_x = SCREEN_WIDTH / 2;
  int left_eye_center_x = screen_center_x - EYE_OFFSET_X;
  int right_eye_center_x = screen_center_x + EYE_OFFSET_X;

  if (eye_height <= 2) {
    u8g2_DrawHLine(u8g2, left_eye_center_x - EYE_WIDTH / 2, EYE_CENTER_Y, EYE_WIDTH);
    u8g2_DrawHLine(u8g2, right_eye_center_x - EYE_WIDTH / 2, EYE_CENTER_Y, EYE_WIDTH);
  } else {
    int top_y = EYE_CENTER_Y - eye_height / 2;
    u8g2_DrawRBox(u8g2, left_eye_center_x - EYE_WIDTH / 2, top_y, EYE_WIDTH, eye_height, EYE_CORNER_RADIUS);
    u8g2_DrawRBox(u8g2, right_eye_center_x - EYE_WIDTH / 2, top_y, EYE_WIDTH, eye_height, EYE_CORNER_RADIUS);
  }
}

/* --- NormalEyes Animation Implementation --- */
static void NormalEyes_start(Animation_t* self, uint32_t currentTime) {
  NormalEyesAnimation_t* anim = (NormalEyesAnimation_t*)self;
  anim->nextBlinkTime = GetNextBlinkTime(currentTime);
}

static Animation_t* NormalEyes_update(Animation_t* self, uint32_t currentTime) {
  NormalEyesAnimation_t* anim = (NormalEyesAnimation_t*)self;

  if (currentTime >= anim->nextBlinkTime) {
    // 返回下一个状态：Blink
    return &self->face->blink.base;
  }

  // 保持当前状态
  return self;
}

static void NormalEyes_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime) {
  (void)self;
  (void)currentTime;
  DrawEyes(u8g2, EYE_HEIGHT);
}

/* --- Blink Animation Implementation --- */
static void Blink_start(Animation_t* self, uint32_t currentTime) {
  BlinkAnimation_t* anim = (BlinkAnimation_t*)self;
  anim->startTime = currentTime;
  anim->internalState = BLINK_STATE_CLOSING;
}

static Animation_t* Blink_update(Animation_t* self, uint32_t currentTime) {
  BlinkAnimation_t* anim = (BlinkAnimation_t*)self;
  uint32_t elapsedTime = currentTime - anim->startTime;

  // 眨眼动画的内部状态机
  switch (anim->internalState) {
    case BLINK_STATE_CLOSING:
      if (elapsedTime > BLINK_DURATION_MS) {
        anim->internalState = BLINK_STATE_CLOSED;
      }
      break;
    case BLINK_STATE_CLOSED:
      if (elapsedTime > BLINK_DURATION_MS + BLINK_CLOSED_MS) {
        anim->internalState = BLINK_STATE_OPENING;
      }
      break;
    case BLINK_STATE_OPENING:
      if (elapsedTime > BLINK_DURATION_MS + BLINK_CLOSED_MS + BLINK_DURATION_MS) {
        // 动画完成，返回到 NormalEyes 状态
        return &self->face->normalEyes.base;
      }
      break;
  }

  // 动画未完成，保持当前状态
  return self;
}

static void Blink_draw(Animation_t* self, u8g2_t* u8g2, uint32_t currentTime) {
  BlinkAnimation_t* anim = (BlinkAnimation_t*)self;
  float eye_height_multiplier = 1.0f;
  uint32_t elapsedTime = currentTime - anim->startTime;
  float progress;

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
  DrawEyes(u8g2, current_eye_height);
}
