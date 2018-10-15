#ifndef PICLIB_H
#define PICLIB_H

#include "Picture.hpp"
#include "Utils.hpp"
#include <mutex>
#include <thread>

/*The synchronization strategy I used here is Optimistic Locking.
  1. find the Position to alter without locking anything
  2. Lock the Position
  3. Validate it (see if the position can be visited from head)
    3.1 If invalid, goto 1
    3.2 If valid, make change with it
*/

class PicLock /*Stores the picture, its name and a lock*/
{
public:
  Picture *pic;
  PicLock *next;
  mutex *m;
  string name;
};

enum concurrency_type /*Types of concurrent transformation*/
{
  ROW,
  COLUMN,
  SECTOR2,
  SECTOR4,
  SECTOR8,
  SEQUENTIAL,
  PIXEL
};

class Position /*Required by Optimistic locking*/
{
public:
  PicLock *curr;
  PicLock *pred;
};

class PicLibrary
{

private:
  //internal picture storage
  PicLock *head;
  PicLock *tail;

public:
  // defaiult constructor/deconstructor
  PicLibrary()
  {
    head = new PicLock();
    tail = new PicLock();

    head->m = new std::mutex();
    tail->m = new std::mutex();

    head->name = "\n";
    tail->name = "\n"; //implicitly set to this since there will never be name like this
    head->next = tail;
  };
  ~PicLibrary()
  {
    PicLock *curr = head->next;
    while (curr->name != "\n")
    {
      unloadpicture(curr->name);
      curr = head->next;
    }
    delete (head->m);
    delete (tail->m);
    delete (head->pic);
    delete (tail->pic);
    delete (head);
    delete (tail);
  };

  Position find(string filename);
  void general(enum concurrency_type, string filename, Colour (*func)(int, int, Picture *), bool shape,
               bool crosspixel); //of transformation
  void general_helper(concurrency_type type, Picture *pic, Colour (*func)(int, int, Picture *), bool shape,
                      bool crosspixel);
  void general_libfunc(bool (*func)(PicLock *, PicLock *, string, string), string path, string filename);
  bool valid(PicLock *pred, PicLock *curr);

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
