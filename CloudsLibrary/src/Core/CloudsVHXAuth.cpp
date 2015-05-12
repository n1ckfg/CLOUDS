//
//  CloudsVHXAuth.cpp
//  vhxTest
//
//  Created by Elias Zananiri on 2015-05-06.
//
//

#include "CloudsVHXAuth.h"
#include "CloudsCrypto.h"
#include "CloudsGlobal.h"

#include "ofxCrypto.h"
#include "ofxJSONElement.h"

#include "Poco/DateTimeParser.h"
#include "Poco/DateTime.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/LocalDateTime.h"
#include "Poco/Timestamp.h"

//--------------------------------------------------------------
CloudsVHXAuth::CloudsVHXAuth()
: ofThread()
, mode(WAITING)
, state(INACTIVE)
, _keysPath("")
, _tokensPath("")
, _clientId("")
, _clientSecret("")
, _packageId("")
, _packageExpiry(0)
, _accessToken("")
, _refreshToken("")
, _tokenExpiry(0)
, _code("")
, _codeExpiry(0)
{
    ofxSSL::appendData = true;
}

//--------------------------------------------------------------
CloudsVHXAuth::~CloudsVHXAuth()
{
    waitForThread(true);
    exit();
}

//--------------------------------------------------------------
bool CloudsVHXAuth::setup()
{
    _keysPath = GetCloudsDataPath(true) + "vhx/client.bin";
    _tokensPath = GetCloudsDataPath(true) + "vhx/tokens.bin";
    
    // Load and decrypt the info from files on disk.
    //string path = GetCloudsDataPath() + "vhx/client.bin";
    if (CloudsCryptoLoadKeys(_clientId, _clientSecret, _packageId, _keysPath)) {
        ofLogNotice("CloudsVHXAuth::setup") << "Loaded keys successfully:\n"
        << "\tClient ID: " << _clientId << "\n"
        << "\tClient Secret: " << _clientSecret << "\n"
        << "\tPackage ID: " << _packageId;
    }
    else {
        ofLogError("CloudsVHXAuth::setup") << "Cannot open file at " << _keysPath;
    }
    
    if (CloudsCryptoLoadTokens(_accessToken, _refreshToken, _tokenExpiry, _tokensPath)) {
        ofLogNotice("CloudsVHXAuth::setup") << "Loaded tokens successfully:\n"
        << "\tAccess Token: " << _clientId << "\n"
        << "\tRefresh Token: " << _clientSecret << "\n"
        << "\tToken Expiry: " << _tokenExpiry;
        
        state = INACTIVE;
    }
    else {
        ofLogError("CloudsVHXAuth::setup") << "Cannot open file at " << _tokensPath;
        
        _tokenExpiry = 0;
        state = INACTIVE;
    }
    
    mode = WAITING;
    bNotifyComplete = false;
    ofAddListener(ofEvents().update, this, &CloudsVHXAuth::update);
    
    if (_packageId.size() && _accessToken.size()) {
        // Try to verify the package immediately.
        verifyPackage();
        return true;
    }
    
    return false;
}

//--------------------------------------------------------------
void CloudsVHXAuth::exit()
{
    ofRemoveListener(ofEvents().update, this, &CloudsVHXAuth::update);
}

//--------------------------------------------------------------
void CloudsVHXAuth::update(ofEventArgs& args)
{
    if (bNotifyComplete) {
        if (mode == REQUEST_TOKEN) {
            ofNotifyEvent(requestTokenComplete, completeArgs);
        }
        else if (mode == REFRESH_TOKEN) {
            ofNotifyEvent(refreshTokenComplete, completeArgs);
        }
        else if (mode == REQUEST_CODE) {
            ofNotifyEvent(requestCodeComplete, completeArgs);
        }
        else if (mode == LINK_CODE) {
            ofNotifyEvent(linkCodeComplete, completeArgs);
        }
        else if (mode == VERIFY_PACKAGE) {
            ofNotifyEvent(verifyPackageComplete, completeArgs);
        }
        mode = WAITING;
        bNotifyComplete = false;
    }
    
    time_t nowTime = (ofGetSystemTime() / 1000.f);
    
    // Refresh token if it's expired.
    if (mode == WAITING && _tokenExpiry && nowTime > _tokenExpiry) {
        if (_refreshToken.empty()) {
            requestToken();
        }
        else {
            refreshToken();
        }
    }
    
    // Reset code if it's expired.
    if (_codeExpiry && nowTime > _codeExpiry) {
        _code = "";
        _codeExpiry = 0;
        
        CloudsVHXEventArgs args;
        ofNotifyEvent(codeExpired, args);
    }
    
    // Update state if package expired.
    if (state == RENTAL && _packageExpiry && nowTime > _packageExpiry) {
        state = EXPIRED;

        CloudsVHXEventArgs args;
        ofNotifyEvent(packageExpired, args);
    }
}

//--------------------------------------------------------------
void CloudsVHXAuth::requestToken()
{
    if (isThreadRunning()) {
        ofLogError("CloudsVHXAuth::requestToken") << "Thread is already running with mode " << mode;
        return;
    }
    if (mode != WAITING) {
        ofLogError("CloudsVHXAuth::requestToken") << "Mode " << mode << " is still active";
        return;
    }
    
    mode = REQUEST_TOKEN;
    startThread();
}

//--------------------------------------------------------------
void CloudsVHXAuth::refreshToken()
{
    if (isThreadRunning()) {
        ofLogError("CloudsVHXAuth::refreshToken") << "Thread is already running with mode " << mode;
        return;
    }
    if (mode != WAITING) {
        ofLogError("CloudsVHXAuth::refreshToken") << "Mode " << mode << " is still active";
        return;
    }
    if (_refreshToken.empty()) {
        ofLogError("CloudsVHXAuth::refreshToken") << "No refresh token set!";
        return;
    }
    
    mode = REFRESH_TOKEN;
    startThread();
}

//--------------------------------------------------------------
void CloudsVHXAuth::requestCode()
{
    if (isThreadRunning()) {
        ofLogError("CloudsVHXAuth::requestCode") << "Thread is already running with mode " << mode;
        return;
    }
    if (mode != WAITING) {
        ofLogError("CloudsVHXAuth::requestCode") << "Mode " << mode << " is still active";
        return;
    }
    
    mode = REQUEST_CODE;
    startThread();
}

//--------------------------------------------------------------
void CloudsVHXAuth::linkCode()
{
    if (isThreadRunning()) {
        ofLogError("CloudsVHXAuth::linkCode") << "Thread is already running with mode " << mode;
        return;
    }
    if (mode != WAITING) {
        ofLogError("CloudsVHXAuth::linkCode") << "Mode " << mode << " is still active";
        return;
    }
    if (_code.empty()) {
        ofLogError("CloudsVHXAuth::linkCode") << "No code set!";
        return;
    }
    
    mode = LINK_CODE;
    startThread();
}

//--------------------------------------------------------------
void CloudsVHXAuth::verifyPackage()
{
    if (isThreadRunning()) {
        ofLogError("CloudsVHXAuth::verifyPackage") << "Thread is already running with mode " << mode;
        return;
    }
    if (mode != WAITING) {
        ofLogError("CloudsVHXAuth::verifyPackage") << "Mode " << mode << " is still active";
        return;
    }
    if (_packageId.empty()) {
        ofLogError("CloudsVHXAuth::verifyPackage") << "No package ID set!";
        return;
    }
    if (_accessToken.empty()) {
        ofLogError("CloudsVHXAuth::verifyPackage") << "No access token set!";
        return;
    }

    mode = VERIFY_PACKAGE;
    startThread();
}

//--------------------------------------------------------------
void CloudsVHXAuth::threadedFunction()
{
    if (mode == REQUEST_TOKEN || mode == REFRESH_TOKEN) {
        _ssl.setup();
        _ssl.setOpt(CURLOPT_CAINFO, ofToDataPath(GetCloudsDataPath(true) + "vhx/cacert.pem"));
        _ssl.setURL("https://api.vhx.tv/oauth/token");
        if (mode == REQUEST_TOKEN) {
            _ssl.addFormField("client_id", _clientId);
            _ssl.addFormField("client_secret", _clientSecret);
            _ssl.addFormField("grant_type", "client_credentials");
        }
        else {
            _ssl.addFormField("refresh_token", _refreshToken);
            _ssl.addFormField("grant_type", "refresh_token");
        }
        
        _ssl.perform();
        
        string response = _ssl.getResponseBody();
        ofLogVerbose("CloudsVHXAuth::threadedFunction") << "Response:" << endl << response;
        
        completeArgs.success = false;
        
        ofxJSONElement json;
        if (json.parse(response)) {
            if (json.isMember("access_token")) {
                _accessToken = json["access_token"].asString();
                _refreshToken = json["refresh_token"].asString();
                _tokenExpiry = (ofGetSystemTime() / 1000.f) + json["expires_in"].asFloat();
                
                // Save the tokens to disk.
                CloudsCryptoSaveTokens(_accessToken, _refreshToken, _tokenExpiry, _tokensPath);
                
                completeArgs.success = true;
                completeArgs.result = _accessToken;
            }
            else {
                ofLogError("CloudsVHXAuth::threadedFunction") << "Unexpected JSON format:" << endl << response;
            }
        }
        else {
            ofLogError("CloudsVHXAuth::threadedFunction") << "Unable to parse JSON:" << endl << response;
        }
        
        _ssl.clear();
        
        bNotifyComplete = true;
    }
    else if (mode == REQUEST_CODE) {
        _ssl.setup();
        _ssl.setOpt(CURLOPT_CAINFO, ofToDataPath(GetCloudsDataPath(true) + "vhx/cacert.pem"));
        _ssl.setURL("https://api.vhx.tv/oauth/codes");
        _ssl.addFormField("client_id", _clientId);
        _ssl.addFormField("client_secret", _clientSecret);

        _ssl.perform();
        
        string response = _ssl.getResponseBody();
        ofLogVerbose("CloudsVHXAuth::threadedFunction") << "Response:" << endl << response;
        
        completeArgs.success = false;
        
        ofxJSONElement json;
        if (json.parse(response)) {
            if (json.isMember("code")) {
                _code = json["code"].asString();
                _codeExpiry = (ofGetSystemTime() / 1000.f) + json["expires_in"].asFloat();
                
                completeArgs.success = true;
                completeArgs.result = _code;
            }
            else {
                ofLogError("CloudsVHXAuth::threadedFunction") << "Unexpected JSON format:" << endl << response;
            }
        }
        else {
            ofLogError("CloudsVHXAuth::threadedFunction") << "Unable to parse JSON:" << endl << response;
        }
        
        _ssl.clear();
        
        bNotifyComplete = true;
    }
    else if (mode == LINK_CODE) {
        stringstream ss;
        ss << "http://www.vhx.tv/activate/clouds";
        ss << "?client_id=" << _clientId;
        ss << "&code=" << _code;
        ofLaunchBrowser(ss.str());
        
        ss.str("");
        ss << "https://api.vhx.tv/oauth/codes/" << _code;
        ss << "?client_id=" << _clientId;
        ss << "&client_secret=" << _clientSecret;
        
        _ssl.setup();
        _ssl.setOpt(CURLOPT_CAINFO, ofToDataPath(GetCloudsDataPath(true) + "vhx/cacert.pem"));
        _ssl.setURL(ss.str());
        
        completeArgs.success = false;

        bool bWaitForLink = true;
        while (bWaitForLink && isThreadRunning()) {
            _ssl.perform();
            
            string response = _ssl.getResponseBody();
            ofLogVerbose("CloudsVHXAuth::threadedFunction") << "Response:" << endl << response;
            
            ofxJSONElement json;
            if (json.parse(response)) {
                if (json.isMember("access_token")) {
                    _accessToken = json["access_token"].asString();
                    _refreshToken = json["refresh_token"].asString();
                    _tokenExpiry = ofGetElapsedTimef() + json["expires_in"].asFloat();
                    
                    // Save the tokens to disk.
                    CloudsCryptoSaveTokens(_accessToken, _refreshToken, _tokenExpiry, _tokensPath);
                    
                    completeArgs.success = true;
                    completeArgs.result = _accessToken;

                    bWaitForLink = false;
                }
                else {
                    ofLogVerbose("CloudsVHXAuth::threadedFunction") << "Unexpected JSON result:" << endl << response;
                }
            }
            else {
                ofLogError("CloudsVHXAuth::threadedFunction") << "Unable to parse JSON:" << endl << response;
                
                bWaitForLink = false;
            }
            
            _ssl.clear();
            
            sleep(10);
        }
        
        bNotifyComplete = true;
    }
    else if (mode == VERIFY_PACKAGE) {
        stringstream ss;
        ss << "Authorization: Bearer " << _accessToken;
        
        _ssl.setup();
        _ssl.setOpt(CURLOPT_CAINFO, ofToDataPath(GetCloudsDataPath(true) + "vhx/cacert.pem"));
        _ssl.setURL("https://api.vhx.tv/me");
        _ssl.addHeader(ss.str());
        
        _ssl.perform();
        
        string response = _ssl.getResponseBody();
        ofLogVerbose("CloudsVHXAuth::threadedFunction") << "Response:" << endl << response;
        
        completeArgs.success = false;

        ofxJSONElement json;
        if (json.parse(response)) {
            if (json.isMember("_embedded")) {
                const ofxJSONElement& embedded = json["_embedded"];
                if (embedded.isMember("packages")) {
                    const ofxJSONElement& packages = embedded["packages"];
                    for (int i = 0; i < packages.size(); ++i) {
                        const ofxJSONElement& element = packages[i];
                        if (element.isMember("id")) {
                            string packageId = element["id"].asString();
                            if (packageId == _packageId) {
                                // Found matching package, check that purchase is valid.
                                if (element.isMember("purchase_type")) {
                                    string purchaseType = element["purchase_type"].asString();
                                    if (purchaseType == "purchase") {
                                        state = PURCHASE;
                                        completeArgs.success = true;
                                        completeArgs.result = "purchase";
                                        break;
                                    }
                                    else if (purchaseType == "rental") {
                                        if (element.isMember("expires_at")) {
                                            if (element["expires_at"].isNull()) {
                                                // No expiry, assume we're good.
                                                state = RENTAL;
                                                completeArgs.success = true;
                                                completeArgs.result = "rental";
                                                break;
                                            }
                                            else {
                                                // Parse the expiry date.
                                                Poco::DateTime dt;
                                                int tzd;
                                                if (Poco::DateTimeParser::tryParse(element["expires_at"].asString(), dt, tzd)) {
                                                    Poco::LocalDateTime ldt(tzd, dt);
                                                    Poco::Timestamp expiryTime = ldt.timestamp();
                                                    Poco::Timestamp nowTime;
                                                    
                                                    // Make sure the rental is valid.
                                                    if (nowTime > expiryTime) {
                                                        // Expires in the future, we're good.
                                                        _packageExpiry = expiryTime.epochTime();
                                                        
                                                        state = RENTAL;
                                                        completeArgs.success = true;
                                                        completeArgs.result = "rental";
                                                        break;
                                                    }
                                                    else {
                                                        // Expired, no good.
                                                        state = EXPIRED;
                                                        completeArgs.success = true;
                                                        completeArgs.result = "expired";
                                                        break;
                                                    }
                                                }
                                                else {
                                                    // Could not parse expiry, assume no good.
                                                    state = EXPIRED;
                                                    completeArgs.success = true;
                                                    completeArgs.result = "expired";
                                                    break;
                                                }
                                            }
                                        }
                                        else {
                                            // No expiry, assume we're good.
                                            state = RENTAL;
                                            completeArgs.success = true;
                                            completeArgs.result = "rental";
                                            break;
                                        }
                                    }
                                    else {
                                        // Not a rental or purchase, assume no good.
                                        state = INACTIVE;
                                        completeArgs.success = true;
                                        completeArgs.result = "inactive";
                                        break;
                                    }
                                }
                                else {
                                    ofLogError("CloudsVHXAuth::threadedFunction") << "Unexpected JSON result, 'purchase_type' not found:" << endl << response;
                                }
                            }
                            else {
                                ofLogNotice("CloudsVHXAuth::threadedFunction") << "Skipping package " << packageId;
                            }
                        }
                        else {
                            ofLogError("CloudsVHXAuth::threadedFunction") << "Unexpected JSON result, 'id' not found:" << endl << response;
                        }
                    }
                }
                else {
                    ofLogError("CloudsVHXAuth::threadedFunction") << "Unexpected JSON result, 'packages' not found:" << endl << response;
                }
            }
            else {
                ofLogError("CloudsVHXAuth::threadedFunction") << "Unexpected JSON result, '_embedded' not found:" << endl << response;
            }
        }
        else {
            ofLogError("CloudsVHXAuth::threadedFunction") << "Unable to parse JSON:" << endl << response;
        }

        bNotifyComplete = true;
    }
    else {
        ofLogError("CloudsVHXAuth::threadedFunction") << "Mode " << mode << " is unrecongized!";
    }
}