/*
 * \author Alex Hills
 *
 * \file  gari_demo.cpp
 *
 * Adapted from chess_demo.cpp.
 */


#include <vrg3d/VRG3D.h>
//Threads
#include <pthread.h>
//Sleep
#include <unistd.h>

#include <stdio.h>
#include <iostream>
//layers
#include "layer.h"
#include "group_layer.h"
#include "ptif_layer.h"

//Math
#include "matrix.h"

#define MAX_TEXTURES 32

float SCALE;
float rotation_matrix[16];
int globalW = 1920;

using namespace G3D;

/** This is a sample VR application using the VRG3D library.  Two key
    methods are filled in here: doGraphics() and doUserInput().  The
    code in these methods demonstrates how to draw graphics and
    respond to input from the mouse, the keyboard, 6D trackers, and VR
    wand buttons.
*/

class MyVRApp : public VRApp
{
public:
  MyVRApp(const std::string &mySetup, char *fname) : VRApp()
  {
    // initialize the VRApp
    Log  *demoLog = new Log("demo-log.txt");
    init(mySetup, demoLog);

    // The default starting point has the eye level with the chess
    // board, which is confusing for the viewer on startup, and
    // renders poorly too. Let's move the virtual space up a few units
    // for a more sensible view.
    //_virtualToRoomSpace = CoordinateFrame();
   
    // This is the background -- the color that appears where there is
    // nothing to render, and we'lfloat SCALE;
    _clearColor = Color3(0.0, 0.0, 0.0);

    // The actual models of the chess pieces are pretty messy, so they
    // are confined to another code file for the sake of tidiness.
	off_x = 0.0;
	off_y = 0.0;
	scale = 0.01;

	//Not sure why to set scale	
	SCALE = 1;

	layercount = 0;
	//add Base Layer
	layers = new group_layer("Base");
	selectedLayer = (layer*)layers;
	selectable_layers += layers;

	//read project file
	//if (argc > 1)
	//	read_project(argv[1]);

	active_image = 0;

	read_project(fname);
  }

  virtual ~MyVRApp() {}

int readLine(FILE *infile, char *line) {
  int i;
  int ch;
#include <stdio.h>

  if (fgets(line, 1023, infile) == NULL)
    return(0);
  else {
    int len = strlen(line);
    if ( (len > 0) && (line[len-1] == '\n'))
      line[len-1] = 0;
    return(len);
  }
}


/*  Very simple, recursive project loader */
void read_project(char *projfile) {  
  FILE *fp;
  static ARRAY < group_layer * > current_group = layers;
  char base[256] = "\0";
  

  if ((fp=fopen(projfile, "r")) == NULL) {
    printf("Unable to open project file %s\n", projfile);
    return;
  }
  char buf[1024];
  while (readLine(fp, buf)) {
    char *delims = " ";
    char *result = NULL;
    float alpha = 1.0;
    float scale = 2.0;
    if (buf[0] == '#')
      continue;
    result = strtok(buf, delims);
   
    if(!strcasecmp(result, "GROUP")) {
      char *filename = strtok(NULL, delims);
      char *stralpha = strtok(NULL, delims);
      char *strscale = strtok(NULL, delims);
      if (filename) {
        if (stralpha)
          alpha = atof(stralpha);
        if (strscale)
          scale = atof(strscale);
        group_layer *glayer = new group_layer(filename);
        glayer->set_alpha(alpha);
        glayer->set_scale(scale);
        current_group.last()->add(glayer);
         selectable_layers += glayer;
       layercount++;
       current_group += glayer;
      }
    }
    else if(!strcasecmp(result, "ENDGROUP")) {
      current_group.pop();
    } 
    else if(!strcasecmp(result, "PTIF")) {
      char *filename = strtok(NULL, delims);
      char *stralpha = strtok(NULL, delims);
      char *strscale = strtok(NULL, delims);
      if (strlen(base) > 1) {
        char tmpfile[256];
        sprintf(tmpfile, "%s/%s", base, filename);
        bzero (filename, sizeof(filename));
        strcpy(filename, tmpfile);
      }
        
      if (filename) {
        if (stralpha)
          alpha = atof(stralpha);
        if (strscale)
          scale = atof(strscale);
        ptif_layer *player = new ptif_layer(filename);
        player->set_alpha(alpha);
        player->set_scale(scale);
        player->set_buffer(BOTH);
        current_group.last()->add(player);
        selectable_layers += player;
       layercount++;
      }
    }
    else if(!strcasecmp(result, "PROJ")) {
      char *filename = strtok(NULL, delims);
      if (strlen(base) > 1) {
        char tmpfile[256];
        sprintf(tmpfile, "%s/%s", base, filename);
        bzero (filename, sizeof(filename));
        strcpy(filename, tmpfile);
      }
      if (filename) {
        read_project(filename);
       }    
    }
    else if(!strcasecmp(result, "BASE")) {
      char *filename = strtok(NULL, delims);
      if (filename) {
        strcpy(base, filename);
       }    
    }
    else if(!strcasecmp(result, "BUFFER")) {
      char *bufname = strtok(NULL, delims);
      if (bufname) {
        if (!strcasecmp(bufname, "LEFT")) {
          fprintf(stderr, "Setting buffer to Left for %s\n", selectable_layers.last()->get_name());
          selectable_layers.last()->set_buffer(LEFT);
        }
        else if (!strcasecmp(bufname, "RIGHT")){
          fprintf(stderr, "Setting buffer to Right for %s\n", selectable_layers.last()->get_name());
          selectable_layers.last()->set_buffer(RIGHT);
        }
       }    
    }

  }

   printf("project  %s loaded \n", projfile);
}

  void doUserInput(Array<VRG3D::EventRef> &events)
  {
    static double joystick_x = 0.0;
    static double joystick_y = 0.0;

    for (int i = 0; i < events.size(); i++) {
	if(events[i]->getName() != "SynchedTime")	 
		fprintf(stderr, "Event %s\n", events[i]->getName().c_str());

      if (events[i]->getName() == "Wand_Joystick_X")
      { 
		joystick_x = events[i]->get1DData();
      }
      else if (events[i]->getName() == "Wand_Joystick_Y")
      { 
		joystick_y = events[i]->get1DData();
      }
	else if (events[i]->getName() == "Wand_Middle_Btn_down")
      {  
	//scale += 0.001;
		off_y += 1.0;
      }
      else if(events[i]->getName() ==  "kbd_RIGHT_down")
      {
                off_x -= 1.0;
      }
      else if(events[i]->getName() ==  "kbd_LEFT_down")
      {
                off_x += 1.0;
      }
	else if (events[i]->getName() == "Wand_Left_Btn_down")
      { 
		active_image--;
		off_x = 0.0;
		if(active_image < 0)active_image = layers->num() - 1;
      }
	else if (events[i]->getName() == "Wand_Right_Btn_down")
      {  
		active_image++;
		off_x = 0.0;
		if(active_image >= layers->num())active_image = 0;
      }
	else if (events[i]->getName() == "B6_down")
      {  
		off_y -= 1.0;
      }
    
      //move
      if (fabs(joystick_x) > 0.2) {
		off_x += joystick_x;
      }
      if (fabs(joystick_y) > 0.2 ) {
		scale += -joystick_y * 0.0001;
      }
    }
  }

#define CLIENT_SLEEP 0.0005


  void doGraphics(RenderDevice *rd, bool left_eye)
  {  
    // The tracker frames above are drawn with the object to world
    // matrix set to the identity because tracking data comes into the
    // system in the Room Space coordinate system.  Room Space is tied
    // to the dimensions of the room and the projection screen within
    // the room, thus it never changes as your program runs.  However,
    // it is often convenient to move objects around in a virtual
    // space that can change relative to the screen.  For these
    // objects, we put a virtual to room space transform on the OpenGL
    // matrix stack before drawing them, as is done here..
    //
  glDisable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glDisable(GL_CULL_FACE);

    rd->disableLighting();
    rd->pushState();
    //rd->setObjectToWorldMatrix(_virtualToRoomSpace);
    glPushMatrix();

    glTranslatef(0,0,-6);
    glScalef(scale,scale,scale);
    glTranslatef(off_x,off_y,0);
    
    layers->get_child(active_image)->draw();

    glPopMatrix();
  }
  

protected:
	double off_x;
	double off_y;
	double scale;
	int active_image;

int layercount;
group_layer *layers;
layer *selectedLayer;
ARRAY <layer*> selectable_layers;
};




int main( int argc, char **argv )
{
  // The first argument to the program tells us which of the known VR
  // setups to start
  std::string setupStr;
  MyVRApp *app;

  if (argc >= 2)
  { setupStr = std::string(argv[1]);
  }

  // This opens up the graphics window, and starts connections to
  // input devices, but doesn't actually start rendering yet.

  app = new MyVRApp(setupStr,argv[2]);

  // This starts the rendering/input processing loop
  app->run();

  return 0;
}


//
////////////////////  end  common/vrg3d/demo/vrg3d_demo.cpp  ///////////////////
