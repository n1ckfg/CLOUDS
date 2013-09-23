
#pragma once

#include "ofMain.h"
#include "CloudsClip.h"
#include "CloudsVisualSystemPreset.h"

class CloudsRun
{
  public:
	CloudsRun(){
		actCount = 0;
	};
	
	int actCount;
    vector<CloudsClip> clipHistory;
    vector<string> presetHistory;
	vector<string> topicHistory;
    map<string, int>timesOnCurrentTopicHistory;
	
	//TODO: acts can be persisted
//	void save();
//	void load();
};