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
     std::string title,
     std::vector<std::string> elements,
     unsigned int activeElement,
     unsigned int selectedElement,
     bool showIndex,
     ofColor colorFocused,
     ofColor colorNotFocused)
{
    _title = title;
    _elements = elements;
    _activeElement = activeElement;
    _selectedElement = selectedElement;
    _showIndex = showIndex;
    _colorFocused = colorFocused;
    _colorNotFocused = colorNotFocused;
    generateDraw();
}

void ListView::setCoordinates(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    _x = x;
    _y = y;
    _w = w;
    _h = h;
    _nbElementsPerPage = _h / _lineSpacing - 3;
    _nbElementsPerPage = min<unsigned int>(_nbElementsPerPage, _elements.size());
    _pageOffset = 0;
    setPageOffset();
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

void ListView::setPageOffset()
{
    if (_selectedElement >= _pageOffset + _nbElementsPerPage - 2)
    {
        _pageOffset = min<int>(_elements.size() - _nbElementsPerPage, _selectedElement - _nbElementsPerPage + 2);
    }
    else if (_selectedElement < _pageOffset + 1)
    {
        _pageOffset = max<int>(0, _selectedElement - 1);
    }
}

void ListView::setSelectedElement(unsigned int index)
{
    _selectedElement = index;
    setPageOffset();
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
    if (_isFocused)
    {
        ofSetColor(_colorFocused);
    }
    else
    {
        ofSetColor(_colorNotFocused);
    }
    ofDrawRectangle(_x - 9, _y - 14, _w - 2, 20);
    if (_isFocused)
    {
        ofSetColor(10);
    }
    else
    {
        ofSetColor(255);
    }
    ofDrawBitmapString(_title, _x, _y);
    ofSetColor(255);
    unsigned int idxMax = _pageOffset + _nbElementsPerPage;
    if (idxMax > _elements.size())
    {
        idxMax = _elements.size();
    }

    for (int i = _pageOffset; i < idxMax; i++)
    {
        ofSetColor(128);
        if (i == _activeElement){
            ofSetColor(255);
        }
        else if (i == _selectedElement) {
            ofSetColor(_colorFocused);
        }
        unsigned int xOffset = 0;
        
        unsigned int iPage = i - _pageOffset;
        if ( (15 + _lineSpacing * (iPage + 1)) > _h )
        {
            break;
        }
        
        if (iPage == (_nbElementsPerPage - 1) && _pageOffset + _nbElementsPerPage < _elements.size())
        {
            ofSetColor(128);
            ofDrawBitmapString("...", _x + 0, _y - 5 + _lineSpacing + _lineSpacing * (_nbElementsPerPage));
        }
        else if (iPage == 0 && _pageOffset > 0)
        {
            ofSetColor(128);
            ofDrawBitmapString("...", _x + 0, _y - 5 + _lineSpacing + _lineSpacing * (0 + 1));
        }
        else
        {
            if (_showIndex)
            {
                ofDrawBitmapString(to_string(i), _x, _y + _lineSpacing + _lineSpacing * (iPage + 1));
                xOffset = 25;
            }
            ofDrawBitmapString(_elements[i], _x + xOffset, _y + _lineSpacing + _lineSpacing * (iPage + 1));
        }

        ofSetColor(255);
    }
}

} // namespace Utils
} // namespace Tonton
