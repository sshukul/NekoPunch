//
//  Key.h
//  Spriter2Moai
//
//  Created by Cartwright Samuel on 6/18/13.
//  Copyright (c) 2013 Samuel Cartwright. All rights reserved.
//

#ifndef __Spriter2Moai__Key__
#define __Spriter2Moai__Key__

#include <vector>
#include "tinyxml2.h"
#include "object.h"

using namespace std;

class BoneRef;
class ObjectRef;
class Bone;
class Animation;

class MainlineKey {
private:
    Animation* m_owner;
    int m_id;
    unsigned int m_time;
    int m_spin;
    
    vector<BoneRef*> m_boneRefs;
    vector<ObjectRef*> m_objectRefs;
    
public:
    MainlineKey(): m_owner(NULL), m_id(-1), m_time(0), m_spin(1) {}
    MainlineKey(Animation* a_owner): m_owner(a_owner), m_id(-1), m_time(0), m_spin(0) {}
    ~MainlineKey();
    
    inline Animation* getOwner() const {return m_owner;}
    inline void setOwner(Animation* a_owner) {m_owner = a_owner;}
    inline int getId() const {return m_id;}
    inline void setId(int a_id) {m_id = a_id;}
    inline unsigned int getTime() const {return m_time;}
    inline void setTime(unsigned int a_time) {m_time = a_time;}
    inline int getSpin() const {return m_spin;}
    inline void setSpin(int a_spin) {m_spin = a_spin;}
    
    void loadXML(const tinyxml2::XMLElement* a_element);
    
//    void writeMoai(ostream& out) const;
    
    void addBoneRef(BoneRef* a_boneRef);
    void addObjectRef(ObjectRef* a_objectRef);
    
    BoneRef* getBoneReference(ObjectRef* a_objectRef);
    BoneRef* getBoneReference(unsigned int a_id);
    
    ObjectRef* findReferenceToObject(unsigned int a_timelineIndex, unsigned int a_keyIndex);
    ObjectRef* findReferenceToObject(unsigned int a_timelineIndex);
    
    MainlineKey* clone();
};

#endif /* defined(__Spriter2Moai__Key__) */
