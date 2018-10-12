#include <iostream>

#include "Colour.hpp"
#include "Utils.hpp"
#include "Picture.hpp"
#include "PicLibrary.hpp"
#include <thread>
#include <list>

using namespace std;
static PicLibrary * pl = new PicLibrary();

int main(int argc, char ** argv)
{
  pl->init();
  std::list<thread *> tl;
  int commands = 0;

  string c1, c2, c3;
  bool terminate = false;
  bool no_th;
  while(! terminate){
    std::thread * th;
    cout << ">>";
    cin >> c1;
    no_th = false;
    if(c1 == "exit"){
      terminate = true;
      no_th = true;
    }else if(c1 == "liststore"){
      no_th = true; //just print it to avoid command line printing mistakes.
      pl->print_picturestore();
    }else {
      cin >> c2;
      if(c1 == "unload"){
        th = new thread(&PicLibrary::unloadpicture, pl,c2);
      }else if(c1 == "display"){
        th = new thread(&PicLibrary::display,pl,c2);
      }else if(c1 == "greyscale"){
        th = new thread(&PicLibrary::grayscale,pl, c2);
      }else if(c1 == "invert"){
        th = new thread(&PicLibrary::invert,pl, c2);
      }else if(c1 == "blur"){
        th = new thread(&PicLibrary::blur,pl, c2);
      }else{
        cin >> c3;
        if(c1 == "save"){
          th = new thread(&PicLibrary::savepicture,pl, c2, c3);
        }else if(c1 == "rotate"){
          th = new thread(&PicLibrary::rotate, pl, stoi(c2), c3);
        }else if(c1 == "flip"){
          th = new thread(&PicLibrary::flipVH, pl, c2[0], c3);
        }else if(c1 == "load"){
          th = new thread(&PicLibrary::loadpicture, pl, c2, c3);
        }else{
          no_th = true;
          cout << "Command not found\n";
        }
      }
    }
    if(!no_th){
      tl.push_back(th);
      commands++;
    }
  }

  for(int i = 0; i < commands; i++){
    std::thread * t = tl.front();
    t -> join();
    tl.pop_front();
    delete(t);
  }

  pl->terminate();
  delete(pl);
  return 0;
}

