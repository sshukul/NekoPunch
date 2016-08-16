//
//  transform.h
//  Spriter2Moai
//
//  Created by Cartwright Samuel on 6/22/13.
//  Copyright (c) 2013 Samuel Cartwright. All rights reserved.
//

#ifndef __Spriter2Moai__transform__
#define __Spriter2Moai__transform__

#include <string>

using namespace std;

class Transform
{
public:
    
    float x, y;
    float angle;
    float scale_x, scale_y;
    float alpha;
    int spin;
    float rotationAngle = 0;
    
    // These are attributes for custom animation curves like cubic, quadratic etc.
    string curve_type = "";
    float c1 = -1.0;
    float c2 = -1.0;
    float c3 = -1.0;
    float c4 = -1.0;
    
    Transform();
    Transform(float x, float y, float angle, float scale_x, float scale_y, int spin, float alpha,
              string curve_type, float c1, float c2, float c3, float c4);
    
    bool operator==(const Transform& t) const;
    bool operator!=(const Transform& t) const;
    
    void lerp(const Transform& transform, float t, int spin);
    void apply_parent_transform(const Transform& parent);
    
    
    inline float lerp(float a, float b, float t) {
        return a + (b-a)*t;
    }
};

#endif /* defined(__Spriter2Moai__transform__) */
