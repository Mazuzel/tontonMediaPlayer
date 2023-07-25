#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"

//========================================================================
int main() {

	ofGLFWWindowSettings settings;

	// main window (projection)
	settings.setSize(800, 600);
	settings.setPosition(glm::vec2(300, 30));
	settings.resizable = true;
	settings.windowMode = OF_WINDOW; // OF_FULLSCREEN
	settings.monitor = 1;
	shared_ptr<ofAppBaseWindow> mainWindow = ofCreateWindow(settings);

	// settings window (gui)
	settings.setSize(800, 600);
	settings.setPosition(glm::vec2(0, 30));
	settings.resizable = false;
	settings.windowMode = OF_WINDOW;
	settings.monitor = 0;
	// uncomment next line to share main's OpenGL resources with gui
	settings.shareContextWith = mainWindow;
	shared_ptr<ofAppBaseWindow> guiWindow = ofCreateWindow(settings);
	guiWindow->setVerticalSync(false);

	shared_ptr<ofApp> mainApp(new ofApp);
	mainApp->setupGui();
	ofAddListener(guiWindow->events().draw, mainApp.get(), &ofApp::drawGui);

	ofRunApp(mainWindow, mainApp);
	ofRunMainLoop();
}
