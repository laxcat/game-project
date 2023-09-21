// SHADER UTILS

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

// hmm not sure about this attenuation function... could at least have better cutoff parameter
// https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
float calcAttenuation(float lightRadius, vec3 lightPos, vec3 fragPos, vec3 fragNormal, float cutoff) {
    // calculate normalized light vector and distance to sphere light surface
    vec3 L = lightPos - fragPos;
    float distance = length(L);
    float d = max(distance - lightRadius, 0.0);
    L /= distance;
     
    // calculate basic attenuation
    float denom = d / lightRadius + 1.0;
    float attenuation = 1.0 / (denom*denom);
     
    // scale and bias attenuation such that:
    //   attenuation == 0 at extent of max influence
    //   attenuation == 1 when d == 0
    attenuation = (attenuation - cutoff) / (1.0 - cutoff);
    attenuation = max(attenuation, 0.0);
     
    float dot = max(dot(L, fragNormal), 0.0);
    return dot * attenuation;
}
