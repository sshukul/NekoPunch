//
//  Timeline.cpp
//  Spriter2Moai
//
//  Created by Cartwright Samuel on 6/18/13.
//  Copyright (c) 2013 Samuel Cartwright. All rights reserved.
//

#include <iostream>
#include <boost/format.hpp>
#include <math.h>
#include "timeline.h"
#include "soundline.h"
#include "mainlineKey.h"
#include "object.h"
#include "bone.h"
#include "animation.h"
#include "objectRef.h"
#include "boneRef.h"
#include "file.h"
#include "transform.h"

using namespace std;
using namespace tinyxml2;

Timeline::~Timeline() {
    
}

void Timeline::loadXML(const tinyxml2::XMLElement* a_element) {
    const XMLAttribute* attb = a_element->FindAttribute("id");
    if(attb) {
        m_id = attb->IntValue();
    }
    
    attb = a_element->FindAttribute("name");
    if(attb) {
        m_name = attb->Value();
    }
    
    attb = a_element->FindAttribute("object_type");
    if(attb) {
        m_objectType = attb->Value();
    }
    
    const tinyxml2::XMLElement* child = a_element->FirstChildElement();
    while(child) {
        if(strcmp(child->Name(), "key") == 0) {
            if (m_objectType.compare("object") == 0 || m_objectType.compare("point") == 0) {
                Object* object = new Object();
                object->loadXML(child);
                addObject(object);
            }
            else if(m_objectType.compare("bone")==0) {
                Bone* bone = new Bone();
                bone->loadXML(child);
                addBone(bone);
            }
        }
        child = child->NextSiblingElement();
    }
}

float Timeline::calculateActualRotationAngle(float startAngle, float endAngle, int spin) {
    if(endAngle < startAngle) {
        if(spin == -1) {
            return endAngle - startAngle;
        } else {
            return (360 - startAngle) + endAngle;
        }
    } else if(endAngle == startAngle) {
        return 0.0;
    } else {
        if(spin == -1) {
            return endAngle - 360 - startAngle;
        } else {
            return endAngle - startAngle;
        }
    }
}

Transform Timeline::buildTransform(BoneRef* boneRef, int key, int time, int length, bool looping) const {
    Bone* bone = m_owner->getBoneByTime(boneRef->getTimeline(), time);
    if(boneRef->getKey() != key) {
        bone = m_owner->getBone(boneRef->getTimeline(), boneRef->getKey());
    }
    Transform boneTransform(0, 0, 0, 0, 0, 0);
    if(bone != NULL) {
        boneTransform = Transform(bone->getX(), bone->getY(), bone->getAngle(), bone->getScaleX(), bone->getScaleY(), bone->getSpin());
        Transform boneNextKeyTransform(bone->getX(), bone->getY(), bone->getAngle(), bone->getScaleX(), bone->getScaleY(), bone->getSpin());
        
        //Bone* boneNextKey = m_owner->getBone(boneRef->getTimeline(), key);
        Bone* boneNextKey = m_owner->getNextBoneByTime(boneRef->getTimeline(), time);
        if(time != bone->getTime() && boneNextKey != NULL && boneNextKey->getTime() != bone->getTime()) {
            float nextFrameTime = boneNextKey->getTime();
            if(!(looping == false && nextFrameTime == 0)) {
                if(nextFrameTime == 0) {
                    nextFrameTime = length;
                }
                float averagingFactor = ((float)time - (float)bone->getTime()) / (nextFrameTime - (float)bone->getTime());
                Transform nextKeyTransform(boneNextKey->getX(), boneNextKey->getY(), boneNextKey->getAngle(), boneNextKey->getScaleX(), boneNextKey->getScaleY(), boneNextKey->getSpin());
                boneTransform.lerp(nextKeyTransform, averagingFactor, bone->getSpin());
            }
        }
        

        
        if(boneNextKey != NULL && boneNextKey->getTime() != bone->getTime()) {
            int nextMainlineKeyTime = 0;
            for(vector<MainlineKey*>::const_iterator it = m_owner->m_mainlineKeys.begin(); it != m_owner->m_mainlineKeys.end(); it++) {
                if((*it)->getTime() == time) {
                    if(it+1 != m_owner->m_mainlineKeys.end()) {
                        it++;
                        nextMainlineKeyTime = (*it)->getTime();
                        break;
                    }
                }
            }
            
            int nextKeyTime = boneNextKey->getTime();
            if(nextMainlineKeyTime == 0) {
                nextMainlineKeyTime = length;
            }
            if(boneNextKey->getTime() == 0) {
                nextKeyTime = length;
            }
            
            // Find the bone angle at the next keyframe, based on lerping if needed. Use it
            // to calculate the rotation angle and spin for the bone.
            if(nextKeyTime != nextMainlineKeyTime) {
                bool skipLerp = false;
                if(boneNextKey->getTime() == 0 && looping == false) {
                    skipLerp = true;
                }
                // If there is no next bone keyframe and we are not looping, then there is no rotation for this
                // bone. Otherwise calculate linear interpolation to find the rotation angle for the
                // next keyframe.
                if(!skipLerp) {
                    float averagingFactor = ((float)nextMainlineKeyTime - (float) bone->getTime()) / ((float) nextKeyTime - (float) bone->getTime());
                    Transform nextKeyTransform(boneNextKey->getX(), boneNextKey->getY(), boneNextKey->getAngle(), boneNextKey->getScaleX(), boneNextKey->getScaleY(), boneNextKey->getSpin());
                    boneNextKeyTransform.lerp(nextKeyTransform, averagingFactor, bone->getSpin());
                    boneTransform.rotationAngle = Timeline::calculateActualRotationAngle(boneTransform.angle, boneNextKeyTransform.angle, bone->getSpin());
                } else {
                    boneTransform.rotationAngle = 0;
                    boneTransform.spin = 0;
                }
            } else {
                boneTransform.rotationAngle = Timeline::calculateActualRotationAngle(boneTransform.angle, boneNextKey->getAngle(), bone->getSpin());
            }
        }
    }
    
    if(boneRef->getParent() != -1) {
        BoneRef* parent = m_owner->getTimedBoneReference(boneRef->getParent(), time);
        if(parent == NULL) {
            parent = m_owner->getBoneReference(boneRef->getParent(), key);
        }
        if(parent != NULL) {
            Transform parentTransform = buildTransform(parent, key, time, length, looping);
            boneTransform.apply_parent_transform(parentTransform);
        }
    }
    
    return boneTransform;
}

// this is the trickest part in the program, because we need to output the objects in each timeline (each timeline
// describes one object), but we need to keep refering back to the mainline to see if the object is attached to any
// bones, and if so, calculate how this affects the final position of the object
// Note that we're only interested in exporting objects - not bones
std::ostream& operator<< (std::ostream& out, const Timeline& timeline) {
    
    if(!timeline.isTypeObject() && !timeline.isTypePoint())
        return out;
    
    out << "\t\t\t[" << timeline.m_owner->objectCounter << "] = {" << endl;
    int keyNum = 0;
    Object* firstResultObj = NULL;
    int firstZIndex = 0;
    Object* prevObj = NULL;
    bool loopbackFrameAlreadyWritten = false;
    bool objectHasNonMainlineFrame = false;
    bool objectHasSoundlineFrame = false;
    int prevObjTime = 0;
    int prevFrameTime = 0;
    vector<Object*>::const_iterator itObj = timeline.m_objects.begin();
    vector<Soundline*>::const_iterator itSoundlines = timeline.m_owner->m_soundlines.begin();
    vector<Object*>::const_iterator itSounds;
    
    // We iterate through the soundline if any and add it's points as keyframes too,
    // to allow attaching a listener on keyframes in Moai and playing the required sounds
    Soundline* soundline = NULL;
    if(itSoundlines != timeline.m_owner->m_soundlines.end()) {
        soundline = *itSoundlines;
        itSounds = soundline->m_objects.begin();
    }
    
    for(vector<MainlineKey*>::const_iterator itMain = timeline.m_owner->m_mainlineKeys.begin(); itMain != timeline.m_owner->m_mainlineKeys.end() || itObj != timeline.m_objects.end() || (soundline != NULL && itSounds != soundline->m_objects.end()); ) {
        unsigned int frameTime = 0;
        
        MainlineKey* mKey = *itMain;
        Object* object = *itObj;
        Object* sound = NULL;
        
        if(soundline != NULL && itSounds != soundline->m_objects.end()) {
            sound = *itSounds;
        }
        
        int mainlineKeyTime = 0;
        int objectTime = 0;
        int soundlineTime = 0;
        bool skipFrame = false;
        if(mKey == NULL || itMain == timeline.m_owner->m_mainlineKeys.end()) {
            mainlineKeyTime = timeline.m_owner->getLength();
            mKey = *(itMain - 1);
        } else {
            mainlineKeyTime = mKey->getTime();
        }
        
        if(object == NULL || itObj == timeline.m_objects.end()) {
            objectTime = timeline.m_owner->getLength();
        } else {
            objectTime = object->getTime();
        }
        
        if(sound == NULL || soundline == NULL || itSounds == soundline->m_objects.end()) {
            soundlineTime = timeline.m_owner->getLength();
        } else {
            soundlineTime = sound->getTime();
        }
        
        if(objectTime < mainlineKeyTime) {
            if(soundlineTime < objectTime) {
                frameTime = soundlineTime;
                objectHasSoundlineFrame = true;
            } else {
                frameTime = objectTime;
            }
            objectHasNonMainlineFrame = true;
        } else if(soundlineTime < mainlineKeyTime) {
            frameTime = soundlineTime;
            objectHasSoundlineFrame = true;
        } else {
            frameTime = mainlineKeyTime;
        }
        
        if(frameTime != soundlineTime) {
            sound = NULL;
        }

        int mainlineKeyId = mKey->getId();
        ObjectRef* objectRef = mKey->findReferenceToObject(timeline.m_id);
        
        if(objectRef == NULL && object != NULL && itObj != timeline.m_objects.end()) {
            objectRef = timeline.m_owner->findReferenceToObject(timeline.m_id, object->getId(), &mainlineKeyId);
        }
        if((object == NULL || itObj == timeline.m_objects.end()) && objectRef != NULL) {
            object = timeline.m_owner->getObject(objectRef->getTimeline(), objectRef->getKey());
        }

        if(objectTime < prevObjTime ) {
            skipFrame = true;
        }
        // For each object we have to check if it is attached to a bone. If it is, then
        // we need to recursively look up the bone's properties and calculate the values to add to the
        // object position.
    
        if(objectHasNonMainlineFrame && object != NULL) {
            mainlineKeyId = object->getId();
        }
    
        if(object != NULL && objectRef != NULL && !skipFrame) {
            // search the mainline for any references to this timeline and key pair
            int z = 0;
            
            Transform objectTransform(object->getX(), object->getY(), object->getAngle(), object->getScaleX(), object->getScaleY(), object->getSpin());
            Transform objectNextKeyTransform(object->getX(), object->getY(), object->getAngle(), object->getScaleX(), object->getScaleY(), object->getSpin());
            
            Object* objectNextKey = timeline.m_owner->getNextObjectByTime(objectRef->getTimeline(), frameTime);
            
            if(frameTime != object->getTime() && objectNextKey != NULL && objectNextKey->getTime() != object->getTime()) {
                float nextFrameTime = objectNextKey->getTime();
                if(!(timeline.m_owner->getLooping() == false && nextFrameTime == 0)) {
                    if(nextFrameTime == 0) {
                        nextFrameTime = timeline.m_owner->getLength();
                    }
                    float averagingFactor = ((float)frameTime - (float)object->getTime()) / (nextFrameTime - (float)object->getTime());
                    Transform nextKeyTransform(objectNextKey->getX(), objectNextKey->getY(), objectNextKey->getAngle(), objectNextKey->getScaleX(), objectNextKey->getScaleY(), objectNextKey->getSpin());
                    objectTransform.lerp(nextKeyTransform, averagingFactor, object->getSpin());
                }
            } else if((objectHasSoundlineFrame || frameTime < object->getTime()) && frameTime != object->getTime() && objectNextKey != NULL && objectNextKey->getTime() != frameTime) {
                float nextFrameTime = objectNextKey->getTime();
                if(!(timeline.m_owner->getLooping() == false && nextFrameTime == 0)) {
                    if(nextFrameTime == 0) {
                        nextFrameTime = timeline.m_owner->getLength();
                    }
                    float averagingFactor = ((float)frameTime - (float)prevFrameTime) / (nextFrameTime - (float)prevFrameTime);
                    objectTransform = *new Transform(prevObj->getX(), prevObj->getY(), prevObj->getAngle(), prevObj->getScaleX(), prevObj->getScaleY(), prevObj->getSpin());
                    Transform nextKeyTransform(objectNextKey->getX(), objectNextKey->getY(), objectNextKey->getAngle(), objectNextKey->getScaleX(), objectNextKey->getScaleY(), objectNextKey->getSpin());
                    objectTransform.lerp(nextKeyTransform, averagingFactor, prevObj->getSpin());
                }
            }
            
            if(objectNextKey != NULL && objectNextKey->getTime() != object->getTime()) {
                int nextMainlineKeyTime = 0;
                for(vector<MainlineKey*>::const_iterator it = timeline.m_owner->m_mainlineKeys.begin(); it != timeline.m_owner->m_mainlineKeys.end(); it++) {
                    if((*it)->getTime() == frameTime) {
                        if(it+1 != timeline.m_owner->m_mainlineKeys.end()) {
                            it++;
                            nextMainlineKeyTime = (*it)->getTime();
                            break;
                        }
                    }
                }
                
                int nextKeyTime = objectNextKey->getTime();
                if(nextMainlineKeyTime == 0) {
                    nextMainlineKeyTime = timeline.m_owner->getLength();
                }
                if(objectNextKey->getTime() == 0) {
                    nextKeyTime = timeline.m_owner->getLength();
                }
                
                // Find the object angle at the next keyframe, based on lerping if needed. Use it
                // to calculate the rotation angle and spin for the object.
                if(nextKeyTime != nextMainlineKeyTime) {
                    bool skipLerp = false;
                    if(objectNextKey->getTime() == 0 && timeline.m_owner->getLooping() == false) {
                        skipLerp = true;
                    }
                    // If there is no next object keyframe and we are not looping, then there is no
                    // rotation for this object. Otherwise calculate linear interpolation to find
                    // the rotation angle for the next keyframe.
                    if(!skipLerp) {
                        float averagingFactor = ((float)nextMainlineKeyTime - (float) object->getTime()) / ((float) nextKeyTime - (float) object->getTime());
                        Transform nextKeyTransform(objectNextKey->getX(), objectNextKey->getY(), objectNextKey->getAngle(), objectNextKey->getScaleX(), objectNextKey->getScaleY(), objectNextKey->getSpin());
                        objectNextKeyTransform.lerp(nextKeyTransform, averagingFactor, object->getSpin());
                        objectTransform.rotationAngle = Timeline::calculateActualRotationAngle(objectTransform.angle, objectNextKeyTransform.angle, object->getSpin());
                    } else {
                        objectTransform.rotationAngle = 0;
                        objectTransform.spin = 0;
                    }
                } else {
                    objectTransform.rotationAngle = Timeline::calculateActualRotationAngle(objectTransform.angle, objectNextKey->getAngle(), object->getSpin());
                }
            }
            
            BoneRef* boneRef = timeline.m_owner->getTimedBoneReference(objectRef, frameTime);
            if(boneRef == NULL) {
                boneRef = timeline.m_owner->getBoneReference(objectRef, mainlineKeyId);
            }
            if (boneRef != NULL) {
                Transform parentTransform = timeline.buildTransform(boneRef, mainlineKeyId, frameTime, timeline.m_owner->getLength(), timeline.m_owner->getLooping());
                objectTransform.apply_parent_transform(parentTransform);
            }
            
            if (objectRef != NULL) {
                z = objectRef->getZIndex();
            }
            
            Object* resultObj = new Object();
            resultObj->setAngle(fmod(objectTransform.angle, 360));
            resultObj->setFile(object->getFile());
            resultObj->setFolder(object->getFolder());
            resultObj->setScaleX(objectTransform.scale_x);
            resultObj->setScaleY(objectTransform.scale_y);
            resultObj->setX(objectTransform.x);
            resultObj->setY(objectTransform.y);
            resultObj->setSpin(objectTransform.spin);
            resultObj->setPivotX(object->getPivotX());
            resultObj->setPivotY(object->getPivotY());
            
            if(itMain == timeline.m_owner->m_mainlineKeys.begin()) {
                firstResultObj = resultObj;
                firstZIndex = objectRef->getZIndex();
            }
            
            bool hasNext = false;
            if(((*itObj) != NULL && itObj != timeline.m_objects.end()) || ((*itMain) != NULL && itMain != timeline.m_owner->m_mainlineKeys.end()) || (soundline != NULL && (*itSounds) != NULL && itSounds != soundline->m_objects.end())) {
                hasNext = true;
            }

            if(prevObj == NULL || !resultObj->equals(*prevObj) || objectHasSoundlineFrame) {
                Timeline::writeObject(frameTime, resultObj, timeline,  out, &keyNum, z, prevObj, hasNext);
                if(frameTime == timeline.m_owner->getLength()) {
                    loopbackFrameAlreadyWritten = true;
                }
            }            
            if(mainlineKeyTime < objectTime || soundlineTime < objectTime) {
                if(prevObj != NULL) {
                    resultObj->setPivotX(prevObj->getPivotX());
                    resultObj->setPivotY(prevObj->getPivotY());
                }
            }
            prevObj = resultObj;
            
        }
        
        if(objectHasNonMainlineFrame) {
            objectHasNonMainlineFrame = false;
        }
        if(objectHasSoundlineFrame) {
            objectHasSoundlineFrame = false;
        }
        
        if(objectTime < mainlineKeyTime && objectTime < soundlineTime) {
            itObj++;
        } else if(mainlineKeyTime < objectTime && mainlineKeyTime < soundlineTime) {
            itMain++;
        } else if(soundlineTime < objectTime && soundlineTime < mainlineKeyTime) {
            itSounds++;
        } else {
            if((*itObj) != NULL && itObj != timeline.m_objects.end()) {
                itObj++;
            }
            if((*itMain) != NULL && itMain != timeline.m_owner->m_mainlineKeys.end()) {
                itMain++;
            }
            if(soundline != NULL && (*itSounds) != NULL && itSounds != soundline->m_objects.end()) {
                itMain++;
            }
        }
        
        // This bit adds a "loopback" keyframe which is the same as the first frame,
        // if the last frame isn't specified as a keyframe and looping is enabled.
        // This makes it tween and loop smoothly like in the Spriter GUI rather than "jerk" back to the first frame after the final frame.
        if(!loopbackFrameAlreadyWritten && timeline.m_owner->getLooping() != false && itMain == timeline.m_owner->m_mainlineKeys.end() && itObj == timeline.m_objects.end() && (soundline == NULL || ((*itSounds) != NULL && itSounds == soundline->m_objects.end()))) {
            //if(prevObj == NULL || !firstResultObj->equals(*prevObj)) {
                Timeline::writeObject(timeline.m_owner->getLength(), firstResultObj, timeline, out, &keyNum, firstZIndex, prevObj, false);
            //}
        }
        
        prevObjTime = objectTime;
        prevFrameTime = frameTime;
    }
    out << "\t\t\t}";
    return out;
}

void Timeline::writeObject(int time, Object* resultObj, const Timeline& timeline, std::ostream& out, int* keyNum, int z, Object* prevObj, bool hasNext) {
    out << "\t\t\t\t[" << ++(*keyNum) << "] = {" << endl;
    
    out << "\t\t\t\t\t['angle'] = " << boost::format("%.4f") % resultObj->getAngle() << "," << endl;
    
    if(timeline.isTypePoint()) {
        out << "\t\t\t\t\t['type'] = 'point'," << endl;
    }
    out << "\t\t\t\t\t['name'] = '" << timeline.getName() << "'," << endl;
    out << "\t\t\t\t\t['texture'] = '" << timeline.m_owner->getFileName(resultObj->getFolder(), resultObj->getFile()) << "'," << endl;
    out << "\t\t\t\t\t['zindex'] = " << z << "," << endl;
    out << "\t\t\t\t\t['scale_x'] = " << boost::format("%.4f") % resultObj->getScaleX() << "," << endl;
    out << "\t\t\t\t\t['scale_y'] = " << boost::format("%.4f") % resultObj->getScaleY() << "," << endl;
    out << "\t\t\t\t\t['time'] = " << time << "," << endl;
    
    // Adjust pivot points to default if they are different.
    float pivot_x = resultObj->getPivotX();
    float pivot_y = resultObj->getPivotY();
    
    if((pivot_x == 0.0 && pivot_y == 0.0)) {
        if(prevObj != NULL && (prevObj->getPivotX() != 0.0 || prevObj->getPivotY() != 0.0)) {
            pivot_x = prevObj->getPivotX();
            pivot_y = prevObj->getPivotY();
        } else if (resultObj->getFolder() != -1 && resultObj->getFile() != -1) {
            pivot_x = timeline.m_owner->getFile(resultObj->getFolder(), resultObj->getFile())->getPivotX();
            pivot_y = timeline.m_owner->getFile(resultObj->getFolder(), resultObj->getFile())->getPivotY();
        }
    }
    
    if((pivot_x != 0.0 || pivot_y != 0.0)) {
        int height = timeline.m_owner->getFile(resultObj->getFolder(), resultObj->getFile())->getHeight();
        int width = timeline.m_owner->getFile(resultObj->getFolder(), resultObj->getFile())->getWidth();
        pivot_x = pivot_x * width;
        pivot_y = pivot_y * height;
    }
    
    out << "\t\t\t\t\t['x'] = " << boost::format("%.6f") % resultObj->getX() << "," << endl;
    out << "\t\t\t\t\t['y'] = " << boost::format("%.6f") % resultObj->getY() << "," << endl;
    out << "\t\t\t\t\t['spin'] = " << resultObj->getSpin() << "," << endl;
    
    out << "\t\t\t\t\t['pivot_x'] = " << pivot_x << "," << endl;
    out << "\t\t\t\t\t['pivot_y'] = " << pivot_y << endl;
    
    out << "\t\t\t\t}";
    
    if(hasNext) {
        out << ", ";
    }
    out << endl;
}

void Timeline::addObject(Object* a_object) {
    if(!isTypeObject() && !isTypePoint())
        return;
    m_objects.push_back(a_object);
}

void Timeline::addBone(Bone* a_bone) {
    if(isTypeObject() || isTypePoint())
        return;
    m_bones.push_back(a_bone);
}

Object* Timeline::getObject(int a_index) {
    if(a_index >= m_objects.size())
        return NULL;
    if(!isTypeObject() && !isTypePoint())
        return NULL;
    return m_objects[a_index];
}

Bone* Timeline::getBone(int a_index) {
    if(a_index >= m_bones.size())
        return NULL;
    if(isTypeObject() || isTypePoint())
        return NULL;
    return m_bones[a_index];
}

Bone* Timeline::getBoneByTime(int time) {
    if(isTypeObject() || isTypePoint())
        return NULL;
    for(vector<Bone*>::const_iterator it = m_bones.begin(); it != m_bones.end(); it++) {
        if((*it)->getTime() == time) {
            return (*it);
        } else if((*it)->getTime() > time) {
            it--;
            return (*it);
        } else if(it+1 == m_bones.end()) {
            return (*it);
        }
    }
    return NULL;
}

Bone* Timeline::getNextBoneByTime(int time) {
    if(isTypeObject() || isTypePoint())
        return NULL;
    for(vector<Bone*>::const_iterator it = m_bones.begin(); it != m_bones.end(); it++) {
        if((*it)->getTime() == time) {
            it++;
            if(it != m_bones.end()) {
                return (*it);
            } else {
                it = m_bones.begin();
                return (*it);
            }
        } else if((*it)->getTime() > time) {
            return (*it);
        } else if(it+1 == m_bones.end()) {
            it = m_bones.begin();
            return (*it);
        }
    }
    return NULL;
}

Object* Timeline::getObjectByTime(int time) {
    if(!isTypeObject() && !isTypePoint())
        return NULL;
    for(vector<Object*>::const_iterator it = m_objects.begin(); it != m_objects.end(); it++) {
        if((*it)->getTime() == time) {
            return (*it);
        } else if((*it)->getTime() > time) {
            it--;
            return (*it);
        } else if(it+1 == m_objects.end()) {
            return (*it);
        }
    }
    return NULL;
}

Object* Timeline::getNextObjectByTime(int time) {
    if(!isTypeObject() && !isTypePoint())
        return NULL;
    for(vector<Object*>::const_iterator it = m_objects.begin(); it != m_objects.end(); it++) {
        if((*it)->getTime() == time) {
            it++;
            if(it != m_objects.end()) {
                return (*it);
            } else {
                it = m_objects.begin();
                return (*it);
            }
        } else if((*it)->getTime() > time) {
            return (*it);
        } else if(it+1 == m_objects.end()) {
            it = m_objects.begin();
            return (*it);
        }
    }
    return NULL;
}


