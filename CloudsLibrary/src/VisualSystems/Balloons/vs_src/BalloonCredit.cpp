//
// CLOUDS Interactive Documentary
//
// VISUAL SYSTEMS
//
//
//
//

#include "BalloonCredit.h"

void BalloonCredit::setup()
{
}

float BalloonCredit::getTextWidth()
{
	//figure out the text width here
	width = font->stringWidth(title);//width = 75; //temp
	
	return width;
}

ofVec3f BalloonCredit::getLeft()
{
	return ofVec3f(pos.x - width*.5, pos.y, pos.z);
}

void BalloonCredit::update()
{
	
	ofNode n;
	n.setPosition( pos );
	n.lookAt(camera->getPosition(), ofVec3f(0,1,0));
	ofVec3f axis; float angle;
	n.getOrientationQuat().getRotate(angle, axis);
	
	width = MAX(font->stringWidth(title), font->stringWidth(name)  ) * .25;
	
	left = ofVec3f(-width * .5, 0, 0) * n.getGlobalTransformMatrix();
	right = ofVec3f(width * .5, 0, 0) * n.getGlobalTransformMatrix();
    
    
//    cout<<"credit pos " <<pos<<endl;
//    cout<<"camera pos " <<camera->getPosition()<<endl;

    
}

void BalloonCredit::draw()
{
	ofPushStyle();
	ofPushMatrix();
	ofSetColor(255);
    
	ofDisableLighting();
	ofDisableAlphaBlending();
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    
	ofNode n;
	n.setPosition( pos );
    ofVec3f lookAtPos = camera->getPosition();
    lookAtPos.y = pos.y;
	n.lookAt(lookAtPos, ofVec3f(0,1,0));
	ofVec3f axis; float angle;
	n.getOrientationQuat().getRotate(angle, axis);
	
	// Translate the object to its position.
	// Perform the rotation.
    
    ofScale(.15,.15,.15);
    
    //ofTranslate( pos );
    ofMultMatrix(n.getGlobalTransformMatrix());
    ofRotate(180, 0, 0, 1);
//	ofRotate(angle, axis.x, axis.y, axis.z);
	


	//draw text at the position
	float titleWidth  = font->stringWidth(title);
	float titleHeight = font->stringHeight(title);
	float nameWidth  = font->stringWidth(name);
	font->drawString(title, -titleWidth/2, titleHeight/2);
	font->drawString(name, -nameWidth/2, titleHeight * 2);
	
	ofEnableLighting();
	ofPopMatrix();
	ofPopStyle();
	
//	glLineWidth(4);
//	ofSetColor(225, 255, 0);
//	ofLine(left, right);
}
	

