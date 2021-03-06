//
//  ColorWorld.cpp
//  Jae Hyun Yoo
//  3/26/14.
//
//

#include "ColorWorld.h"

// FOR MOUSE INTERACTION
#define MAX_WIDTH 0.6
#define MIN_WIDTH 0.4

// FOR LEAP INTERACTION
#define LEAP_THRES 6    // -6 ~ 6 in x-axis and y-axis will be static zone
#define LEAP_ACTIV 90   // -6 ~ -96 or 6 ~ 96 would be moving zone
#define XOFFSET 300
#define ZOFFSET 200

// API KEY - Google Street View API KEY
#define API_KEY "AIzaSyDGHze5XJ3_oQ0_2pAVK87lLNhfycbG070"

// MAP MOVEMENT SPEED
#define MOVEMENT  0.0001
#define FRAMERATE 60

// COLLECTED DRAW FOR MAP DATA
#define MAPSIZE 100
#define XORIGIN 140
#define YORIGIN 100
#define PIXELSIZE 5
#define COLORCOUNTLIMIT 250 // COUNT OF COLOR SAVE MATRIX 3x3x3

// TEMP CONSTANT FOR DEMO
#define CITYINDEX 4



//--------------------------------------------------------------
void ColorWorld::setup()
{
    // openFrameWorks settings
    ofSetVerticalSync(true);
	ofSetFrameRate(FRAMERATE);
    ofEnableSmoothing();
	ofEnableAlphaBlending();
    
    ofSetWindowTitle( "World of Color" );
    
    
    // controller setting
    m_enabledLeap = true;
    
    // use internet option init
    m_enabledInternet = true;

    // use cache color to multiply with srand string length color
    m_enabledClrCache = true;
    
    // music visualization
    m_enabledMusicVisualization = false;
    
    // Color Search Toggle
    m_enabledClrSearch = false;

    
    // Location-----------------------------------------------
    // Image Data
    if ( !m_enabledInternet )
        Utils::loadImages( m_colorData,  m_imageData,   "cityData" );

    // Color Data
    Utils::loadColors( m_streetData, m_coordinates, "cityData" );

    // Elevation Data
    Utils::loadElevations( m_elevationData, "elevationData" );
    

    // Open Street Map----------------------------------------------------
    // Map setup
    m_map.setup(std::shared_ptr<OpenStreetMapProvider>( new OpenStreetMapProvider() ),
              ofGetWidth() * 0.45,
              ofGetWidth() * 0.5  );
    
    // Map zoom setup
	m_map.setZoom(15);
    
    // Initial Map Setup
    m_map.setGeoLocationCenter(GeoLocation(m_latitude , m_longitude));
    
    Utils::citySetup( m_cityLocations );

    // Initial random city location for DEMO
    size_t id = ofRandom( m_cityLocations.size() );
    City cityLocation = m_cityLocations[id];
    
    m_latitude  = cityLocation.latitude;
    m_longitude = cityLocation.longitude;
    m_cityName  = cityLocation.name;
    m_cityId    = cityLocation.city_id;
    
    
    // Leap setup
    m_leapObj.open();
    
    // Leap gesture setup
	m_leapObj.setupGestures();   // we enable our gesture detection here
    
    // bgm setting
    Utils::setMusic( m_bgm, "bgm.mp3" );
    Utils::setMusic( m_sfx, "choppin.mp3" );

    // Multidimensional Deque for each city's color storage
    for (int i = 0; i < m_cityLocations.size(); i++)
    {
        deque<ofColor> vectorCd;
        deque<ofPoint> vectorPt;
        for (int i = 0; i < 1; i++)
        {
            // temporarly assigning empty queue
            vectorCd.push_back( ofColor(0, 0, 0, 0) );
            vectorPt.push_back( ofPoint(0, 0, 0) );
        }
        m_colors.push_back(vectorCd);
        m_points.push_back(vectorPt);
    }
    
    // ofxTween initialize
    clamp = true;
    easingType = ofxTween::easeInOut;
}




//--------------------------------------------------------------
void ColorWorld::update(){
    
    // get elapsed time
    m_elapsedTime = ofGetElapsedTimef();
    //cout << m_etime << endl;
    
    // Leap gesture setup for update function
    m_leapObj.updateGestures();  // check for gesture updates
	m_leapObj.markFrameAsOld();	//IMPORTANT! - tell ofxLeapMotion that the frame is no longer new.
    
    // Leap hand initialize
    m_leapHands = m_leapObj.getSimpleHands();
    
    // Store palm position
    for (int i = 0; i < m_leapHands.size(); i++)
    {
        // no matter how mand hands, it will use first one
        m_leapPalmPos = m_leapHands[0].handPos;
        m_leapPalmPos = ofVec3f( m_leapPalmPos.x - XOFFSET,
                                m_leapPalmPos.y,
                                m_leapPalmPos.z + ZOFFSET );
    }
    
    if ( m_enabledMusicVisualization )
    {
        m_timeFrame = int( m_elapsedTime * ( 5.2 + m_musicSpeed ) ) %
                        ( m_colors[CITYINDEX].size() );
    }

    if ( !m_enabledMusicVisualization )
    {
        m_bgm.setVolume(15);
        m_sfx.setVolume(0);
    }
    else
    {
        m_bgm.setVolume(0);
        m_sfx.setVolume(45);
        
        m_musicClr = ( m_colors[CITYINDEX][m_timeFrame].r +
                       m_colors[CITYINDEX][m_timeFrame].g +
                       m_colors[CITYINDEX][m_timeFrame].b) * 0.3;
        
        m_musicClrRemapped = ofMap(m_musicClr, 50, 200, 0.13, 0.86);
        m_sfx.setVolume( 45 * ofMap(m_remappedHeight[m_timeFrame], 10, 100, 1.1, 2) * 2 );
        m_sfx.setSpeed( ofMap(m_remappedHeight[m_timeFrame], 5, 100, 0.9, 2.5) );
        m_sfx.setPosition(m_musicClrRemapped);
    }
    
    // Enable/Disable Color Search
    if ( m_leapObj.iGestures == 9 )
        m_enabledClrSearch = true;

    else if (m_leapObj.iGestures == 10)
        m_enabledClrSearch = false;
    
    
    // Navigated location updates
    if ( !m_enabledClrSearch )
    {
        if (m_enabledReset)
        {
            m_latitude  = 37.7749300;
            m_longitude = -122.4194200;
            m_enabledReset = false;
        }
        else if (m_enabledReset && !m_enabledMusicVisualization)
        {
            m_latitude  -= getMoveY(m_leapPalmPos.x);
            m_longitude -= getMoveX(m_leapPalmPos.z);
        }
        else if (!m_enabledMusicVisualization)
        {
            m_latitude  -= getMoveMouseY();
            m_longitude -= getMoveMouseX();
        }
    }

    
    // location to string
    m_str_latitude  = ofToString( m_latitude  );
    m_str_longitude = ofToString( m_longitude );


    // Map update
    m_map.setGeoLocationCenter( GeoLocation(m_latitude , m_longitude) );

    // Art Map update
    // Get position from geoLocation
    m_onMapPos.clear();
    m_onMapClr.clear();
    for (int i = 0; i < m_coordinates.size(); i++)
    {
        double x = m_coordinates[i][0];
        double y = m_coordinates[i][1];
        
        ofPoint center(m_latitude, m_longitude);
        
        if ( (x < center.x + 0.01) &&
            (y < center.y + 0.01 + 0.004) &&
            (x >= center.x - 0.01) &&
            (y >= center.y - 0.01 - 0.004) )
        {
            GeoLocation geoLoc = GeoLocation( x, y );
            ofPoint pt = m_map.geoLocationToPoint( geoLoc );
            
            m_onMapPos.push_back( pt );
            m_onMapClr.push_back( ofPoint(x,y) );
        }
    }
    
}




//--------------------------------------------------------------
void ColorWorld::draw(){
    
    string cityName;

    // DRAW MAP-------------------------------------------------
    if (!m_enabledMusicVisualization)
    {
        // Background Color
        if ( m_enabledClrSearch )
            ofBackgroundGradient( ofColor(50, 140, 245), ofColor(0, 5, 15), OF_GRADIENT_CIRCULAR );
        else
            ofBackgroundGradient( ofColor(50, 150, 255), ofColor(5, 10, 45), OF_GRADIENT_CIRCULAR );

        // Drawing Map Pixels(Circles)
        ofPushMatrix();
        
        ofRotate(50, 1, 0, 0);
        ofTranslate(390, 190);
        
        for (int i = 0; i < m_onMapClr.size(); i++)
        {
            string str = ofToString(m_onMapClr[i][0]) + "," + ofToString(m_onMapClr[i][1]);
            
            ofPoint center    = m_map.geoLocationToPoint(GeoLocation(m_latitude, m_longitude));
            double dist = ofDist(center.x, center.y, m_onMapPos[i].x, m_onMapPos[i].y);
            
            // alpha gets smaller towards the end
            float alpha = ofxTween::map(dist, 40, 280, 180, 0, true, easeQuad, easingType);
            
            // radius gets smaller towards the end
            float radius = ofxTween::map(dist, 40, 280, 4.5, 0.5, true, easeQuad, easingType);
            
            // Sphere Shape
            float height = ofxTween::map(dist, 40, 280, 15, 0, true, easeQuad, easingType);

            // alpha for string
            float alpha2 = ofxTween::map(dist, 70, 280, 255, 0, true, easeQuad, easingType);
            
            float elevation = ofxTween::map(m_elevationData[str],
                                            0, 280,
                                            -15, 50,
                                            true,
                                            easeQuad,
                                            easingType);
            
            float h_radius = ofxTween::map(m_elevationData[str],
                                           0, 280,
                                           0, -1.5,
                                           true,
                                           easeQuad,
                                           easingType);
            
            ofColor clr;
            
            if ( m_enabledClrCache )
            {
                // r
                srand(m_streetData[str].street.size()+1);
                int r = rand() % (m_streetData[str].street.size()+10);
                
                // g
                srand(m_streetData[str].street.size()+2);
                int g = rand() % (m_streetData[str].street.size()+20);
                
                // b
                srand(m_streetData[str].street.size()+3);
                int b = rand() % (m_streetData[str].street.size()+30);
                
                clr = ofColor( r * 56 * m_colorData[str].r,
                               g * 56 * m_colorData[str].g,
                               b * 36 * m_colorData[str].b );
            }
            else
            {
                clr = ofColor( m_colorData[str].r,
                               m_colorData[str].g,
                               m_colorData[str].b );
            }
            
            clr.setSaturation(320);
            clr.setBrightness(200);
            
            ofSetColor( clr, alpha );

            ofCircle(m_onMapPos[i].x, m_onMapPos[i].y, height + elevation, radius + h_radius);
            
            // Display Location Info - Street Name
            if ( i % 200 == 0 )
            {
                ofSetColor( clr * 30, alpha2 );
                
                ofDrawBitmapString(m_streetData[str].street,
                                   m_onMapPos[i].x+15, m_onMapPos[i].y,
                                   height + elevation + 30);
                
                ofLine(m_onMapPos[i].x,    m_onMapPos[i].y, height + elevation,
                       m_onMapPos[i].x+15, m_onMapPos[i].y, height + elevation + 30);
            }
            
            // Display Location Info - City Name
            if ( i % 500 == 0 )
            {
                cityName = m_streetData[str].city;
            }

        }
    }
    
    
    // COLOR COLLECT------------------------------------------------------------------
    if ( !m_enabledMusicVisualization )
    {
        if ( m_enabledClrSearch )
        {
            // check each hands - usually two if its human
            for ( int k = 0; k < m_leapHands.size(); k++ )
            {
                // check each fingers
                for ( int i = 0; i < m_leapHands[k].fingers.size(); i++ ){
                    ofPoint fingerPos = m_leapHands[k].fingers[i].pos;
                    
                    double x = fingerPos.x + ofGetWidth()  * 0.5 - XOFFSET;
                    double z = fingerPos.z + ofGetHeight() * 0.5 + ZOFFSET - 180;
                    
                    ofPoint location = m_map.pointToGeolocation( ofVec2d( x , z ) );
                    
                    ofDrawBitmapString( ofToString(location.x), ofPoint( x - 10, z + 25) );
                    ofDrawBitmapString( ofToString(location.y), ofPoint( x - 10, z + 50) );
                    
                    // Update path for street view
                    string imgStreetPath;
                    if ( m_enabledInternet )
                    {
                        imgStreetPath = "http://maps.googleapis.com/maps/api/streetview?size=50x50&location="
                        + ofToString(location.x)
                        + ","
                        + ofToString(location.y)
                        + "&sensor=false"
                        + "&key=" + API_KEY;
                        m_gStreetView.loadImage(imgStreetPath);
                    }
                    else
                    {
                        float eval_x = 0;
                        float eval_y = 0;
                        
                        for (int id = 0; id < m_coordinates.size(); id++)
                        {
                            if ( eval_x == 0)
                            {
                                double tmp_x = m_coordinates[id].x;
                                if ( tmp_x + 0.0004 > location.x &&
                                     tmp_x - 0.0004 < location.x )
                                {
                                    eval_x = tmp_x;
                                }
                            }
                            
                            if ( eval_y == 0 )
                            {
                                double tmp_y = m_coordinates[id].y;
                                if ( tmp_y + 0.0005 > location.y &&
                                     tmp_y - 0.0005 < location.y )
                                {
                                    eval_y = tmp_y;
                                }
                            }
                        }
                        
                        string str = ofToString(eval_x) + "," + ofToString(eval_y);
                        m_gStreetView = m_imageData[str];
                    }
                    
                    
                    if ( m_enabledInternet )
                        m_tmpPalette = m_gStreetView.getColor( 25, 25 );
                    else
                        m_tmpPalette = m_gStreetView.getColor( 65, 65 );
 
                    
                    if ( m_tmpPalette.r > 254 &&
                         m_tmpPalette.g > 254 &&
                         m_tmpPalette.b > 254 ){
                        // TODO - Keep current color - implement option later
                    }
                    else
                    {
                        if ( m_colors[m_cityId].size() > COLORCOUNTLIMIT )
                        {
                            m_colors[m_cityId].pop_front();
                            m_points[m_cityId].pop_front();
                        }
                        
                        m_tmpPalette.setSaturation( 200 );
                        m_tmpPalette.setBrightness( 300 );
                        
                        m_colors[m_cityId].push_back( m_tmpPalette );
                        m_points[m_cityId].push_back( ofPoint( location.x, location.y ) );
                        m_remappedHeight.push_back( ofMap(m_leapPalmPos.z, 220, 385, 10, 100) );
                    }
                    
                    // Draw finger position box with palette
                    ofSetColor(m_tmpPalette);
                    
                    float height;
                    for (int i = 0; i < m_onMapClr.size(); i++)
                    {
                        float dist = ofDist( m_onMapPos[i].x, m_onMapPos[i].y, x, z );
                        if ( dist <= 7 )
                        {
                            string str = ofToString(m_onMapClr[i][0]) + "," + ofToString(m_onMapClr[i][1]);
                            height = ofxTween::map(m_elevationData[str],
                                                   0, 280,
                                                   -15, 50,
                                                   true,
                                                   easeQuad,
                                                   easingType);
                        }
                    }
                    
                    ofNoFill();
                    ofRect(x + 5, z + 5, height, 13, 13);
                    
                    ofPushMatrix();
                    
                    int up = m_elapsedTime;
                    ofTranslate(0, 0, (up * 3 ) % 6 * 10 );
                    ofSetColor(m_tmpPalette, 100);
                    ofFill();
                    ofBox(x , z , height, 8, 8, 8);
                    ofNoFill();
                    ofBox(x , z , height, 9, 9, 9);
                    
                    ofPopMatrix();
                    
                    ofFill();
                    ofSetColor(m_tmpPalette);
                    ofLine(x + 10, z + 10, height,
                           x + 10, z + 10, m_leapPalmPos.z * 0.25 + height);
                    ofRect(x +4, z +4, height, 11, 11);
                    ofRect(x +2, z +2, m_leapPalmPos.z * 0.25 + height, 4, 4);
                    
                    // Palette and Street View information
                    ofPushMatrix();
                    
                    ofRotate(-50, 1, 0, 0);
                    ofTranslate(-100, -30);
                    ofRect( 60, 60 * i + 190, 32, 32 );
                    ofSetColor(255, 255, 255, 150);
                    m_gStreetView.draw( 10, 60 * i + 190, 50, 50 );
                    
                    ofPopMatrix();
                }
            }
        }
    }
    ofPopMatrix();
    
    
    
    // UX Informations------------------------------------------------------
    ofSetColor(155, 255, 255);
    
    if (!m_enabledMusicVisualization)
        ofDrawBitmapString(cityName, 10, 20);
    else
        ofDrawBitmapString("Music Sheet", 10, 20);
    
    
    if (!m_enabledMusicVisualization) {
        
        if ( !m_enabledLeap )
        {
            ofDrawBitmapString("Mouse Mode\nPress \"L\" key to go Leap Mode", 10, 64);
            ofSetColor(255, 50, 50);
            ofDrawBitmapString("NO COLOR COLLECTION FOR Mouse Mode", 10, 96);
            ofSetColor(255, 255, 155);
        }
        else
        {
            ofDrawBitmapString("Leap  Mode\nPress \"L\" key to go Mouse Mode", 10, 64);
        }
        
        if ( !m_enabledInternet )
            ofDrawBitmapString("Offline Mode\nPress \"E\" key to go Online Mode", 10, 128);
        else
            ofDrawBitmapString("Online  Mode\nPress \"E\" key to go Offline Mode", 10, 128);
    
        if ( m_enabledClrSearch )
            ofDrawBitmapString("Color Collection Mode\n\"Circle Right\" key to go Navigation Mode", 10, 192);
        else
            ofDrawBitmapString("Navigation  Mode\n\"Circle Left\" to go Color Collection Mode\n\n\"O\" key to go back to origin", 10, 192);
    }
    else
    {
        // time visualize
        ofBackgroundGradient( ofColor(65, 65, 76), ofColor(35, 30, 45), OF_GRADIENT_CIRCULAR );
        ofSetColor(255, 255, 155, 255);
        ofDrawBitmapString("TIME FRAME:"  +ofToString(m_timeFrame), 10, 48);
        ofDrawBitmapString("ARRAY SIZE:"  +ofToString(m_colors[CITYINDEX].size()), 10, 72);
        ofDrawBitmapString("CLR AVERAGE:" +ofToString(m_musicClrRemapped), 10, 96);
        ofDrawBitmapString("FREQUENCY :"  +ofToString(m_musicClrRemapped), 10, 120);
    }

    
    
    // COLOR COLLECTION Vizualization---------------------------------
    if ( !m_enabledMusicVisualization )
    {
        for ( int i = 0; i < m_colors[CITYINDEX].size(); i++ )
        {
                ofSetColor( m_colors[CITYINDEX][i] );
                ofPushMatrix();
                ofTranslate((ofGetWidth() - i * (ofGetWidth() / m_colors[CITYINDEX].size()) ), ofGetHeight()*0.835);
                ofBox(0, 0, 0, (ofGetWidth() / m_colors[CITYINDEX].size()), m_remappedHeight[i], 1);
                ofPopMatrix();
        }
    }
    else
    {
        for ( int i = 0; i < m_colors[CITYINDEX].size(); i++ )
        {
            ofSetColor( m_colors[CITYINDEX][i] );
            
            ofPushMatrix();
            ofTranslate((ofGetWidth() - i * (ofGetWidth() / m_colors[CITYINDEX].size()) ),
                            ofGetHeight()/2);
            
            ofBox(0, 0, 0,
                  ( ofGetWidth() / m_colors[CITYINDEX].size() ),
                  m_remappedHeight[i],
                  1);
            
            ofPopMatrix();
        }
        
        float x_val = ofGetWidth() - m_timeFrame *
        ( ofGetWidth() / m_colors[CITYINDEX].size() ) - ofGetWidth() / m_colors[CITYINDEX].size();
        
        ofLine( x_val, 0, x_val, ofGetHeight() );
    }
}


//--------------------------------------------------------------
void ColorWorld::keyPressed(int key)
{
    // Enables Leap
    if ( key == 'l' )
        m_enabledLeap = !m_enabledLeap;
    
    // Enables Live Image Loading
    if ( key == 'e' )
        m_enabledInternet = !m_enabledInternet;

    // Enables Cache Colors
    if ( key == 'c' )
        m_enabledClrCache = !m_enabledClrCache;
    
    // Enables Music Player
    if ( key == 'r' )
        m_enabledMusicVisualization = !m_enabledMusicVisualization;
    
    // Reset position to origin
    if ( key == 'o' )
        m_enabledReset = true;
    
    // Music Speed
    if ( key == '-' )
        m_musicSpeed -= 0.1;
    
    // Music Speed
    if ( key == '+' )
        m_musicSpeed += 0.1;
}

//--------------------------------------------------------------
void ColorWorld::keyReleased(int key){}

//--------------------------------------------------------------
void ColorWorld::mouseMoved(int x, int y){}

//--------------------------------------------------------------
void ColorWorld::mouseDragged(int x, int y, int button){}

//--------------------------------------------------------------
void ColorWorld::mousePressed(int x, int y, int button){}

//--------------------------------------------------------------
void ColorWorld::mouseReleased(int x, int y, int button){}

//--------------------------------------------------------------
void ColorWorld::windowResized(int w, int h){}

//--------------------------------------------------------------
void ColorWorld::gotMessage(ofMessage msg){}

//--------------------------------------------------------------
void ColorWorld::dragEvent(ofDragInfo dragInfo){}


//Option 1 - Using Mouse as navigator---------------------------
double ColorWorld::getMoveMouseX()
{
    double output = 0;
    if ( mouseX <= ofGetWidth() * MIN_WIDTH )
    {
        output = ofxTween::map(mouseX,
                               0, ofGetWidth() * MIN_WIDTH,
                               MOVEMENT, 0,
                               clamp,
                               easeQuad,
                               easingType);
        return output;
    }
    else if ( mouseX > ofGetWidth() * MIN_WIDTH &&
             mouseX <= ofGetWidth() * MAX_WIDTH )
    {
        return output;
    }
    else if ( mouseX > ofGetWidth() * MAX_WIDTH )
    {
        output = ofxTween::map(mouseX,
                               ofGetWidth() * MAX_WIDTH, ofGetWidth(),
                               0, -MOVEMENT,
                               clamp,
                               easeQuad,
                               easingType);
        return output;
    }
}

double ColorWorld::getMoveMouseY()
{
    double output = 0;
    if ( mouseY <= ofGetHeight() * MIN_WIDTH )
    {
        output = ofxTween::map(mouseY,
                               0, ofGetHeight() * MIN_WIDTH,
                               -MOVEMENT, 0,
                               clamp,
                               easeQuad,
                               easingType);
        return output;
    }
    else if ( mouseY > ofGetHeight() * MIN_WIDTH &&
             mouseY <= ofGetHeight() * MAX_WIDTH )
    {
        return output;
    }
    else if ( mouseY > ofGetHeight() * MAX_WIDTH )
    {
        output = ofxTween::map(mouseY,
                               ofGetHeight() * MAX_WIDTH, ofGetHeight(),
                               0, MOVEMENT,
                               clamp,
                               easeQuad,
                               easingType);
        return output;
    }
}



// Option 2 - Using Leap as navigator---------------------------
double ColorWorld::getMoveX( float x )
{
    double output = 0;
    if ( m_leapPalmPos.x <= -LEAP_THRES - XOFFSET )
    {
        output = ofxTween::map(m_leapPalmPos.x,
                               -LEAP_THRES - LEAP_ACTIV - XOFFSET, -LEAP_THRES - XOFFSET,
                               MOVEMENT, 0,
                               clamp,
                               easeQuad,
                               easingType);
        return output;
    }
    else if ( m_leapPalmPos.x > -LEAP_THRES - XOFFSET &&
              m_leapPalmPos.x <= LEAP_THRES - XOFFSET )
    {
        return output;
    }
    else if ( m_leapPalmPos.x > LEAP_THRES - XOFFSET )
    {
        output = ofxTween::map(m_leapPalmPos.x,
                               LEAP_THRES - XOFFSET, LEAP_THRES + LEAP_ACTIV - XOFFSET,
                               0, -MOVEMENT,
                               clamp,
                               easeQuad,
                               easingType);
        return output;
    }
}

double ColorWorld::getMoveY( float z )
{
    double output = 0;
    if ( m_leapPalmPos.z <= -LEAP_THRES + ZOFFSET)
    {
        output = ofxTween::map(m_leapPalmPos.z,
                               -LEAP_THRES - LEAP_ACTIV + ZOFFSET, -LEAP_THRES + ZOFFSET,
                               -MOVEMENT, 0,
                               clamp,
                               easeQuad,
                               easingType);
        return output;
    }
    else if ( m_leapPalmPos.z > -LEAP_THRES + ZOFFSET &&
              m_leapPalmPos.z <= LEAP_THRES+ZOFFSET )
    {
        return output;
    }
    else if ( m_leapPalmPos.z > LEAP_THRES + ZOFFSET )
    {
        output = ofxTween::map(m_leapPalmPos.z,
                               LEAP_THRES + ZOFFSET, LEAP_THRES + LEAP_ACTIV + ZOFFSET,
                               0, MOVEMENT,
                               clamp,
                               easeQuad,
                               easingType);
        return output;
    }
}




//--------------------------------------------------------------
void ColorWorld::exit()
{
    m_leapObj.close();
}


//--------------------------------------------------------------
void ColorWorld::gestureDebug()
{
    string msg;
    switch (m_leapObj.iGestures)
    {
        case 1:
            msg = "Screen Tap";
            break;
            
        case 2:
            msg = "Key Tap";
            break;
            
        case 3:
            msg = "Swipe Right";
            break;
            
        case 4:
            msg = "Swipe Left";
            break;
            
        case 5:
            msg = "Swipe Down";
            break;
            
        case 6:
            msg = "Swipe Up";
            break;
            
        case 7:
            msg = "Swipe Forward";
            break;
            
        case 8:
            msg = "Swipe Backwards";
            break;
            
        case 9:
            msg = "Circle Left";
            break;
            
        case 10:
            msg = "Circle Right";
            break;
            
        default:
            msg = "Waiting for hand movement...";
            break;
    }
    ofSetColor(120, 150, 180);
    
    // let's draw our message to the screen
    ofDrawBitmapString(msg, 10, 80);
}


