#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"
#include "ofxXmlSettings.h"

//========================================================================
int main() {

	ofGLFWWindowSettings settings;

	ofxXmlSettings xmlSettings;
	xmlSettings.load("settings.xml");

	// main window (daw)
	settings.setSize(760, 520);
	settings.setPosition(glm::vec2(10, 30));
	settings.resizable = false;
	settings.windowMode = OF_WINDOW;
	settings.monitor = 1;
	settings.title = "Tonton Media Player - DAW";
	shared_ptr<ofAppBaseWindow> dawWindow = ofCreateWindow(settings);

	shared_ptr<ofApp> mainApp(new ofApp);

	// secondary window (mapping)
	if (xmlSettings.getValue("settings:show_video_window", 1) == 1)
	{
		int x = xmlSettings.getValue("settings:window_pos_x", 20);
		int y = xmlSettings.getValue("settings:window_pos_y", 20);
		int w = xmlSettings.getValue("settings:window_pos_w", 1800);
		int h = xmlSettings.getValue("settings:window_pos_h", 1200);
		settings.setSize(w, h);
		settings.setPosition(glm::vec2(x, y));
		settings.resizable = true;
		settings.windowMode = OF_WINDOW;
		settings.monitor = 0;
		settings.title = "Tonton Media Player - Video Output";
		// uncomment next line to share main's OpenGL resources with gui
		settings.shareContextWith = dawWindow;
		shared_ptr<ofAppBaseWindow> mappingWindow = ofCreateWindow(settings);
		mappingWindow->setVerticalSync(false);

		ofAddListener(mappingWindow->events().draw, mainApp.get(), &ofApp::drawMapping);
		mainApp->mappingWindow = mappingWindow;
	}

	ofRunApp(dawWindow, mainApp);
	ofRunMainLoop();
}
