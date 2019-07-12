/*
 * menus.h
 *
 *  Created on: Jul 12, 2019
 *      Author: george
 */

#ifndef UI_MENUS_MENUS_H_
#define UI_MENUS_MENUS_H_

extern uint8_t mission_picking;
void ui_mainmenu_do(UInt events);
void ui_missions_do(UInt events);
void ui_scan_do(UInt events);
void ui_files_do(UInt events);
void ui_info_do(UInt events);

#endif /* UI_MENUS_MENUS_H_ */
