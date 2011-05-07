#ifndef GUI_DRAW_H
#define GUI_DRAW_H

#include <vector>
#include <string>

void GUI_GL_Translate(float x, float y);
void GUI_GL_SetTranslation(float x, float y);
void GUI_GL_PushTranslation();
void GUI_GL_PopTranslation();
void GUI_GL_SetClipRect(float x, float y, float w, float h);

void GUI_GL_Line(float x1, float y1, float x2, float y2);
void GUI_GL_Triangle(float x0, float y0, float x1, float y1, float x2, float y2, const cml::vector4f &col);
void GUI_GL_Rect(float x, float y, float w, float h, const cml::vector4f &col);
void GUI_GL_RectOutline(float x, float y, float w, float h, float thickness, const cml::vector4f &col);
void GUI_GL_RectRaised(float x, float y, float w, float h, float border, const cml::vector4f &shade1, const cml::vector4f &shade2, const cml::vector4f &fill_col);
void GUI_GL_RectTextured(float x, float y, float w, float h, float tminx, float tminy, float tmaxx, float tmaxy, GLuint tex_id, const cml::vector4f &col);
void GUI_GL_TextAligned(float x, float y, float w, float h, int horz, int vert, sf::Font *font, int font_size, const cml::vector4f &font_col, const std::string &str);


void GUI_DrawBegin();
void GUI_DrawEnd();

void GUI_DrawSetLayer(int layer);

void GUI_DrawTranslate(float x, float y);
void GUI_DrawPushTranslation();
void GUI_DrawPopTranslation();

void GUI_DrawSetScreenRect(float x, float y, float w, float h);
void GUI_DrawSetClipRect(float x, float y, float w, float h);

void GUI_DrawSetButtonMode(int mode);
int  GUI_DrawGetButtonMode();
void GUI_DrawFont(sf::Font *sf_font, int size, const cml::vector4f &color);

void GUI_DrawLine(float x0, float y0, float x1, float y1, const cml::vector4f &col);
void GUI_DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, const cml::vector4f &col);
void GUI_DrawRect(float x, float y, float w, float h, const cml::vector4f &col);
void GUI_DrawRectOutline(float x, float y, float w, float h, float thickness, const cml::vector4f &col);
void GUI_DrawRectRaised(float x, float y, float w, float h, float border, const cml::vector4f &shade1, const cml::vector4f &shade2, const cml::vector4f &fill_col);
void GUI_DrawRectTextured(float x, float y, float w, float h, float tminx, float tminy, float tmaxx, float tmaxy, GLuint tex_id, const cml::vector4f &col);
void GUI_DrawText(float x, float y, const std::string &str);
void GUI_DrawTextAligned(float x, float y, float w, float h, int horz, int vert, const std::string &str);
void GUI_DrawArrow(float x, float y, float w, float h, int dir, const cml::vector4f &col);

void GUI_DrawLabel(float x, float y, float w, float h, const std::string &str);
void GUI_DrawButton(float x, float y, float w, float h, bool hot, bool active, const std::string &str);
void GUI_DrawCheckbox(float x, float y, float w, float h, bool selected);
void GUI_DrawCheckboxLabelled(float x, float y, float w, float h, float cx, float cy, float cw, float ch, bool hot, const std::string &str, bool selected);
void GUI_DrawEditBox(float x, float y, float w, float h, bool active, int caret_pos, int selection, float str_offset, const std::string &str);
void GUI_DrawListbox(float x, float y, float w, float h, float item_height, const std::vector<std::string> &data, int data_offset, int choice);
void GUI_DrawListboxMulti(float x, float y, float w, float h, float item_height, const std::vector<std::string> &data, int data_offset, const std::vector<bool> &selected);
void GUI_DrawDropListHeader(float x, float y, float w, float h, bool open, const std::string &text);
void GUI_DrawSlider(float x, float y, float w, float h, float thumb_x, float thumb_y, float thumb_w, float thumb_h); 

void GUI_DrawDropMenuHeaderItem(float x, float y, float w, float h, bool hot, bool active, const std::string &str);
void GUI_DrawPopup(float x, float y, float w, float h);
void GUI_DrawPopupButton(float x, float y, float w, float h, bool hot, bool active, const std::string &left_str, const std::string &right_str);
void GUI_DrawPopupSubMenuButton(float x, float y, float w, float h, bool hot, bool active, const std::string &str);
void GUI_DrawSeparator(float x, float y, float w, float h);

void GUI_DrawFrame(float x, float y, float w, float h, float padding_x, float padding_y);
void GUI_DrawWindow(float x, float y, float w, float h, const std::string &title);

#endif /* GUI_DRAW_H */

