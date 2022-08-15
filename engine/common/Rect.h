#pragma once
#include <stdio.h>

struct Rect {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;

    glm::vec2 pos() const { return {x, y}; }
    glm::vec2 size() const { return {w, h}; }
    float ratio() const { return w / h; }

    void zoom(float z, glm::vec2 const & center = {.5f, .5f}) {
        float addw = w * z - w;
        float addh = h * z - h;
        x -= addw * center.x;
        y -= addh * center.y;
        w += addw;
        h += addh;
    }

    void move(glm::vec2 const & moveBy) {
        x += moveBy.x;
        y += moveBy.y;
    }

    Rect zoomed(float z, glm::vec2 const & center = {.5f, .5f}) const {
        Rect r = *this;
        r.zoom(z, center);
        return r;
    }

    Rect moved(glm::vec2 const & moveBy) const {
        Rect r = *this;
        r.move(moveBy);
        return r;
    }

    // return rect that fits inside, centered
    Rect aspectFit(float innerRatio) const {
        Rect inner;

        // inner is wider, touching sides
        // +--------------------------+
        // |          outer           |
        // |+------------------------+|
        // ||                        ||
        // ||         inner          ||
        // ||                        ||
        // |+------------------------+|
        // |                          |
        // +--------------------------+
        if (innerRatio > ratio()) {
            inner.w = w;
            inner.h = w / innerRatio;
            inner.x = x;
            inner.y = y + (h - inner.h) / 2.f;
        }

        // inner is taller, touching top/bottom
        // +---------+=======+--------+
        // |         |       |        |
        // |         |       |        |
        // |         |       |        |
        // |  outer  | inner |        |
        // |         |       |        |
        // |         |       |        |
        // |         |       |        |
        // +---------+=======+--------+
        else {
            inner.w = h * innerRatio;
            inner.h = h;
            inner.x = x + (w - inner.w) / 2.f;
            inner.y = y;
        }
        return inner;
    }
    Rect aspectFit(float w, float h) const { return aspectFit(w/h); }

    char const * toString(char * buf, size_t size) const {
        snprintf(buf, size, "rect((%.2f, %.2f), (%.2f, %.2f))", x, y, w, h);
        return buf;
    }

};

