// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000,2001 Alistair Riddoch

#include "Entity.h"
#include "Script.h"

#include "common/log.h"
#include "common/debug.h"
#include "common/inheritance.h"

#include "common/Setup.h"
#include "common/Tick.h"
#include "common/Chop.h"
#include "common/Cut.h"
#include "common/Eat.h"
#include "common/Nourish.h"
#include "common/Burn.h"

#include <Atlas/Objects/Operation/Create.h>
#include <Atlas/Objects/Operation/Sight.h>
#include <Atlas/Objects/Operation/Set.h>
#include <Atlas/Objects/Operation/Delete.h>
#include <Atlas/Objects/Operation/Imaginary.h>
#include <Atlas/Objects/Operation/Move.h>
#include <Atlas/Objects/Operation/Sound.h>
#include <Atlas/Objects/Operation/Touch.h>
#include <Atlas/Objects/Operation/Look.h>
#include <Atlas/Objects/Operation/Appearance.h>
#include <Atlas/Objects/Operation/Disappearance.h>

static const bool debug_flag = false;

std::set<std::string> Entity::m_immutable;

const std::set<std::string> & Entity::immutables()
{
    if (m_immutable.empty()) {
        m_immutable.insert("parents");
        m_immutable.insert("pos");
        m_immutable.insert("loc");
        m_immutable.insert("velocity");
        m_immutable.insert("orientation");
        m_immutable.insert("contains");
    }
    return m_immutable;
}

Entity::Entity(const std::string & id) : BaseEntity(id),
                                         script(new Script), seq(0), status(1),
                                         type("entity"), mass(-1),
                                         perceptive(false), world(NULL), update_flags(0)
{
}

Entity::~Entity()
{
    if (script != NULL) {
        delete script;
    }
}

const Fragment Entity::get(const std::string & aname) const
{
    if (aname == "status") {
        return Fragment(status);
    } else if (aname == "id") {
        return Fragment(getId());
    } else if (aname == "name") {
        return Fragment(name);
    } else if (aname == "mass") {
        return Fragment(mass);
    } else if (aname == "bbox") {
        return location.bBox.asList();
    } else if (aname == "contains") {
        Fragment::ListType contlist;
        for(EntitySet::const_iterator I = contains.begin();
            I != contains.end(); I++) {
            contlist.push_back(*I);
        }
        return Fragment(contlist);
    } else {
        Fragment::MapType::const_iterator I = attributes.find(aname);
        if (I != attributes.end()) {
            return I->second;
        } else {
            return Fragment();
        }
    }
}

void Entity::set(const std::string & aname, const Fragment & attr)
{
    if ((aname == "status") && attr.IsNum()) {
        status = attr.AsNum();
	update_flags |= a_status;
    } else if (aname == "id") {
        return;
    } else if ((aname == "name") && attr.IsString()) {
        name = attr.AsString();
	update_flags |= a_name;
    } else if ((aname == "mass") && attr.IsNum()) {
        mass = attr.AsNum();
	update_flags |= a_mass;
    } else if ((aname == "bbox") && attr.IsList() &&
               (attr.AsList().size() > 2)) {
	update_flags |= a_bbox;
        location.bBox = BBox(attr.AsList());
    } else {
	update_flags |= a_attr;
        attributes[aname] = attr;
    }
}

void Entity::setScript(Script * scrpt)
{
    if (script != NULL) {
        delete script;
    }
    script = scrpt;
    return;
}

void Entity::destroy()
{
    assert(location.ref != NULL);
    EntitySet & refContains = location.ref->contains;
    for(EntitySet::const_iterator I=contains.begin(); I != contains.end(); I++){
        Entity * obj = *I;
        obj->location.ref = location.ref;
        obj->location.coords += location.coords;
        refContains.insert(obj);
    }
    refContains.erase(this);
    if (location.ref->contains.empty()) {
        location.ref->update_flags |= a_cont;
        location.ref->updated.emit();
    }
    destroyed.emit();
}

void Entity::addToObject(Fragment::MapType & omap) const
{
    // We need to have a list of keys to pull from attributes.
    Fragment::MapType::const_iterator I = attributes.begin();
    for (; I != attributes.end(); I++) {
        omap[I->first] = I->second;
    }
    if (!name.empty()) {
        omap["name"] = name;
    }
    omap["type"] = type;
    omap["mass"] = mass;
    omap["status"] = status;
    omap["stamp"] = (double)seq;
    omap["parents"] = Fragment(Fragment::ListType(1,Fragment(type)));
    location.addToObject(omap);
    Fragment::ListType contlist;
    for(EntitySet::const_iterator I = contains.begin(); I!=contains.end(); I++){
        contlist.push_back(Fragment((*I)->getId()));
    }
    if (!contlist.empty()) {
        omap["contains"] = Fragment(contlist);
    }
    BaseEntity::addToObject(omap);
}

void Entity::merge(const Fragment::MapType & ent)
{
    const std::set<std::string> & imm = immutables();
    for(Fragment::MapType::const_iterator I = ent.begin(); I != ent.end(); I++){
        const std::string & key = I->first;
        if (imm.find(key) != imm.end()) continue;
        set(key, I->second);
    }
}

bool Entity::getLocation(const Fragment::MapType & entmap,
                         const EntityDict & eobjects)
{
    debug( std::cout << "Thing::getLocation" << std::endl << std::flush;);
    Fragment::MapType::const_iterator I = entmap.find("loc");
    if ((I == entmap.end()) || !I->second.IsString()) {
        debug( std::cout << getId() << ".. has no loc" << std::endl << std::flush;);
        return true;
    }
    try {
        const std::string & ref_id = I->second.AsString();
        EntityDict::const_iterator J = eobjects.find(ref_id);
        if (J == eobjects.end()) {
            debug( std::cout << "ERROR: Can't get ref from objects dictionary" << std::endl << std::flush;);
            return true;
        }
            
        location.ref = J->second;
        I = entmap.find("pos");
        if (I != entmap.end()) {
            location.coords = Vector3D(I->second.AsList());
        }
        I = entmap.find("velocity");
        if (I != entmap.end()) {
            location.velocity = Vector3D(I->second.AsList());
        }
        I = entmap.find("orientation");
        if (I != entmap.end()) {
            location.orientation = Quaternion(I->second.AsList());
        }
        I = entmap.find("bbox");
        if (I != entmap.end()) {
            location.bBox = BBox(I->second.AsList());
        }
    }
    catch (Atlas::Message::WrongTypeException) {
        log(ERROR, "getLocation: Bad location data");
        return true;
    }
    return false;
}

Vector3D Entity::getXyz() const
{
    //Location l=location;
    if (!location.isValid()) {
        static Vector3D ret(0.0,0.0,0.0);
        return ret;
    }
    if (location.ref) {
        return Vector3D(location.coords) += location.ref->getXyz();
    } else {
        return location.coords;
    }
}

void Entity::scriptSubscribe(const std::string & op)
{
    OpNo n = Inheritance::instance().opEnumerate(op);
    if (n != OP_INVALID) {
        debug(std::cout << "SCRIPT requesting subscription to " << op
                        << std::endl << std::flush;);
        subscribe(op, n);
    } else {
        std::string msg = std::string("SCRIPT requesting subscription to ")
                        + op + " but inheritance could not give me a reference";
        log(ERROR, msg.c_str());
    }
}

OpVector Entity::SetupOperation(const Setup & op)
{
    OpVector res;
    script->Operation("setup", op, res);
    return res;
}

OpVector Entity::TickOperation(const Tick & op)
{
    OpVector res;
    script->Operation("tick", op, res);
    return res;
}

OpVector Entity::ActionOperation(const Action & op)
{
    OpVector res;
    script->Operation("action", op, res);
    return res;
}

OpVector Entity::ChopOperation(const Chop & op)
{
    OpVector res;
    script->Operation("chop", op, res);
    return res;
}

OpVector Entity::CreateOperation(const Create & op)
{
    OpVector res;
    script->Operation("create", op, res);
    return res;
}

OpVector Entity::CutOperation(const Cut & op)
{
    OpVector res;
    script->Operation("cut", op, res);
    return res;
}

OpVector Entity::DeleteOperation(const Delete & op)
{
    OpVector res;
    script->Operation("delete", op, res);
    return res;
}

OpVector Entity::EatOperation(const Eat & op)
{
    OpVector res;
    script->Operation("eat", op, res);
    return res;
}

OpVector Entity::BurnOperation(const Burn & op)
{
    OpVector res;
    script->Operation("burn", op, res);
    return res;
}

OpVector Entity::ImaginaryOperation(const Imaginary & op)
{
    OpVector res;
    script->Operation("imaginary", op, res);
    return res;
}

OpVector Entity::MoveOperation(const Move & op)
{
    OpVector res;
    script->Operation("move", op, res);
    return res;
}

OpVector Entity::NourishOperation(const Nourish & op)
{
    OpVector res;
    script->Operation("nourish", op, res);
    return res;
}

OpVector Entity::SetOperation(const Set & op)
{
    OpVector res;
    script->Operation("set", op, res);
    return res;
}

OpVector Entity::SightOperation(const Sight & op)
{
    OpVector res;
    script->Operation("sight", op, res);
    return res;
}

OpVector Entity::SoundOperation(const Sound & op)
{
    OpVector res;
    script->Operation("sound", op, res);
    return res;
}

OpVector Entity::TouchOperation(const Touch & op)
{
    OpVector res;
    script->Operation("touch", op, res);
    return res;
}

OpVector Entity::LookOperation(const Look & op)
{
    OpVector res;
    if (script->Operation("look", op, res) != 0) {
        return res;
    }

    Sight * s = new Sight( Sight::Instantiate());
    Fragment::ListType & args = s->GetArgs();
    args.push_back(Fragment::MapType());
    Fragment::MapType & amap = args.front().AsMap();
    addToObject(amap);
    s->SetTo(op.GetFrom());

    return OpVector(1,s);
}

OpVector Entity::AppearanceOperation(const Appearance & op)
{
    OpVector res;
    script->Operation("appearance", op, res);
    return res;
}

OpVector Entity::DisappearanceOperation(const Disappearance & op)
{
    OpVector res;
    script->Operation("disappearance", op, res);
    return res;
}

OpVector Entity::OtherOperation(const RootOperation & op)
{
    const std::string & op_type = op.GetParents().front().AsString();
    OpVector res;
    debug(std::cout << "Entity " << getId() << " got custom " << op_type << " op"
               << std::endl << std::flush;);
    script->Operation(op_type, op, res);
    return res;
}
