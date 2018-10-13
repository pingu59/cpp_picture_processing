#include "PicLibrary.hpp"
#include "Colour.hpp"
#include <thread>
#define MAX_COLOUR 255

static Utils u = Utils();

Colour invert_single(int x, int y, Picture * pic);
Colour grayscale_single(int x, int y, Picture * pic);
Colour FlipV_single(int x, int y, Picture * pic);
Colour FlipH_single(int x, int y, Picture * pic);
Colour blur_single(int x, int y, Picture * pic);
Colour rotate90_single(int x, int y, Picture * pic);
Colour rotate180_single(int x, int y, Picture * pic);
Colour rotate270_single(int x, int y, Picture * pic);
static void thread_func(Picture ptemp,int i,int w,Picture * pic, Colour (* func)(int, int, Picture*),
                                bool crosspixel, bool shape);


Position PicLibrary::find(string filename){
    PicLock  * curr = head->next;
    PicLock  * pred = head;
    Position p = Position();
    while(curr->name != "\n"){
        if(curr->name >= filename){
            p.curr = curr;
            p.pred = pred;
            return p;
        }
        pred = curr;
        curr = curr->next;
    }
    p.curr = curr;
    p.pred = pred;
    return p;
}

void PicLibrary::init(){
  head = new PicLock();
  tail = new PicLock();

  head->m = new std::mutex();
  tail->m = new std::mutex();

  head->name = "\n";
  tail->name = "\n";  //implicitly set to this since there will never be name as this
  head->next = tail;
}

void PicLibrary::terminate(){
    delete(head -> m);
    delete(tail -> m);
    delete(head -> pic);
    delete(tail -> pic);
    delete(head);
    delete(tail);
 }

  void PicLibrary::print_picturestore(){
    PicLock * pred = head;
    pred -> m->lock();
    pred->next -> m->lock();
    PicLock * curr = pred->next;
    while(curr -> name != "\n"){
        cout << curr -> name << endl;
        curr -> next -> m->lock();
        pred -> m->unlock();
        pred = curr;
        curr = curr -> next;
    }
    pred -> m->unlock();
    curr -> m->unlock();
  }

  bool PicLibrary::valid(PicLock *pred, PicLock *curr){
      PicLock * pl = head;
      while(pl-> name <= pred->name){
          if(pl -> name == pred->name){
              return pl -> next -> name == curr->name;
          }
          pl = pl -> next;
      }
      return false;
  }


  void PicLibrary::loadpicture(string path, string filename){
      bool ndone = true;
      while(ndone){
        Position p= find(filename);
        p.pred->m->lock(); p.curr->m->lock();
        if(valid(p.pred, p.curr)){
            if(p.curr->name == filename){
                p.pred->m->unlock(); p.curr->m->unlock();
                cerr << "FAILED : Picture of the same name already exists.\n";
                return;
            }
            Picture * pic = new Picture(path);
            if(!pic -> getimage().empty()){
            //cout << pic -> getimage()<< "!!"<<endl;
                PicLock * pl = new PicLock();
                pl->next = p.curr;
                pl->m = new std::mutex();
                pl->name = filename;
                pl->pic = pic;
                p.pred->next = pl;
            }
            ndone = false;
        }
        p.pred->m->unlock(); p.curr->m->unlock();
      }
  }

  void PicLibrary::unloadpicture(string filename){
      bool ndone = true;
      while(ndone){
        Position p= find(filename);
        p.pred->m->lock(); p.curr ->m->lock();
        if(valid(p.pred, p.curr)){
            if(p.curr->name == filename){
                p.pred->next = p.curr->next; 
                p.curr ->m->unlock();
                delete(p.curr->m);
                delete(p.curr->pic);
                delete(p.curr); 
                p.pred->m->unlock(); 
                return;
            }else{
                p.pred->m->unlock(); p.curr->m->unlock();
                cerr << "FAILED : Picture not found.\n";
                return;
            }
            ndone = false;
        }
        p.pred->m->unlock(); p.curr->m->unlock();
    }
  }

  
  void PicLibrary::savepicture(string filename, string path){
      bool ndone = true;
      while(ndone){
        Position p= find(filename);
        p.pred->m->lock(); p.curr->m->lock();
        if(valid(p.pred, p.curr)){
            if(p.curr->name == filename){
                u.saveimage(p.curr->pic->getimage(), path);
            }else{
                p.pred->m->unlock(); p.curr->m->unlock();
                cerr << "FAILED : Picture not found.\n";
                return;
            }
            ndone = false;
        }
        p.pred->m->unlock(); p.curr->m->unlock();
      }
  }


  void PicLibrary::display(string filename){
      bool ndone = true;
      while(ndone){
        Position p = find(filename);
        p.pred->m->lock(); p.curr->m->lock();
        if(valid(p.pred, p.curr)){
            if(p.curr->name == filename){
                u.displayimage(p.curr->pic->getimage());
            }else{
                p.pred->m->unlock(); p.curr->m->unlock();
                cerr << "FAILED : Picture not found.\n";
                return;
            }
            ndone = false;
        }
        p.pred->m->unlock(); p.curr->m->unlock();
      }
  }

 Colour invert_single(int x, int y, Picture * pic){
    Colour thisc = pic -> getpixel(x, y);
    thisc.setred(MAX_COLOUR - thisc.getred());
    thisc.setblue(MAX_COLOUR - thisc.getblue());
    thisc.setgreen(MAX_COLOUR - thisc.getgreen());
    return thisc;
  }

  void PicLibrary::general_by_row_helper(Picture * pic, Colour (* func)(int, int, Picture *), bool shape,
                    bool crosspixel){
    Picture ptemp;
    int w = pic->getwidth();
    int h = pic->getheight();
          if(crosspixel){
            if(shape){
                ptemp = Picture(h, w);
            }else{
                ptemp = Picture(w, h);
            }
          }
          thread th[h];
          for(int i = 0; i < h; i ++){
              th[i] = thread(&thread_func, ptemp, i, w, pic, func, crosspixel, shape);
          }
          
          for(int i = 0; i < h; i ++){
              th[i].join();
          }
          
          if(crosspixel){
              pic->setimage(ptemp.getimage());
          }
    }

  void PicLibrary::general_by_row(string filename, Colour (* func)(int, int, Picture*), bool shape, bool
                        crosspixel){
    bool ndone = true;
    while(ndone){
        Position p = find(filename);
        p.pred->m->lock(); p.curr->m->lock();
        if(valid(p.pred, p.curr)){
            if(p.curr->name == filename){
                general_by_row_helper(p.curr->pic, func, shape, crosspixel);
            }else{
                p.pred->m->unlock(); p.curr->m->unlock();
                cerr << "FAILED : Picture not found.\n";
                return;
            }
            ndone = false;
        }
        p.pred->m->unlock(); p.curr->m->unlock();
    }
  }
    static void thread_func(Picture ptemp,int i,int w, Picture *pic, Colour (* func)(int, int, Picture*),
                                bool crosspixel, bool shape){
                  if(!crosspixel){
                    for(int j = 0; j < w; j++){
                        pic->setpixel(j, i, func(j, i, pic));
                    }
                  }else{
                    if(shape){
                        for(int j = 0; j < w; j++){
                            ptemp.setpixel(i, j, func(j, i, pic));
                        }
                    }else{
                        for(int j = 0; j < w; j++){
                            ptemp.setpixel(j, i, func(j, i, pic));
                        }
                    }
                  }                    
    }

  void PicLibrary::invert(string filename){
      general_by_row(filename, &invert_single, false, false);
  }

  Colour grayscale_single(int x, int y, Picture * pic){
    Colour thisc = pic -> getpixel(x, y);
    int avg  = (thisc.getred() + thisc.getblue() + thisc.getgreen())/3;
    thisc.setred(avg);
    thisc.setblue(avg);
    thisc.setgreen(avg);
    return thisc;
  }


  void PicLibrary::grayscale(string filename){
    general_by_row(filename, &grayscale_single, false, false);
  }

  Colour rotate90_single(int x, int y, Picture * pic){
      return pic -> getpixel(x, pic -> getheight() - y - 1);
  }

  Colour rotate180_single(int x, int y, Picture * pic){
      return pic -> getpixel(pic -> getwidth() - x - 1, pic -> getheight() - y - 1);
  }

  Colour rotate270_single(int x, int y, Picture * pic){
      return pic -> getpixel(pic -> getwidth() - x - 1, y);
  }

  void PicLibrary::rotate(int angle, string filename){
      if(angle == 90){
          general_by_row(filename, &rotate90_single, true, true);
      }else if (angle  == 180){
          general_by_row(filename, &rotate180_single, false, true);
      }else{
          general_by_row(filename, &rotate270_single, true, true);
      }
  }

  Colour FlipV_single(int x, int y, Picture * pic){
    return pic -> getpixel(x, pic -> getheight() - y - 1);
  }
  Colour FlipH_single(int x, int y, Picture * pic){
    return pic -> getpixel(pic -> getwidth() - x - 1, y);
  }
  void PicLibrary::flipVH(char plane, string filename){
      if(plane == 'H'){
          general_by_row(filename, &FlipH_single, false, true);
      }else if(plane == 'V'){
          general_by_row(filename, &FlipV_single, false, true);
      }else{
          cerr << "False input\n";
      }
  }

  Colour blur_single(int x, int y, Picture * pic){
      Colour thisc = pic -> getpixel(x, y);
      if ( x != 0 && y != 0 && y  < pic -> getheight() - 1 && x < pic -> getwidth() - 1){
          int accr = 0;
          int accg = 0;
          int accb = 0;
          for(int i = -1; i < 2; i++){
              for(int j = -1; j < 2; j++){
                  Colour c = pic -> getpixel(x + i, y + j);
                  accr += c.getred();
                  accg += c.getgreen();
                  accb += c.getblue();
              }
          }
        thisc.setred(accr / 9);
        thisc.setgreen(accg / 9);
        thisc.setblue(accb / 9);
      }
      return thisc;
  }

  void PicLibrary::blur(string filename){
      general_by_row(filename, &blur_single, false, true);
  }
