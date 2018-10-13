#include <iostream>

#include "Colour.hpp"
#include "Utils.hpp"
#include "Picture.hpp"
#include "PicLibrary.hpp"
#include <thread>
#include <list>

using namespace std;
static PicLibrary * pl = new PicLibrary();
void configure(int argc, char ** argv);
static list<thread *> tl;

int main(int argc, char ** argv)
{
  pl->init();
  configure(argc, argv);

  string c1, c2, c3;
  bool terminate = false;
  bool no_th;
  while(! terminate){
    std::thread * th;
    cout<< "<<";
    cin >> c1;
    no_th = false;
    string name = "\n";
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
      }else if(c1 == "grayscale"){
        th = new thread(&PicLibrary::grayscale,pl, c2);
      }else if(c1 == "invert"){
        th = new thread(&PicLibrary::invert,pl, c2);
      }else if(c1 == "blur"){
        th = new thread(&PicLibrary::blur,pl, c2);
      }else{
        cin >> c3;
        if(c1 == "save"){
          no_th = true;
          th = new thread(&PicLibrary::savepicture,pl, c2, c3);
          th->join();
        }else if(c1 == "rotate"){
          th = new thread(&PicLibrary::rotate, pl, stoi(c2), c3);
        }else if(c1 == "flip"){
          th = new thread(&PicLibrary::flipVH, pl, c2[0], c3);
        }else if(c1 == "load"){
          no_th = true;
          th = new thread(&PicLibrary::loadpicture, pl, c2, c3);
          th->join();
        }else{
          no_th = true;
          cout << "Command not found\n";
        }
      }
    }
    if(!no_th){
      tl.push_back(th);
    }
  }

  while(!tl.empty()){
    thread * t = tl.front();
    t -> join();
    tl.pop_front();
    delete(t);
  }
  cout << "###exit" << endl;

  pl->terminate();
  delete(pl);
  return 0;
}


void configure(int argc, char ** argv){
  for(int i = 1; i < argc; i++){
    string filepath = argv[i];
    string basename = filepath.substr(filepath.find_last_of('/')+1);
    pl->loadpicture(filepath, basename);
  }
}