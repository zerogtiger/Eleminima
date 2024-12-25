#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include "Image.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
class Interpolation {
  private:
    static double eval_cubic_bezier(double t, double p0, double p1, double p2, double p3) {
        return pow(1 - t, 3) * p0 + 3 * pow(1 - t, 2) * t * p1 + 3 * (1 - t) * pow(t, 2) * p2 + pow(t, 3) * p3;
    }

  public:
    static Color bilinear(Image& image, double r, double c, bool interp_edge_with_fill, Color fill = Color(0, 0, 0)) {
        Color ret(0, 0, 0);
        if (round(r) < 0 || round(r) > image.h || round(c) < 0 || round(c) > image.w) {
            ret.r = -1;
            ret.g = -1;
            ret.b = -1;
            ret.a = -1;
            return ret;
        }

        if (!interp_edge_with_fill) {
            r = std::clamp(r, 0.0, image.h - 1.0);
            c = std::clamp(c, 0.0, image.w - 1.0);
        }
        int fr = floor(r), cr = ceil(r), fc = floor(c), cc = ceil(c);
        if (r == fr && c == fc) {
            ret.set(image.get_color_or_default(fr, fc, fill));
        }
        else if (r == fr) {
            ret.set(image.get_color_or_default(r, fc, fill) * (cc - c) +
                    image.get_color_or_default(r, c, fill) * (c - fc));
        }
        else if (c == fc) {
            ret.set(image.get_color_or_default(fr, c, fill) * (cr - r) +
                    image.get_color_or_default(cr, c, fill) * (r - fr));
        }
        else {
            ret.set(image.get_color_or_default(fr, fc, fill) * (cc - c) * (cr - r) +
                    image.get_color_or_default(cr, fc, fill) * (cc - c) * (r - fr) +
                    image.get_color_or_default(fr, cc, fill) * (c - fc) * (cr - r) +
                    image.get_color_or_default(cr, cc, fill) * (c - fc) * (r - fr));
        }
        return ret;
    }

    static std::vector<double> constant(std::vector<std::pair<double, double>> points,
                                        std::vector<double> interp_points) {
        std::vector<double> ret;
        int ceil_idx;
        for (double point : interp_points) {
            ceil_idx = std::upper_bound(points.begin(), points.end(), std::make_pair(point, -1.0)) - points.begin();
            if (ceil_idx == 0) {
                ret.push_back(points[0].second);
            }
            else {
                ret.push_back(points[ceil_idx - 1].second);
            }
        }
        return ret;
    }
    // Notes: interp_points must have values in [0, 1]
    static std::vector<double> linear(std::vector<std::pair<double, double>> points,
                                      std::vector<double> interp_points) {
        std::vector<double> ret;
        int ceil_idx;
        for (double point : interp_points) {
            ceil_idx = std::upper_bound(points.begin(), points.end(), std::make_pair(point, -1.0)) - points.begin();
            if (ceil_idx == 0) {
                ret.push_back(points[0].second);
            }
            else if (ceil_idx == points.size()) {
                ret.push_back(points[ceil_idx - 1].second);
            }
            else {
                ret.push_back((point - points[ceil_idx - 1].first) * points[ceil_idx].second /
                                  (points[ceil_idx].first - points[ceil_idx - 1].first) +
                              (points[ceil_idx].first - point) * points[ceil_idx - 1].second /
                                  (points[ceil_idx].first - points[ceil_idx - 1].first));
            }
        }
        return ret;
    }

    // Notes: assuming control_points is sorted in the order it wish to be interpolated
    static std::vector<double> single_cubic_bezier(std::vector<std::pair<double, double>> control_points,
                                                   std::vector<double> interp_points, double error_bound = 0.0001) {
        std::vector<double> ret;
        double l = 0, r = 1, mid, rslt;
        for (double x : interp_points) {
            l = 0;
            r = 1;
            while (abs(l - r) >= error_bound) {
                mid = (l + r) / 2.0;
                rslt = eval_cubic_bezier(mid, control_points[0].first, control_points[1].first, control_points[2].first,
                                         control_points[3].first);
                if (abs(x - rslt) < error_bound) {
                    break;
                }
                else if (rslt < x) {
                    l = mid;
                }
                else {
                    r = mid;
                }
            }
            mid = (l + r) / 2.0;
            ret.push_back(eval_cubic_bezier(mid, control_points[0].second, control_points[1].second,
                                            control_points[2].second, control_points[3].second));
        }
        return ret;
    }
    // Notes: no error handling for handles%3 != 1 cases
    static std::vector<double> cubic_bezier(std::vector<std::pair<double, double>> handles,
                                            std::vector<double> interp_points, double error_bound = 0.0001) {
        if (handles.size() % 3 != 1) {
            throw std::invalid_argument("Handles not able to be interpret as a bezier spline (count != 1 (mod 3))\n");
        }
        std::vector<double> ret;
        std::vector<double> tmp;

        uint32_t interp_lft = 0, interp_rht = 0;
        for (int i = 3; i < handles.size(); i += 3) {
            while (interp_rht < interp_points.size() && interp_points[interp_rht] <= handles[i].first) {
                interp_rht++;
            }
            if (interp_rht == interp_lft) {
                continue;
            }
            else {
                tmp.clear();
                for (; interp_lft < interp_rht; interp_lft++) {
                    tmp.push_back(interp_points[interp_lft]);
                }
                tmp = single_cubic_bezier(
                    std::vector<std::pair<double, double>>{handles[i - 3], handles[i - 2], handles[i - 1], handles[i]},
                    tmp, error_bound);
                for (double a : tmp) {
                    ret.push_back(a);
                }
            }
        }
        return ret;
    }
    static std::vector<double> b_spline(std::vector<std::pair<double, double>> control_points,
                                        std::vector<double> interp_points, double error_bound = 0.0001) {
        std::vector<double> ret;
        std::vector<std::pair<double, double>> thirds;
        std::vector<std::vector<std::pair<double, double>>> bezier;
        std::vector<std::pair<double, double>> interval;
        std::vector<double> tmp;

        std::pair<double, double> l_third, r_third;
        for (int i = 0; i < control_points.size() - 1; i++) {
            l_third.first = control_points[i].first + (control_points[i + 1].first - control_points[i].first) / 3.0;
            l_third.second = control_points[i].second + (control_points[i + 1].second - control_points[i].second) / 3.0;
            r_third.first =
                control_points[i].first + (control_points[i + 1].first - control_points[i].first) * 2.0 / 3.0;
            r_third.second =
                control_points[i].second + (control_points[i + 1].second - control_points[i].second) * 2.0 / 3.0;
            thirds.push_back(l_third);
            thirds.push_back(r_third);
        }

        interval.push_back(control_points[0]);
        for (int i = 1; i < thirds.size() - 1; i += 2) {
            interval.push_back(
                {(thirds[i].first + thirds[i + 1].first) / 2.0, (thirds[i].second + thirds[i + 1].second) / 2.0});
        }
        interval.push_back(control_points[control_points.size() - 1]);

        for (int i = 0; i < interval.size() - 1; i++) {
            std::vector<std::pair<double, double>> tmp;
            tmp.push_back(interval[i]);
            tmp.push_back(thirds[2 * i]);
            tmp.push_back(thirds[2 * i + 1]);
            tmp.push_back(interval[i + 1]);
            bezier.push_back(tmp);
        }

        uint32_t interp_lft = 0, interp_rht = 0;
        for (uint32_t curr = 1; curr < interval.size(); curr++) {
            tmp.clear();
            while (interp_rht < interp_points.size() && interp_points[interp_rht] <= interval[curr].first) {
                interp_rht++;
            }
            if (interp_lft == interp_rht) {
                continue;
            }
            else {
                for (; interp_lft < interp_rht; interp_lft++) {
                    tmp.push_back(interp_points[interp_lft]);
                }
                tmp = single_cubic_bezier(bezier[curr - 1], tmp, error_bound);
                for (double a : tmp) {
                    ret.push_back(a);
                }
            }
        }
        return ret;
    }

    static double dot_product_2d(double lx1, double ly1, double lx2, double ly2) {
        return lx1 * lx2 + ly1 * ly2;
    }
};

#endif
