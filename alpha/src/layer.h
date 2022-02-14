#ifndef LAYER_H
#define LAYER_H
#include <GL/glut.h>
#include <math.h>

#include <stdio.h>
#include "matrix.h"

#define METERS2RAD 59274.6975232
//#define METERS2RAD 1

typedef struct POINT {
  float x;
  float y;
  float z;
};

enum DrawBuffer { UNSET, BOTH, LEFT, RIGHT };
enum LayerType {LAYER, IMAGE, SHAPE, GROUP };


class layer {
  protected:
    float _alpha;
    float _scale;
    int _visible;
    POINT center;
    double meterspp;
    char _name[80];
    int _group;
    DrawBuffer _draw_buffer;
    LayerType type;
    float gamma, gmin, gmax;
    Matrix transform;
    
  public:
    virtual void draw() {}
    virtual int load_ready() { return 0;}
    virtual void load() {}
    virtual void set_alpha(float alpha) {_alpha = alpha;}
    float get_alpha() {return _alpha; } 
    virtual void set_scale(float scale) {_scale =scale; }
    void set_visible(int visible) { _visible = visible; }
    char* get_name() {return _name; }
    int get_type() { return type; }
    void set_buffer(DrawBuffer buf) { _draw_buffer = buf; };
    DrawBuffer get_buffer() { return _draw_buffer; };
    double distance(double x, double y, double z) {
      double tx, ty, tz;
      tx = center.x - x;
      ty = center.y - y;
      tz = center.z - z;
      return (sqrt(tx*tx + ty*ty + tz*tz));
    }
    virtual void set_gamma(float g) { gamma = g; }
    virtual void set_gmax(float g) { gmax = g; }
    virtual void set_gmin(float g) { gmin = g; }
    float get_gamma() { return gamma; }
    float get_gmax() { return gmax; }
    float get_gmin() { return gmin; }
    void translate(float x, float y, float z) {
      translate_matrix(x, y, z, transform);
    }

}; 
#endif
