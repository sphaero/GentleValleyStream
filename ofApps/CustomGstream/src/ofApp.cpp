#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(255,255,255);
	ofSetVerticalSync(true);

    gst.setPipeline("videotestsrc", OF_PIXELS_NATIVE, true);
    gst.startPipeline();
    gst.play();
    //tex.allocate(1920,1080,OF_PIXELS_I420);
    ofLogError() << ofIsGLProgrammableRenderer();
}

//--------------------------------------------------------------
void ofApp::update(){
    gst.update();
    /*if(gst.isFrameNew()){
        if (tex.getWidth() != gst.getWidth())
        {
            tex.allocate(gst.getWidth(), gst.getHeight(), OF_PIXELS_I420);
        }
        tex.loadData(gst.getPixels(),gst.getWidth(),gst.getHeight(),OF_PIXELS_I420);
    }*/
}

//--------------------------------------------------------------
void ofApp::draw(){
    if(gst.getTexture() != NULL ) gst.getTexture()->draw(20,20);
    else ofLogError() << "NULL";

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
