#pragma once

#include <cstdint>

#include "u8g2.h"

class Face;

extern Face g_faceInstance;

/**
 * @brief Abstract Base Class for all face animations.
 *
 * This defines the common interface for animations like Normal, Blink, Look, etc.
 */
class Animation {
 public:
  // Using a virtual destructor is crucial for polymorphic base classes
  virtual ~Animation() = default;

  virtual void start(uint32_t currentTime) = 0;
  virtual Animation* update(uint32_t currentTime) = 0;
  virtual void draw(u8g2_t* u8g2, uint32_t currentTime) = 0;
  virtual int get_offset_x(uint32_t currentTime) const = 0;

  // Default pause/resume do nothing. Can be overridden.
  virtual void pause(uint32_t currentTime);
  virtual void resume(uint32_t currentTime);

 protected:
  // Constructor requires a pointer to the parent Face object
  Animation(Face* face);

  uint32_t getElapsedTime(uint32_t currentTime) const;

  Face* const m_face;  // Pointer to the main Face object

  bool m_isPaused = false;
  uint32_t m_pausedElapsed = 0;
  uint32_t m_startTime = 0;
};

/**
 * @brief Normal, resting eye animation.
 */
class NormalEyesAnimation : public Animation {
 public:
  explicit NormalEyesAnimation(Face* face);
  void start(uint32_t currentTime) override;
  Animation* update(uint32_t currentTime) override;
  void draw(u8g2_t* u8g2, uint32_t currentTime) override;
  int get_offset_x(uint32_t currentTime) const override;

 private:
  uint32_t m_nextLookTime = 0;
};

/**
 * @brief Eye blinking animation.
 * This is an "interrupting" animation that returns to a previous state.
 */
class BlinkAnimation : public Animation {
 public:
  explicit BlinkAnimation(Face* face);
  void start(uint32_t currentTime) override;
  Animation* update(uint32_t currentTime) override;
  void draw(u8g2_t* u8g2, uint32_t currentTime) override;
  int get_offset_x(uint32_t currentTime) const override;

  void setReturnAnimation(Animation* anim);

 private:
  enum State { CLOSING, CLOSED, OPENING };
  State m_internalState = CLOSING;
  Animation* m_returnAnimation = nullptr;
};

/**
 * @brief Look-around animation.
 */
class LookAnimation : public Animation {
 public:
  explicit LookAnimation(Face* face);
  void start(uint32_t currentTime) override;
  Animation* update(uint32_t currentTime) override;
  void draw(u8g2_t* u8g2, uint32_t currentTime) override;
  int get_offset_x(uint32_t currentTime) const override;
  void pause(uint32_t currentTime) override;
  void resume(uint32_t currentTime) override;

  void setTarget(int start_offset, int target_offset);

 private:
  uint32_t m_holdUntilTime = 0;
  int m_start_offset_x = 0;
  int m_target_offset_x = 0;
};

/**
 * @brief The main class managing the face's state and animations.
 */
class Face {
 public:
  Face();
  // No copying or moving, this is a singleton-like object
  Face(const Face&) = delete;
  Face& operator=(const Face&) = delete;

  void init();
  void update(uint32_t currentTime);
  void draw(u8g2_t* u8g2, uint32_t currentTime);

 private:
  friend class NormalEyesAnimation;
  friend class BlinkAnimation;
  friend class LookAnimation;

  NormalEyesAnimation m_normalEyes;
  BlinkAnimation m_blink;
  LookAnimation m_look;

  Animation* m_currentAnimation;
  uint32_t m_nextBlinkTime;
};
