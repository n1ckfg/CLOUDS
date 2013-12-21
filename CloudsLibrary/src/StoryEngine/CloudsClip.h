
#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"
#include "CloudsProjectExample.h"

class CloudsClip {
  public:
	CloudsClip();
	
	string name;
    string person;
    string clip;

	string getSpeakerLastName();
	string getSpeakerFirstName();
	string getSpeakerGender();
	
	string fcpFileId;
    string sourceVideoFilePath;
	
	ofVec3f networkPosition;
	
	bool voiceOverAudio;
    float currentScore;
    int startFrame;
    int endFrame;
	
    //called from the FCPParser
    void setOriginalKeywords(vector<string>& keywords);
    //called from the CloudsLinker UI
    void setDesiredKeywords(vector<string>& desiredKeywords);
    //called during ParseLinks
    void addKeyword(string keyword);
    void revokeKeyword(string keyword);
    bool hasAdditionalKeywords();
    bool hasRevokedKeywords();
    bool hasSpecialKeyword(string keyword);
    bool hasKeyword(string keyword);
    void setProjectExample(string projectExample);
	
    vector<string>& getOriginalKeywords();
    //used to save out links
    vector<string>& getAdditionalKeywords();
    vector<string>& getRevokedKeywords();
    vector<string>& getAllTopicsWithQuestion();

    //use everywhere for real keywoords
	vector<string>& getKeywords();
    //get special # keywords
    vector<string>& getSpecialKeywords();
    
    //questions with topics
    bool hasStartingQuestion();
    void addQuestionTopicPair(string topic, string question);
	bool hasQuestion();
    map<string,string>& getAllQuestionTopicPairs();
    string getQuestionForTopic(string topic);
	vector<string> getTopicsWithQuestions();
    vector<string> getQuestions();
	
	//project example stuff
	bool hasProjectExample;
	string projectExampleTitle;
	CloudsProjectExample projectExample;

    //overlapping clips
    vector<string> getOverlappingClips();
    bool hasOverlappingClips();
    
	bool is30FPS();
	float getDuration();
    string getLinkName();
	string getMetaInfo();
	string getSceneFolder();
	
	string getID();
	string getCombinedPNGExportFolder();
	string getCombinedMovieFile();
	string getCombinedCalibrationXML();
    string getFFMpegLine(string alternativeVideoPath, string exportFolder);
    
    void addOverlappingClipName(string clipName);
	void removeOverlappingClipName(string clipName);

	bool hasCombinedVideo;
	string combinedVideoPath;
	string combinedCalibrationXMLPath;
	
	string getRelinkedVideoFilePath();
	
	//adjustment paramters set by the exporter
	string getAdjustmentXML();
	void loadAdjustmentFromXML(bool forceReload = false);
	void saveAdjustmentToXML();
	
	//adjustment parameters
	float minDepth;
	float maxDepth;
	ofVec3f adjustTranslate;
	ofVec3f adjustRotate;
	ofVec3f adjustScale;
	ofVec2f faceCoord;
	    
    
    //SURYA TODO: Replace contour parameters colour parameters
    //Skin Parameters
    ofFloatColor skinTargetColor;
	
    float skinLowerThreshold;
    float skinUpperThreshold;
    float skinHueWeight;
    float skinSatWeight;
    float skinBrightWeight;
	
	bool adjustmentLoaded;
    
  protected:
    vector<string> originalKeywords;    //came in from FCP
    vector<string> additionalKeywords;  //added manually
    vector<string> revokedKeywords;     //manually removed
    vector<string> specialKeywords;        //special Keywords start with #
    vector<string> topicWithQuestions;
    map<string,string> questionTopicMap; //question-topic pairs
    vector<string> overlappingClips;
    bool keywordsDirty;
    void collateKeywords();
    vector<string> keywords; //collated

};
