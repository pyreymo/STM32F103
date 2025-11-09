#ifndef FACE_WRAPPER_H
#define FACE_WRAPPER_H

#include <stdint.h>

#include "u8g2.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque pointer to hide the C++ Face object from C code
typedef void* FaceHandle;

FaceHandle Face_Create(void);
void Face_Destroy(FaceHandle handle);
void Face_Init(FaceHandle handle);
void Face_Update(FaceHandle handle, uint32_t currentTime);
void Face_Draw(FaceHandle handle, u8g2_t* u8g2, uint32_t currentTime);

#ifdef __cplusplus
}
#endif

#endif  // FACE_WRAPPER_H
