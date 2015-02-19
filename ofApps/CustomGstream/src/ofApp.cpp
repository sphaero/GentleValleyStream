#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(255,255,255);
	ofSetVerticalSync(true);
	ofSetFrameRate(30);

	// Uncomment this to show movies with alpha channels
	// fingerMovie.setPixelFormat(OF_PIXELS_RGBA);
	
	fingerMovie.setPlayer(ofPtr<ofCustomGstVideoPlayer>(new ofCustomGstVideoPlayer));

	fingerMovie.load("movies/fingers.mov");
	//fingerMovie.setLoopState(OF_LOOP_NORMAL);
	//fingerMovie.play();
}

//--------------------------------------------------------------
void ofApp::update(){
	fingerMovie.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	fingerMovie.draw(20,20);
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
