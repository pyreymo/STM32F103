#ifndef FACE_H_
#define FACE_H_

#include <stdbool.h>
#include <stdint.h>

#include "u8g2.h"

typedef struct Face_t Face_t;

/**
 * @brief 创建并初始化一个新的表情实例
 * @note  使用完毕后，应调用 Face_Destroy() 释放内存。
 * @return 指向新创建的 Face_t 实例的指针，如果内存分配失败则返回 NULL。
 */
Face_t* Face_Create(void);

/**
 * @brief 销毁一个表情实例并释放相关内存
 * @param face 指向要销毁的 Face_t 实例的指针
 */
void Face_Destroy(Face_t* face);

/**
 * @brief 重置表情到初始状态
 * @note  这个函数可以被调用来重新开始动画序列。
 * @param face 指向 Face_t 实例的指针
 */
void Face_Init(Face_t* face);

/**
 * @brief 更新表情的内部状态机
 * @note  这个函数应该在主循环中被频繁调用，以驱动动画逻辑。
 * @param face        指向 Face_t 实例的指针
 * @param currentTime 当前的系统时间戳 (例如 osKernelGetTickCount())
 */
void Face_Update(Face_t* face, uint32_t currentTime);

/**
 * @brief 将当前帧的表情绘制到 u8g2 的缓冲区
 * @note  这个函数应该在 u8g2_ClearBuffer() 之后，u8g2_SendBuffer() 之前调用。
 * @param face        指向 Face_t 实例的指针
 * @param u8g2        指向 u8g2 实例的指针
 * @param currentTime 当前的系统时间戳，用于计算动画插值
 */
void Face_Draw(Face_t* face, u8g2_t* u8g2, uint32_t currentTime);

#endif /* FACE_H_ */