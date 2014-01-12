//
//  CloudsRun.cpp
//  CloudsTransitionTest
//
//  Created by James George on 9/1/13.
//
//

#include "CloudsRun.h"

void CloudsRun::actBegan(CloudsActEventArgs &args){
    actCount++;
	cout << "ACT COUNT IS NOW: " << actCount << endl;
}

void CloudsRun::actEnded(CloudsActEventArgs &args){
    
}

void CloudsRun::clipBegan(CloudsClipEventArgs &args){
    
	cout << "CloudsRun::clipBegan ADDING CLIP TO HISTORY " << args.chosenClip.getLinkName() << endl;
    for(int i = 0; i < args.chosenClip.getKeywords().size(); i++){
        accumuluatedTopics[ args.chosenClip.getKeywords()[i] ]++;
    }
    
	clipHistory.push_back(args.chosenClip);
}

void CloudsRun::visualSystemBegan(CloudsVisualSystemEventArgs& args){
    
    if( ! ofContains(presetHistory, args.preset.getID())){
        presetHistory.push_back(args.preset.getID());
    }
    
}

void CloudsRun::visualSystemEnded(CloudsVisualSystemEventArgs& args){
 //   cout<<"VS ended, current run isnt doing anything about it"<<endl;
}

void CloudsRun::questionAsked(CloudsQuestionEventArgs& args){
//    cout<<"Question asked, current run isnt doing anything about it"<<endl;
//    timesQuestionAsked[args.questionClip]
}

void CloudsRun::topicChanged(CloudsTopicEventArgs& args){
    
    if(!ofContains(topicHistory, args.topic)){
        topicHistory.push_back(args.topic);
    }
}

void CloudsRun::preRollRequested(CloudsPreRollEventArgs& args){
//    cout<<"preRoll Requested, current run doesnt care"<<endl;
}