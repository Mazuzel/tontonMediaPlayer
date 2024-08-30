#pragma once

#include "ofMain.h"

#include <string>

std::vector<std::string> split_sentence(std::string sen);

void shortenString(std::string& str, unsigned int len, int maxWordLen = -1, unsigned int maxWordCount = -1);

std::string replaceSpacesWithNewline(std::string& str);

ofVec2f estimateStringSize(std::string& str);

unsigned int getProgramNumberFromElektronPatternStr(std::string elektronPatternStr);
