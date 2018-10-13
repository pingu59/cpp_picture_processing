#ifndef PICLIB_H
#define PICLIB_H

#include "Picture.hpp"
#include "Utils.hpp"
#include <mutex>
#include <thread>

class PicLock{
public:
  Picture * pic;
  PicLock * next;
  mutex * m;
  string name;
};

enum concurrency_type{
    ROW, COLUMN, SECTOR2, SECTOR4, SECTOR8, PIXEL
};


class Position{
  public:
    PicLock * curr;
    PicLock * pred;
};

class PicLibrary {

  private:
  // TODO: define internal picture storage 
  PicLock * head;
  PicLock * tail;

  public:
  // defaiult constructor/deconstructor
  PicLibrary(){};
  ~PicLibrary(){};


  Position find(string filename);
  void general(enum concurrency_type, string filename, Colour (* func)(int, int, Picture*), bool shape,
                    bool crosspixel);
  void general_helper(concurrency_type type, Picture * pic, Colour (* func)(int, int, Picture*), bool shape,
                    bool crosspixel);
  bool valid(PicLock *pred, PicLock * curr);
  void init();
  void terminate();

  // command-line interpreter routines
  void print_picturestore();
  void loadpicture(string path, string filename);
  void unloadpicture(string filename);
  void savepicture(string filename, string path);
  void display(string filename);
  
  // picture transformation routines
  void invert(string filename);
  void grayscale(string filename);
  void rotate(int angle, string filename);
  void flipVH(char plane, string filename);
  void blur(string filename);
};

#endif

