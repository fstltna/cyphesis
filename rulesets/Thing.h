#ifndef THING_H
#define THING_H

typedef int bad_type; // Remove this to get unset type reporting

#define None 0 // Remove this to deal with un-initialied vars

#include <string.h>

#include <common/BaseEntity.h>

class Thing : public BaseEntity {
  public:
    string description;
    string mode;
    double status;
    double age;

    Thing();

    virtual void addObject(Message::Object *);
    bad_type send_world(bad_type msg);
    bad_type setup_operation(bad_type op);
    bad_type tick_operation(bad_type op);
    bad_type create_operation(bad_type op);
    bad_type delete_operation(bad_type op);
    bad_type move_operation(bad_type op);
    bad_type set_operation(bad_type op);
};

class ThingFactory {
  public:
    static Thing * new_thing(const string & type, const Message::Object & ent);
};

#endif /* THING_H */
