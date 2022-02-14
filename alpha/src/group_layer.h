#ifndef GROUP_LAYER_H
#define GROUP_LAYER_H

#include "layer.h"
#include "list.h"

#include <stdio.h>
#include <string.h>
/*  This is a layer class that can hold other layers, including other groups. */

class group_layer : public layer {
  private:
    ARRAY<layer *> child;  

  public:
  
    group_layer(char * name) { 
      strcpy(this->_name, name); 
      type = GROUP; 
      gmin=0.0; 
      gmax =1.0; 
      gamma=1.0;
      identity_matrix(transform);
    }
    int add(layer *thing) {child += thing; }
    int remove(int ID) {child.remove(ID); } /* ID is the index into the array */
    layer *get_child(int ID) { return (child[ID]); }
    int num(){return child.num();}
    void draw(bool left) {
      glPushMatrix();
      glMultMatrixf(transform);
      /* This will set left/right or both as the draw buffer */
      //active_buffer(_draw_buffer);  
      for (int i = 0; i < child.num(); i++){
		  if(child[i]->get_buffer() == UNSET ||
		  child[i]->get_buffer() == BOTH ||
		  (child[i]->get_buffer() == LEFT && left )||
		  (child[i]->get_buffer() == RIGHT && !left ))
		   child[i]->draw();
	   }
      glPopMatrix();
    }
    
    void set_alpha(float alpha) {
      _alpha = alpha; 
      for (int i = 0; i < child.num(); i++) { child[i]->set_alpha(alpha); } 
    }

    void set_gmax(float g) {
      gmax = g; 
      for (int i = 0; i < child.num(); i++) { child[i]->set_gmax(g); } 
    }
    void set_gmin(float g) {
      gmin = g; 
      for (int i = 0; i < child.num(); i++) { child[i]->set_gmin(g); } 
    }
    void set_gamma(float g) {
      gamma = g; 
      for (int i = 0; i < child.num(); i++) { child[i]->set_gamma(g); } 
    }

    void set_scale(float scale) { 
      _scale =scale; 
      for (int i = 0; i < child.num(); i++) { child[i]->set_scale(scale); } 
    }
    
    /* This looks to see if each child is ready to load it's data, then tells it
      to load.  Will re-rewrite to suport threading */
    void load() { 
      for (int i=0; i<child.num(); i++) 
        if (child[i]->load_ready()) child[i]->load(); 
    }
};



#endif
