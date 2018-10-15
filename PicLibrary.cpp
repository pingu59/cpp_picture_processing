#include "PicLibrary.hpp"
#include "Colour.hpp"
#include <thread>
#include <unistd.h>
#include <time.h>
#define MAX_COLOUR 255

static Utils u = Utils();
/* functions for transformation */
Colour invert_single(int x, int y, Picture *pic);
Colour grayscale_single(int x, int y, Picture *pic);
Colour FlipV_single(int x, int y, Picture *pic);
Colour FlipH_single(int x, int y, Picture *pic);
Colour blur_single(int x, int y, Picture *pic);
Colour rotate90_single(int x, int y, Picture *pic);
Colour rotate180_single(int x, int y, Picture *pic);
Colour rotate270_single(int x, int y, Picture *pic);

/* function of different procesing methods */
static void row_thread_func(Picture ptemp, int i, int w, Picture *pic, Colour (*func)(int, int, Picture *),
                            bool crosspixel, bool shape);
static void pixel_thread_func(Picture ptemp, int i, int j, Picture *pic, Colour (*func)(int, int, Picture *),
                              bool crosspixel, bool shape);
static void column_thread_func(Picture ptemp, int i, int h, Picture *pic, Colour (*func)(int, int, Picture *),
                               bool crosspixel, bool shape);
static void sector_thread_func(Picture ptemp, int left, int right, int up, int down, Picture *pic, Colour (*func)(int, int, Picture *),
                               bool crosspixel, bool shape);
static void sequential(Picture ptemp, int w, int h, Picture *pic, Colour (*func)(int, int, Picture *),
                       bool crosspixel, bool shape);

/* functions for managing the library */
static bool load_helper(PicLock *pred, PicLock *curr, string path, string filename);
static bool unload_helper(PicLock *pred, PicLock *curr, string path, string filename);
static bool save_helper(PicLock *pred, PicLock *curr, string path, string filename);
static bool display_helper(PicLock *pred, PicLock *curr, string path, string filename);

Position PicLibrary::find(string filename)
{
    PicLock *curr = head->next;
    PicLock *pred = head;
    Position p = Position();
    while (curr->name != "\n")
    {
        if (curr->name >= filename)
        {
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

void PicLibrary::general_libfunc(bool (*func)(PicLock *, PicLock *, string, string), string path, string filename)
{
    bool ndone = true;
    while (ndone)
    {
        bool unlock = false;
        Position p = find(filename);
        p.pred->m->lock();
        p.curr->m->lock();
        if (valid(p.pred, p.curr))
        {
            unlock = func(p.pred, p.curr, path, filename);
            ndone = false;
        }
        if (unlock)
        {
            p.pred->m->unlock();
            p.curr->m->unlock();
        }
    }
}

bool PicLibrary::valid(PicLock *pred, PicLock *curr)
{
    PicLock *pl = head;
    while (pl->name <= pred->name)
    {
        if (pl->name == pred->name)
        {
            return pl->next->name == curr->name;
        }
        pl = pl->next;
    }
    return false;
}

void PicLibrary::print_picturestore()
{
    PicLock *pred = head;
    pred->m->lock();
    pred->next->m->lock();
    PicLock *curr = pred->next;
    while (curr->name != "\n")
    {
        cout << curr->name << endl;
        curr->next->m->lock();
        pred->m->unlock();
        pred = curr;
        curr = curr->next;
    }
    pred->m->unlock();
    curr->m->unlock();
}

static bool load_helper(PicLock *pred, PicLock *curr, string path, string filename)
{
    if (curr->name == filename)
    {
        cerr << "FAILED : Picture of the same name already exists." << endl;
    }
    else
    {
        Picture *pic = new Picture(path);
        if (!pic->getimage().empty())
        {
            PicLock *pl = new PicLock();
            pl->next = curr;
            pl->m = new std::mutex();
            pl->name = filename;
            pl->pic = pic;
            pred->next = pl;
        }
        else
        {
            delete (pic);
        }
    }
    return true;
}

void PicLibrary::loadpicture(string path, string filename)
{
    general_libfunc(load_helper, path, filename);
}

static bool unload_helper(PicLock *pred, PicLock *curr, string path, string filename)
{
    if (curr->name == filename)
    {
        pred->next = curr->next;
        delete (curr->pic);
        curr->m->unlock();
        delete (curr->m);
        delete (curr);
        pred->m->unlock();
        usleep(1);
        return false;
    }
    else
    {
        cerr << "FAILED : Picture not found." << endl;
        return true;
    }
}

void PicLibrary::unloadpicture(string filename)
{
    general_libfunc(unload_helper, "Not used", filename);
}

static bool save_helper(PicLock *pred, PicLock *curr, string path, string filename)
{
    if (curr->name == filename)
    {
        u.saveimage(curr->pic->getimage(), path);
    }
    else
    {
        cerr << "FAILED : Picture not found." << endl;
    }
    return true;
}

void PicLibrary::savepicture(string filename, string path)
{
    general_libfunc(save_helper, path, filename);
}

static bool display_helper(PicLock *pred, PicLock *curr, string path, string filename)
{
    if (curr->name == filename)
    {
        u.displayimage(curr->pic->getimage());
    }
    else
    {
        cerr << "FAILED : Picture not found." << endl;
    }
    return true;
}

void PicLibrary::display(string filename)
{
    general_libfunc(display_helper, "Not used", filename);
}

Colour invert_single(int x, int y, Picture *pic)
{
    Colour thisc = pic->getpixel(x, y);
    thisc.setred(MAX_COLOUR - thisc.getred());
    thisc.setblue(MAX_COLOUR - thisc.getblue());
    thisc.setgreen(MAX_COLOUR - thisc.getgreen());
    return thisc;
}

void PicLibrary::general_helper(concurrency_type type, Picture *pic, Colour (*func)(int, int, Picture *), bool shape,
                                bool crosspixel)
{
    /*check if there is a need to create a new picture or change the shape of the picture */
    Picture ptemp;
    int w = pic->getwidth();
    int h = pic->getheight();
    if (crosspixel)
    {
        if (shape)
        {
            ptemp = Picture(h, w);
        }
        else
        {
            ptemp = Picture(w, h);
        }
    }

    /*find the suitable way to do concurrency accroding to parameter type.*/
    switch (type)
    {
    case ROW:
    {
        thread th[h];
        for (int i = 0; i < h; i++)
        {
            th[i] = thread(&row_thread_func, ptemp, i, w, pic, func, crosspixel, shape);
        }

        for (int i = 0; i < h; i++)
        {
            th[i].join();
        }
    }

    break;
    case COLUMN:
    {
        thread th[w];
        for (int i = 0; i < w; i++)
        {
            th[i] = thread(&column_thread_func, ptemp, i, h, pic, func, crosspixel, shape);
        }

        for (int i = 0; i < w; i++)
        {
            th[i].join();
        }
    }

    break;
    case SECTOR2:
    {
        int midH = w / 2;
        thread t1 = thread(&sector_thread_func, ptemp, 0, midH, h, 0, pic, func, crosspixel, shape);
        thread t2 = thread(&sector_thread_func, ptemp, midH, w, h, 0, pic, func, crosspixel, shape);
        t1.join();
        t2.join();
    }
    break;
    case SECTOR4:
    {
        int midV = h / 2;
        int midH = w / 2;
        thread t1 = thread(&sector_thread_func, ptemp, 0, midH, midV, 0, pic, func, crosspixel, shape);
        thread t2 = thread(&sector_thread_func, ptemp, midH, w, h, midV, pic, func, crosspixel, shape);
        thread t3 = thread(&sector_thread_func, ptemp, 0, midH, h, midV, pic, func, crosspixel, shape);
        thread t4 = thread(&sector_thread_func, ptemp, midH, w, midV, 0, pic, func, crosspixel, shape);
        t1.join();
        t2.join();
        t3.join();
        t4.join();
    }

    break;
    case SECTOR8:
    {
        int oneeighthH = w / 8;
        thread th[8];
        for (int i = 0; i < 7; i++)
        {
            th[i] = thread(&sector_thread_func, ptemp, i * oneeighthH, (i + 1) * oneeighthH, h, 0, pic, func, crosspixel, shape);
        }
        th[7] = thread(&sector_thread_func, ptemp, 7 * oneeighthH, w, h, 0, pic, func, crosspixel, shape);
        for (int i = 0; i < 8; i++)
        {
            th[i].join();
        }
    }
    break;
    case PIXEL:
    {
        thread th[w][h];
        for (int i = 0; i < w; i++)
        {
            for (int j = 0; j < h; j++)
            {
                th[i][j] = thread(&pixel_thread_func, ptemp, i, j, pic, func, crosspixel, shape);
            }
        }
        for (int i = 0; i < w; i++)
        {
            for (int j = 0; j < h; j++)
            {
                th[i][j].join();
            }
        }
    }
    break;
    default: //sequentially
        sequential(ptemp, w, h, pic, func, crosspixel, shape);
        break;
    }

    if (crosspixel)
    {
        pic->setimage(ptemp.getimage());
    }
}

static void pixel_thread_func(Picture ptemp, int i, int j, Picture *pic, Colour (*func)(int, int, Picture *),
                              bool crosspixel, bool shape)
{
    if (!crosspixel)
    {
        pic->setpixel(i, j, func(i, j, pic));
    }
    else
    {
        if (shape)
        {

            ptemp.setpixel(j, i, func(i, j, pic));
        }
        else
        {

            ptemp.setpixel(i, j, func(i, j, pic));
        }
    }
}

void PicLibrary::general(concurrency_type type, string filename, Colour (*func)(int, int, Picture *), bool shape, bool crosspixel)
{
    bool ndone = true;
    while (ndone)
    {
        Position p = find(filename);
        p.pred->m->lock();
        p.curr->m->lock();
        if (valid(p.pred, p.curr))
        {
            if (p.curr->name == filename)
            {
                /*To test the time duration of processing the picture*/
                // struct timespec *tp = (struct timespec *)malloc(sizeof(struct timespec));
                // clock_gettime(CLOCK_MONOTONIC, tp);

                general_helper(type, p.curr->pic, func, shape, crosspixel);

                // struct timespec *tp2 = (struct timespec *)malloc(sizeof(struct timespec));
                // clock_gettime(CLOCK_MONOTONIC, tp2);
                // long long difference = (tp2->tv_sec * 1000000000 + tp2->tv_nsec) - (tp->tv_sec * 1000000000 + tp->tv_nsec);
                // cout << "Execution time is: " << difference << endl;
                // free(tp);
                // free(tp2);
            }
            else
            {
                p.pred->m->unlock();
                p.curr->m->unlock();
                cerr << "FAILED : Picture not found.\n";
                return;
            }
            ndone = false;
        }
        p.pred->m->unlock();
        p.curr->m->unlock();
    }
}
static void sequential(Picture ptemp, int w, int h, Picture *pic, Colour (*func)(int, int, Picture *),
                       bool crosspixel, bool shape)
{
    if (!crosspixel)
    {
        for (int i = 0; i < w; i++)
        {
            for (int j = 0; j < h; j++)
            {
                pic->setpixel(i, j, func(i, j, pic));
            }
        }
    }
    else
    {
        if (shape)
        {
            for (int i = 0; i < w; i++)
            {
                for (int j = 0; j < h; j++)
                {
                    ptemp.setpixel(j, i, func(i, j, pic));
                }
            }
        }
        else
        {
            for (int i = 0; i < w; i++)
            {
                for (int j = 0; j < h; j++)
                {
                    ptemp.setpixel(i, j, func(i, j, pic));
                }
            }
        }
    }
}

static void row_thread_func(Picture ptemp, int i, int w, Picture *pic, Colour (*func)(int, int, Picture *),
                            bool crosspixel, bool shape)
{
    if (!crosspixel)
    {
        for (int j = 0; j < w; j++)
        {
            pic->setpixel(j, i, func(j, i, pic));
        }
    }
    else
    {
        if (shape)
        {
            for (int j = 0; j < w; j++)
            {
                ptemp.setpixel(i, j, func(j, i, pic));
            }
        }
        else
        {
            for (int j = 0; j < w; j++)
            {
                ptemp.setpixel(j, i, func(j, i, pic));
            }
        }
    }
}

static void column_thread_func(Picture ptemp, int i, int h, Picture *pic, Colour (*func)(int, int, Picture *),
                               bool crosspixel, bool shape)
{
    if (!crosspixel)
    {
        for (int j = 0; j < h; j++)
        {
            pic->setpixel(i, j, func(i, j, pic));
        }
    }
    else
    {
        if (shape)
        {
            for (int j = 0; j < h; j++)
            {
                ptemp.setpixel(j, i, func(i, j, pic));
            }
        }
        else
        {
            for (int j = 0; j < h; j++)
            {
                ptemp.setpixel(i, j, func(i, j, pic));
            }
        }
    }
}

static void sector_thread_func(Picture ptemp, int left, int right, int up, int down, Picture *pic, Colour (*func)(int, int, Picture *),
                               bool crosspixel, bool shape)
{
    if (!crosspixel)
    {
        for (int i = left; i < right; i++)
        {
            for (int j = down; j < up; j++)
            {
                pic->setpixel(i, j, func(i, j, pic));
            }
        }
    }
    else
    {
        if (shape)
        {
            for (int i = left; i < right; i++)
            {
                for (int j = down; j < up; j++)
                {
                    ptemp.setpixel(j, i, func(i, j, pic));
                }
            }
        }
        else
        {
            for (int i = left; i < right; i++)
            {
                for (int j = down; j < up; j++)
                {
                    ptemp.setpixel(i, j, func(i, j, pic));
                }
            }
        }
    }
}

void PicLibrary::invert(string filename)
{
    general(SEQUENTIAL, filename, &invert_single, false, false);
}

Colour grayscale_single(int x, int y, Picture *pic)
{
    Colour thisc = pic->getpixel(x, y);
    int avg = (thisc.getred() + thisc.getblue() + thisc.getgreen()) / 3;
    thisc.setred(avg);
    thisc.setblue(avg);
    thisc.setgreen(avg);
    return thisc;
}

void PicLibrary::grayscale(string filename)
{
    general(SEQUENTIAL, filename, &grayscale_single, false, false);
}

Colour rotate90_single(int x, int y, Picture *pic)
{
    return pic->getpixel(x, pic->getheight() - y - 1);
}

Colour rotate180_single(int x, int y, Picture *pic)
{
    return pic->getpixel(pic->getwidth() - x - 1, pic->getheight() - y - 1);
}

Colour rotate270_single(int x, int y, Picture *pic)
{
    return pic->getpixel(pic->getwidth() - x - 1, y);
}

void PicLibrary::rotate(int angle, string filename)
{
    if (angle == 90)
    {
        general(SEQUENTIAL, filename, &rotate90_single, true, true);
    }
    else if (angle == 180)
    {
        general(SEQUENTIAL, filename, &rotate180_single, false, true);
    }
    else
    {
        general(SEQUENTIAL, filename, &rotate270_single, true, true);
    }
}

Colour FlipV_single(int x, int y, Picture *pic)
{
    return pic->getpixel(x, pic->getheight() - y - 1);
}
Colour FlipH_single(int x, int y, Picture *pic)
{
    return pic->getpixel(pic->getwidth() - x - 1, y);
}
void PicLibrary::flipVH(char plane, string filename)
{
    if (plane == 'H')
    {
        general(SEQUENTIAL, filename, &FlipH_single, false, true);
    }
    else if (plane == 'V')
    {
        general(SEQUENTIAL, filename, &FlipV_single, false, true);
    }
    else
    {
        cerr << "False input\n";
    }
}

Colour blur_single(int x, int y, Picture *pic)
{
    Colour thisc = pic->getpixel(x, y);
    if (x != 0 && y != 0 && y < pic->getheight() - 1 && x < pic->getwidth() - 1)
    {
        int accr = 0;
        int accg = 0;
        int accb = 0;
        for (int i = -1; i < 2; i++)
        {
            for (int j = -1; j < 2; j++)
            {
                Colour c = pic->getpixel(x + i, y + j);
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

void PicLibrary::blur(string filename)
{
    /*Change the enum concurrency_type here to swich between different concurrency ways to blur.
      Also works for other transformation functions.*/
    general(SECTOR4, filename, &blur_single, false, true);
}
