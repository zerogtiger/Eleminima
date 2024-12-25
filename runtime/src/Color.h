#ifndef COLOR_H
#define COLOR_H

#include "Adjustment.h"
#include "Enums.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

struct Color {
    double r, g, b, a = 255;
    Color() : r(0), g(0), b(0) {}
    Color(double r, double g, double b) : r(r), g(g), b(b) {}
    Color(double r, double g, double b, double a) : r(r), g(g), b(b), a(a) {}
    Color(Color* color) : r(color->r), g(color->g), b(color->b), a(color->a) {}
    ~Color() {}

    void hsv_to_rgb(double h_deg, double s, double v) {
        if (abs(s) > 1 || abs(v) > 1) {
            throw std::invalid_argument("saturation or value not in the range [0, 1]");
        }
        const int mod = 360;
        h_deg = fmod(fmod(h_deg, mod) + mod, mod);

        // hsl -> rgb
        double c = s * v;
        double x = c * (1 - abs(fmod((h_deg / 60), 2) - 1));
        double m = v - c;

        double rt, gt, bt;
        rt = gt = bt = 0;
        if (h_deg < 60) {
            rt = c;
            gt = x;
        }
        else if (h_deg < 120) {
            rt = x;
            gt = c;
        }
        else if (h_deg < 180) {
            gt = c;
            bt = x;
        }
        else if (h_deg < 240) {
            gt = x;
            bt = c;
        }
        else if (h_deg < 300) {
            bt = c;
            rt = x;
        }
        else {
            bt = x;
            rt = c;
        }

        r = std::clamp(round(255 * (rt + m)), 0.0, 255.0);
        g = std::clamp(round(255 * (gt + m)), 0.0, 255.0);
        b = std::clamp(round(255 * (bt + m)), 0.0, 255.0);
    }
    Color& to_hsv() {
        r = r / 255.0;
        g = g / 255.0;
        b = b / 255.0;
        double c_max = fmax(r, fmax(g, b)), c_min = fmin(r, fmin(g, b)), delta = c_max - c_min;
        if (delta == 0) {
            r = 0;
        }
        else if (c_max == r) {
            r = 60.0 * fmod(fmod(((g - b) / delta), 6) + 6, 6);
        }
        else if (c_max == g) {
            r = 60.0 * (fmod(fmod((b - r) / delta, 6) + 6, 6) + 2);
        }
        else if (c_max == b) {
            r = 60.0 * (fmod(fmod((r - g) / delta, 6) + 6, 6) + 4);
        }

        if (c_max == 0) {
            g = 0;
        }
        else {
            g = delta / c_max;
        }
        b = c_max;
        return *this;
    }
    Color& rgb_to_hsv(double rr, double gg, double bb) {
        rr = rr / 255.0;
        gg = gg / 255.0;
        bb = bb / 255.0;
        double c_max = fmax(rr, fmax(gg, bb)), c_min = fmin(rr, fmin(gg, bb)), delta = c_max - c_min;
        Color* ret = new Color(0, 0, 0);
        if (delta == 0) {
            ret->r = 0;
        }
        else if (c_max == rr) {
            ret->r = 60.0 * fmod(fmod(((gg - bb) / delta), 6) + 6, 6);
        }
        else if (c_max == gg) {
            ret->r = 60.0 * (fmod(fmod((bb - rr) / delta, 6) + 6, 6) + 2);
        }
        else if (c_max == bb) {
            ret->r = 60.0 * (fmod(fmod((rr - gg) / delta, 6) + 6, 6) + 4);
        }

        if (c_max == 0) {
            ret->g = 0;
        }
        else {
            ret->g = delta / c_max;
        }
        ret->b = c_max;
        return *ret;
    }

    double get(int col) {
        switch (col) {
        case 0:
            return r;
            break;
        case 1:
            return g;
            break;
        case 2:
            return b;
            break;
        case 3:
            return a;
            break;
        default:
            return -1;
            break;
        }
    }
    void set(double rr, double gg = 0, double bb = 0, double aa = 255) {
        r = rr;
        g = gg;
        b = bb;
        a = aa;
    }
    void set(Color color) {
        r = color.r;
        g = color.g;
        b = color.b;
        a = color.a;
    }

    bool set(int col, double val) {
        switch (col) {
        case 0:
            r = val;
            return true;
            break;
        case 1:
            g = val;
            return true;
            break;
        case 2:
            b = val;
            return true;
            break;
        case 3:
            a = val;
            return true;
            break;
        default:
            return false;
            break;
        }
    }

    bool operator<(const Color& other) const {
        return r == other.r ? (g == other.g ? b < other.b : g < other.g) : r < other.r;
    }

    Color operator+(const Color& other) const {
        return Color(r + other.r, g + other.g, b + other.b, a + other.a);
    }

    Color operator*(const Color& other) const {
        return Color(r * other.r, g * other.g, b * other.b, a * other.a);
    }

    Color operator*(double mult) const {
        return Color(r * mult, g * mult, b * mult, a * mult);
    }

    Color operator/(const Color& other) const {
        return Color(r / other.r, g / other.g, b / other.b, a / other.a);
    }

    Color operator/(double div) const {
        return Color(r / div, g / div, b / div, a / div);
    }

    static double luminance(double r, double g, double b) {
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }

    double luminance() {
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }

    static double luminance(const Color& c) {
        return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
    }

    Color& apply_adj_rgb(Adjustment adj, double factor) {
        // double brightness = 0, contrast = 0, hue = 0, saturation = 0, value = 0, lift = 1, gamma = 1, gain = 1;

        double F = 259.0 * (adj.contrast * factor + 255) / (255.0 * (259 - adj.contrast * factor));
        r = std::clamp(F * (r - 128) + 128 + adj.brightness * factor, 0.0, 255.0);
        g = std::clamp(F * (g - 128) + 128 + adj.brightness * factor, 0.0, 255.0);
        b = std::clamp(F * (b - 128) + 128 + adj.brightness * factor, 0.0, 255.0);

        Color c = rgb_to_hsv(r, g, b);
        r = c.r;
        g = c.g;
        b = c.b;
        hsv_to_rgb(r + adj.hue, std::clamp(g + adj.saturation * factor, 0.0, 1.0),
                   std::clamp(b + adj.value * factor, 0.0, 1.0));

        r = std::clamp(pow((adj.gain * (factor) + (1 - factor)) * (r / 255.0 + adj.lift * factor * (1 - r / 255.0)),
                           1.0 / (adj.gamma * (factor) + (1 - factor))) *
                           255.0,
                       0.0, 255.0);
        g = std::clamp(pow((adj.gain * (factor) + (1 - factor)) * (g / 255.0 + adj.lift * factor * (1 - g / 255.0)),
                           1.0 / (adj.gamma * (factor) + (1 - factor))) *
                           255.0,
                       0.0, 255.0);
        b = std::clamp(pow((adj.gain * (factor) + (1 - factor)) * (b / 255.0 + adj.lift * factor * (1 - b / 255.0)),
                           1.0 / (adj.gamma * (factor) + (1 - factor))) *
                           255.0,
                       0.0, 255.0);

        return *this;
    }
};

#endif
