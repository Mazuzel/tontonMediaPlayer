#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"

//========================================================================
int main() {

	ofGLFWWindowSettings settings;

	// main window (daw)
	settings.setSize(800, 600);
	settings.setPosition(glm::vec2(300, 30));
	settings.resizable = false;
	settings.windowMode = OF_WINDOW;
	settings.monitor = 1;
	shared_ptr<ofAppBaseWindow> dawWindow = ofCreateWindow(settings);

	// secondary window (mapping)
	settings.setSize(1600, 1000);
	settings.setPosition(glm::vec2(0, 30));
	settings.resizable = true;
	settings.windowMode = OF_WINDOW;
	settings.monitor = 0;
	// uncomment next line to share main's OpenGL resources with gui
	settings.shareContextWith = dawWindow;
	shared_ptr<ofAppBaseWindow> mappingWindow = ofCreateWindow(settings);
	mappingWindow->setVerticalSync(false);

	shared_ptr<ofApp> mainApp(new ofApp);
	ofAddListener(mappingWindow->events().draw, mainApp.get(), &ofApp::drawMapping);
	mainApp->mappingWindow = mappingWindow;

	ofRunApp(dawWindow, mainApp);
	ofRunMainLoop();
}
