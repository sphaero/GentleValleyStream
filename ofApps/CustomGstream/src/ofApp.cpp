#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(255,255,255);
	ofSetVerticalSync(true);

    gst.setPipeline("udpsrc port=5000 ! application/x-rtp,encoding-name=H264,payload=96 ! rtph264depay ! h264parse ! decodebin", OF_PIXELS_NATIVE, true);
    gst.setPixelFormat(OF_PIXELS_NATIVE);
    gst.startPipeline();
    gst.play();
    tex.allocate(1920,1080,OF_PIXELS_I420);
}

//--------------------------------------------------------------
void ofApp::update(){
    gst.update();
    if(gst.isFrameNew()){
        if (tex.getWidth() != gst.getWidth())
        {
            tex.allocate(gst.getWidth(), gst.getHeight(), OF_PIXELS_I420);
        }
        tex.loadData(gst.getPixels(),gst.getWidth(),gst.getHeight(),OF_PIXELS_I420);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    tex.draw(20,20);
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
