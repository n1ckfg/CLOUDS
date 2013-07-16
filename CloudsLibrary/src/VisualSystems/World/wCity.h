//
//  wPoint.h
//  VSWorld
//
//  Created by Patricio Gonzalez Vivo on 6/24/13.
//
//

#pragma once

#include "ofMain.h"

class wCity : public ofPoint {
public:
    
    wCity();
    
    void place(float _lat, float _long);
    void update();
    void draw(float _alpha = 1.0);

    ofColor color;
    
    ofQuaternion latRot,longRot;
    float   latitud, longitud, freq;
    float   size;
    
    float   noise;
    float   nNoise;
    float   *noisePeaks;
    float   *noiseThreshold;
    
    float   ripplePct;
    float   rippleScale;
    
    bool    bRipple;
    
private:
    void drawRipple( float _pct );
    
};
