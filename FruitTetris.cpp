/*
CMPT 361 Assignment 1 - FruitTetris implementation Sample Skeleton Code

- This is ONLY a skeleton code showing:
How to use multiple buffers to store different objects
An efficient scheme to represent the grids and blocks

- Compile and Run:
Type make in terminal, then type ./FruitTetris

This code is extracted from Connor MacLeod's (crmacleo@sfu.ca) assignment submission
by Rui Ma (ruim@sfu.ca) on 2014-03-04. 

Modified in Sep 2014 by Honghua Li (honghual@sfu.ca).
*/

#include "include/Angel.h"
#include <cstdlib>
#include <iostream>

using namespace std;

// xsize and ysize represent the window size - updated if window is reshaped to prevent stretching of the game
int xsize = 400; 
int ysize = 720;

// current tile
vec2 tile[4]; // An array of 4 2d vectors representing displacement from a 'center' piece of the tile, on the grid
vec2 tilepos = vec2(5, 19); // The position of the current tile using grid coordinates ((0,0) is the bottom left corner)

// An array storing all possible orientations of all possible tiles
// The 'tile' array will always be some element [i][j] of this array (an array of vec2)
vec2 allRotations[24][4] = {
	// right L (1, 2, 3(center), 4)
	{vec2(-1, -1), vec2(-1, 0), vec2(0, 0), vec2(1, 0)}, 	
	{vec2(1, -1), vec2(0, -1), vec2(0, 0), vec2(0, 1)},     
	{vec2(1, 1), vec2(1, 0), vec2(0, 0), vec2(-1, 0)},  
	{vec2(-1, 1), vec2(0, 1), vec2(0, 0), vec2(0, -1)},

	// left L (1, 2, 3(center), 4)
	{vec2(1, -1), vec2(1, 0), vec2(0, 0), vec2(-1, 0)},
	{vec2(1, 1), vec2(0, 1), vec2(0, 0), vec2(0, -1)},     
	{vec2(-1, 1), vec2(-1, 0), vec2(0, 0), vec2(1, 0)},  
	{vec2(-1, -1), vec2(0, -1), vec2(0, 0), vec2(0, 1)},

	// T (1, 2(center), 3, 4)
	{vec2(-1, 0), vec2(0, 0), vec2(1, 0), vec2(0, -1)},
	{vec2(0, -1), vec2(0, 0), vec2(0, 1), vec2(1, 0)},     
	{vec2(1, 0), vec2(0, 0), vec2(-1, 0), vec2(0, 1)},  
	{vec2(0, 1), vec2(0, 0), vec2(0, -1), vec2(-1, 0)},	

	// I (1, 2(center), 3, 4)
	{vec2(0, 1), vec2(0, 0), vec2(0, -1), vec2(0, -2)}, 
	{vec2(-1, 0), vec2(0, 0), vec2(1, 0), vec2(2, 0)},
	{vec2(0, -1), vec2(0, 0), vec2(0, 1), vec2(0, 2)},
	{vec2(1, 0), vec2(0, 0), vec2(-1, 0), vec2(-2, 0)},

	// right S (1, 2(center), 3, 4)
	{vec2(-1, 0), vec2(0, 0), vec2(0, 1), vec2(1, 1)},
	{vec2(0, -1), vec2(0, 0), vec2(-1, 0), vec2(-1, 1)},
	{vec2(1, 0), vec2(0, 0), vec2(0, -1), vec2(-1, -1)},
	{vec2(0, 1), vec2(0, 0), vec2(1, 0), vec2(1, -1)},

	// left S (1, 2(center), 3, 4)
	{vec2(-1, 0), vec2(0, 0), vec2(0, -1), vec2(1, -1)},
	{vec2(0, -1), vec2(0, 0), vec2(1, 0), vec2(1, 1)},
	{vec2(1, 0), vec2(0, 0), vec2(0, 1), vec2(-1, 1)},
	{vec2(0, 1), vec2(0, 0), vec2(-1, 0), vec2(-1, -1)}

};

// colours
vec4 orange = vec4(1.0, 0.5, 0.0, 1.0); // orange
vec4 red = vec4(1.0, 0.0, 0.0, 1.0); // apple
vec4 purple = vec4(1.0, 0.0, 1.0, 1.0); // grape
vec4 green = vec4(0.0, 0.5, 0.0, 1.0); // pear
vec4 yellow = vec4(0.9, 0.9, 0.0, 1.0); // banana
vec4 white  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black  = vec4(0.0, 0.0, 0.0, 1.0);

// all colours
vec4 allColours[5] = {orange, red, purple, green, yellow};

// board[x][y] represents whether the cell (x,y) is occupied
bool board[10][20]; 

// An array containing the colour of each of the 10*20*2*3 vertices that make up the board
// Initially, all will be set to black. As tiles are placed, sets of 6 vertices (2 triangles; 1 square)
// will be set to the appropriate colour in this array before updating the corresponding VBO
vec4 boardcolours[1200*6];

// location of vertex attributes in the shader program
GLuint vPosition;
GLuint vColor;

// locations of uniform variables in shader program
GLuint locxsize;
GLuint locysize;
// for 2D to 3D transformation
GLuint locMVP;
mat4 model, view, projection;

// VAO and VBO
GLuint vaoIDs[3]; // One VAO for each object: the grid, the board, the current piece
GLuint vboIDs[6]; // Two Vertex Buffer Objects for each VAO (specifying vertex positions and colours, respectively)

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

// When the current tile is moved or rotated (or created), update the VBO containing its vertex position data
void updatetile()
{
	// Bind the VBO containing current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]); 

	// For each of the 4 'cells' of the tile,
	for (int i = 0; i < 4; i++) 
	{
		// Calculate the grid coordinates of the cell
		GLfloat x = tilepos.x + tile[i].x; 
		GLfloat y = tilepos.y + tile[i].y;

		// Create the 4 corners of the square - these vertices are using location in pixels
		// These vertices are later converted by the vertex shader
		vec4 p1 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), 33.1, 1); // front left bottom
		vec4 p2 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), 33.1, 1); // front left top
		vec4 p3 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), 33.1, 1); // front right bottom
		vec4 p4 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), 33.1, 1); // front right top

		vec4 p5 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), -33.1, 1); // back left bottom
		vec4 p6 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), -33.1, 1); // back left top
		vec4 p7 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), -33.1, 1); // back right bottom
		vec4 p8 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), -33.1, 1); // back right top

		// Two points are used by two triangles each
		// 6 points for a 2D square; 6 * 6 points for a 3D cube
		vec4 newpoints[36] = {
								p1, p2, p3, p2, p3, p4, // front side
								p5, p6, p7, p6, p7, p8, // back side
								p3, p4, p7, p4, p7, p8, // right side
								p1, p2, p5, p2, p5, p6, // left side
								p1, p5, p3, p5, p3, p7, // bottom side
								p2, p6, p4, p6, p4, p8 // top side
							}; 

		// Put new data in the VBO
		glBufferSubData(GL_ARRAY_BUFFER, i*6*sizeof(vec4), 6*sizeof(vec4), newpoints); 
	}

	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

void shiftfix(int x, int y)
{
	int locX = tilepos[0] + x;
	int locY = tilepos[1] + y;

	if (locX < 0)
	{
		tilepos[0] = tilepos[0] - x;
	} 
	else if (locX > 9)
	{
		tilepos[0] = 8;
	}

	if (locY > 19)
	{
		tilepos[1] = 18;
	}
}

//-------------------------------------------------------------------------------------------------------------------

// Called at the start of play and every time a tile is placed
void newtile()
{
	srand(time(NULL));
	tilepos = vec2(rand() % 8, 19); // Put the tile at the top of the board

	int block = rand() % 6;
	int orientation = rand() % 4;

	// Update the geometry VBO of current tile

	for (int i = 0; i < 4; i++) {
		tile[i] = allRotations[block * 4 + orientation][i]; // Get the 4 pieces of the new tile
		shiftfix(tilepos[0], tilepos[1]);
	}

	updatetile(); 

	// Update the color VBO of current tile
	vec4 newcolours[24*6];
	for (int i = 0; i < 24*6; i+=6) {
		vec4 currentColour = allColours[rand() % 5];
		newcolours[i] = currentColour;
		newcolours[i+1] = currentColour;
		newcolours[i+2] = currentColour;
		newcolours[i+3] = currentColour;
		newcolours[i+4] = currentColour;
		newcolours[i+5] = currentColour;				
	}

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours); // Put the colour data in the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

void initGrid()
{
	// ***Generate geometry data
	vec4 gridpoints[64*2]; // Array containing the 128 points of the 64 (front and back) total lines to be later put in the VBO
	vec4 gridcolours[64*2]; // One colour per vertex

	// Vertical lines 
	for (int i = 0; i < 11; i++){
		// Front lines
		gridpoints[2*i] = vec4((33.0 + (33.0 * i)), 33.0, 33.0, 1);
		gridpoints[2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 33.0, 1);

		// Back lines
		// The back vertices are 64 away from the front vertices
		gridpoints[2*i + 64] = vec4((33.0 + (33.0 * i)), 33.0, -33.0, 1);
		gridpoints[2*i + 64 + 1] = vec4((33.0 + (33.0 * i)), 693.0, -33.0, 1);
	}

	// Horizontal lines
	for (int i = 0; i < 21; i++){
		// Front lines
		gridpoints[22 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 33.0, 1);
		gridpoints[22 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 33.0, 1);

		// Back lines
		// The back vertices are 64 away from the front vertices
		gridpoints[22 + 2*i + 64] = vec4(33.0, (33.0 + (33.0 * i)), -33.0, 1);
		gridpoints[22 + 2*i + 64 + 1] = vec4(363.0, (33.0 + (33.0 * i)), -33.0, 1);
	}

	// Make all grid lines white
	for (int i = 0; i < 64*2; i++)
		gridcolours[i] = white;

	// *** set up buffer objects
	// Set up first VAO (representing grid lines)
	glBindVertexArray(vaoIDs[0]); // Bind the first VAO
	glGenBuffers(2, vboIDs); // Create two Vertex Buffer Objects for this VAO (positions, colours)

	// Grid vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]); // Bind the first grid VBO (vertex positions)
	glBufferData(GL_ARRAY_BUFFER, 64*2*sizeof(vec4), gridpoints, GL_DYNAMIC_DRAW); // Put the grid points in the VBO
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0); 
	glEnableVertexAttribArray(vPosition); // Enable the attribute
	
	// Grid vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]); // Bind the second grid VBO (vertex colours)
	glBufferData(GL_ARRAY_BUFFER, 64*2*sizeof(vec4), gridcolours, GL_DYNAMIC_DRAW); // Put the grid colours in the VBO
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor); // Enable the attribute
}


void initBoard()
{
	// Generate the geometric data
	vec4 boardpoints[1200*6];
	for (int i = 0; i < 1200*6; i++)
		boardcolours[i] = black; // Let the empty cells on the board be black

	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), 33.1, 1); // front left bottom
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), 33.1, 1); // front left top
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), 33.1, 1); // front right bottom
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), 33.1, 1); // front right top

			vec4 p5 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), -33.1, 1); // back left bottom
			vec4 p6 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), -33.1, 1); // back left top
			vec4 p7 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), -33.1, 1); // back right bottom
			vec4 p8 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), -33.1, 1); // back right top
			
			// Front side
			boardpoints[36*(10*i + j)    ] 	= p1;
			boardpoints[36*(10*i + j) + 1] 	= p2;
			boardpoints[36*(10*i + j) + 2] 	= p3;
			boardpoints[36*(10*i + j) + 3] 	= p2;
			boardpoints[36*(10*i + j) + 4] 	= p3;
			boardpoints[36*(10*i + j) + 5] 	= p4;

			// Back side
			boardpoints[36*(10*i + j) + 6] = p5;
			boardpoints[36*(10*i + j) + 7] = p6;
			boardpoints[36*(10*i + j) + 8] = p7;
			boardpoints[36*(10*i + j) + 9] = p6;
			boardpoints[36*(10*i + j) + 10] = p7;
			boardpoints[36*(10*i + j) + 11] = p8;

			// Right side
			boardpoints[36*(10*i + j) + 12] = p3;
			boardpoints[36*(10*i + j) + 13] = p4;
			boardpoints[36*(10*i + j) + 14] = p7;
			boardpoints[36*(10*i + j) + 15] = p4;
			boardpoints[36*(10*i + j) + 16] = p7;
			boardpoints[36*(10*i + j) + 17] = p8;

			// Left side
			boardpoints[36*(10*i + j) + 18] = p1;
			boardpoints[36*(10*i + j) + 19] = p2;
			boardpoints[36*(10*i + j) + 20] = p5;
			boardpoints[36*(10*i + j) + 21] = p2;
			boardpoints[36*(10*i + j) + 22] = p5;
			boardpoints[36*(10*i + j) + 23] = p6;

			// Bottom side
			boardpoints[36*(10*i + j) + 24] = p1;
			boardpoints[36*(10*i + j) + 25] = p5;
			boardpoints[36*(10*i + j) + 26] = p3;
			boardpoints[36*(10*i + j) + 27] = p5;
			boardpoints[36*(10*i + j) + 28] = p3;
			boardpoints[36*(10*i + j) + 29] = p7;

			// Top side
			boardpoints[36*(10*i + j) + 30] = p2;
			boardpoints[36*(10*i + j) + 31] = p6;
			boardpoints[36*(10*i + j) + 32] = p4;
			boardpoints[36*(10*i + j) + 33] = p6;
			boardpoints[36*(10*i + j) + 34] = p4;
			boardpoints[36*(10*i + j) + 35] = p8;
		}
	}

	// Initially no cell is occupied
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 20; j++)
			board[i][j] = false; 


	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*6*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*6*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

// No geometry for current tile initially
void initCurrentTile()
{
	glBindVertexArray(vaoIDs[2]);
	glGenBuffers(2, &vboIDs[4]);

	// Current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);
	glBufferData(GL_ARRAY_BUFFER, 24*6*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Current tile vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	glBufferData(GL_ARRAY_BUFFER, 24*6*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

void init()
{
	// Load shaders and use the shader program
	GLuint program = InitShader("vshader.glsl", "fshader.glsl");
	glUseProgram(program);

	// Get the location of the attributes (for glVertexAttribPointer() calls)
	vPosition = glGetAttribLocation(program, "vPosition");
	vColor = glGetAttribLocation(program, "vColor");

	// Create 3 Vertex Array Objects, each representing one 'object'. Store the names in array vaoIDs
	glGenVertexArrays(3, &vaoIDs[0]);

	// Initialize the grid, the board, and the current tile
	initGrid();
	initBoard();
	initCurrentTile();

	// The location of the uniform variables in the shader program
	locxsize = glGetUniformLocation(program, "xsize"); 
	locysize = glGetUniformLocation(program, "ysize");
	locMVP = glGetUniformLocation(program, "MVP");

	view = LookAt(vec3(33*3, 33*11, 1000), vec3(33*5, 33*10, 0), vec3(0,1,0));
	// Game initialization
	newtile(); // create new next tile

	// set to default
	glBindVertexArray(0);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0);
	glClearColor(0, 0, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------------

// Given (x,y), tries to move the tile x squares to the right and y squares down
// Returns true if the tile was successfully moved, or false if there was some issue
bool movetile(vec2 direction)
{
	for (int i = 0; i < 4; i++)
	{
		int locX = tilepos[0] + tile[i][0] + direction[0];
		int locY = tilepos[1] + tile[i][1] + direction[1];

		if (locX < 0 || locX > 9) // checks if X and Y are in the bounds of the game space
			return false;
		if (locY < 0 || locY > 19)
			return false;
		if (board[locX][locY]) // checks if the location of X and Y is occupied (if true, return false)
			return false;
	}
	return true;
}

//-------------------------------------------------------------------------------------------------------------------

// Rotates the current tile, if there is room
void rotate()
{   
	int oldorientation = -1;
	int neworientation = -1;
	int j;

	for (int i = 0; i < 24; i++) 
	{
		for (j = 0; j < 4; j++) 
		{
			if (tile[j][0] != allRotations[i][j][0])
				break;
			else if (tile[j][1] != allRotations[i][j][1])
				break;
		}

		if (j == 4) {
			oldorientation = i; 
			neworientation = oldorientation + 1;		
			break;			
		}
	}

	// Checks if it's at the end of one of the shape's rotations
	if (neworientation >= 0 && neworientation % 4 == 0) 
	{
		neworientation = neworientation - 4;
	}

	// Changes the current tile to the next orientation
	for (int i = 0; i < 4; i++) 
	{
		tile[i] = allRotations[neworientation][i];
	}

	// Reverts changes if the current tile is out of the game space
	if (!movetile(vec2(0,0)) && oldorientation >= 0)
	{
		for (int i = 0; i < 4; i++) 
		{
			tile[i] = allRotations[oldorientation][i];
		}		
	}	
}

//-------------------------------------------------------------------------------------------------------------------

// Checks if the specified row (0 is the bottom 19 the top) is full
// If every cell in the row is occupied, it will clear that cell and everything above it will shift down one row
void checkfullrow(int row)
{
	int i;
	int isrowfull = 0;

	// Checks every cell in a row 
	for (i = 0; i < 10; i++) 
	{
		// If a cell is empty, break out of loop and do not need to check any further
		if (board[i][row] == true) 
		{
			isrowfull++;
		}
	}

	// Check if the row is full (10 tiles across)
	if (isrowfull != 10) return;

	// Shift down all rows above
	for (i = row; i < 20; i++)
	{
		// Values of board is shifted down
		for (int j = 0; j < 10; j++) 
		{
			board[j][i] = board[j][i + 1];

			// Colour of the board is shifted down
			for (int k = 0; k < 6; k++)
			{
				boardcolours[j * 6 + i * 60 + k] = boardcolours[j * 6 + (i + 1) * 60 + k];
			}
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
}

//-------------------------------------------------------------------------------------------------------------------

// Places the current tile - update the board vertex colour VBO and the array maintaining occupied cells
void settile()
{
	vec4 colours[24];
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(colours), colours);

	for (int i = 0; i < 4; i++)
	{
		int locX = tilepos[0] + tile[i][0];
		int locY = tilepos[1] + tile[i][1];
		board[locX][locY] = true;

		boardcolours[locX * 6 + locY * 60] 		= colours[i * 6];
		boardcolours[locX * 6 + locY * 60 + 1] 	= colours[i * 6 + 1];
		boardcolours[locX * 6 + locY * 60 + 2] 	= colours[i * 6 + 2];
		boardcolours[locX * 6 + locY * 60 + 3] 	= colours[i * 6 + 3];
		boardcolours[locX * 6 + locY * 60 + 4] 	= colours[i * 6 + 4];
		boardcolours[locX * 6 + locY * 60 + 5] 	= colours[i * 6 + 5];
	}

	// Checks if rows are full and shifts down the rows above (does not work entirely)
	// for (int i = 0; i < 20; i++)
	// {
	// 	checkfullrow(i);
	// }

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

// Starts the game over - empties the board, creates new tiles, resets line counters
void restart()
{
	initBoard();
	newtile();
}

//-------------------------------------------------------------------------------------------------------------------

// Draws the game
void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glUniform1i(locxsize, xsize); // x and y sizes are passed to the shader program to maintain shape of the vertices on screen
	glUniform1i(locysize, ysize);

	projection = Perspective(45, GLfloat(xsize/ysize), 0.1, 200);
	model = Translate(0, 0, 0); 	

	mat4 MVP = projection * model * view;
	glUniformMatrix4fv(locMVP, 1, GL_TRUE, MVP);


	glBindVertexArray(vaoIDs[1]); // Bind the VAO representing the grid cells (to be drawn first)
	glDrawArrays(GL_TRIANGLES, 0, 1200*6); // Draw the board (10*20*2*2 = 800 triangles)

	glBindVertexArray(vaoIDs[2]); // Bind the VAO representing the current tile (to be drawn on top of the board)
	glDrawArrays(GL_TRIANGLES, 0, 24*6); // Draw the current tile (8 triangles)

	glBindVertexArray(vaoIDs[0]); // Bind the VAO representing the grid lines (to be drawn on top of everything else)
	glDrawArrays(GL_LINES, 0, 64*2); // Draw the grid lines ((21+11)*2 = 64 lines)

	glutSwapBuffers();
}

//-------------------------------------------------------------------------------------------------------------------

// Reshape callback will simply change xsize and ysize variables, which are passed to the vertex shader
// to keep the game the same from stretching if the window is stretched
void reshape(GLsizei w, GLsizei h)
{
	xsize = w;
	ysize = h;
	glViewport(0, 0, w, h);
}

//-------------------------------------------------------------------------------------------------------------------

// Handle arrow key keypresses
void special(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_UP:
			rotate();
			updatetile();
			break;
		case GLUT_KEY_DOWN:
			if (movetile(vec2(0,-1))) {
				tilepos[1] = tilepos[1] - 1;
				updatetile();
			}
			break;
		case GLUT_KEY_RIGHT:
			if (movetile(vec2(1,0))) {
				tilepos[0] = tilepos[0] + 1;
				updatetile();
			}
			break;
		case GLUT_KEY_LEFT:
			if (movetile(vec2(-1,0))) {
				tilepos[0] = tilepos[0] - 1;	
				updatetile();
			}
			break;
		default:
			break;
	}
}

//-------------------------------------------------------------------------------------------------------------------

// Handles standard keypresses
void keyboard(unsigned char key, int x, int y)
{
	switch(key) 
	{
		case 033: // Both escape key and 'q' cause the game to exit
			exit(EXIT_SUCCESS);
			break;
		case 'q':
			exit (EXIT_SUCCESS);
			break;
		case 'r': // 'r' key restarts the game
			restart();
			break;
	}
	glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

// Checks if tile can be shifted downwards (similar to movetile())
bool movetilepos()
{
	for (int i = 0; i < 4; i++)
	{
		int locX = tilepos[0] + tile[i][0];
		int locY = tilepos[1] + tile[i][1];
		if (locY - 1 < 0)
			return false;
		if (board[locX][locY - 1])
			return false;
	}
	return true;
}

//-------------------------------------------------------------------------------------------------------------------

//
void tileDrop(int timer)
{
	if (movetilepos()) {
		tilepos[1] = tilepos[1] - 1;
	}
	else
	{
		settile();
		newtile();
	}

	updatetile();
	glutTimerFunc(800, tileDrop, 1);
}

//-------------------------------------------------------------------------------------------------------------------

void idle(void)
{
	glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(xsize, ysize);
	glutInitWindowPosition(680, 178); // Center the game window (well, on a 1920x1080 display)
	glutCreateWindow("Fruit Tetris");
	glewInit();
	init();

	// Callback functions
	glutTimerFunc(800, tileDrop, 1);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);

	glutMainLoop(); // Start main loop
	return 0;
}
