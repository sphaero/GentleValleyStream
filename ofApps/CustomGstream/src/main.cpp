#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
    ofGLWindowSettings settings;
    settings.width = 1280;
    settings.height = 720;
    settings.setGLVersion(4, 4);
    ofCreateWindow(settings);

    ofRunApp( new ofApp());
}
