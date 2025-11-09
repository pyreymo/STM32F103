#include "face.hpp"

#include <cmath>
#include <cstdlib>

#include "cmsis_os.h"

// --- Constants ---
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
#define LOOK_INTERVAL_MIN_MS 6000
#define LOOK_INTERVAL_MAX_MS 15000
#define LOOK_DURATION_MIN_MS 1000
#define LOOK_DURATION_MAX_MS 5000

// --- Utility Functions ---
static uint32_t getRandomInterval(uint32_t min, uint32_t max) {
  if (min >= max) return min;
  return min + (rand() % (max - min + 1));
}

static void drawEyes(u8g2_t* u8g2, int eye_height, int x_offset) {
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

// --- Animation Base Class Implementation ---
Animation::Animation(Face* face) : m_face(face) {}

void Animation::pause(uint32_t currentTime) {
  if (!m_isPaused) {
    m_pausedElapsed = currentTime - m_startTime;
    m_isPaused = true;
  }
}

void Animation::resume(uint32_t currentTime) {
  if (m_isPaused) {
    m_startTime = currentTime - m_pausedElapsed;
    m_isPaused = false;
  }
}

uint32_t Animation::getElapsedTime(uint32_t currentTime) const {
  if (m_isPaused) {
    return m_pausedElapsed;
  }
  return currentTime - m_startTime;
}

// --- Face Class Implementation ---
Face::Face() : m_normalEyes(this), m_blink(this), m_look(this), m_currentAnimation(nullptr), m_nextBlinkTime(0) {}

void Face::init() {
  srand(osKernelGetTickCount());
  m_currentAnimation = &m_normalEyes;
  m_nextBlinkTime = osKernelGetTickCount() + getRandomInterval(BLINK_INTERVAL_MIN_MS, BLINK_INTERVAL_MAX_MS);
  m_currentAnimation->start(osKernelGetTickCount());
}

void Face::update(uint32_t currentTime) {
  if (!m_currentAnimation) return;

  Animation* nextAnimation = m_currentAnimation->update(currentTime);
  if (nextAnimation != m_currentAnimation) {
    Animation* previousAnimation = m_currentAnimation;
    m_currentAnimation = nextAnimation;

    if (nextAnimation == &m_blink) {
      previousAnimation->pause(currentTime);
      m_blink.setReturnAnimation(previousAnimation);
      m_currentAnimation->start(currentTime);
    } else if (previousAnimation == &m_blink) {
      m_currentAnimation->resume(currentTime);
    } else {
      m_currentAnimation->start(currentTime);
    }
  }
}

void Face::draw(u8g2_t* u8g2, uint32_t currentTime) {
  if (m_currentAnimation) {
    m_currentAnimation->draw(u8g2, currentTime);
  }
}

// --- NormalEyesAnimation Implementation ---
NormalEyesAnimation::NormalEyesAnimation(Face* face) : Animation(face) {}

void NormalEyesAnimation::start(uint32_t currentTime) {
  m_startTime = currentTime;
  m_nextLookTime = currentTime + getRandomInterval(LOOK_INTERVAL_MIN_MS, LOOK_INTERVAL_MAX_MS);
}

Animation* NormalEyesAnimation::update(uint32_t currentTime) {
  if (currentTime >= m_face->m_nextBlinkTime) {
    return &m_face->m_blink;
  }
  if (currentTime >= m_nextLookTime) {
    int direction = (rand() % 2 == 0) ? -1 : 1;
    m_face->m_look.setTarget(0, direction * LOOK_OFFSET_X);
    return &m_face->m_look;
  }
  return this;
}

void NormalEyesAnimation::draw(u8g2_t* u8g2, uint32_t currentTime) {
  drawEyes(u8g2, EYE_HEIGHT, get_offset_x(currentTime));
}

int NormalEyesAnimation::get_offset_x(uint32_t currentTime) const { return 0; }

// --- BlinkAnimation Implementation ---
BlinkAnimation::BlinkAnimation(Face* face) : Animation(face) {}

void BlinkAnimation::setReturnAnimation(Animation* anim) { m_returnAnimation = anim; }

void BlinkAnimation::start(uint32_t currentTime) {
  m_startTime = currentTime;
  m_internalState = CLOSING;
}

Animation* BlinkAnimation::update(uint32_t currentTime) {
  uint32_t elapsedTime = getElapsedTime(currentTime);
  switch (m_internalState) {
    case CLOSING:
      if (elapsedTime > BLINK_DURATION_MS) m_internalState = CLOSED;
      break;
    case CLOSED:
      if (elapsedTime > BLINK_DURATION_MS + BLINK_CLOSED_MS) m_internalState = OPENING;
      break;
    case OPENING:
      if (elapsedTime > BLINK_DURATION_MS + BLINK_CLOSED_MS + BLINK_DURATION_MS) {
        m_face->m_nextBlinkTime = currentTime + getRandomInterval(BLINK_INTERVAL_MIN_MS, BLINK_INTERVAL_MAX_MS);
        return m_returnAnimation;
      }
      break;
  }
  return this;
}

void BlinkAnimation::draw(u8g2_t* u8g2, uint32_t currentTime) {
  float eye_height_multiplier = 1.0f;
  uint32_t elapsedTime = getElapsedTime(currentTime);

  int current_look_offset = 0;
  if (m_returnAnimation) {
    current_look_offset = m_returnAnimation->get_offset_x(currentTime);
  }

  switch (m_internalState) {
    case CLOSING:
      eye_height_multiplier = cosf(fminf(1.0f, (float)elapsedTime / BLINK_DURATION_MS) * PI * 0.5f);
      break;
    case CLOSED:
      eye_height_multiplier = 0.0f;
      break;
    case OPENING:
      elapsedTime -= (BLINK_DURATION_MS + BLINK_CLOSED_MS);
      eye_height_multiplier = sinf(fminf(1.0f, (float)elapsedTime / BLINK_DURATION_MS) * PI * 0.5f);
      break;
  }
  drawEyes(u8g2, (int)(EYE_HEIGHT * eye_height_multiplier), current_look_offset);
}

int BlinkAnimation::get_offset_x(uint32_t currentTime) const {
  return m_returnAnimation ? m_returnAnimation->get_offset_x(currentTime) : 0;
}

// --- LookAnimation Implementation ---
LookAnimation::LookAnimation(Face* face) : Animation(face) {}

void LookAnimation::setTarget(int start_offset, int target_offset) {
  m_start_offset_x = start_offset;
  m_target_offset_x = target_offset;
}

void LookAnimation::start(uint32_t currentTime) {
  m_startTime = currentTime;
  m_isPaused = false;
  uint32_t hold_duration = getRandomInterval(LOOK_DURATION_MIN_MS, LOOK_DURATION_MAX_MS);
  m_holdUntilTime = currentTime + LOOK_TRANSITION_MS + hold_duration;
}

Animation* LookAnimation::update(uint32_t currentTime) {
  if (currentTime >= m_face->m_nextBlinkTime) {
    return &m_face->m_blink;
  }
  if (currentTime >= m_holdUntilTime + LOOK_TRANSITION_MS) {  // Add return transition time
    return &m_face->m_normalEyes;
  }
  return this;
}

void LookAnimation::draw(u8g2_t* u8g2, uint32_t currentTime) { drawEyes(u8g2, EYE_HEIGHT, get_offset_x(currentTime)); }

void LookAnimation::pause(uint32_t currentTime) { Animation::pause(currentTime); }
void LookAnimation::resume(uint32_t currentTime) { Animation::resume(currentTime); }

int LookAnimation::get_offset_x(uint32_t currentTime) const {
  uint32_t elapsed = getElapsedTime(currentTime);

  // Transition to target
  if (elapsed <= LOOK_TRANSITION_MS) {
    float progress = (float)elapsed / LOOK_TRANSITION_MS;
    float eased_progress = sinf(progress * PI * 0.5f);
    return m_start_offset_x + (int)((m_target_offset_x - m_start_offset_x) * eased_progress);
  }
  // Hold at target
  if (elapsed <= m_holdUntilTime) {
    return m_target_offset_x;
  }
  // Transition back to center
  uint32_t return_elapsed = elapsed - m_holdUntilTime;
  if (return_elapsed <= LOOK_TRANSITION_MS) {
    float progress = (float)return_elapsed / LOOK_TRANSITION_MS;
    float eased_progress = sinf(progress * PI * 0.5f);
    return m_target_offset_x - (int)(m_target_offset_x * eased_progress);
  }

  // After transition back
  return 0;
}
