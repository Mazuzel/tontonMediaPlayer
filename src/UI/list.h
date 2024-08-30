#pragma once

#include <string>

#include "ofMain.h"

namespace Tonton {
namespace Utils {

class ListView {
public:
    ListView();
    virtual ~ListView();
    void setup(
        unsigned int x,
        unsigned int y,
        unsigned int w,
        unsigned int h,
        std::string title,
        std::vector<std::string> elements,
        unsigned int activeElement,
        unsigned int selectedElement,
        bool showIndex,
        ofColor colorFocused,
        ofColor colorNotFocused);
    void setFocus(bool focus);
    void setTitleColor(ofColor color);
    void setSelectedElement(unsigned int index);
    void setActiveElement(unsigned int index);
    void generateDraw();
    void draw();
protected:
    unsigned int _x;
    unsigned int _y;
    unsigned int _w;
    unsigned int _h;
    bool _isFocused = false;
    std::string _title;
    std::vector<std::string> _elements;
    ofColor _titleColor;
    unsigned int _activeElement = 0;
    unsigned int _selectedElement = 0;
    bool _showIndex = false;
    unsigned int _lineSpacing = 15;
    ofPath _border;
    ofColor _colorFocused;
    ofColor _colorNotFocused;
};

} // namespace Utils
} // namespace Tonton
