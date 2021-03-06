//
//  SpriterData.cpp
//  Spriter2Moai
//
//  Created by Cartwright Samuel on 6/17/13.
//  Copyright (c) 2013 Samuel Cartwright. All rights reserved.
//

#include <iostream>
#include "spriterData.h"
#include "tinyxml2.h"
#include "folder.h"
#include "entity.h"
#include "animation.h"
#include "taglineKey.h"
#include "tagList.h"
#include "tag.h"
#include "file.h"

using namespace std;
using namespace tinyxml2;

SpriterData::~SpriterData() {
    for (vector<Folder*>::iterator it = m_folders.begin(); it != m_folders.end(); it++) {
        delete (*it);
    }
    m_folders.clear();
    for (vector<Entity*>::iterator it = m_entities.begin(); it != m_entities.end(); it++) {
        delete (*it);
    }
    m_entities.clear();
}

void SpriterData::loadXML(const tinyxml2::XMLElement* element) {
    const XMLAttribute* attb = element->FindAttribute("scml_version");
    if(attb) {
        m_version = (float)attb->DoubleValue();
    }
    
    attb = element->FindAttribute("generator");
    if(attb) {
        m_generator = attb->Value();
    }
    
    attb = element->FindAttribute("generator_version");
    if(attb) {
        m_generatorVersion = attb->Value();
    }
    
    const tinyxml2::XMLElement* child = element->FirstChildElement();
    while (child) {
        if(strcmp(child->Name(), "folder") == 0) {
            Folder* folder = new Folder();
            folder->loadXML(child);
            addFolder(folder);
        }
        else if(strcmp(child->Name(), "entity") == 0) {
            Entity* entity = new Entity(this);
            entity->loadXML(child);
            addEntity(entity);
        }
        else if(strcmp(child->Name(), "tag_list") == 0) {
            TagList* tagList = new TagList(this);
            tagList->loadXML(child);
            addTagList(tagList);
        }
        
        child = child->NextSiblingElement();
    }
}

void SpriterData::addFolder(Folder* a_folder) {
    m_folders.push_back(a_folder);
}

void SpriterData::addEntity(Entity* a_entity) {
    m_entities.push_back(a_entity);
}

void SpriterData::addTagList(TagList* a_tagList) {
    m_tagLists.push_back(a_tagList);
}

string SpriterData::getFilePath(unsigned int a_folderIndex, unsigned int a_fileIndex) {
    if(a_folderIndex >= m_folders.size())
        return "";
    return m_folders[a_folderIndex]->getFilePath(a_fileIndex);
}


File* SpriterData::getFile(unsigned int a_folderIndex, unsigned int a_fileIndex) {
    if(a_folderIndex >= m_folders.size())
        return NULL;
    return m_folders[a_folderIndex]->getFile(a_fileIndex);
}

std::ostream& operator<< (std::ostream& out, const SpriterData& spriter) {
    out << "local anim = {" << endl;
    for(vector<Entity*>::const_iterator it = spriter.m_entities.begin(); it != spriter.m_entities.end(); it++) {
        out << *(*it);
        if(it+1 != spriter.m_entities.end()) {
            out << ",";
        }
        out << endl;
    }
    out << "}" << endl << endl;
    out << "local charMaps = {" << endl;
    for(vector<Entity*>::const_iterator it = spriter.m_entities.begin(); it != spriter.m_entities.end(); it++) {
        Entity* entity = *(it);
        if(entity->hasCharacterMaps()) {
            entity->writeCharacterMaps(out);
            if(it+1 != spriter.m_entities.end()) {
                out << ",";
            }
            out << endl;
        }
    }
    out << "}" << endl << endl;
    out << "return anim, charMaps";
    return out;
}

void SpriterData::populateTagNameFromTagList(Tag* a_tag) {
    for(vector<TagList*>::const_iterator it = m_tagLists.begin(); it != m_tagLists.end(); it++) {
        TagList* tagList = *(it);
        Tag* taglistTag = tagList->getTag(a_tag->getId());
        if(taglistTag != NULL) {
            a_tag->setName(taglistTag->getName());
        }
    }
}



