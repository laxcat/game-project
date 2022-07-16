// return 0-1. a must be positive. bigger a = slower approach
// paste this: \frac{\left(a\ -\ \left(x+\frac{1}{a}\right)^{-1}\right)}{a}
// here: https://www.desmos.com/calculator
inline float approach(float x, float const a = 1.0f) {
    return (a - powf(x + 1.f/a, -1.f)) / a;
}

inline float clamp(float min, float max, float v)
{
    const float t = v < min ? min : v;
    return t > max ? max : t;
}

inline float smoothstep(float edge0, float edge1, float v)
{
    const float t = clamp(0.0, 1.0, (v - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

inline float sineInOut(float p) {
    return -.5f * (cosf(M_PI * p) - 1.f) + 0.f;
};

inline float expoIn(float p) {
    return (p == 0.f) ? 0.f : powf(2.f, 10.f * (p - 1.f));
};


inline float expoOut(float p) {
    return (p == 1.f) ? 1.f : 1.f - powf(2.f, -10.f * p);
};


inline float expoInOut(float p) {
    if (p == 0.f) return 0.f;
    if (p == 1.f) return 1.f;
    if (p  < .5f) return .5f * powf(2.f, 10.f * (p * 2.f - 1.f));
    return .5f * (-pow(2.f, -10.f * (p/.5f - 1.f)) + 2.f);
};

inline float expoInside(float p) {
    if (p == 0.f) return 0.f;
    if (p == 1.f) return 1.f;
    if (p  < .5f) return expoOut(p * 2.f) * .5f;
    return expoIn((p - .5f) * 2.f) * .5f + .5f;
};

inline float quadIn(float p) {
    return p*p;
    
};


inline float quadOut(float p) {
    return -p * (p - 2.f);
};


inline float quadInOut(float p) {
    if (p < .5f) return p * p * 2.f;
    return 4.f * p - 2.f * p * p - 1.f;
};


inline float quartIn(float p) {
    return p * p * p * p;
};


inline float quartOut(float p) {
    p -= 1.f;
    return 1.f - p * p * p * p;
};


inline float quartInOut(float p) {
    p *= 2.f;
    if (p < 1.f) return .5f * p * p * p * p;
    p -= 2.f;
    return 1.f - .5f * p * p * p * p;
};


inline float quintIn(float p) {
    return p * p * p * p * p;
};


inline float quintOut(float p) {
    p -= 1.f;
    return p * p * p * p * p + 1.f;
};

inline float quintOut2(float p) {
    return 1 - powf(1 - p, 5.f);
}

inline float quintOutInv(float p) {
    return 1 - powf(1 - p, 1/5.f);;
}


inline float quintInOut(float p) {
    p *= 2.f;
    if (p < 1.f) return .5f * p * p * p * p * p;
    p -= 2.f;
    return .5 * p * p * p * p * p + 1.f;
};
