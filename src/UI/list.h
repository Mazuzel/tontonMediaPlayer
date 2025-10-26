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
        std::string title,
        std::vector<std::string> elements,
        unsigned int activeElement,
        unsigned int selectedElement,
        bool showIndex,
        ofColor colorFocused,
        ofColor colorNotFocused,
        bool drawBackground = false);
    void setFocus(bool focus);
    void setTitleColor(ofColor color);
    void setSelectedElement(unsigned int index);
    void setActiveElement(unsigned int index);
    void generateDraw();
    void draw();
    void setCoordinates(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    void setPageOffset();
protected:
    unsigned int _x = 0;
    unsigned int _y = 0;
    unsigned int _w = 100;
    unsigned int _h = 100;
    bool _isFocused = false;
    bool _drawBackground = false;
    std::string _title;
    std::vector<std::string> _elements;
    ofColor _titleColor;
    unsigned int _activeElement = 0;
    unsigned int _selectedElement = 0;
    unsigned int _nbElementsPerPage = 1;
    unsigned int _pageOffset = 0;
    bool _showIndex = false;
    unsigned int _lineSpacing = 15;
    ofPath _border;
    ofColor _colorFocused;
    ofColor _colorNotFocused;
};

} // namespace Utils
} // namespace Tonton
