#include "face.h"

#include <math.h>
#include <stdlib.h>

#include "cmsis_os.h"

/* --- 私有宏定义 --- */
#ifndef PI
#define PI 3.14159265f
#endif

/* --- 私有定义和变量 --- */
// 屏幕尺寸 (作为脸的边界)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// 眼睛参数 (圆角矩形)
#define EYE_CENTER_Y 28
#define EYE_OFFSET_X 28
#define EYE_WIDTH 12
#define EYE_HEIGHT_MAX 16
#define EYE_CORNER_RADIUS 4

// 嘴巴参数 (微笑弧线)
#define MOUTH_CENTER_Y 48
#define MOUTH_RADIUS_H 18
#define MOUTH_RADIUS_V 12

// 动画时间（单位：毫秒）
#define BLINK_DURATION_MS 150
#define BLINK_CLOSED_MS 80

// 表情状态机定义
typedef enum { STATE_NORMAL, STATE_BLINK_CLOSING, STATE_BLINK_CLOSED, STATE_BLINK_OPENING } FaceState_t;

// 表情状态变量
static FaceState_t currentFaceState = STATE_NORMAL;
static uint32_t lastStateChangeTime = 0;
static uint32_t nextBlinkTime = 0;

/* --- 公共函数实现 --- */

void Face_Init(void) {
  srand(osKernelGetTickCount());
  currentFaceState = STATE_NORMAL;
  lastStateChangeTime = 0;
  nextBlinkTime = osKernelGetTickCount() + (rand() % 5000) + 2000;
}

void Face_Update(uint32_t currentTime) {
  switch (currentFaceState) {
    case STATE_NORMAL:
      if (currentTime >= nextBlinkTime) {
        currentFaceState = STATE_BLINK_CLOSING;
        lastStateChangeTime = currentTime;
      }
      break;
    case STATE_BLINK_CLOSING:
      if (currentTime - lastStateChangeTime > BLINK_DURATION_MS) {
        currentFaceState = STATE_BLINK_CLOSED;
        lastStateChangeTime = currentTime;
      }
      break;
    case STATE_BLINK_CLOSED:
      if (currentTime - lastStateChangeTime > BLINK_CLOSED_MS) {
        currentFaceState = STATE_BLINK_OPENING;
        lastStateChangeTime = currentTime;
      }
      break;
    case STATE_BLINK_OPENING:
      if (currentTime - lastStateChangeTime > BLINK_DURATION_MS) {
        currentFaceState = STATE_NORMAL;
        lastStateChangeTime = currentTime;
        nextBlinkTime = currentTime + (rand() % 5000) + 2000;
      }
      break;
  }
}

void Face_Draw(u8g2_t* u8g2, uint32_t currentTime) {
  // 1. 绘制嘴巴
  int mouth_center_x = SCREEN_WIDTH / 2;
  u8g2_DrawEllipse(u8g2, mouth_center_x, MOUTH_CENTER_Y, MOUTH_RADIUS_H, MOUTH_RADIUS_V,
                   U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);

  // 2. 绘制眼睛
  float eye_height_multiplier = 1.0f;

  switch (currentFaceState) {
    case STATE_NORMAL:
      eye_height_multiplier = 1.0f;
      break;

    case STATE_BLINK_CLOSING: {
      float progress = (float)(currentTime - lastStateChangeTime) / BLINK_DURATION_MS;
      if (progress >= 1.0f) {
        eye_height_multiplier = 0.0f;
      } else {
        eye_height_multiplier = (cosf(progress * PI) + 1.0f) / 2.0f;
      }
      break;
    }

    case STATE_BLINK_CLOSED:
      eye_height_multiplier = 0.0f;
      break;

    case STATE_BLINK_OPENING: {
      float progress = (float)(currentTime - lastStateChangeTime) / BLINK_DURATION_MS;
      if (progress >= 1.0f) {
        eye_height_multiplier = 1.0f;
      } else {
        eye_height_multiplier = 1.0f - ((cosf(progress * PI) + 1.0f) / 2.0f);
      }
      break;
    }
  }

  // 数值范围检查，防止浮点计算误差
  if (eye_height_multiplier < 0.0f) eye_height_multiplier = 0.0f;
  if (eye_height_multiplier > 1.0f) eye_height_multiplier = 1.0f;

  int current_eye_height = (int)(EYE_HEIGHT_MAX * eye_height_multiplier + 0.5f);

  int left_eye_center_x = SCREEN_WIDTH / 2 - EYE_OFFSET_X;
  int right_eye_center_x = SCREEN_WIDTH / 2 + EYE_OFFSET_X;

  if (current_eye_height > 1) {
    int eye_top_y = EYE_CENTER_Y - current_eye_height / 2;
    u8g2_DrawRBox(u8g2, left_eye_center_x - EYE_WIDTH / 2, eye_top_y, EYE_WIDTH, current_eye_height, EYE_CORNER_RADIUS);
    u8g2_DrawRBox(u8g2, right_eye_center_x - EYE_WIDTH / 2, eye_top_y, EYE_WIDTH, current_eye_height,
                  EYE_CORNER_RADIUS);
  } else {
    u8g2_DrawHLine(u8g2, left_eye_center_x - EYE_WIDTH / 2, EYE_CENTER_Y, EYE_WIDTH);
    u8g2_DrawHLine(u8g2, right_eye_center_x - EYE_WIDTH / 2, EYE_CENTER_Y, EYE_WIDTH);
  }
}