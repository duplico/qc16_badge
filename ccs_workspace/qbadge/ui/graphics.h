/*
 * graphics.h
 *
 *  Created on: Jun 29, 2019
 *      Author: george
 */

#ifndef UI_GRAPHICS_H_
#define UI_GRAPHICS_H_

void fadeRectangle(Graphics_Context *gr_context, Graphics_Rectangle *rect);
void qc16gr_drawImage(const Graphics_Context *context,
                      const Graphics_Image *bitmap,
                      int16_t x,
                      int16_t y);
#endif /* UI_GRAPHICS_H_ */