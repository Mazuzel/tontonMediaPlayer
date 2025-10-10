#include "stringUtils.h"


std::vector<std::string> split_sentence(std::string sen) {
    
    // Create a stringstream object
    std::stringstream ss(sen);
    
    // Variable to hold each word
    std::string word;
    
    // Vector to store the words
    std::vector<std::string> words;
    
    // Extract words from the sentence
    while (ss >> word) {
        
        // Add the word to the vector
        words.push_back(word);
    }
    
    return words;
}

void shortenString(std::string& str, unsigned int len, int maxWordLen, unsigned int maxWordCount)
{
    unsigned int strLen = str.length();
    int offset = strLen - len;
    if (offset <= 0) return;
    
    if (maxWordLen > 1 || maxWordCount > 0)  // start reducing words
    {
        // split string into words and abreviate them
        auto words = split_sentence(str);
        for (unsigned int i = 0; i < words.size(); i++)
        {
            unsigned int wordLen = words[i].length();
            int wordOffset = wordLen - maxWordLen;
            if (wordOffset > 0)
            {
                words[i].resize(wordLen - wordOffset - 1);
                words[i] += ".";
            }
        }
        
        unsigned int wordCount = words.size();
        int wordsToRemove = wordCount - maxWordCount;
        if (wordsToRemove > 0)
        {
            // remove, starting at second word
            for (unsigned int i = 0; i < wordsToRemove; i++)
            {
                words.erase(words.begin() + i + 1);
            }
        }
        
        // recombine string
        std::stringstream ss;
        for (unsigned int i = 0; i < words.size(); i++)
        {
            ss << words[i];
            if (i < words.size() - 1)
            {
                ss << " ";
            }
        }
        str = ss.str();
    }
    
    // check final len again and cut if necessary
    strLen = str.length();
    offset = strLen - len;
    if (offset > 0 && strLen - offset - 3 > 0)
    {
        str.resize(strLen - offset - 3);
        str += "...";
    }
}

string replaceSpacesWithNewline(string& str)
{
    auto words = split_sentence(str);
    // recombine string
    std::stringstream ss;
    for (unsigned int i = 0; i < words.size(); i++)
    {
        ss << words[i];
        if (i < words.size() - 1)
        {
            ss << "\n";
        }
    }
    return ss.str();
}

ofVec2f estimateStringSize(std::string& str)
{
    unsigned int maxWidth = 0;
    
    std::vector<string> lines;
    std::stringstream ss(str);

    if (str.size() > 0)
    {
        std::string to;
        while(std::getline(ss, to, '\n')){
            lines.push_back(to);
        }
    }
    
    // making hypothesis for character average width and height
    for (auto line : lines)
    {
        unsigned int width = 6.7 * line.size();
        if (width > maxWidth) maxWidth = width;
    }
    
    unsigned int height = 7 * lines.size();
    if (lines.size() == 1) height = 5;
    
    return ofVec2f(maxWidth, height);
}

unsigned int getProgramNumberFromElektronPatternStr(std::string elektronPatternStr)
{
    if (elektronPatternStr.size() != 3)
    {
        // TODO show error for bad config
        return 0;
    }

    char bankName = elektronPatternStr[0];
    int bankOffset = 0;
    switch (bankName) {
    case 'A':
        break;
    case 'B':
        bankOffset = 16;
        break;
    case 'C':
        bankOffset = 32;
        break;
    case 'D':
        bankOffset = 48;
        break;
    case 'E':
        bankOffset = 64;
        break;
    case 'F':
        bankOffset = 80;
        break;
    case 'G':
        bankOffset = 96;
        break;
    case 'H':
        bankOffset = 112;
        break;
    }

    std::stringstream strm;
    strm << elektronPatternStr.substr(1, 2);
    int num = std::stoi(strm.str());
    
    return bankOffset + num - 1;
}
