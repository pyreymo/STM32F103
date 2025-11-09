#include "face_wrapper.h"

#include "face.hpp"

extern "C" {

FaceHandle Face_Create(void) {
  // Make this a singleton to avoid C++ style new/delete op
  return &g_faceInstance;
}

void Face_Destroy(FaceHandle handle) {
  (void)handle;  // NO-OP, never destruct for now
}

void Face_Init(FaceHandle handle) {
  Face* face = static_cast<Face*>(handle);
  if (face) {
    face->init();
  }
}

void Face_Update(FaceHandle handle, uint32_t currentTime) {
  Face* face = static_cast<Face*>(handle);
  if (face) {
    face->update(currentTime);
  }
}

void Face_Draw(FaceHandle handle, u8g2_t* u8g2, uint32_t currentTime) {
  Face* face = static_cast<Face*>(handle);
  if (face) {
    face->draw(u8g2, currentTime);
  }
}

}  // extern "C"
