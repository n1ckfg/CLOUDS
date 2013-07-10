//
//  CloudsStoryEngine.cpp
//  CloudsFCPParser
//
//  Created by James George on 3/17/13.
//
//

#include "CloudsStoryEngine.h"


CloudsStoryEngine::CloudsStoryEngine(){
	parser = NULL;
	visualSystems = NULL;

	hasclip = false;
	isSetup = false;
	printDecisions = true;
	combinedClipsOnly = false;
	totalFramesWatched = 0;
	waitingForNextClip = false;
	watchingVisualSystem = false;
	printCriticalDecisions = false;
	
	fixedClipDelay = 5;
	maxTimesOnTopic = 4;

}

CloudsStoryEngine::~CloudsStoryEngine(){
	if(isSetup){
		ofRemoveListener(ofEvents().update, this, &CloudsStoryEngine::update);
	}
}

void CloudsStoryEngine::setup(){
	if(!isSetup){
		ofAddListener(ofEvents().update, this, &CloudsStoryEngine::update);
		isSetup = true;

        KeywordDichotomy d;
        dichotomyThreshold = 3;

        d.left = "#art";
        d.right = "#tech";
        d.balance =0;
        dichotomies.push_back(d);

        d.left = "#emotional";
        d.right = "#logical";
        d.balance =0;
        dichotomies.push_back(d);
        
        d.left = "#breakthrough";
        d.right = "#obstacle";
        d.balance =0;
        dichotomies.push_back(d);

        d.left = "#inspiring";
        d.right = "#discouraging";
        d.balance =0;
        dichotomies.push_back(d);

        d.left = "#fun";
        d.right = "#serious";
        d.balance =0;
        dichotomies.push_back(d);
        
        d.left = "#sincere";
        d.right = "#ironic";
        d.balance =0;
        dichotomies.push_back(d);

        d.left = "#mindblowing";
        d.right = "#mundane";
        d.balance =0;
        dichotomies.push_back(d);

        d.left = "#rational";
        d.right = "#surreal";
        d.balance =0;
        dichotomies.push_back(d);
	}
}

void CloudsStoryEngine::update(ofEventArgs& args){
	if(waitingForNextClip && nextClipTime < ofGetElapsedTimef()){
		playNextClip();
	}
}

void CloudsStoryEngine::seedWithClip(CloudsClip& seed){
	
	seedWithClip(seed, seed.getKeywords()[ ofRandom(seed.getKeywords().size()) ]);

}

void CloudsStoryEngine::seedWithClip(CloudsClip& seed, string topic){
	clipHistory.clear();
	topicHistory.clear();
	peopleVisited.clear();
	allNextClips.clear();
	
	hasclip = false;
	freeTopic = false;
	
	//freeTopic = true;
	currentTopic =   topic;
	//select a random topic from the clip
	
	buildAct(seed, 10*60);
	//CloudsStoryEventArgs args(seed,allNextClips,currentTopic);
	//ofNotifyEvent(events.storyBegan,args);
	
	//loadClip( seed );
}

void CloudsStoryEngine::buildAct(CloudsClip seed, float seconds){
	
	float totalSecondsEnqueued = 0;
	bool deadEnd = false;
	string topic = currentTopic;
	clipQueue.clear();
    act.clearAct();
	
	CloudsClip clip = seed;

    act.addClipToAct(clip,totalSecondsEnqueued);
	clearDichotomiesBalance();
    
	vector<string> topicHistory;
	topicHistory.push_back(topic);
	act.setTopicInHistory(topic);
  
	int timesOnCurrentTopic = 0;
	bool freeTopic = false;
	while( totalSecondsEnqueued < seconds ){
		freeTopic |= timesOnCurrentTopic > maxTimesOnTopic;

		if(freeTopic){
			string newTopic = selectTopic(clip, topicHistory, topic);
			if(newTopic == topic){
				break;

			}
			
			topic = newTopic;
			timesOnCurrentTopic = 1;
			freeTopic = false;
			topicHistory.push_back(topic);

		}
        cout<<"Times on topic :"<< topic<<", "<<timesOnCurrentTopic<<endl;
        //storing a copy of current topic for each clip
		act.setTopicInHistory(topic);
        //get all meta data options
		vector<CloudsClip> nextOptions = parser->getClipsWithKeyword(topic);
		
		//add all manual links
		vector<CloudsLink>& links = parser->getLinksForClip( clip );
		for(int i = 0; i < links.size(); i++){
            
            bool valid = true;
            for(int c = 0; c < nextOptions.size(); c++){
                if(nextOptions[c].getLinkName() == links[i].targetName){
                    //don't add...
                    valid = false;
                    break;
                }
            }
            
            if(valid){
                nextOptions.push_back( parser->getClipWithLinkName(links[i].targetName) );
            }
		}
        
		
		//remove suppressions
		vector<CloudsLink>& suppressions = parser->getSuppressionsForClip( clip );
		for(int i = 0; i < suppressions.size(); i++){
			for(int nt = nextOptions.size()-1; nt >= 0; nt--){
				if(nextOptions[nt].getLinkName() == suppressions[i].targetName){
					nextOptions.erase(nextOptions.begin() + nt);
				}
			}
		}
        
        //remove clips that share just one keyword...
		int topScore = 0;
		for(int i = 0; i < nextOptions.size(); i++){
			CloudsClip& nextClipOption = nextOptions[ i ];
			int score = scoreForClip(act.getAllClips(), nextClipOption, topic);
			totalPoints += score;
			topScore = MAX(topScore, score);
			nextClipOption.currentScore = score;
		}
	
		if(topScore == 0){
			//Dead end!
			cout << "Dead end, no clips left" << endl;
			freeTopic = true;
			continue;
		}
		
		vector<CloudsClip> winningClips;
		for(int i = 0; i < nextOptions.size(); i++){
			if(nextOptions[i].currentScore == topScore){
				winningClips.push_back(nextOptions[i]);
			}
		}
		
		//select next clip
		clip = winningClips[ofRandom(winningClips.size())];
		
        updateDichotomies(clip);

        act.addClipToAct(clip,totalSecondsEnqueued);
        act.setTopicForClip(topic, clip.getLinkName());
		totalSecondsEnqueued += clip.getDuration();

		timesOnCurrentTopic++;
		//Decide if a question is to be asked
		
	}
	
    // do the same thing again for Visual Systems
    float lastVisualSystemEnded = 0;
    float currentPosition = 0;
    bool systemRunning = false;
    
    float systemMaxRunTime = 60*2;
    float maxVisualSystemGapTime = 60*3;
    float longClipThreshold = 30;
    float longClipFadeInPercent = .5;
    float visualSystemStartTime;
    string currentTopic = act.getTopicInHistory(0);
    string previousTopic = currentTopic;
    float visualSystemDuration  = 0;
    for(int i=0; i < act.getAllClips().size(); i++){
        
        CloudsClip& currentClip = act.getClipInAct(i);
        string currentTopic = act.getTopicInHistory(i);
        float clipStartTime = act.getItemForClip(currentClip).startTime;
        float clipEndTime = act.getItemForClip(currentClip).endTime;
        
        if( systemRunning ) {
            
            visualSystemDuration = clipStartTime - visualSystemStartTime;
            if(visualSystemDuration > systemMaxRunTime || currentTopic != previousTopic){
                if(currentClip.getDuration() > longClipThreshold){
                    visualSystemDuration += currentClip.getDuration()*longClipFadeInPercent;
                }
                act.addVisualSystem(visualSystems->getRandomVisualSystem(), visualSystemStartTime, visualSystemDuration);
                systemRunning = false;
                lastVisualSystemEnded = visualSystemStartTime + visualSystemDuration;
            }
        }
        else {
            float timeSinceLastVisualSystem = clipEndTime - lastVisualSystemEnded;
            if(timeSinceLastVisualSystem > maxVisualSystemGapTime){
                
                //if the clip is shorter than the 30 seconds dont start the VS during the clip.
                if (currentClip.getDuration() > longClipThreshold) {
                    visualSystemStartTime  = clipStartTime + currentClip.getDuration()*longClipFadeInPercent;
                }
                else{
                    visualSystemStartTime = clipStartTime;
                }
                systemRunning = true;
            }
        }

        previousTopic = currentTopic;
        
    }

    if(systemRunning){
        float clipStartTime = act.getItemForClip(act.getClipInAct(act.getAllClips().size()-1)).startTime;
        float clipEndTime = act.getItemForClip(act.getClipInAct(act.getAllClips().size()-1)).endTime;
        act.addVisualSystem(visualSystems->getRandomVisualSystem(), visualSystemStartTime, visualSystemDuration);
        systemRunning = false;
        lastVisualSystemEnded = visualSystemStartTime + visualSystemDuration;
    }
    
    act.populateTime(); 
    
	cout << "CLIPS:" << endl;
	for(int i = 0; i < act.getAllClips().size(); i++){
		cout << "	CLIP: " << act.getClipInAct(i).getLinkName() << endl;
	}
	
	cout << "TOPICS: " << endl;
	for(int i = 0; i < act.getAllTopics().size(); i++){
		cout << "	TOPIC: " << act.getTopicInHistory(i) << endl;
	}
	
    
    cout<<"TOPIC SIZE: "<<act.getAllTopics().size()<<endl;
    cout<<"Clips SIZE: "<<act.getAllClips().size()<<endl;
    
	// TEMP
//	CloudsStoryEventArgs argsA(seed,allNextClips,currentTopic);
//	ofNotifyEvent(events.storyBegan,argsA);
	
	
//	CloudsStoryEventArgs argsB(act.getClipInAct(0), allNextClips, act.getTopicInHistory(0));
//	ofNotifyEvent(events.clipBegan,argsB);
	
}

void CloudsStoryEngine::clearDichotomiesBalance(){
    
    for(int i=0; i <dichotomies.size();i++){
        dichotomies[i].balance =0;
    }
}

void CloudsStoryEngine::updateDichotomies(CloudsClip& clip){
    vector<string> specialkeywords= clip.getSpecialKeywords();
    
    for(int i=0; i <dichotomies.size();i++){
        for(int j=0; j<specialkeywords.size();j++){
         
            if(dichotomies[i].left == specialkeywords[j]){
                dichotomies[i].balance -= 1;
                cout<<dichotomies[i].left<<": +1"<<endl;
                
            }
            else if (dichotomies[i].right == specialkeywords[j]){
                cout<<dichotomies[i].right<<": +1"<<endl;
                dichotomies[i].balance += 1;
            }
            
        }
    }
}

//TODO: use coehsion and map distance to fix dead ends
string CloudsStoryEngine::selectTopic(CloudsClip& clip, vector<string>& topicHistory, string topic){
	
	vector<string>& topics = clip.getKeywords();
	vector<float> topicScores;
	topicScores.resize(topics.size());
	float topicHighScore = 0;
	for(int i = 0; i < topics.size(); i++){
		topicScores[i] = scoreForTopic(topicHistory, act.getAllClips(), topic, topics[i]);
		topicHighScore = MAX(topicHighScore,topicScores[i]);
	}
	
	if(topicHighScore == 0){
		//Dead end!
		cout << "Dead end, no topics left" << endl;
		return topic;
	}
	
	vector<string> winningTopics;
	for(int i = 0; i < topics.size(); i++){
		if(topicScores[i] == topicHighScore){
			winningTopics.push_back(topics[i]);
		}
	}
	
	cout << "topic changed from " << topic;
	topic = winningTopics[ ofRandom(winningTopics.size()) ];
	cout << " to " << topic << endl;
	
	return topic;
}

bool CloudsStoryEngine::playNextClip(){
	
	waitingForNextClip = false;
	
	//CHECK PRECONDITION
	if(parser == NULL){
		ofLogError("Clouds Visualizer or Database is null! exiting select clip");
		return false;
	}
	
	//We need a clip to be able to select a new one.
	if(!hasclip){
		ofLogError("Cannot select new clip without a seed");
		return false;
	}
	
	if(atDeadEnd()){
        ofLogError("CloudsStoryEngine::playNextClip") << " At dead end!";
		return false;
	}
	
	//Do a weighted selection based on the random value
	int randomClip = ofRandom( validNextClips.size() );
	cout << "SELECTED CLIP:" << randomClip << "/" << validNextClips.size();
	cout << " " << validNextClips[randomClip].getLinkName() << endl;
		
	loadClip( validNextClips[randomClip] );

	return true;
}

void CloudsStoryEngine::loadClip(CloudsClip& clip){

	hasclip = true;
	
	timesOnTopic++;
	if(timesOnTopic >= maxTimesOnTopic){
		freeTopic = true;
	}

	if(hasclip && (freeTopic || currentTopic == "") ){
		chooseNewTopic(clip);
	}

	checkVisualSystems();
		
	currentClip = clip;
	clipHistory.push_back( clip );
	peopleVisited[ clip.person ]++;
	
	populateNextClips();
	
	CloudsStoryEventArgs args(currentClip, currentTopic);
	ofNotifyEvent(events.clipBegan,args);
}

bool CloudsStoryEngine::clipEnded(){

	totalFramesWatched += (currentClip.endFrame - currentClip.startFrame);
	
	CloudsStoryEventArgs args(currentClip,currentTopic);
	args.timeUntilNextClip = getNextClipDelay();
	ofNotifyEvent(events.clipEnded, args, this);
	if(atDeadEnd()){
		ofNotifyEvent(events.storyEnded, args, this);
	}
	else{
		waitingForNextClip = true;
		nextClipTime = ofGetElapsedTimef() + args.timeUntilNextClip;
	}
	
	buildAct( act.getAllClips()[act.getAllClips().size()-1], 60*5 );
}

void CloudsStoryEngine::chooseNewTopic(CloudsClip& upcomingClip){
	
	//TO CHOOSE A NEW TOPIC WE WANT TO FIND THE MOST "SIMILAR" TOPIC
	//this means the topic that shares the highest percent of clips with the current topic
	//This will prioritize
	vector<string> topics = parser->getSharedKeywords(currentClip, upcomingClip);
	//vector<string> topics = upcomingClip.keywords;
	bool topicSwitched = false;
	if(topics.size() == 0){
		cout << "	TOPIC SWITCH: NO SHARED TOPICS" << endl;
		return;
	}

	cout << "Switching topic from " << currentTopic << " with " << topics.size() << " shared topics" << endl;
	string winningTopic = "";
	float highScore = 0;
	for(int i = 0; i < topics.size(); i++){
		
		if(ofContains(topicHistory, topics[i])){
			continue;
		}
		
		int sharedClips = parser->getNumberOfSharedClips(currentTopic,topics[i]);
		int totalClips = parser->getNumberOfClipsWithKeyword(topics[i]);
		float score = 1.0 * sharedClips / totalClips;
		cout << "TOPIC " << topics[i] << " SCORE " << score << " : " << sharedClips << "/" << totalClips << endl;
		if(highScore <= score){
			currentTopic = topics[i];
			highScore = score;
			topicSwitched = true;
		}
	}
	
	topicHistory.push_back(currentTopic);
	if(topicSwitched){
		timesOnTopic = 0;
		freeTopic = false;
		cout << "	TOPIC SWITCH TO " << currentTopic << endl;
		if(watchingVisualSystem){
			CloudsVisualSystemEventArgs args(currentVisualSystem);
			ofNotifyEvent(events.visualSystemEnded, args);
			watchingVisualSystem = false;
		}
		CloudsStoryEventArgs args(currentClip, currentTopic);
		ofNotifyEvent(events.topicChanged, args);

	}
	else{
		cout << "	FAILED TO SWITCH TOPIC " << currentTopic << endl;
	}
}

void CloudsStoryEngine::checkVisualSystems(){
	if(visualSystems == NULL || visualSystems->getPresets().size() == 0){
		return;
	}
	
	if(timesOnTopic == 2){
		CloudsVisualSystemPreset preset = visualSystems->getRandomVisualSystem();
		CloudsVisualSystemEventArgs args(visualSystems->getRandomVisualSystem());
		ofNotifyEvent(events.visualSystemBegan, args);
		cout << "SHOW VISUAL SYSTEM!" << endl;
		watchingVisualSystem = true;
	}
	
	if(timesOnTopic == maxTimesOnTopic - 1){
		watchingVisualSystem = false;
		CloudsVisualSystemEventArgs args(currentVisualSystem);
		ofNotifyEvent(events.visualSystemEnded, args);
	}
}

bool CloudsStoryEngine::populateNextClips(){
	
	//get all the adjascent clips, assign weights to them and select
	vector<CloudsClip> nextClips;
	if(freeTopic){
		nextClips = parser->getClipsWithKeyword(currentClip.getKeywords());
	}
	else{
		nextClips = parser->getClipsWithKeyword(currentTopic);
	}
	
	vector<CloudsLink>& links = parser->getLinksForClip( currentClip );

	cout << "RELATED CLIPS TO: " << currentTopic << " " << nextClips.size() << " AND " << links.size() << " LINKS. ASSIGNING VALUES:" << endl;
	
	for(int i = 0; i < links.size(); i++){
		nextClips.push_back( parser->getClipWithLinkName(links[i].targetName) );
	}
	
//	if(printDecisions){
//		cout << "REALTED CLIPS:" << endl;
//		for(int i = 0; i < related.size(); i++){
//			cout << "	\"" << related[i].getLinkName() << "\""
//				 << " Topics: [" << ofJoinString(related[i].keywords, ",") << "]" << endl;
//		}
//	}
	
	totalPoints = 0;

	validNextClips.clear();
	allNextClips.clear();

	int topScore = 0;
	for(int i = 0; i < nextClips.size(); i++){
		CloudsClip& m = nextClips[ i ];
		int score = scoreForClip( m );
		if(score != 0){
			totalPoints += score;
			if(score > topScore) topScore = score;
			m.currentScore = score;
			allNextClips.push_back(m);
		}
	}
	
	//if no clips were valid, return false
	if(totalPoints == 0) {
		cout << "DEAD END ENCOUNTRED, FREEING TOPIC" << endl;
		
		// if we are at a dead end because of the lack of clips,
		// recurse the call with an open topic
		// May still be dead from exhausted clips or topics on the same node
		
		if(!freeTopic){
			freeTopic = true;
			return populateNextClips();
		}
				
		ofLogError("Dead end found at clip " + currentClip.getLinkName());
		return false;
	}
	

	for(int i = 0; i < allNextClips.size(); i++){
		if(allNextClips[i].currentScore == topScore){
			validNextClips.push_back( allNextClips[i] );
		}
	}
	
	if(printDecisions){
		cout << "VALID CLIPS WITH SCORE " << topScore << endl;
		for(int i = 0; i < validNextClips.size(); i++){
			cout << "	" << validNextClips[i].getLinkName() << endl;
		}
	}
	
	return true;
}

float CloudsStoryEngine::scoreForTopic(vector<string>& topicHistory, vector<CloudsClip>& history,
									 string currentTopic, string newTopic)
{
	if(currentTopic == newTopic){
		if(printDecisions) cout << "	REJECTED Topic " << currentTopic << ": same as new topic" << endl;
		return 0;
	}
	
	if(ofContains(topicHistory, newTopic)){
		if(printDecisions) cout << "	REJECTED Topic " << newTopic << ": has already been explored" << endl;
		return 0;
	}
	
	int score = 5;
	if(history.size() > 1 && ofContains( history[history.size()-2].getKeywords(), newTopic) ){
		if(printDecisions) cout << "	LAST CLIP " << history[history.size()-2].getLinkName() << " shares topic " << newTopic << endl;
		score += 10;
	}
	
	if(history.size() > 2 && ofContains( history[history.size()-3].getKeywords(), newTopic) ){
		if(printDecisions) cout << "	TWO CLIPS AGO " << history[history.size()-3].getLinkName() << " shares topic " << newTopic << endl;
		score += 10;
	}
	
	int sharedClips = parser->getNumberOfSharedClips(currentTopic,newTopic);
	int totalClips = parser->getNumberOfClipsWithKeyword(newTopic);
	float relevancyScore = (1.0 * sharedClips / totalClips) * 10;
	score += relevancyScore;
	cout << "	TOPIC " << newTopic << " SCORE " << relevancyScore << " : " << sharedClips << "/" << totalClips << endl;
	
	float cohesionScore;//Add cohesion score to favor topics

	return score;
}

float CloudsStoryEngine::scoreForClip(vector<CloudsClip>& history, CloudsClip& potentialNextClip, string topic){
	
	CloudsClip& currentlyPlayingClip = history[history.size()-1];
	
	//rejection criteria -- flat out reject clips on some basis
	if(combinedClipsOnly && !potentialNextClip.hasCombinedVideo){
		if(printDecisions) cout << "	REJECTED Clip " << potentialNextClip.getLinkName() << ": no combined video file" << endl;
		return 0;
	}
    
	
//    if(potentialNextClip.cluster.Id == ""){
//		if(printDecisions) cout << "	REJECTED Clip " << potentialNextClip.getLinkName() << ": disconnect from cluster map =(" << endl;
//        return 0;
//    }
    
	bool link = parser->clipLinksTo( currentlyPlayingClip.getLinkName(), potentialNextClip.getLinkName() );
	if(!link && potentialNextClip.person == currentlyPlayingClip.person){
		if(printDecisions) cout << "	REJECTED Clip " << potentialNextClip.getLinkName() << ": same person" << endl;
		return 0;
	}
	
	//reject any nodes we've seen already
	if(historyContainsClip(potentialNextClip, history)){ //TODO discourage...
		if(printDecisions) cout << "	REJECTED Clip " << potentialNextClip.getLinkName() << ": already visited" << endl;
		return 0;
	}
	
	int occurrences = occurrencesOfPerson(potentialNextClip.person, 20, history);
	if(occurrences > 4){
		if(printDecisions) cout << "	REJECTED Clip " << potentialNextClip.getLinkName() << ": person appeared more than 4 times in the last 20 clips" << endl;
		return 0;
	}
	
	//Base score
	int score = 0;
	int topicsInCommon = parser->getSharedKeywords(currentlyPlayingClip, potentialNextClip).size();
	score += topicsInCommon*10;
	
	if(history.size() > 1){
		int topicsInCommonWithPrevious = parser->getSharedKeywords(history[history.size()-2], potentialNextClip ).size();
		score += topicsInCommonWithPrevious * 5;
	}
	
	//If this clip is a link weight it highly
	if( link ){
		score += 20;
	}
	
	//penalize for the person occurring
	score -= occurrences*4;
	
    //history should contain #keywords dichotomies, and then augment score
    vector<string> specialKeywords = potentialNextClip.getSpecialKeywords();
    
    for(int i=0; i< dichotomies.size(); i++){
        for( int k=0; k<specialKeywords.size(); k++){
            if(dichotomies[i].right == specialKeywords[k]){
                
                if(dichotomies[i].balance > dichotomyThreshold){
                    dichotomies[i].balance = -1;
                }
                else{
                    score+= dichotomies[i].balance*2;
                }
                
            }
            else if(dichotomies[i].left == specialKeywords[k]){
                
                if(dichotomies[i].balance < -dichotomyThreshold){
                    dichotomies[i].balance = 1;
                }
                else{
                    score +=  -dichotomies[i].balance*2;
                    
                }
                
            }
        }
    }
    
	if(printDecisions) cout << "	ACCEPTED " << (link ? "LINK " : "") << score << " Clip " << potentialNextClip.getLinkName() << " occurrences " << occurrences << " and " << topicsInCommon << " topics in common" << endl;
	
	return MAX(score, 0);
}


//IGNORE
float CloudsStoryEngine::scoreForClip(CloudsClip& clip){
    
    return 0;
    
	/*
	//rejection criteria -- flat out reject clips on some basis
	if(combinedClipsOnly && !clip.hasCombinedVideo){
		if(printDecisions) cout << "	REJECTED Clip " << clip.getLinkName() << ": no combined video file" << endl;
		return 0;
	}
    
    if(clip.cluster.Id == ""){
		if(printDecisions) cout << "	REJECTED Clip " << clip.getLinkName() << ": disconnect from cluster map =(" << endl;        
        return 0;
    }
    
	if(clip.person == currentClip.person){
		if(printDecisions) cout << "	REJECTED Clip " << clip.getLinkName() << ": same person" << endl;
		return 0;
	}
	
	bool containsCurrentTopic = ofContains(clip.getKeywords(), currentTopic);
	if(!freeTopic && !containsCurrentTopic){
		if(printDecisions) cout << "	REJECTED Clip " << clip.getLinkName() << ": not on topic " << currentTopic << endl;
		return 0;
	}
	//maybe a bug... 
	else if(freeTopic && containsCurrentTopic){
		if(printDecisions) cout << "	REJECTED Clip " << clip.getLinkName() << ": same topic as before " << currentTopic << endl;
		return 0;
	}

	//reject any nodes we've seen already
	if(historyContainsClip(clip)){
		if(printDecisions) cout << "	REJECTED Clip " << clip.getLinkName() << ": already visited" << endl;
		return 0;
	}
	
	int occurrences = occurrencesOfPerson(clip.person, 20);
	if(occurrences > 4){
		if(printDecisions) cout << "	REJECTED Clip " << clip.getLinkName() << ": person appeared more than 4 times in the last 20 clips" << endl;
		return 0;
	}
	
	if(parser->linkIsSuppressed(currentClip.getLinkName(), clip.getLinkName())) {
		if(printDecisions) cout << "	REJECTED clip " << clip.getLinkName() << ": link is suppressed" << endl;
		return 0;
	}
	
	//Base score
	int score = 10;
	int topicsInCommon = parser->getSharedKeywords(currentClip, clip).size();
	score += (topicsInCommon-1)*10;

	bool link = parser->clipLinksTo( currentClip.getLinkName(), clip.getLinkName() );
	//If this clip is a link weight it highly
	if( link ){
		score += 20;
	}

	//penalize for the person occurring
	score -= occurrences*4;
	
	if(printDecisions) cout << "	ACCEPTED " << (link ? "LINK " : "") << score << " Clip " << clip.getLinkName() << " occurrences " << occurrences << " and " << topicsInCommon << " topics in common" << endl;
	
	return MAX(score, 0);
     */
}

CloudsAct& CloudsStoryEngine::getAct(){
    return act;
}
void CloudsStoryEngine::drawStoryEngineDebug(){
	ofPushStyle();
	
	ofSetColor(255);
	string debugString = "";
	debugString += "Current Clip:    " + currentClip.getLinkName() + "\n";
	debugString += "Current Topic:   " + getCurrentTopic() + "\n";
	debugString += "Times on Topic:  " + ofToString(timesOnTopic) + "/" + ofToString(maxTimesOnTopic) + "\n";
	

	ofDrawBitmapString(debugString,25,25);
	
	ofPopStyle();
}

void CloudsStoryEngine::drawActDebug(){
	float totalTime = 0;
	for(int i = 0; i < act.getAllClips().size(); i++){
		totalTime += act.getClipInAct(i).getDuration();
	}
	
	int currentTime = 0;
	for(int i = 0; i < act.getAllClips().size(); i++){
		float screenX = ofMap(currentTime, 0, totalTime,  0, ofGetWidth());
		float width = ofMap(act.getClipInAct(i).getDuration(), 0, totalTime,  0, ofGetWidth());
		currentTime += act.getClipInAct(i).getDuration();
		ofNoFill();
		ofRect(screenX, 100 + 30*i, width, 30);
		ofDrawBitmapString(act.getClipInAct(i).getLinkName(), screenX+10, 100 + 30*(i+.75));
	}
    
    string dichotomiesString = "";
    
    for (int i =0; i<dichotomies.size(); i++) {
        dichotomiesString += dichotomies[i].left + "," + dichotomies[i].right + " : " + ofToString(dichotomies[i].balance) + "\n";
    }
    ofDrawBitmapString(dichotomiesString, ofGetWidth()-500, 100);
  	
}

float CloudsStoryEngine::getTotalSecondsWatched(){
	return totalFramesWatched / 23.976;
}

CloudsClip& CloudsStoryEngine::getCurrentClip(){
	return currentClip;
}

vector<CloudsClip>& CloudsStoryEngine::getClipHistory(){
	return clipHistory;
}

string CloudsStoryEngine::getCurrentTopic(){
	return currentTopic;
}

int CloudsStoryEngine::getTimesOnTopic(){
	return timesOnTopic;
}

float CloudsStoryEngine::getNextClipDelay(){
	return fixedClipDelay;
}

bool CloudsStoryEngine::isWaiting(){
	return waitingForNextClip;
}

bool CloudsStoryEngine::atDeadEnd(){
	return validNextClips.size() == 0;
}

bool CloudsStoryEngine::historyContainsClip(CloudsClip& m){
	return historyContainsClip(m, clipHistory);
}

bool CloudsStoryEngine::historyContainsClip(CloudsClip& m, vector<CloudsClip>& history){
	string clipLinkName = m.getLinkName();
	for(int i = 0; i < history.size(); i++){
		if(clipLinkName == history[i].getLinkName()){
			return true;
		}
	}
	return false;
}

int CloudsStoryEngine::occurrencesOfPerson(string person, int stepsBack){
	return occurrencesOfPerson(person, stepsBack, clipHistory);
}

int CloudsStoryEngine::occurrencesOfPerson(string person, int stepsBack, vector<CloudsClip>& history){
	int occurrences = 0;
	int startPoint = history.size() - MIN(stepsBack, history.size() );
//	cout << "finding occurrences ... " << person << " " << clipHistory.size() << " steps back " << stepsBack << " start point " << startPoint << "/" << clipHistory.size() << endl;
	for(int i = startPoint; i < history.size()-1; i++){ // -1 because the current clip is part of history
//		cout << "COMPARING " << clipHistory[i].person << " to " << person << endl;
		if(history[i].person == person){
			occurrences++;
		}
	}
	return occurrences;
}

CloudsEvents& CloudsStoryEngine::getEvents(){
	return events;
}
