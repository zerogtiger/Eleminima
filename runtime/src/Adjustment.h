#ifndef ADJUSTMENT_H
#define ADJUSTMENT_H

#include "Enums.h"

struct Adjustment {
    double brightness = 0, contrast = 0, hue = 0, saturation = 0, value = 0, lift = 0, gamma = 1, gain = 1;

    Adjustment(){};

    Adjustment(double brightness, double contrast, double hue, double saturation, double value, double lift,
               double gamma, double gain)
        : brightness(brightness), contrast(contrast), hue(hue), saturation(saturation), value(value), lift(lift),
          gamma(gamma), gain(gain) {}

    ~Adjustment() {}

    static Adjustment& create_adj_hsv(double hue, double saturation, double value) {
        Adjustment* ret = new Adjustment(0, 0, hue, saturation, value, 0, 1, 1);
        return *ret;
    }

    static Adjustment& create_adj_bc(double brightness, double contrast) {
        Adjustment* ret = new Adjustment(brightness, contrast, 0, 0, 0, 0, 1, 1);
        return *ret;
    }

    static Adjustment& create_adj_lgg(double lift, double gamma, double gain) {
        Adjustment* ret = new Adjustment(0, 0, 0, 0, 0, lift, gamma, gain);
        return *ret;
    }
};

#endif
