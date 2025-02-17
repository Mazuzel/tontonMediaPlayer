#include "list.h"

namespace Tonton {
namespace Utils {


ListView::ListView()
{
}

ListView::~ListView()
{
}

void ListView::setup(
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
     ofColor colorNotFocused)
{
    _x = x;
    _y = y;
    _w = w;
    _h = h;
    _title = title;
    _elements = elements;
    _activeElement = activeElement;
    _selectedElement = selectedElement;
    _showIndex = showIndex;
    _colorFocused = colorFocused;
    _colorNotFocused = colorNotFocused;
    generateDraw();
}

void ListView::setFocus(bool focus)
{
    _isFocused = focus;
    generateDraw();
}

void ListView::setTitleColor(ofColor color)
{
    _titleColor = color;
}

void ListView::setSelectedElement(unsigned int index)
{
    _selectedElement = index;
}

void ListView::setActiveElement(unsigned int index)
{
    _activeElement = index;
}

void ListView::generateDraw()
{
    ofSetColor(255);
    _border.clear();
    if (_isFocused)
    {
        _border.setStrokeColor(_colorFocused);
    }
    else
    {
        _border.setStrokeColor(_colorNotFocused);
    }
    _border.setStrokeWidth(2);
    _border.setFilled(false);
    _border.moveTo(_x - 10, _y - 15);
    _border.lineTo(_x - 10 + _w, _y - 15);
    _border.lineTo(_x - 10 + _w, _y + _h - 15);
    _border.lineTo(_x - 10, _y + _h - 15);
    _border.lineTo(_x - 10, _y - 15);
}

void ListView::draw()
{
    ofSetColor(255);
    _border.draw();
    ofDrawBitmapString(_title, _x, _y);
    for (int i = 0; i < _elements.size(); i++)
    {
        ofSetColor(128);
        if (i == _activeElement){
            ofSetColor(255);
        }
        else if (i == _selectedElement) {
            ofSetColor(_colorFocused);
        }
        unsigned int xOffset = 0;
        
        if ( (15 + _lineSpacing * (i + 1)) > _h )
        {
            break;
        }
        
        if (_showIndex)
        {
            ofDrawBitmapString(to_string(i), _x, _y + 15 + _lineSpacing * (i + 1));
            xOffset = 25;
        }
        ofDrawBitmapString(_elements[i], _x + xOffset, _y + 15 + _lineSpacing * (i + 1));
        ofSetColor(255);
    }
}

} // namespace Utils
} // namespace Tonton
