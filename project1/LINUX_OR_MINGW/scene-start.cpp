 
#include "Angel.h" 
 
#include <stdlib.h> 
#include <dirent.h> 
#include <time.h> 
 
// Open Asset Importer header files (in ../../assimp--3.0.1270/include) 
// This is a standard open source library for loading meshes, see gnatidread.h 
#include <assimp/cimport.h> 
#include <assimp/scene.h> 
#include <assimp/postprocess.h> 
 
GLint windowHeight=640, windowWidth=960; 
 
// gnatidread.cpp is the CITS3003 "Graphics n Animation Tool Interface & Data Reader" code 
// This file contains parts of the code that you shouldn't need to modify (but, you can). 
#include "gnatidread.h"
#include "gnatidread2.h" 
 
using namespace std;    // Import the C++ standard functions (e.g., min)  
 
 
// IDs for the GLSL program and GLSL variables. 
GLuint shaderProgram; // The number identifying the GLSL shader program 
GLuint vPosition, vNormal, vTexCoord, vBoneIDs, vBoneWeights; // IDs for vshader input vars (from glGetAttribLocation) 
GLuint projectionU, modelViewU, uBoneTransforms; // IDs for uniform variables (from glGetUniformLocation) 
 
static float viewDist = 15.0; // Distance from the camera to the centre of the scene 
static float camRotSidewaysDeg=0; // rotates the camera sideways around the centre 
static float camRotUpAndOverDeg=20; // rotates the camera up and over the centre. 
 
mat4 projection; // Projection matrix - set in the reshape function 
mat4 view; // View matrix - set in the display function. 
 
// These are used to set the window title 
char lab[] = "Project1"; 
char *programName = NULL; // Set in main   
int numDisplayCalls = 0; // Used to calculate the number of frames per second 

// Used to determine the current time since start
int totalDisplayCalls = 0;  // Used to calculate the number of frames since start

// -----Meshes---------------------------------------------------------- 
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h 
//                      (numMeshes is defined in gnatidread.h) 
aiMesh* meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw 
const aiScene* scenes[numMeshes];
GLuint vaoIDs[numMeshes]; // and a corresponding VAO ID from glGenVertexArrays 
 
// -----Textures--------------------------------------------------------- 
//                      (numTextures is defined in gnatidread.h) 
texture* textures[numTextures]; // An array of texture pointers - see gnatidread.h 
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures 
 
 
// ------Scene Objects---------------------------------------------------- 
// 
// For each object in a scene we store the following 
// Note: the following is exactly what the sample solution uses, you can do things differently if you want. 
typedef struct { 
    vec4 loc; 
    float scale; 
    float angles[3]; // rotations around X, Y and Z axes. 
    float diffuse, specular, ambient; // Amount of each light component 
    float shine; 
    vec3 rgb; 
    float brightness; // Multiplies all colours 
    int meshId; 
    int texId;
    float animDist;	// Sets maximum distance to run
    float animSpeed;	// Sets speed of model
    float texScale; 
    int animNum;        //Sets Animation Type
    bool animRun;       //Flag to set animation on and off
} SceneObject; 
 
const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely 
 
SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene. 
int nObjects=0; // How many objects are currenly in the scene. 
int currObject=-1; // The current object 
int toolObj = -1;  // The object currently being modified 
 
 
//------------------------------------------------------------ 
// Loads a texture by number, and binds it for later use.   
void loadTextureIfNotAlreadyLoaded(int i) { 
    if(textures[i] != NULL) return; // The texture is already loaded. 
 
    textures[i] = loadTextureNum(i); CheckError(); 
    glActiveTexture(GL_TEXTURE0); CheckError(); 
 
    // Based on: http://www.opengl.org/wiki/Common_Mistakes 
    glBindTexture(GL_TEXTURE_2D, textureIDs[i]); 
    CheckError(); 
 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i]->width, textures[i]->height, 
                 0, GL_RGB, GL_UNSIGNED_BYTE, textures[i]->rgbData); CheckError(); 
    glGenerateMipmap(GL_TEXTURE_2D); CheckError(); 
 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); CheckError(); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); CheckError(); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CheckError(); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); CheckError(); 
 
    glBindTexture(GL_TEXTURE_2D, 0); CheckError(); // Back to default texture 
} 
 
 
//------Mesh loading ---------------------------------------------------- 
// 
// The following uses the Open Asset Importer library via loadMesh in  
// gnatidread.h to load models in .x format, including vertex positions,  
// normals, and texture coordinates. 
// You shouldn't need to modify this - it's called from drawMesh below. 
 
void loadMeshIfNotAlreadyLoaded(int meshNumber) { 
    if(meshNumber>=numMeshes || meshNumber < 0) { 
        printf("Error - no such  model number"); 
        exit(1); 
    } 
 
    if(meshes[meshNumber] != NULL) 
        return; // Already loaded 
 
    const aiScene* scene = loadScene(meshNumber);
    scenes[meshNumber] = scene;
    aiMesh* mesh = scene->mMeshes[0];
    meshes[meshNumber] = mesh; 
 
    glBindVertexArray( vaoIDs[meshNumber] ); 
 
    // Create and initialize a buffer object for positions and texture coordinates, initially empty. 
    // mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2. 
    GLuint buffer[1]; 
    glGenBuffers( 1, buffer ); 
    glBindBuffer( GL_ARRAY_BUFFER, buffer[0] ); 
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*(3+3+3)*mesh->mNumVertices, 
                  NULL, GL_STATIC_DRAW ); 
 
    int nVerts = mesh->mNumVertices; 
    // Next, we load the position and texCoord data in parts.   
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(float)*3*nVerts, mesh->mVertices ); 
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*3*nVerts, sizeof(float)*3*nVerts, mesh->mTextureCoords[0] ); 
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*6*nVerts, sizeof(float)*3*nVerts, mesh->mNormals); 
 
    // Load the element index data 
    GLuint elements[mesh->mNumFaces*3]; 
    for(GLuint i=0; i < mesh->mNumFaces; i++) { 
        elements[i*3] = mesh->mFaces[i].mIndices[0]; 
        elements[i*3+1] = mesh->mFaces[i].mIndices[1]; 
        elements[i*3+2] = mesh->mFaces[i].mIndices[2]; 
    } 
 
    GLuint elementBufferId[1]; 
    glGenBuffers(1, elementBufferId); 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId[0]); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements, GL_STATIC_DRAW); 
 
    // vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0          
    glVertexAttribPointer( vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) ); 
    glEnableVertexAttribArray( vPosition ); 
 
    // vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0) 
    glVertexAttribPointer( vTexCoord, 3, GL_FLOAT, GL_FALSE, 0, 
                           BUFFER_OFFSET(sizeof(float)*3*mesh->mNumVertices) ); 
    glEnableVertexAttribArray( vTexCoord ); 
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0, 
                           BUFFER_OFFSET(sizeof(float)*6*mesh->mNumVertices) ); 
    glEnableVertexAttribArray( vNormal ); 
    CheckError(); 

    // Get boneIDs and boneWeights for each vertex from the imported mesh data
    GLint boneIDs[mesh->mNumVertices][4];
    GLfloat boneWeights[mesh->mNumVertices][4];
    getBonesAffectingEachVertex(mesh, boneIDs, boneWeights);

    GLuint buffers[2];
    glGenBuffers( 2, buffers );  // Add two vertex buffer objects
    
    glBindBuffer( GL_ARRAY_BUFFER, buffers[0] ); CheckError();
    glBufferData( GL_ARRAY_BUFFER, sizeof(int)*4*mesh->mNumVertices, boneIDs, GL_STATIC_DRAW ); CheckError();
    glVertexAttribIPointer(vBoneIDs, 4, GL_INT, 0, BUFFER_OFFSET(0)); CheckError();
    glEnableVertexAttribArray(vBoneIDs);	CheckError();
  
    glBindBuffer( GL_ARRAY_BUFFER, buffers[1] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*4*mesh->mNumVertices, boneWeights, GL_STATIC_DRAW );
    glVertexAttribPointer(vBoneWeights, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vBoneWeights);	CheckError();
} 
 
// -------------------------------------- 
static void mouseClickOrScroll(int button, int state, int x, int y) { 
    if(button==GLUT_LEFT_BUTTON && state == GLUT_DOWN) { 
         if(glutGetModifiers()!=GLUT_ACTIVE_SHIFT) activateTool(button); 
         else activateTool(GLUT_MIDDLE_BUTTON); 
    } 
    else if(button==GLUT_LEFT_BUTTON && state == GLUT_UP) deactivateTool(); 
    else if(button==GLUT_MIDDLE_BUTTON && state==GLUT_DOWN) { activateTool(button); } 
    else if(button==GLUT_MIDDLE_BUTTON && state==GLUT_UP) deactivateTool(); 
     
    else if (button == 3) { // scroll up 
        viewDist = (viewDist < 0.0 ? viewDist : viewDist*0.8) - 0.05; 
    } 
    else if(button == 4) { // scroll down 
       viewDist = (viewDist < 0.0 ? viewDist : viewDist*1.25) + 0.05; 
    } 
} 
 
static void mousePassiveMotion(int x, int y) { 
    mouseX=x; 
    mouseY=y; 
} 
 
 
mat2 camRotZ() { return rotZ(-camRotSidewaysDeg) * mat2(10.0, 0, 0, -10.0); } 
 
 
//---- callback functions for doRotate below and later 
static void adjustCamrotsideViewdist(vec2 cv) 
{   //cout << cv << endl;   // Debugging 
    camRotSidewaysDeg+=cv[0]; viewDist+=cv[1]; } 
 
static void adjustcamSideUp(vec2 su) 
  { camRotSidewaysDeg+=su[0]; camRotUpAndOverDeg+=su[1]; } 
   
static void adjustLocXZ(vec2 xz)  
  { sceneObjs[toolObj].loc[0]+=xz[0];  sceneObjs[toolObj].loc[2]+=xz[1]; } 
 
static void adjustScaleY(vec2 sy)  
  { sceneObjs[toolObj].scale+=sy[0];  sceneObjs[toolObj].loc[1]+=sy[1]; } 
 
//------Set the mouse buttons to rotate the camera around the centre of the scene.  
static void doRotate() { 
    setToolCallbacks(adjustCamrotsideViewdist, mat2(400,0,0,-2), 
                     adjustcamSideUp, mat2(400, 0, 0,-90) ); 
} 

// ------   Scrolls through the currently selected object forwards and backwards. Both 
//          functions avoid the Ground object but include the Light objects. 

static void nextObject(void)                    // Iterates to the next object in the array
{ 
    if(currObject == nObjects - 1)  // Rolls back to start of list if at the end of array            
    { 
        currObject = 1; 
	   toolObj = currObject;
    } 
    else 
    { 
        currObject++; 
	   toolObj++; 
    } 
    setToolCallbacks(adjustLocXZ, camRotZ(), 
		     adjustScaleY, mat2(0.05, 0, 0, 10.0) ); 

} 

static void previousObject(void)                // Iterates to the previous object in the array
{ 
    if(currObject == 1)  // Jumps to end of array if previous object is ground
    {
	   currObject = nObjects - 1; 
	   toolObj = currObject;
    } 
    else 
    { 
        currObject--; 
	   toolObj--; 
    }
    setToolCallbacks(adjustLocXZ, camRotZ(), 
		     adjustScaleY, mat2(0.05, 0, 0, 10.0) ); 

}
				    
//------Add an object to the scene 
 
static void addObject(int id) { 
  vec2 currPos = currMouseXYworld(camRotSidewaysDeg); 
  sceneObjs[nObjects].loc[0] = currPos[0]; 
  sceneObjs[nObjects].loc[1] = 0.0; 
  sceneObjs[nObjects].loc[2] = currPos[1]; 
  sceneObjs[nObjects].loc[3] = 1.0; 
 
  if(id!=0 && id!=55) 
      sceneObjs[nObjects].scale = 0.005; 
 
  sceneObjs[nObjects].rgb[0] = 0.7; sceneObjs[nObjects].rgb[1] = 0.7; 
  sceneObjs[nObjects].rgb[2] = 0.7; sceneObjs[nObjects].brightness = 1.0; 
 
  sceneObjs[nObjects].diffuse = 1.0; sceneObjs[nObjects].specular = 0.5; 
  sceneObjs[nObjects].ambient = 0.7; sceneObjs[nObjects].shine = 10.0; 
 
  sceneObjs[nObjects].angles[0] = 0.0; sceneObjs[nObjects].angles[1] = 180.0; 
  sceneObjs[nObjects].angles[2] = 0.0; 
 
  sceneObjs[nObjects].meshId = id; 
  sceneObjs[nObjects].texId = rand() % numTextures; 
  sceneObjs[nObjects].texScale = 2.0; 

  sceneObjs[nObjects].animDist   =   0.0;
  sceneObjs[nObjects].animSpeed  =   0.0;
  sceneObjs[nObjects].animNum    =   0;
  sceneObjs[nObjects].animRun    =   true;

  if(id >= 56)
  {
    sceneObjs[nObjects].animDist    = 2.0;
    sceneObjs[nObjects].animSpeed   = 1.0;
  }
 
  toolObj = currObject = nObjects++; 
  setToolCallbacks(adjustLocXZ, camRotZ(), 
                   adjustScaleY, mat2(0.05, 0, 0, 10.0) ); 
  glutPostRedisplay(); 
} 
 
//------Duplicate the current object 
 
static void duplicateObject(void ) 
{ 
    // Do not attempt to duplicate the ground or the lights. Only 2 lights have been implemented. 
    if( currObject < 3 ) 
    {  
	printf("Cannot duplicate the ground or the lights\n");  
    } 
    // Do not duplicate an object if the maximum number of objects has been reached 
    else if ( nObjects < maxObjects) 
    { 
	// Set the new object 
	sceneObjs[nObjects] = sceneObjs[currObject]; 
	// Offset the new object so it is not on top of the old one 
	sceneObjs[nObjects].loc[0]+=0.5; 
	// Set the new object to be the current object 
	toolObj = currObject = nObjects++; 
	setToolCallbacks(adjustLocXZ, camRotZ(), 
			 adjustScaleY, mat2(0.05, 0, 0, 10.0) ); 
	glutPostRedisplay(); 
    } 
    else 
    { 
	printf("The maximum number of objects has already been added\n");  
    } 
 
} 
 
//------Remove the current object from the scene (currently will always be last created) 
 
static void removeObject(void )  
{ 
    // Do not remove the ground or the lights 
    if( currObject > 2 ) 
    { 
	int temp = currObject; // Start at the current object.  
	// If temp = nObjects (i.e. removing the last object) then this for loop will 
	// not run. 
	for( int i = temp; i < nObjects; i++) 
	{ 
	    sceneObjs[i] = sceneObjs[i+1]; // Move the object from the next object position down  
	} 
	nObjects--; 
	previousObject();
	glutPostRedisplay(); 
    } 
    else 
    { 
	printf("Cannot remove the ground or the lights\n");  
    } 
} 
 
// ------ The init function 
 
void init( void ) 
{ 
    srand ( time(NULL) ); /* initialize random seed - so the starting scene varies */ 
    aiInit(); 
 
//    for(int i=0; i<numMeshes; i++) 
//        meshes[i] = NULL; 
 
    glGenVertexArrays(numMeshes, vaoIDs); CheckError(); // Allocate vertex array objects for meshes 
    glGenTextures(numTextures, textureIDs); CheckError(); // Allocate texture objects 
 
    // Load shaders and use the resulting shader program 
    shaderProgram = InitShader( "vStart.glsl", "fStart.glsl" ); 
 
    glUseProgram( shaderProgram ); CheckError(); 
 
    // Initialize the vertex position attribute from the vertex shader     
    vPosition = glGetAttribLocation( shaderProgram, "vPosition" ); 
    vNormal = glGetAttribLocation( shaderProgram, "vNormal" ); CheckError(); 
 
    // Likewise, initialize the vertex texture coordinates attribute.   
    vTexCoord = glGetAttribLocation( shaderProgram, "vTexCoord" ); CheckError(); 
 
    projectionU = glGetUniformLocation(shaderProgram, "Projection"); 
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView"); 
    
    //Added initialisation for bones in animation models

    vBoneIDs        = glGetAttribLocation(shaderProgram, "boneIDs"); CheckError();
    vBoneWeights    = glGetAttribLocation(shaderProgram, "boneWeights"); CheckError();
    uBoneTransforms = glGetUniformLocation(shaderProgram, "boneTransforms");


    // Objects 0, and 1 are the ground and the first light. 
    addObject(0); // Square for the ground 
    sceneObjs[0].loc = vec4(0.0, 0.0, 0.0, 1.0); 
    sceneObjs[0].scale = 10.0; 
    sceneObjs[0].angles[0] = 90.0; // Rotate it. 
    sceneObjs[0].texScale = 5.0; // Repeat the texture. 
 
    addObject(55); // Sphere for the first light 
    sceneObjs[1].loc = vec4(2.0, 1.0, 1.0, 1.0); 
    sceneObjs[1].scale = 0.1; 
    sceneObjs[1].texId = 0; // Plain texture 
    sceneObjs[1].brightness = 1.5; // The light's brightness is 5 times this (below). 
 
    // Object 2 is the second light. 
    addObject(55); // Sphere for the second light 
    sceneObjs[2].loc = vec4(3.0, 4.0, 5.0, 1.0); 
    sceneObjs[2].scale = 0.1; 
    sceneObjs[2].texId = 0; // Plain texture 
    sceneObjs[2].brightness = 1.5; // The light's brightness is 5 times this (below). 
 
 
    //addObject(rand() % numMeshes); // A test mesh 
    addObject(56); // add gingerbread man
    sceneObjs[3].loc = vec4(0.0, 0.0, 0.0, 1.0); 
    addObject(57); // add monkey head
    sceneObjs[4].loc = vec4(-1.0, 0.0, 0.0, 1.0); 
    addObject(58); // add gingerbread man circle path
    sceneObjs[5].loc = vec4(1.0, 0.0, 1.0, 1.0); 

    // We need to enable the depth test to discard fragments that 
    // are behind previously drawn fragments for the same pixel. 
    glEnable( GL_DEPTH_TEST ); 
    doRotate(); // Start in camera rotate mode. 
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); /* black background */ 
} 
 
//---------------------------------------------------------------------------- 
 
void drawMesh(SceneObject sceneObj, float elTime) { 
    // Activate a texture, loading if needed. 
    loadTextureIfNotAlreadyLoaded(sceneObj.texId); 
    glActiveTexture(GL_TEXTURE0 ); 
    glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]); 
 
    // Texture 0 is the only texture type in this program, and is for the rgb colour of the 
    // surface but there could be separate types for, e.g., specularity and normals.  
    glUniform1i( glGetUniformLocation(shaderProgram, "texture"), 0 ); 
 
    // Set the texture scale for the shaders 
    glUniform1f( glGetUniformLocation( shaderProgram, "texScale"), sceneObj.texScale ); 
 
 
    // Set the projection matrix for the shaders 
    glUniformMatrix4fv( projectionU, 1, GL_TRUE, projection ); 

    // Set the model matrix  
    float xAngle = sceneObj.angles[0]; 
    float yAngle = sceneObj.angles[1]; 
    float zAngle = sceneObj.angles[2]; 
    
    float period = 1.0;
    if(sceneObj.meshId == 58 && sceneObj.animRun == true)
    {
	//period = 10.0 * 3.141593 * sceneObj.animDist / sceneObj.animSpeed;
	// angular velocity = velocity / radius. Also convert to rad/s to deg/s
	float periodRot = 180.0 * sceneObj.animSpeed / ( sceneObj.animDist * 3.141593 ); 
	// period of 1 revolution
	period = 2.0 * 3.141593 * sceneObj.animDist / sceneObj.animSpeed;
	float modifyRot = fmod( fmod(elTime,period) * periodRot, 360.0); // Angle = s * deg/s from 0 to 360
	// Note here i take the mod of the total time with the period, as otherwise the total
	// time will begin to dominate the formula and wash out the effect of the rotation term.
	// The effect of this is the rotation of the model slowly becomes out of sync with the rotation
	// which is still evident but not as prevelant
	yAngle += modifyRot;
    }
    else
    {
        period = sceneObj.animDist / sceneObj.animSpeed;    
    }
    //Now set the modifer for the animation frame
    float modify = sin( elTime * 2.0 / period);
    
    mat4 rotateMat = RotateZ(zAngle) *  RotateY(yAngle) * RotateX(xAngle); 

    mat4 model = Translate(sceneObj.loc) * Scale(sceneObj.scale) * rotateMat; 

    // Set the model-view matrix for the shaders 
    glUniformMatrix4fv( modelViewU, 1, GL_TRUE, view * model ); 
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId); CheckError(); 
    int nBones = meshes[sceneObj.meshId]->mNumBones;
    if(nBones == 0)  nBones = 1;  // If no bones, just a single identity matrix is used
    // We need to determine a frame number between 0 and 40. to do this, do 20 * (1 + time). 
    // The 2 / period in sin scales the time to change the animation speed
    // Similarly * period scales the animation speed to the distance
    // fmod takes the modulus to give a number between 0 and 40

    //float animFrame = fmod( (20 * ( 1 + modify ) * period ), 40);
 
    float animFrame = 0.0;
    if(sceneObj.animRun == true)
    {
        animFrame = (float)(sceneObj.animNum*50 +1) + fmod( 20 * ( 1 + modify ) * period, 40);
    }
    // get boneTransforms for the first (0th) animation at the given time (a float measured in frames)
    mat4 boneTransforms[nBones];     // was: mat4 boneTransforms[mesh->mNumBones];
    calculateAnimPose(meshes[sceneObj.meshId], scenes[sceneObj.meshId], 0, animFrame, boneTransforms);
    glUniformMatrix4fv(uBoneTransforms, nBones, GL_TRUE, (const GLfloat *)boneTransforms);
    // Activate the VAO for a mesh, loading if needed. 
    glBindVertexArray( vaoIDs[sceneObj.meshId] ); CheckError(); 
 
    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3, GL_UNSIGNED_INT, NULL); CheckError();
} 
 
 
void 
display( void )  
{ 
    numDisplayCalls++; 
    totalDisplayCalls++;
 
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); 
    CheckError(); // May report a harmless GL_INVALID_OPERATION with GLEW on the first frame 
 
 
    // Set the view matrix. The camera will be rotated according to the respective mouse 
    // commands and translated backwards by viewDist 
    view = Translate(0.0, 0.0, -viewDist) * RotateX(camRotUpAndOverDeg) *  RotateY(camRotSidewaysDeg); 
 
    SceneObject lightObj1 = sceneObjs[1];  
    SceneObject lightObj2 = sceneObjs[2]; 
    vec4 lightPosition1 = view * lightObj1.loc; 
    vec4 lightPosition2 = view * lightObj2.loc; 
 
    glUniform4fv( glGetUniformLocation(shaderProgram, "LightPosition1"), 1, lightPosition1); CheckError(); 
    glUniform4fv( glGetUniformLocation(shaderProgram, "LightPosition2"), 1, lightPosition2); CheckError(); 
 
 
    for(int i=0; i<nObjects; i++) { 
        SceneObject so = sceneObjs[i]; 
	vec3 totalLight = lightObj1.rgb * lightObj1.brightness * lightObj2.rgb * lightObj2.brightness; 
        vec3 rgb = so.rgb * so.brightness * totalLight * 2.0; 
        glUniform3fv( glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, so.ambient * rgb ); CheckError(); 
        glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), 1, so.diffuse * rgb ); 
        glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, so.specular * rgb ); 
        glUniform1f( glGetUniformLocation(shaderProgram, "Shininess"), so.shine ); CheckError(); 
	
	float period = so.animDist / so.animSpeed;		    // Time to reach distance
        float elTime = (float)glutGet(GLUT_ELAPSED_TIME) * 0.001;   // Time since start in seconds

	if( so.meshId >= 56 && so.animRun == true)
	{
	    // Check to see if distance or speed are less than 0. If they are set them to 0.1
	    if( so.animDist < 0.1)
	    {
		sceneObjs[i].animDist = 0.1;
	    }
	    if( so.animSpeed < 0.1)
	    {
		sceneObjs[i].animSpeed = 0.1;
	    }
	    float modify = sin( elTime * 2.0 / period);
	    // Define vector to describe the new location
	    vec4 posVec;
	    // Using rotation matrices set the direction of motion as defined by the direction the model 
	    // is facing. Use this to scale the distance it needs to travel in each direction to
	    // determine the new location.  The distance to travel is dependent on the time
	    posVec =  RotateZ(so.angles[2]) * RotateY(so.angles[1]) * RotateX(so.angles[0])
		      * vec4(0.0, 0.0, so.animDist * modify, 0.0);
	    if( so.meshId == 58)
	    {   
		// recalculate period given by circumference of circle
		period = 2.0 * 3.141593 * so.animDist / so.animSpeed;
		modify =  sin( elTime * 10.0 / period);

		// To make gus go in a full circle he needs to go both in positive and negative z
		// directions. When the cos of the time is negative he will need to be moving in the 
		//opposite direction, so set the zDir to be 1.0 or -1.0 depending on the cos value 
		float modify2 = cos( elTime * 10.0 / period);
		float zDir = modify2 / abs(modify2);
		
		float radius = so.animDist;
		float x = radius * modify;
		float z = zDir * sqrt( pow(radius,2) - pow(x,2) );
		posVec = RotateZ(so.angles[2]) * RotateY(so.angles[1]) * RotateX(so.angles[0])
		         * vec4(x, 0.0, z, 0.0);
	    }
	    so.loc += posVec;
	}
        
	drawMesh(so, elTime); 
    } 
 
    glutSwapBuffers(); 
 
} 
 
//--------------Menus 
 
static void objectMenu(int id) { 
  deactivateTool(); 
  addObject(id); 
} 
 
static void texMenu(int id) { 
    deactivateTool(); 
    if(currObject>=0) { 
        sceneObjs[currObject].texId = id; 
        glutPostRedisplay(); 
    } 
} 
 
static void groundMenu(int id) { 
        deactivateTool(); 
        sceneObjs[0].texId = id; 
        glutPostRedisplay(); 
} 
 
static void adjustBrightnessY(vec2 by)  
  { sceneObjs[toolObj].brightness+=by[0]; sceneObjs[toolObj].loc[1]+=by[1]; } 
 
static void adjustRedGreen(vec2 rg)  
  { sceneObjs[toolObj].rgb[0]+=rg[0]; sceneObjs[toolObj].rgb[1]+=rg[1]; } 
 
static void adjustBlueBrightness(vec2 bl_br)  
  { sceneObjs[toolObj].rgb[2]+=bl_br[0]; sceneObjs[toolObj].brightness+=bl_br[1]; } 
 
  static void lightMenu(int id) { 
    deactivateTool(); 
    //Light 1 
    if(id == 70) { 
	    toolObj = 1; 
        setToolCallbacks(adjustLocXZ, camRotZ(), 
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) ); 
 
    } else if(id>=71 && id<=74) { 
	    toolObj = 1; 
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0), 
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) ); 
    } 
    //Light 2 
    else if(id == 80) { 
	    toolObj = 2; 
        setToolCallbacks(adjustLocXZ, camRotZ(), 
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) ); 
 
    } else if(id>=81 && id<=84) { 
	    toolObj = 2; 
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0), 
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) ); 
    } 
 
    else { printf("Error in lightMenu\n"); exit(1); } 
} 
 
static int createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int)) { 
    int nSubMenus = (size-1)/10 + 1; 
    int subMenus[nSubMenus]; 
 
    for(int i=0; i<nSubMenus; i++) { 
        subMenus[i] = glutCreateMenu(menuFn); 
        for(int j = i*10+1; j<=min(i*10+10, size); j++) 
     glutAddMenuEntry( menuEntries[j-1] , j); CheckError(); 
    } 
    int menuId = glutCreateMenu(menuFn); 
 
    for(int i=0; i<nSubMenus; i++) { 
        char num[6]; 
        sprintf(num, "%d-%d", i*10+1, min(i*10+10, size)); 
        glutAddSubMenu(num,subMenus[i]); CheckError(); 
    } 
    return menuId; 
} 
 
static void adjustAmbDif(vec2 ad)  
  { sceneObjs[toolObj].ambient+=ad[0]; sceneObjs[toolObj].specular+=ad[1]; } 
 
static void adjustSpecShine(vec2 ss)  
  { sceneObjs[toolObj].diffuse+=ss[0]; sceneObjs[toolObj].shine+=ss[1]; } 
 
static void materialMenu(int id) { 
  deactivateTool(); 
  if(currObject<0) return; 
  if(id==10) { 
	 toolObj = currObject; 
     setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0), 
                      adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) ); 
  } 
  else if(id==20) 
  { 
    toolObj = currObject; 
    setToolCallbacks(adjustAmbDif,mat2(1.0,0,0,1.0), 
		     adjustSpecShine, mat2(1.0,0,0,10.0) ); 
  }				     
					   
  else { printf("Error in materialMenu\n"); } 
} 
 
static void objectOptionsMenu(int id) 
{ 
    deactivateTool();
    if(id==100)
    {
	removeObject(); 
    }
    else if(id==110)
    {
	duplicateObject(); 
    } 
} 
 
static void adjustAngleYX(vec2 angle_yx)  
  {  sceneObjs[currObject].angles[1]+=angle_yx[0]; sceneObjs[currObject].angles[0]+=angle_yx[1]; } 
 
static void adjustAngleZTexscale(vec2 az_ts)  
  {  sceneObjs[currObject].angles[2]+=az_ts[0]; sceneObjs[currObject].texScale+=az_ts[1]; } 

// Left mouse button up/down to change distance. Middle mouse button up/down changes speed
static void adjustAnimDist(vec2 ad_ad)
{
    sceneObjs[currObject].animDist += ad_ad[1];
    //sceneObjs[currObject].animSpeed += ad_ad[1];  //Anim Speed mmay need to be implemented here.  Need to go over the videos again.
} 
static void adjustAnimSpeed(vec2 ad_as)
{
    sceneObjs[currObject].animSpeed += ad_as[1];
}

static void animationMenu(int id)
{
    
    if(id == 60)
    {
        setToolCallbacks(adjustAnimDist, mat2(20.0,0,0,1.0),
                         adjustAnimSpeed, mat2(30.0,0,0,1.0));
    }

    if(id == 61)
    {
        if(sceneObjs[currObject].animRun == true)
        {
            sceneObjs[currObject].animRun = false;
        }
        else
        {
            sceneObjs[currObject].animRun = true;
        }
    }
    if(id == 62)
    {
        sceneObjs[currObject].animNum = 0;
    }
    if(id == 63)
    {
        sceneObjs[currObject].animNum = 1;
    }

}

static void mainmenu(int id) { 
    deactivateTool(); 
    if(id == 41 && currObject>=0) { 
	toolObj=currObject;  
        setToolCallbacks(adjustLocXZ, camRotZ(), 
                         adjustScaleY, mat2(0.05, 0, 0, 10) ); 
    } 
    if(id == 50) 
        doRotate(); 
    if(id == 55 && currObject>=0) { 
        setToolCallbacks(adjustAngleYX, mat2(400, 0, 0, -400), 
                         adjustAngleZTexscale, mat2(400, 0, 0, 15) );
    } 

    if(id == 99) exit(0); 
} 
 
static void makeMenu() { 
  int objectId = createArrayMenu(numMeshes, objectMenuEntries, objectMenu); 
 
  int materialMenuId = glutCreateMenu(materialMenu); 
  glutAddMenuEntry("R/G/B/All",10); 
  glutAddMenuEntry("Ambient/Diffuse/Specular/Shine",20); 
 
  int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu); 
  int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu); 
 
  int lightMenuId = glutCreateMenu(lightMenu); 
  glutAddMenuEntry("Move Light 1",70); 
  glutAddMenuEntry("R/G/B/All Light 1",71); 
  glutAddMenuEntry("Move Light 2",80); 
  glutAddMenuEntry("R/G/B/All Light 2",81); 
 
  // Create a new submenu that contains all the object options     
  int objectOptionsMenuId = glutCreateMenu(objectOptionsMenu); 
  glutAddSubMenu("Add object", objectId); 
  // Implement an option to remove an object 
  glutAddMenuEntry("Remove object", 100); 
  // Implement an option to duplicate an object 
  glutAddMenuEntry("Duplicate object", 110); 
 
 // Create a submenu to deal with animations
  int animationMenuId = glutCreateMenu(animationMenu);
  glutAddMenuEntry("Walk Distance/Speed",60);
  glutAddMenuEntry("On/Off",61);
  glutAddMenuEntry("Animation 1",62);
  glutAddMenuEntry("Animation 2",63);
 
 
  glutCreateMenu(mainmenu); 
  glutAddMenuEntry("Rotate/Move Camera",50); 
  glutAddSubMenu("Object Options",objectOptionsMenuId); 
  glutAddMenuEntry("Position/Scale", 41); 
  glutAddMenuEntry("Rotation/Texture Scale", 55); 
  glutAddSubMenu("Material", materialMenuId); 
  glutAddSubMenu("Texture",texMenuId); 
  glutAddSubMenu("Ground Texture",groundMenuId); 
  glutAddSubMenu("Lights",lightMenuId); 
  glutAddSubMenu("Animation Tools",animationMenuId);
  glutAddMenuEntry("EXIT", 99); 
  glutAttachMenu(GLUT_RIGHT_BUTTON); 
 
 
} 
 
 
//---------------------------------------------------------------------------- 
 
void 
keyboard( unsigned char key, int x, int y ) 
{ 
    switch ( key ) {      
    case 011:                       //TAB Key
        nextObject();
	break;
    case 0140:                      //` Key
        previousObject(); 
	break;
    case 033: 
        exit( EXIT_SUCCESS ); 
        break; 
    } 
} 
 
 
 
//---------------------------------------------------------------------------- 
 
 
 
void idle( void ) { 
  glutPostRedisplay(); 
} 
 
 
 
void reshape( int width, int height ) { 
 
    windowWidth = width; 
    windowHeight = height; 
 
    glViewport(0, 0, width, height); 
       
    GLfloat nearDist = 0.02; //0.2, 0.02
    float ratio = (float)width/(float)height; 
    float mod = 0.05; //0.05
    // If the window is higher than it is wide, then we need  
    // to modify the height components of the Frustrum 
    if( ratio < 1.0)  
    //larger height 
    { 
	projection = Frustum(-nearDist * mod, nearDist * mod, 
	                    -nearDist * mod / ratio, nearDist * mod / ratio, 
		            nearDist, 100.0); 
    } 
    else  
    //square or larger width - note if it is square than ratio is 1.0  
    //so it makes no difference to the projection 
    { 
	projection = Frustum(-nearDist * ratio * mod, nearDist * ratio * mod, 
	                    -nearDist * mod, nearDist * mod, 
		            nearDist, 100.0); 
    } 
} 
 
void timer(int unused) 
{ 
    char title[256]; 
    sprintf(title, "%s %s: %d Frames Per Second @ %d x %d", 
            lab, programName, numDisplayCalls, windowWidth, windowHeight ); 

    glutSetWindowTitle(title); 
    
    numDisplayCalls = 0; 
    glutTimerFunc(1000, timer, 1); 
} 
 
char dirDefault1[] = "models-textures"; 
char dirDefault2[] = "/c/temp/models-textures"; 
char dirDefault3[] = "/tmp/models-textures"; 
char dirDefault4[] = "/cslinux/examples/CITS3003/project-files/models-textures"; 
 
void fileErr(char* fileName) { 
    printf("Error reading file: %s\n\n", fileName); 
 
    printf("Download and unzip the models-textures folder - either:\n"); 
    printf("a) as a subfolder here (on your machine)\n"); 
    printf("b) at c:\\temp\\models-textures (labs Windows)\n"); 
    printf("c) or /tmp/models-textures (labs Linux).\n\n"); 
    printf("Alternatively put to the path to the models-textures folder on the command line.\n"); 
 
    exit(1); 
} 
 
int main( int argc, char* argv[] ) 
{ 
    // Get the program name, excluding the directory, for the window title 
    programName = argv[0]; 
    for(char *cpointer = argv[0]; *cpointer != 0; cpointer++) 
        if(*cpointer == '/' || *cpointer == '\\') programName = cpointer+1; 
 
    // Set the models-textures directory, via the first argument or some handy defaults. 
    if(argc>1)                     strcpy(dataDir, argv[1]); 
    else if(opendir(dirDefault1))  strcpy(dataDir, dirDefault1); 
    else if(opendir(dirDefault2))  strcpy(dataDir, dirDefault2); 
    else if(opendir(dirDefault3))  strcpy(dataDir, dirDefault3); 
    else if(opendir(dirDefault4))  strcpy(dataDir, dirDefault4); 
    else fileErr(dirDefault1); 
 
    glutInit( &argc, argv ); 
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH ); 
    glutInitWindowSize( windowWidth, windowHeight ); 
 
    glutInitContextVersion( 3, 2); 
    //glutInitContextProfile( GLUT_CORE_PROFILE );        // May cause issues, sigh, but you 
    glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE ); // should still use only OpenGL 3.2 Core 
                                                          // features. 
    glutCreateWindow( "Initialising..." ); 
 
    glewInit(); // With some old hardware yields GL_INVALID_ENUM, if so use glewExperimental. 
    CheckError(); // This bug is explained at: http://www.opengl.org/wiki/OpenGL_Loading_Library 
 
    init();     CheckError(); // Use CheckError after an OpenGL command to print any errors. 
 
    glutDisplayFunc( display ); 
    glutKeyboardFunc( keyboard ); 
    glutIdleFunc( idle ); 
 
    glutMouseFunc( mouseClickOrScroll ); 
    glutPassiveMotionFunc(mousePassiveMotion); 
    glutMotionFunc( doToolUpdateXY ); 
  
    glutReshapeFunc( reshape ); 
    glutTimerFunc(1000, timer, 1);   CheckError(); 
 
    makeMenu(); CheckError(); 
    glutMainLoop(); 
    return 0; 
} 
