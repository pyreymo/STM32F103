#ifndef FACE_H_
#define FACE_H_

#include "u8g2.h"
#include <stdint.h>

/**
 * @brief 初始化表情模块
 * @note  应该在主循环开始前调用一次。
 */
void Face_Init(void);

/**
 * @brief 更新表情的内部状态机
 * @note  这个函数应该在主循环中被频繁调用，以驱动动画逻辑。
 * @param currentTime 当前的系统时间戳 (例如 osKernelGetTickCount())
 */
void Face_Update(uint32_t currentTime);

/**
 * @brief 将当前帧的表情绘制到 u8g2 的缓冲区
 * @note  这个函数应该在 u8g2_ClearBuffer() 之后，u8g2_SendBuffer() 之前调用。
 * @param u8g2        指向 u8g2 实例的指针
 * @param currentTime 当前的系统时间戳，用于计算动画插值
 */
void Face_Draw(u8g2_t *u8g2, uint32_t currentTime);

#endif /* FACE_H_ */