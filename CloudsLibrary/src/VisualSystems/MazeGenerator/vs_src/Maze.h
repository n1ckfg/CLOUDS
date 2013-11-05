//
//  Maze.h
//  MazeGenerator
//
//  Created by Gal Sasson on 10/25/13.
//
//

#ifndef __MazeGenerator__Maze__
#define __MazeGenerator__Maze__

#include <ofMain.h>
#include "MazeCamera.h"
#include "MazeCell.h"
#include "ParamManager.h"

#define NUM_CELLS_X 151
#define NUM_CELLS_Y 1000

class Maze
{
public:
    float cellSize;
    float wallThickness;
    float wallHeight;
    
    Maze(float cSize, float wThick, float wHeight, ofVec3f p = ofVec3f());
    ~Maze();
    void generate();
    
    void update(ofCamera* cam);
    void draw(ofCamera* cam);
    
    float getWidth();
    
    
protected:
    void generateStep();
    void buildModel();
	
	
	//LB
	ofVec3f normalFrom3Points(ofVec3f p0, ofVec3f p1, ofVec3f p2)
	{
		return (p2 - p1).cross( p0 - p1).normalized();
	}

    
    ofVec3f pos;
    ofVbo geometry;
    int vertexCount;
    int vertexIndexForLines[NUM_CELLS_Y];
    
    MazeCell* cells[NUM_CELLS_X][NUM_CELLS_Y];
    std::stack<MazeCell*> cellStack;
    std::vector<ofVec3f> blocks;
    int step;
    int currentYLimit;
    MazeCell* currentCell;
    MazeCell* exitCell;

    bool finishedGenerating;
    bool finishedSolving;
};

#endif /* defined(__MazeGenerator__Maze__) */